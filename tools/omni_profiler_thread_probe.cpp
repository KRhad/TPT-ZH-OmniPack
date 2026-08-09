#include "simulation/FrameTime.h"

#include <atomic>
#include <iostream>
#include <memory>
#include <thread>

int main()
{
	constexpr uint64_t FrameCount = 64;
	constexpr uint64_t RenderingSpanCount = 4096;

	FrameTime frameTime;
	frameTime.SetSubsystemProfilerEnabled(true);
	std::atomic_bool start = false;
	std::thread renderingThread([&]() {
		while (!start.load(std::memory_order_acquire))
		{
			std::this_thread::yield();
		}
		for (uint64_t index = 0; index < RenderingSpanCount; index++)
		{
			FrameTime::SubsystemSpan renderingSpan(&frameTime, FrameTime::Subsystem::Rendering);
		}
	});

	start.store(true, std::memory_order_release);
	for (uint64_t index = 0; index < FrameCount; index++)
	{
		FrameTime::Frame frame(&frameTime);
		FrameTime::SubsystemSpan frameSpan(&frameTime, FrameTime::Subsystem::Frame);
		FrameTime::SubsystemSpan simulationSpan(&frameTime, FrameTime::Subsystem::Simulation);
	}
	renderingThread.join();

	auto metrics = frameTime.GetSubsystemMetrics();
	auto frameIndex = static_cast<size_t>(FrameTime::Subsystem::Frame);
	auto simulationIndex = static_cast<size_t>(FrameTime::Subsystem::Simulation);
	auto renderingIndex = static_cast<size_t>(FrameTime::Subsystem::Rendering);
	if (!metrics.enabled ||
		metrics.subsystems[frameIndex].calls != FrameCount ||
		metrics.subsystems[simulationIndex].calls != FrameCount ||
		metrics.subsystems[renderingIndex].calls != RenderingSpanCount)
	{
		std::cerr << "omni-profiler-thread-probe: FAIL\n";
		return 1;
	}

	// The renderer takes a shared owner for its span. Disable and release the
	// main-thread owner while the span is in flight; the span must remain safe
	// and the object must be released after it completes.
	auto lifetimeFrameTime = std::make_shared<FrameTime>();
	lifetimeFrameTime->SetSubsystemProfilerEnabled(true);
	std::weak_ptr<FrameTime> lifetimeWeak = lifetimeFrameTime;
	std::atomic_bool lifetimeSpanStarted = false;
	std::atomic_bool releaseLifetimeSpan = false;
	std::thread lifetimeThread([&]() {
		FrameTime::SubsystemSpan renderingSpan(lifetimeFrameTime, FrameTime::Subsystem::Rendering);
		lifetimeSpanStarted.store(true, std::memory_order_release);
		while (!releaseLifetimeSpan.load(std::memory_order_acquire))
		{
			std::this_thread::yield();
		}
	});
	while (!lifetimeSpanStarted.load(std::memory_order_acquire))
	{
		std::this_thread::yield();
	}
	lifetimeFrameTime->ResetSubsystemProfiler();
	lifetimeFrameTime->SetSubsystemProfilerEnabled(false);
	lifetimeFrameTime.reset();
	if (lifetimeWeak.expired())
	{
		std::cerr << "omni-profiler-thread-probe: FAIL (span did not retain owner)\n";
		releaseLifetimeSpan.store(true, std::memory_order_release);
		lifetimeThread.join();
		return 1;
	}
	releaseLifetimeSpan.store(true, std::memory_order_release);
	lifetimeThread.join();
	if (!lifetimeWeak.expired())
	{
		std::cerr << "omni-profiler-thread-probe: FAIL (owner survived completed span)\n";
		return 1;
	}

	std::cout << "omni-profiler-thread-probe: PASS\n";
	std::cout << "frame_calls=" << metrics.subsystems[frameIndex].calls << "\n";
	std::cout << "simulation_calls=" << metrics.subsystems[simulationIndex].calls << "\n";
	std::cout << "rendering_calls=" << metrics.subsystems[renderingIndex].calls << "\n";
	std::cout << "lifetime_disable_reset_race=PASS\n";
	std::cout << "frame_time_bytes=" << sizeof(FrameTime) << "\n";
	return 0;
}
