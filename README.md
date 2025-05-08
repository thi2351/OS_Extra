# CFS Test-suite

This folder contains automated tests for the **OS_Extra Completely Fair Scheduler**.

## Files

| File | Purpose |
|------|---------|
| `run.sh` | Build project with `make` and execute every `.in` file in `./testcase`. Output for each test is saved as `.out`. |
| `testcase/*.in` | Input vectors for the scheduler (see format below). |
| `testcase/*.out` | Captured console output after `run.sh` completes. |

### Input format
N pid niceness arrival_time burst_time ...

pgsql
Sao chép
Chỉnh sửa
*Time units are arbitrary; the reference code treats them as nanoseconds.*

### What each test covers

| Test | Goal |
|------|------|
| **test01_equal_nice** | Validates equal CPU share when all weights are identical. |
| **test02_weighted_nice** | Checks proportional slices for different niceness. |
| **test03_staggered_arrival** | Confirms that late arrivals quickly “catch up” to older tasks. |
| **test04_preemption** | Demonstrates immediate pre-emption when a lower-vruntime task appears. |
| **test05_min_granularity** | Ensures the scheduler obeys `MIN_GRANULARITY_NSEC` and terminates tiny tasks. |


|Test | Goal |
|------|------|
|**test01_singleCPU** | Interupt handling when new process arrival. |
|**test02_singleCPU** | Timeslice handling when remain time < `MIN_GRANULARITY_NSEC` |
|**test03_multiCPU** | Multi CPU handling of **test01_singleCPU** |
|**test04_multiCPU** | Multi CPU handling use least-work CPU |

## Running

```bash
./run.sh
Outputs are written to testcase/<name>.out.
