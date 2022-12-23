#include "SnapshotHistory.h"
#include "common/tpt-compat.h"

// * SnapshotDelta d is the difference between the two Snapshots A and B (i.e. d = B - A)
//   if auto d = SnapshotDelta::FromSnapshots(A, B). In this case, a Snapshot that is
//   identical to B can be constructed from d and A via d.Forward(A) (i.e. B = A + d)
//   and a Snapshot that is identical to A can be constructed from d and B via
//   d.Restore(B) (i.e. A = B - d). SnapshotDeltas often consume less memory than Snapshots,
//   although pathological cases of pairs of Snapshots exist, the SnapshotDelta constructed
//   from which actually consumes more than the two snapshots combined.
// * SnapshotHistory::history is an N-item deque of HistoryEntry structs, each of which owns either
//   a SnapshotDelta, except for history[N-1], which always owns a Snapshot. A logical Snapshot
//   accompanies each item in SnapshotHistory::history. This logical Snapshot may or may not be
//   materialised (present in memory). If an item owns an actual Snapshot, the aforementioned
//   logical Snapshot is this materialised Snapshot. If, however, an item owns a SnapshotDelta d,
//   the accompanying logical Snapshot A is the Snapshot obtained via A = d.Restore(B), where B
//   is the logical Snapshot accompanying the next (at an index that is one higher than the
//   index of this item) item in history. Slightly more visually:
//
//      i   |    history[i]   |  the logical Snapshot   | relationships |
//          |                 | accompanying history[i] |               |
//   -------|-----------------|-------------------------|---------------|
//          |                 |                         |               |
//    N - 1 |   Snapshot A    |       Snapshot A        |            A  |
//          |                 |                         |           /   |
//    N - 2 | SnapshotDelta b |       Snapshot B        |  B+b=A   b-B  |
//          |                 |                         |           /   |
//    N - 3 | SnapshotDelta c |       Snapshot C        |  C+c=B   c-C  |
//          |                 |                         |           /   |
//    N - 4 | SnapshotDelta d |       Snapshot D        |  D+d=C   d-D  |
//          |                 |                         |           /   |
//     ...  |      ...        |          ...            |   ...    ...  |
//
// * SnapshotHistory::historyPosition is an integer in the closed range 0 to N, which is decremented
//   by SnapshotHistory::HistoryRestore and incremented by SnapshotHistory::HistoryForward, by 1 at a time.
//   SnapshotHistory::historyCurrent "follows" historyPosition such that it always holds a Snapshot
//   that is identical to the logical Snapshot of history[historyPosition], except when
//   historyPosition = N, in which case it's empty. This following behaviour is achieved either
//   by "stepping" historyCurrent by Forwarding and Restoring it via the SnapshotDelta in
//   history[historyPosition], cloning the Snapshot in history[historyPosition] into it if
//   historyPosition = N-1, or clearing if it historyPosition = N.
// * SnapshotHistory::historyCurrent is lost when a new Snapshot item is pushed into SnapshotHistory::history.
//   This item appears wherever historyPosition currently points, and every other item above it
//   is deleted. If historyPosition is below N, this gets rid of the Snapshot in history[N-1].
//   Thus, N is set to historyPosition, after which the new Snapshot is pushed and historyPosition
//   is incremented to the new N.
// * Pushing a new Snapshot into the history is a bit involved:
//   * If there are no history entries yet, the new Snapshot is simply placed into SnapshotHistory::history.
//     From now on, we consider cases in which SnapshotHistory::history is originally not empty.
//
//     === after pushing Snapshot A' into the history
//
//        i   |    history[i]   |  the logical Snapshot   | relationships |
//            |                 | accompanying history[i] |               |
//     -------|-----------------|-------------------------|---------------|
//            |                 |                         |               |
//        0   |   Snapshot A    |       Snapshot A        |            A  |
//
//   * If there were discarded history entries (i.e. the user decided to continue from some state
//     which they arrived to via at least one Ctrl+Z), history[N-2] is a SnapshotDelta that when
//     Forwarded with the logical Snapshot of history[N-2] yields the logical Snapshot of history[N-1]
//     from before the new item was pushed. This is not what we want, so we replace it with a
//     SnapshotDelta that is the difference between the logical Snapshot of history[N-2] and the
//     Snapshot freshly placed in history[N-1].
//
//     === after pushing Snapshot A' into the history
//
//        i   |    history[i]   |  the logical Snapshot   | relationships |
//            |                 | accompanying history[i] |               |
//     -------|-----------------|-------------------------|---------------|
//            |                 |                         |               |
//      N - 1 |   Snapshot A'   |       Snapshot A'       |            A' | b needs to be replaced with b',
//            |                 |                         |           /   | B+b'=A'; otherwise we'd run
//      N - 2 | SnapshotDelta b |       Snapshot B        |  B+b=A   b-B  | into problems when trying to
//            |                 |                         |           /   | reconstruct B from A' and b
//      N - 3 | SnapshotDelta c |       Snapshot C        |  C+c=B   c-C  | in HistoryRestore.
//            |                 |                         |           /   |
//      N - 4 | SnapshotDelta d |       Snapshot D        |  D+d=C   d-D  |
//            |                 |                         |           /   |
//       ...  |      ...        |          ...            |   ...    ...  |
//
//     === after replacing b with b'
//
//        i   |    history[i]   |  the logical Snapshot   | relationships |
//            |                 | accompanying history[i] |               |
//     -------|-----------------|-------------------------|---------------|
//            |                 |                         |               |
//      N - 1 |   Snapshot A'   |       Snapshot A'       |            A' |
//            |                 |                         |           /   |
//      N - 2 | SnapshotDelta b'|       Snapshot B        | B+b'=A' b'-B  |
//            |                 |                         |           /   |
//      N - 3 | SnapshotDelta c |       Snapshot C        |  C+c=B   c-C  |
//            |                 |                         |           /   |
//      N - 4 | SnapshotDelta d |       Snapshot D        |  D+d=C   d-D  |
//            |                 |                         |           /   |
//       ...  |      ...        |          ...            |   ...    ...  |
//
//   * If there weren't any discarded history entries, history[N-2] is now also a Snapshot. Since
//     the freshly pushed Snapshot in history[N-1] should be the only Snapshot in history, this is
//     replaced with the SnapshotDelta that is the difference between history[N-2] and the Snapshot
//     freshly placed in history[N-1].
//
//     === after pushing Snapshot A' into the history
//
//        i   |    history[i]   |  the logical Snapshot   | relationships |
//            |                 | accompanying history[i] |               |
//     -------|-----------------|-------------------------|---------------|
//            |                 |                         |               |
//      N - 1 |   Snapshot A'   |       Snapshot A'       |            A' | A needs to be converted to a,
//            |                 |                         |               | otherwise Snapshots would litter
//      N - 1 |   Snapshot A    |       Snapshot A        |            A  | SnapshotHistory::history, which we
//            |                 |                         |           /   | want to avoid because they
//      N - 2 | SnapshotDelta b |       Snapshot B        |  B+b=A   b-B  | waste a ton of memory
//            |                 |                         |           /   |
//      N - 3 | SnapshotDelta c |       Snapshot C        |  C+c=B   c-C  |
//            |                 |                         |           /   |
//      N - 4 | SnapshotDelta d |       Snapshot D        |  D+d=C   d-D  |
//            |                 |                         |           /   |
//       ...  |      ...        |          ...            |   ...    ...  |
//
//     === after replacing A with a
//
//        i   |    history[i]   |  the logical Snapshot   | relationships |
//            |                 | accompanying history[i] |               |
//     -------|-----------------|-------------------------|---------------|
//            |                 |                         |               |
//      N - 1 |   Snapshot A'   |       Snapshot A'       |            A' |
//            |                 |                         |           /   |
//      N - 1 | SnapshotDelta a |       Snapshot A        |  A+a=A'  a-A  |
//            |                 |                         |           /   |
//      N - 2 | SnapshotDelta b |       Snapshot B        |  B+b=A   b-B  |
//            |                 |                         |           /   |
//      N - 3 | SnapshotDelta c |       Snapshot C        |  C+c=B   c-C  |
//            |                 |                         |           /   |
//      N - 4 | SnapshotDelta d |       Snapshot D        |  D+d=C   d-D  |
//            |                 |                         |           /   |
//       ...  |      ...        |          ...            |   ...    ...  |
//
//   * After all this, the front of the deque is truncated such that there are on more than
//     undoHistoryLimit entries left.

