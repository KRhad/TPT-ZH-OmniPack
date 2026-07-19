#include "SnapshotDelta.h"

#include "common/tpt-compat.h"
#include "common/tpt-minmax.h"
#include "simulation/ElementDataContainer.h"
#include "simulation/Simulation.h"

#include <utility>

// * A SnapshotDelta is a bidirectional difference type between Snapshots, defined such
//   that SnapshotDelta d = SnapshotDelta::FromSnapshots(A, B) yields a SnapshotDelta which can be
//   used to construct a Snapshot identical to A via d.Restore(B) and a Snapshot identical
//   to B via d.Forward(A). Thus, d = B - A, A = B - d and B = A + d.
// * Fields in Snapshot can be classified into two groups:
//   * Fields of static size, whose sizes are identical to the size of the corresponding field
//     in all other Snapshots. Example of these fields include AmbientHeat (whose size depends
//     on XRES, YRES and CELL, all compile-time constants) and WirelessData (whose size depends
//     on CHANNELS, another compile-time constant). Note that these fields would be of "static
//     size" even if their sizes weren't derived from compile-time constants, as they'd still
//     be the same size throughout the life of a Simulation, and thus any Snapshot created from it.
//   * Fields of dynamic size, whose sizes may be different between Snapshots. These are, fortunately,
//     the minority: Particles, signs and Authors.
// * Each field in Snapshot has a mirror set of fields in SnapshotDelta. Fields of static size
//   have mirror fields whose type is HunkVector, templated by the item type of the
//   corresponding field; these fields are handled in a uniform manner. Fields of dynamic size are
//   handled in a non-uniform, case-by-case manner. 
// * A HunkVector is generated from two streams of identical size and is a collection
//   of Hunks, a Hunk is an offset combined with a collection of Diffs, and a Diff is a pair of values,
//   one originating from one stream and the other from the other. Thus, Hunks represent contiguous
//   sequences of differences between the two streams, and a HunkVector is a compact way to represent
//   all differences between the two streams it's generated from. In this case, these streams are
//   the data in corresponding fields of static size in two Snapshots, and the HunkVector is the
//   respective field in the SnapshotDelta that is the difference between the two Snapshots.
//   * FillHunkVectorPtr is the d = B - A operation, which takes two Snapshot fields of static size and
//     the corresponding SnapshotDelta field, and fills the latter with the HunkVector generated
//     from the former streams.
//   * ApplyHunkVector<true> is the A = B - d operation, which takes a field of a SnapshotDelta and
//     the corresponding field of a "newer" Snapshot, and fills the latter with the "old" values.
//   * ApplyHunkVector<false> is the B = A + d operation, which takes a field of a SnapshotDelta and
//     the corresponding field of an "older" Snapshot, and fills the latter with the "new" values.
//   * This difference type is intended for fields of static size. This covers all fields in Snapshot
//     except for Particles, signs, and Authors.
// * A SingleDiff is, unsurprisingly enough, a single Diff, with an accompanying bool that signifies
//   whether the Diff does in fact hold the "old" value of a field in the "old" Snapshot and the "new"
//   value of the same field in the "new" Snapshot. If this bool is false, the data in the fields
//   of both Snapshots are equivalent and the SingleDiff should be ignored. If it's true, the
//   SingleDiff represents the difference between these fields.
//   * FillSingleDiff is the d = B - A operation, while ApplySingleDiff<false> and ApplySingleDiff<true>
//     are the A = B - d and B = A + d operations. These are self-explanatory.
//   * This difference type is intended for fields of dynamic size whose data doesn't change often and
//     doesn't consume too much memory. This covers the Snapshot fields signs and Authors.
// * This leaves Snapshot::Particles. This field mirrors Simulation::parts, which is actually also
//   a field of static size, but since most of the time most of this array is empty, it doesn't make
//   sense to store all of it in a Snapshot (unlike Air::hv, which can be fairly chaotic (i.e. may have
//   a lot of interesting data in all of its cells) when ambient heat simulation is enabled, or
//   Simulation::wireless, which is not big enough to need compression). This makes Snapshots smaller,
//   but the life of a SnapshotDelta developer harder. The following, relatively simple approach is
//   taken, as a sort of compromise between simplicity and memory usage:
//   * The common part of the Particles arrays in the old and the new Snapshots is identified: this is
//     the overlapping part, i.e. the first size cells of both arrays, where
//     size = min(old.Particles.size(), new.Particles.size()), and a HunkVector is generated from it,
//     as though it was a field of static size. For our purposes, it is indeed Static Enough:tm:, for
//     it only needs to be the same size as the common part of the Particles arrays of the two Snapshots.
//   * The rest of both Particles arrays is copied to the extra fields extraPartsOld and extraPartsNew.
// * One more trick is at work here: Particle structs are actually compared property-by-property rather
//   than as a whole. This ends up being beneficial to memory usage, as many properties (e.g. type
//   and ctype) don't often change over time, while others (e.g. x and y) do. Currently, all Particle
//   properties are 4-byte integral values, which makes it feasible to just reinterpret_cast Particle
//   structs as arrays of uint32_t values and generate HunkVectors from the resulting streams instead.
//   This assumption is enforced by the following static_asserts. The same trick is used for playerst
//   structs, even though Snapshot::stickmen is not big enough for us to benefit from this. The
//   alternative would have been to implement operator ==(const playerst &, const playerst &), which
//   would have been tedious.

