Chuyện m cần làm:

Sửa hàm main.c lại, t đã viết sẵn một hàm load_processes để thực hiện lôi 1 lốc process từ testcase lên để chạy.
Format testcase:
(n)
(n dòng tiếp theo chứa <pid> <niceness> <arrival time> <burst time>)

Hàm main cần đạt:
- Sử dụng event driven để chạy testcase (không đếm từng đơn vị thời gian lên nha cha, mà nếu bí quá thì đếm cũng được)
- Handle được idle (khoảng thời gian trống giữa các process thì time vẫn chạy, không có dừng lại mãi)
- Handle được vấn đề khi 1 process đang chạy trong CPU, 1 process nữa đến thì phải enqueue process đó, hoàn tất chạy process đang chạy trong CPU, tính toán lại process tiếp theo được chạy trong CPU, cho process đó chạy.

Dữ liệu: burst time tính theo ms -> 4000ms trong test01


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

## Running

```bash
./run.sh
Outputs are written to testcase/<name>.out.