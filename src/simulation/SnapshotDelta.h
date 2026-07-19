#pragma once

#include "Snapshot.h"

#include <memory>
#include <cstdint>

constexpr size_t ParticleUint32Count = sizeof(particle) / sizeof(uint32_t);
static_assert(sizeof(particle) % sizeof(uint32_t) == 0, "fix me");

struct SnapshotDelta
{
	template<class Item>
	struct Diff
	{
		Item oldItem, newItem;
	};

	template<class Item>
	struct Hunk
	{
		int offset;
		std::vector<Diff<Item>> diffs;
	};

	template<class Item>
	struct SingleDiff
	{
		bool valid = false;
		Diff<Item> diff;
	};

	template<class Item>
	struct HalfHunk
	{
		int offset;
		std::vector<Item> items;
	};

	template<class Item>
	using HunkVector = std::vector<Hunk<Item>>;

	template<class Item>
	struct HalfHunkVectorPair
	{
		std::vector<HalfHunk<Item>> oldHunks, newHunks;
	};

	HunkVector<float> AirPressure;
	HunkVector<float> AirVelocityX;
	HunkVector<float> AirVelocityY;
	HunkVector<float> AmbientHeat;

	HunkVector<uint32_t> commonParticles;
	std::vector<particle> extraPartsOld, extraPartsNew;

	// Element data does not use delta, instead store a complete copy of both old and new element data
	std::unique_ptr<ElementDataContainer> elementDataOld[PT_NUM];
	std::unique_ptr<ElementDataContainer> elementDataNew[PT_NUM];
	//
	std::unique_ptr<ElementDataContainerDelta> elementDataDelta[PT_NUM];

	HunkVector<float> GravVelocityX;
	HunkVector<float> GravVelocityY;
	HunkVector<float> GravValue;
	HunkVector<float> GravMap;

	HunkVector<unsigned char> BlockMap;
	HunkVector<unsigned char> ElecMap;

	HunkVector<float> FanVelocityX;
	HunkVector<float> FanVelocityY;

	SingleDiff<std::vector<Sign>> signs;

	SingleDiff<Json::Value> Authors;

	template<class Item>
	static void FillHunkVectorPtr(const Item *oldItems, const Item *newItems, SnapshotDelta::HunkVector<Item> &out, size_t size)
	{
		auto i = 0U;
		bool different = false;
		auto offset = 0U;
		auto markDifferent = [oldItems, newItems, &out, &i, &different, &offset](bool mark) {
			if (mark && !different)
			{
				different = true;
				offset = i;
			}
			else if (!mark && different)
			{
				different = false;
				auto size = i - offset;
				out.emplace_back();
				auto &hunk = out.back();
				hunk.offset = offset;
				auto &diffs = hunk.diffs;
				diffs.resize(size);
				for (auto j = 0U; j < size; ++j)
				{
					diffs[j].oldItem = oldItems[offset + j];
					diffs[j].newItem = newItems[offset + j];
				}
			}
		};
		while (i < size)
		{
			markDifferent(!(oldItems[i] == newItems[i]));
			i += 1U;
		}
		markDifferent(false);
	}

	template<class Item>
	static void FillHunkVector(const std::vector<Item> &oldItems, const std::vector<Item> &newItems, SnapshotDelta::HunkVector<Item> &out)
	{
		FillHunkVectorPtr<Item>(&oldItems[0], &newItems[0], out, std::min(oldItems.size(), newItems.size()));
	}

	template<class Item>
	static void FillSingleDiff(const Item &oldItem, const Item &newItem, SnapshotDelta::SingleDiff<Item> &out)
	{
		if (oldItem != newItem)
		{
			out.valid = true;
			out.diff.oldItem = oldItem;
			out.diff.newItem = newItem;
		}
	}

	template<bool UseOld, class Item>
	static void ApplyHunkVectorPtr(const SnapshotDelta::HunkVector<Item> &in, Item *items)
	{
		for (auto &hunk : in)
		{
			auto offset = hunk.offset;
			auto &diffs = hunk.diffs;
			for (auto j = 0U; j < diffs.size(); ++j)
			{
				items[offset + j] = UseOld ? diffs[j].oldItem : diffs[j].newItem;
			}
		}
	}

	template<bool UseOld, class Item>
	static void ApplyHunkVector(const SnapshotDelta::HunkVector<Item> &in, std::vector<Item> &items)
	{
		ApplyHunkVectorPtr<UseOld, Item>(in, &items[0]);
	}

	template<bool UseOld, class Item>
	static void ApplySingleDiff(const SnapshotDelta::SingleDiff<Item> &in, Item &item)
	{
		if (in.valid)
		{
			item = UseOld ? in.diff.oldItem : in.diff.newItem;
		}
	}

	static std::unique_ptr<SnapshotDelta> FromSnapshots(const Snapshot &oldSnap, const Snapshot &newSnap);
	std::unique_ptr<Snapshot> Forward(const Snapshot &oldSnap);
	std::unique_ptr<Snapshot> Restore(const Snapshot &newSnap);
};