// * Needed by FillSingleDiff for handling Snapshot::signs.
bool operator ==(const std::vector<Sign> &lhs, const std::vector<Sign> &rhs)
{
	if (lhs.size() != rhs.size())
	{
		return false;
	}
	for (auto i = 0U; i < lhs.size(); ++i)
	{
		if (!(lhs[i].GetRealPos().X     == rhs[i].GetRealPos().X    &&
			  lhs[i].GetRealPos().Y     == rhs[i].GetRealPos().Y    &&
			  lhs[i].GetJustification() == rhs[i].GetJustification()   &&
			  lhs[i].GetText()          == rhs[i].GetText()))
		{
			return false;
		}
	}
	return true;
}

std::unique_ptr<SnapshotDelta> SnapshotDelta::FromSnapshots(const Snapshot &oldSnap, const Snapshot &newSnap)
{
	auto ptr = std::make_unique<SnapshotDelta>();
	auto &delta = *ptr;
	FillHunkVector(oldSnap.AirPressure    , newSnap.AirPressure    , delta.AirPressure    );
	FillHunkVector(oldSnap.AirVelocityX   , newSnap.AirVelocityX   , delta.AirVelocityX   );
	FillHunkVector(oldSnap.AirVelocityY   , newSnap.AirVelocityY   , delta.AirVelocityY   );
	FillHunkVector(oldSnap.AmbientHeat    , newSnap.AmbientHeat    , delta.AmbientHeat    );
	FillHunkVector(oldSnap.GravVelocityX  , newSnap.GravVelocityX  , delta.GravVelocityX  );
	FillHunkVector(oldSnap.GravVelocityY  , newSnap.GravVelocityY  , delta.GravVelocityY  );
	FillHunkVector(oldSnap.GravValue      , newSnap.GravValue      , delta.GravValue      );
	FillHunkVector(oldSnap.GravMap        , newSnap.GravMap        , delta.GravMap        );
	FillHunkVector(oldSnap.BlockMap       , newSnap.BlockMap       , delta.BlockMap       );
	FillHunkVector(oldSnap.ElecMap        , newSnap.ElecMap        , delta.ElecMap        );
	FillHunkVector(oldSnap.FanVelocityX   , newSnap.FanVelocityX   , delta.FanVelocityX   );
	FillHunkVector(oldSnap.FanVelocityY   , newSnap.FanVelocityY   , delta.FanVelocityY   );
	FillSingleDiff(oldSnap.Signs          , newSnap.Signs          , delta.signs          );
	FillSingleDiff(oldSnap.Authors        , newSnap.Authors        , delta.Authors        );

	// * Slightly more interesting; this will only diff the common parts, the rest is copied separately.
	auto commonSize = std::min(oldSnap.Particles.size(), newSnap.Particles.size());
	FillHunkVectorPtr(reinterpret_cast<const uint32_t *>(&oldSnap.Particles[0]), reinterpret_cast<const uint32_t *>(&newSnap.Particles[0]), delta.commonParticles, commonSize * ParticleUint32Count);
	delta.extraPartsOld.resize(oldSnap.Particles.size() - commonSize);
	std::copy(oldSnap.Particles.begin() + commonSize, oldSnap.Particles.end(), delta.extraPartsOld.begin());
	delta.extraPartsNew.resize(newSnap.Particles.size() - commonSize);
	std::copy(newSnap.Particles.begin() + commonSize, newSnap.Particles.end(), delta.extraPartsNew.begin());

	for (int i = 0; i < PT_NUM; i++)
	{
#ifndef NOMOD
		// ANIM is very huge, better to never take snapshots of it unless necessary, or else memory usage skyrockets
		// This is incorrect behavior, but unless someone does something weird with deleting ANIM between snapshots, nobody will notice
		// ANIM's data structure is too weird and I don't want to write delta support for it right now
		if (i == PT_ANIM && !globalSim->elementCount[PT_ANIM])
			continue;
#endif
		if (newSnap.elementData[i] && oldSnap.elementData[i])
		{
			std::unique_ptr<ElementDataContainerDelta> elementDataDelta = newSnap.elementData[i]->Compare(oldSnap.elementData[i], newSnap.elementData[i]);
			if (elementDataDelta != nullptr)
			{
				delta.elementDataDelta[i].swap(elementDataDelta);
				continue;
			}
		}
		if (newSnap.elementData[i])
			delta.elementDataNew[i] = newSnap.elementData[i]->Clone();
		else
			delta.elementDataNew[i] = nullptr;

		if (oldSnap.elementData[i])
			delta.elementDataOld[i] = oldSnap.elementData[i]->Clone();
		else
			delta.elementDataOld[i] = nullptr;
	}

	return ptr;
}

