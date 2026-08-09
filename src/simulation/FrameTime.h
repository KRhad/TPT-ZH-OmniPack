#pragma once
#include "common/String.h"
#include <array>
#include <cstdint>
#include <vector>
#include <chrono>
#include <map>
#include <mutex>
#include <optional>
#include <atomic>
#include <memory>

class FrameTime
{
	using Clock = std::chrono::steady_clock;

public:
	enum class Subsystem : uint8_t
	{
		Frame,
		Simulation,
		ParticleUpdate,
		Air,
		AmbientHeat,
		Gravity,
		Thermal,
		Chemistry,
		Lua,
		RenderSnapshotCopy,
		Rendering,
		Gpu,
		GpuSynchronization,
		Count,
	};
	static constexpr size_t SubsystemCount = static_cast<size_t>(Subsystem::Count);

	struct SubsystemMetric
	{
		uint64_t calls{};
		uint64_t lastNanoseconds{};
		uint64_t totalNanoseconds{};
		uint64_t maximumNanoseconds{};
	};

	struct SubsystemMetrics
	{
		bool enabled{};
		bool threadedRenderingObserved{};
		bool gpuBackendActive{};
		bool gpuTimingsAvailable{};
		bool gpuSynchronizationTimingsAvailable{};
		bool processVramAvailable{};
		uint64_t processVramBytes{};
		char const *processVramStatus{};
		std::array<SubsystemMetric, SubsystemCount> subsystems{};
	};

private:

	std::optional<Clock::time_point> lastFrameEndAt;
	struct ActiveSpan
	{
		int retiredIndex;
		Clock::time_point begin;
	};
	std::vector<ActiveSpan> activeSpans;
	struct RetiredSpan
	{
		int level;
		const char *name;
		Clock::duration duration;
	};
	std::vector<RetiredSpan> retiredSpans;
	std::map<ByteString, double> durationAverages;
	struct AveragedSpan
	{
		int level;
		const char *name;
		double duration;
	};
	std::vector<AveragedSpan> lastAveragedSpans;
	std::array<SubsystemMetric, SubsystemCount> subsystemMetrics{};
	bool threadedRenderingObserved = false;
	std::atomic_bool subsystemProfilerEnabled = false;
	std::atomic_uint64_t subsystemProfilerGeneration = 0;
	mutable std::mutex subsystemMutex;

	void BeginFrame();
	void EndFrame();
	void PushSpanInner(const char *name, Clock::time_point now);
	void PushSpan(const char *name);
	void PopSpan();
	void RecordSubsystem(Subsystem subsystem, Clock::duration duration, uint64_t generation);

public:
	static char const *SubsystemName(Subsystem subsystem);
	void SetSubsystemProfilerEnabled(bool enabled);
	void ResetSubsystemProfiler();
	SubsystemMetrics GetSubsystemMetrics() const;
	// RenderSimulation is called with handleEvents=false only by the renderer
	// worker. This records that the real threaded path ran while profiling.
	void RecordThreadedRenderingObserved();
	bool HasActiveFrame() const
	{
		return !activeSpans.empty();
	}

	const std::vector<AveragedSpan> &GetLastSpans()
	{
		return lastAveragedSpans;
	}

	class Span
	{
		FrameTime *frameTime;

	public:
		Span(FrameTime *newFrameTime, const char *name) : frameTime(newFrameTime)
		{
			if (frameTime)
			{
				frameTime->PushSpan(name);
			}
		}

		~Span()
		{
			if (frameTime)
			{
				frameTime->PopSpan();
			}
		}

		Span(const Span &) = delete;
		Span &operator =(const Span &) = delete;
	};

	class SubsystemSpan
	{
		std::shared_ptr<FrameTime> owner;
		FrameTime *frameTime;
		Subsystem subsystem;
		Clock::time_point begin;
		uint64_t generation{};

		void Begin(FrameTime *newFrameTime)
		{
			if (newFrameTime && newFrameTime->subsystemProfilerEnabled.load(std::memory_order_acquire))
			{
				generation = newFrameTime->subsystemProfilerGeneration.load(std::memory_order_acquire);
				if (newFrameTime->subsystemProfilerEnabled.load(std::memory_order_acquire))
				{
					frameTime = newFrameTime;
					begin = Clock::now();
				}
			}
		}

	public:
		SubsystemSpan(FrameTime *newFrameTime, Subsystem newSubsystem):
			owner(),
			frameTime(nullptr),
			subsystem(newSubsystem),
			begin(),
			generation()
		{
			Begin(newFrameTime);
		}

		SubsystemSpan(std::shared_ptr<FrameTime> newFrameTime, Subsystem newSubsystem):
			owner(std::move(newFrameTime)),
			frameTime(nullptr),
			subsystem(newSubsystem),
			begin(),
			generation()
		{
			Begin(owner.get());
		}

		~SubsystemSpan()
		{
			if (frameTime)
			{
				frameTime->RecordSubsystem(subsystem, Clock::now() - begin, generation);
			}
		}

		SubsystemSpan(const SubsystemSpan &) = delete;
		SubsystemSpan &operator =(const SubsystemSpan &) = delete;
	};

	class OptionalFrame
	{
		FrameTime *frameTime;
		Clock::time_point begin;
		uint64_t generation{};

	public:
		OptionalFrame(FrameTime *newFrameTime):
			frameTime(newFrameTime && !newFrameTime->HasActiveFrame() ? newFrameTime : nullptr),
			begin(frameTime ? Clock::now() : Clock::time_point{}),
			generation(frameTime ? frameTime->subsystemProfilerGeneration.load(std::memory_order_acquire) : 0)
		{
			if (frameTime)
			{
				frameTime->BeginFrame();
			}
		}

		~OptionalFrame()
		{
			if (frameTime)
			{
				if (frameTime->subsystemProfilerEnabled.load(std::memory_order_relaxed))
				{
					frameTime->RecordSubsystem(Subsystem::Frame, Clock::now() - begin, generation);
				}
				frameTime->EndFrame();
			}
		}

		OptionalFrame(const OptionalFrame &) = delete;
		OptionalFrame &operator =(const OptionalFrame &) = delete;
	};

	struct Frame
	{
		FrameTime *frameTime;

		Frame(FrameTime *newFrameTime) : frameTime(newFrameTime)
		{
			if (frameTime)
			{
				frameTime->BeginFrame();
			}
		}

		~Frame()
		{
			if (frameTime)
			{
				frameTime->EndFrame();
			}
		}

		Frame(const Frame &) = delete;
		Frame &operator =(const Frame &) = delete;
	};
};
