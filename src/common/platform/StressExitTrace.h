#pragma once

// Temporary, opt-in diagnostics for the native stress-client exit path.
//
// This helper deliberately avoids SDL, UI state, streams, and objects owned by
// shutdown. It is enabled only with OMNI_STRESS_EXIT_TRACE=1, and every record
// is flushed immediately so an orderly native exit cannot hide its last stage
// in a buffered pipe.

#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <thread>

namespace OmniStressExitTrace
{
inline bool Enabled() noexcept
{
	const auto *value = std::getenv("OMNI_STRESS_EXIT_TRACE");
	return value && std::strcmp(value, "1") == 0;
}

inline void Log(const char *format, ...)
{
	if (!Enabled())
		return;

	static const auto start = std::chrono::steady_clock::now();
	const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now() - start).count();
	const auto threadId = std::hash<std::thread::id>{}(std::this_thread::get_id());
	std::fprintf(stderr, "OMNI_STRESS_EXIT_TRACE elapsed_ms=%lld tid=%zu event=",
		static_cast<long long>(elapsed), threadId);
	va_list args;
	va_start(args, format);
	std::vfprintf(stderr, format, args);
	va_end(args);
	std::fputc('\n', stderr);
	std::fflush(stderr);
}
}