std::unique_ptr<Snapshot> SnapshotDelta::Forward(const Snapshot &oldSnap)
{
	auto ptr = std::make_unique<Snapshot>(oldSnap);
	auto &newSnap = *ptr;
	ApplyHunkVector<false>(AirPressure    , newSnap.AirPressure    );
	ApplyHunkVector<false>(AirVelocityX   , newSnap.AirVelocityX   );
	ApplyHunkVector<false>(AirVelocityY   , newSnap.AirVelocityY   );
	ApplyHunkVector<false>(AmbientHeat    , newSnap.AmbientHeat    );
	ApplyHunkVector<false>(GravVelocityX  , newSnap.GravVelocityX  );
	ApplyHunkVector<false>(GravVelocityY  , newSnap.GravVelocityY  );
	ApplyHunkVector<false>(GravValue      , newSnap.GravValue      );
	ApplyHunkVector<false>(GravMap        , newSnap.GravMap        );
	ApplyHunkVector<false>(BlockMap       , newSnap.BlockMap       );
	ApplyHunkVector<false>(ElecMap        , newSnap.ElecMap        );
	ApplyHunkVector<false>(FanVelocityX   , newSnap.FanVelocityX   );
	ApplyHunkVector<false>(FanVelocityY   , newSnap.FanVelocityY   );
	ApplySingleDiff<false>(signs          , newSnap.Signs          );
	ApplySingleDiff<false>(Authors        , newSnap.Authors        );

	// * Slightly more interesting; apply the common hunk vector, copy the extra portion separaterly.
	ApplyHunkVectorPtr<false>(commonParticles, reinterpret_cast<uint32_t *>(&newSnap.Particles[0]));
	auto commonSize = oldSnap.Particles.size() - extraPartsOld.size();
	newSnap.Particles.resize(commonSize + extraPartsNew.size());
	std::copy(extraPartsNew.begin(), extraPartsNew.end(), newSnap.Particles.begin() + commonSize);

	for (int i = 0; i < PT_NUM; i++)
	{
		if (elementDataDelta[i])
		{
			newSnap.elementData[i]->Forward(newSnap.elementData[i], elementDataDelta[i]);
		}
		else if (elementDataNew[i])
			newSnap.elementData[i] = elementDataNew[i]->Clone();
		else if (newSnap.elementData[i])
			newSnap.elementData[i].reset();
	}

	return ptr;
}

std::unique_ptr<Snapshot> SnapshotDelta::Restore(const Snapshot &newSnap)
{
	auto ptr = std::make_unique<Snapshot>(newSnap);
	auto &oldSnap = *ptr;
	ApplyHunkVector<true>(AirPressure    , oldSnap.AirPressure    );
	ApplyHunkVector<true>(AirVelocityX   , oldSnap.AirVelocityX   );
	ApplyHunkVector<true>(AirVelocityY   , oldSnap.AirVelocityY   );
	ApplyHunkVector<true>(AmbientHeat    , oldSnap.AmbientHeat    );
	ApplyHunkVector<true>(GravVelocityX  , oldSnap.GravVelocityX  );
	ApplyHunkVector<true>(GravVelocityY  , oldSnap.GravVelocityY  );
	ApplyHunkVector<true>(GravValue      , oldSnap.GravValue      );
	ApplyHunkVector<true>(GravMap        , oldSnap.GravMap        );
	ApplyHunkVector<true>(BlockMap       , oldSnap.BlockMap       );
	ApplyHunkVector<true>(ElecMap        , oldSnap.ElecMap        );
	ApplyHunkVector<true>(FanVelocityX   , oldSnap.FanVelocityX   );
	ApplyHunkVector<true>(FanVelocityY   , oldSnap.FanVelocityY   );
	ApplySingleDiff<true>(signs          , oldSnap.Signs          );
	ApplySingleDiff<true>(Authors        , oldSnap.Authors        );

	// * Slightly more interesting; apply the common hunk vector, copy the extra portion separaterly.
	ApplyHunkVectorPtr<true>(commonParticles, reinterpret_cast<uint32_t *>(&oldSnap.Particles[0]));
	auto commonSize = newSnap.Particles.size() - extraPartsNew.size();
	oldSnap.Particles.resize(commonSize + extraPartsOld.size());
	std::copy(extraPartsOld.begin(), extraPartsOld.end(), oldSnap.Particles.begin() + commonSize);

	for (int i = 0; i < PT_NUM; i++)
	{
		if (elementDataDelta[i])
		{
			oldSnap.elementData[i]->Restore(oldSnap.elementData[i], elementDataDelta[i]);
		}
		else if (elementDataOld[i])
			oldSnap.elementData[i] = elementDataOld[i]->Clone();
		else if (oldSnap.elementData[i])
			oldSnap.elementData[i].reset();
	}

	return ptr;
}
