#pragma once

#include "Snapshot.h"
#include "SnapshotDelta.h"

struct HistoryEntry
{
	std::unique_ptr<Snapshot> snap;
	std::unique_ptr<SnapshotDelta> delta;

	~HistoryEntry();
};

class SnapshotHistory
{
	static unsigned int undoHistoryLimit;
	static unsigned int historyPosition;
	static std::deque<HistoryEntry> history;
	static std::unique_ptr<Snapshot> beforeRestore;
	static std::unique_ptr<Snapshot> historyCurrent;
public:

	// manage snapshots list
	static void TakeSnapshot(Simulation * sim);
	static bool HistoryRestore(Simulation * sim);
	static bool HistoryForward(Simulation *sim);

	static void SetUndoHistoryLimit(unsigned int newLimit) { undoHistoryLimit = std::min(newLimit, (unsigned int)200); }
	static unsigned int GetUndoHistoryLimit() { return undoHistoryLimit; }

};