unsigned int SnapshotHistory::undoHistoryLimit = 5;
unsigned int SnapshotHistory::historyPosition = 0;
std::deque<HistoryEntry> SnapshotHistory::history = std::deque<HistoryEntry>();
std::unique_ptr<Snapshot> SnapshotHistory::beforeRestore = nullptr;
std::unique_ptr<Snapshot> SnapshotHistory::historyCurrent = nullptr;

HistoryEntry::~HistoryEntry()
{
	// * Needed because Snapshot and SnapshotDelta are incomplete types in SnapshotHistory.h,
	//   so the default dtor for ~HistoryEntry cannot be generated.
}

void SnapshotHistory::TakeSnapshot(Simulation * sim)
{
	std::unique_ptr<Snapshot> snap = Snapshot::Create(sim);
	if (!snap)
		return;

	Snapshot *rebaseOnto = nullptr;
	if (historyPosition)
	{
		rebaseOnto = history.back().snap.get();
		if (historyPosition < history.size())
		{
			historyCurrent = history[historyPosition - 1U].delta->Restore(*historyCurrent);
			rebaseOnto = historyCurrent.get();
		}
	}

	while (historyPosition < history.size())
	{
		history.pop_back();
	}

	if (rebaseOnto)
	{
		auto &prev = history.back();
		prev.delta = SnapshotDelta::FromSnapshots(*rebaseOnto, *snap);
		prev.snap.reset();
	}
	history.emplace_back();
	history.back().snap = std::move(snap);
	historyPosition++;
	historyCurrent.reset();

	// Delete items at back of history
	if (history.size() >= undoHistoryLimit)
	{
		history.pop_front();
		historyPosition--;
	}
}


