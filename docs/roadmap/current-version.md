# OmniCore incremental version state

```text
CURRENT_VERSION=1.0.1
CURRENT_PHASE=Diagnostics & Stability Foundation
PHASE_STATUS=COMPLETE
BASE_COMMIT=ca3d643c8b24a8588a5e84477e145312707d354b
IMPLEMENTATION_HEAD=97d2fc2c175818a66636421526e4f562d4d1de01
BRANCH=integration/omnicore-vnext
BUILD=GREEN
TESTS=GREEN
RUNTIME=GREEN
CONCURRENCY=GREEN
BENCHMARK=RECORDED
PROFILER_OVERHEAD=MEASURED
SAVE_LUA_SMOKE=GREEN
INDEPENDENT_PROFILER_REVIEW=not_available
KNOWN_BLOCKERS=G0 physical conservation; complete source-sink/correction accounting; unsampled full-state positivity; process VRAM; accepted performance budget
NEXT_VERSION=1.0.2
NEXT_PHASE=Latest TPT Upstream Compatibility
```

The 1.0.1 gate is GREEN because its own required diagnostics, runtime and
benchmark evidence is complete. Global OmniCore G0 remains RED and continues to
forbid OmniAtmosphere implementation. The next version begins with a fresh
official stable/master check and an impact audit; it does not enter the 1.0.3 UI
or materials scope.

The milestone report is [phase-1-profiler-export.md](../vnext/phase-1-profiler-export.md).
