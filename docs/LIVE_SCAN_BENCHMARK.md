# Live scan benchmark

`AsyncScan_benchmark` is an opt-in Windows executable that compares the real
`AsyncScanEngine` with and without the app's live-layout cadence. It is not
registered with CTest and does not run when SpaceMonger starts. Its timing
instrumentation is compiled only into this executable; the app and test targets
use the ordinary tree mutex with no benchmark counters or timer calls.

## Build and run

From a Visual Studio 2022 Developer PowerShell with the x64 C++ and MFC tools:

```powershell
cmake --preset vs2022-x64
cmake --build --preset vs2022-x64-release --target AsyncScan_benchmark
.\build\vs2022-x64\Release\AsyncScan_benchmark.exe
```

The default creates 2,000 directories with 1,000 files each (2,000,000 files)
and runs five scan-only/live-layout pairs. File lengths vary deterministically
from 1 through 4,096 bytes. The fixture is created in a uniquely reserved temp
directory and removed recursively by the benchmark on both success and
exception paths. It reports fixture creation and cleanup duration, manifest
counts, logical bytes, approximate temp-volume free-space change, Windows
build, processor identifier, and logical CPU count. Free-space change is an
estimate that can include unrelated activity on the same volume. For a smaller
deliberate probe, pass `--directories N`, `--files-per-directory N`, and/or
`--samples N`; a live-layout sample that never reaches a cadence-triggered
layout with treemap nodes is reported as an invalid run and returns nonzero.
Each call logs progress files/folders/bytes, treemap node count, and whether
the produced layout was non-empty.

Each scan uses the engine's default worker count. The live mode checks progress
every 15 ms; the first render check is immediately eligible, matching
`last_render_tick = 0` in `FolderTree.cpp`. At each 250 ms render tick, it calls
`GenerateLiveLayout` only if
file/folder entry count or scanned bytes changed, matching `FolderTree.cpp`.
Runs alternate order by pair. The benchmark checks every completed scan's file
count, folder count, and `bytesScanned` against values accumulated independently
while creating the fixture. This verifies scanner accounting for the selected
logical byte mode (`clusterMask=0`, `aligned=false`); it does not prove
treemap-area accuracy or user-visible rendering accuracy.

Each sample reports wall scan duration, files/s, logical bytes/s, actual worker
count, and layout-call count. It reports total worker mutation acquisitions as
the verified final file and folder counts. Worker lock timing samples one in
every 256 mutations per worker, deterministically starting with that worker's
first mutation; all other worker mutations use a normal lock with no clock
reads or metric updates. Worker counts and histograms are plain per-worker data
aggregated after the scan. Every layout lock is timed. Wait and hold statistics
are cumulative, mean, maximum, and p95 upper bounds from fixed power-of-two
nanosecond histogram buckets. Summary throughput is the median across runs for
each mode; lock totals aggregate the corresponding runs and summary p95 is the
median of per-run p95 bucket bounds. Worker wait and hold totals sum across
worker threads, so they can exceed the scan's wall duration.

Results depend on CPU, storage, filesystem, antivirus/indexing activity, and
cache state. Fixture creation and repeated directory enumeration affect cache
and OS scheduling; alternating order reduces a single warm/cold-order bias but
does not eliminate it. Treat the figures as evidence for this machine and this
fixture, not as a general SpaceMonger performance claim. The benchmark performs
two steady-clock reads for each sampled worker mutation and every layout lock;
per-worker plain counters and histograms are updated after unlocking, so metric
aggregation does not serialize workers or inflate the measured lock hold
interval. Sampling limits timer overhead on mutation-heavy scans, but the
instrumented executable still differs from the normal app build, so compare
paired modes from the same executable and treat absolute rates cautiously.