bool SnapshotHistory::HistoryRestore(Simulation * sim)
{
	if (!history.size() || historyPosition <= 0)
		return false;
	// When undoing, save the current state as a final redo
	// This way ctrl+y will always bring you back to the point right before your last ctrl+z
	if (historyPosition == history.size())
	{
		std::unique_ptr<Snapshot> newSnap = Snapshot::Create(sim);
		beforeRestore.swap(newSnap);
	}

	historyPosition--;
	// Restore full snapshot
	if (history[historyPosition].snap)
	{
		historyCurrent = std::make_unique<Snapshot>(*history[historyPosition].snap);
	}
	// Restore delta
	else
	{
		historyCurrent = history[historyPosition].delta->Restore(*historyCurrent);
	}
	Snapshot::Restore(sim, *historyCurrent);

	return true;
}

bool SnapshotHistory::HistoryForward(Simulation *sim)
{
	if (historyPosition >= history.size())
		return false;

	historyPosition++;
	// Restore special snapshot taken on first undo
	if (historyPosition == history.size())
	{
		historyCurrent.swap(beforeRestore);
		beforeRestore.reset();
	}
	// Restore full snapshot
	else if (history[historyPosition].snap)
	{
		historyCurrent = std::make_unique<Snapshot>(*history[historyPosition].snap);
	}
	// Restore delta
	else
	{
		historyCurrent = history[historyPosition - 1].delta->Forward(*historyCurrent);
	}
	Snapshot::Restore(sim, *historyCurrent);

	return true;
}
