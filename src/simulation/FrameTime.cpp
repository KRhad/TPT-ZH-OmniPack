#include "FrameTime.h"
#include "common/Assert.h"
#include <algorithm>
#include <string.h>

char const *FrameTime::SubsystemName(Subsystem subsystem)
{
	switch (subsystem)
	{
	case Subsystem::Frame: return "frame";
	case Subsystem::Simulation: return "simulation";
	case Subsystem::ParticleUpdate: return "particle_update";
	case Subsystem::Air: return "air";
	case Subsystem::AmbientHeat: return "ambient_heat";
	case Subsystem::Gravity: return "gravity_dispatch_wait";
	case Subsystem::Thermal: return "thermal";
	case Subsystem::Chemistry: return "chemistry";
	case Subsystem::Lua: return "lua";
	case Subsystem::RenderSnapshotCopy: return "render_snapshot_copy";
	case Subsystem::Rendering: return "rendering";
	case Subsystem::Gpu: return "gpu";
	case Subsystem::GpuSynchronization: return "gpu_synchronization";
	case Subsystem::Count: break;
	}
	return "unknown";
}

void FrameTime::SetSubsystemProfilerEnabled(bool enabled)
{
	subsystemProfilerGeneration.fetch_add(1, std::memory_order_acq_rel);
	subsystemProfilerEnabled.store(enabled, std::memory_order_release);
	if (!enabled)
	{
		ResetSubsystemProfiler();
	}
}

void FrameTime::ResetSubsystemProfiler()
{
	subsystemProfilerGeneration.fetch_add(1, std::memory_order_acq_rel);
	std::lock_guard lock(subsystemMutex);
	subsystemMetrics = {};
	threadedRenderingObserved = false;
}

FrameTime::SubsystemMetrics FrameTime::GetSubsystemMetrics() const
{
	std::lock_guard lock(subsystemMutex);
	SubsystemMetrics metrics{};
	metrics.enabled = subsystemProfilerEnabled.load(std::memory_order_relaxed);
	metrics.threadedRenderingObserved = threadedRenderingObserved;
	metrics.processVramStatus = "not_tested_no_gpu_backend";
	metrics.subsystems = subsystemMetrics;
	return metrics;
}

void FrameTime::RecordThreadedRenderingObserved()
{
	if (!subsystemProfilerEnabled.load(std::memory_order_acquire))
	{
		return;
	}
	std::lock_guard lock(subsystemMutex);
	if (subsystemProfilerEnabled.load(std::memory_order_acquire))
	{
		threadedRenderingObserved = true;
	}
}

void FrameTime::RecordSubsystem(Subsystem subsystem, Clock::duration duration, uint64_t generation)
{
	auto index = static_cast<size_t>(subsystem);
	if (index >= SubsystemCount ||
		!subsystemProfilerEnabled.load(std::memory_order_acquire) ||
		generation != subsystemProfilerGeneration.load(std::memory_order_acquire))
	{
		return;
	}
	auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
	if (nanoseconds < 0)
	{
		return;
	}
	auto value = static_cast<uint64_t>(nanoseconds);
	std::lock_guard lock(subsystemMutex);
	if (!subsystemProfilerEnabled.load(std::memory_order_acquire) ||
		generation != subsystemProfilerGeneration.load(std::memory_order_acquire))
	{
		return;
	}
	auto &metric = subsystemMetrics[index];
	metric.calls++;
	// This is the last completed span, not a main-thread UI-frame aggregate.
	// That distinction keeps asynchronous renderer timings truthful.
	metric.lastNanoseconds = value;
	metric.totalNanoseconds += value;
	metric.maximumNanoseconds = std::max(metric.maximumNanoseconds, value);
}

void FrameTime::BeginFrame()
{
	PushSpanInner("Frame time", lastFrameEndAt ? *lastFrameEndAt : Clock::now());
}

void FrameTime::EndFrame()
{
	PopSpan();
	assert(activeSpans.empty());
	lastAveragedSpans.clear();
	std::map<ByteString, double> lastDurationAverages;
	std::vector<AveragedSpan> averagedSpans;
	std::swap(durationAverages, lastDurationAverages);
	for (auto &span : retiredSpans)
	{
		auto currDuration = double(std::chrono::duration_cast<std::chrono::nanoseconds>(span.duration).count());
		auto prevDuration = lastDurationAverages[span.name];
		auto duration = prevDuration + (currDuration - prevDuration) * 0.05;
		durationAverages[span.name] = duration;
		averagedSpans.push_back({ span.level, span.name, duration });
	}
	retiredSpans.clear();
	std::swap(averagedSpans, lastAveragedSpans);
	lastFrameEndAt = Clock::now();
}

void FrameTime::PushSpanInner(const char *name, Clock::time_point now)
{
	retiredSpans.push_back({ int(activeSpans.size()), name, {} });
	activeSpans.push_back({ int(retiredSpans.size()) - 1, now });
}

void FrameTime::PushSpan(const char *name)
{
	assert(!activeSpans.empty());
	PushSpanInner(name, Clock::now());
}

void FrameTime::PopSpan()
{
	assert(!activeSpans.empty());
	auto now = Clock::now();
	retiredSpans[activeSpans.back().retiredIndex].duration = now - activeSpans.back().begin;
	activeSpans.pop_back();
}
