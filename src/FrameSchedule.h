#pragma once
#include <cstdint>
#include <algorithm>
#include <cmath>
#include <limits>

class FrameSchedule
{
	uint64_t startNs = 0;
	uint64_t oldStartNs = 0;

public:
	void SetNow(uint64_t nowNs)
	{
		oldStartNs = startNs;
		startNs = nowNs;
	}

	uint64_t GetNow() const
	{
		return startNs;
	}

	uint64_t GetFrameTime() const
	{
		return startNs - oldStartNs;
	}

	uint64_t Arm(float fps)
	{
		if (!std::isfinite(fps) || fps <= 0.0f)
			return 0;
		auto oldNowNs = startNs;
		auto timeBlockDurationNs = uint64_t(std::clamp(1e9 / static_cast<double>(fps), 1.0, 1e9));
		auto oldStartTimeBlock = oldStartNs / timeBlockDurationNs;
		auto startTimeBlock = oldStartTimeBlock == std::numeric_limits<uint64_t>::max()
			? oldStartTimeBlock : oldStartTimeBlock + 1U;
		auto nextStartNs = startTimeBlock > std::numeric_limits<uint64_t>::max() / timeBlockDurationNs
		? std::numeric_limits<uint64_t>::max() : startTimeBlock * timeBlockDurationNs;
		startNs = std::max(startNs, nextStartNs);
		return startNs - oldNowNs;
	}

	bool HasElapsed(uint64_t nowNs) const
	{
		return nowNs >= startNs;
	}
};
