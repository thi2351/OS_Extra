Chuyện m cần làm:

Sửa hàm main.c lại, t đã viết sẵn một hàm load_processes để thực hiện lôi 1 lốc process từ testcase lên để chạy.
Format testcase:
(n)
(n dòng tiếp theo chứa <pid> <niceness> <arrival time> <burst time>)

Hàm main cần đạt:
- Sử dụng event driven để chạy testcase (không đếm từng đơn vị thời gian lên nha cha, mà nếu bí quá thì đếm cũng được)
- Handle được idle (khoảng thời gian trống giữa các process thì time vẫn chạy, không có dừng lại mãi)
- Handle được vấn đề khi 1 process đang chạy trong CPU, 1 process nữa đến thì phải enqueue process đó, hoàn tất chạy process đang chạy trong CPU, tính toán lại process tiếp theo được chạy trong CPU, cho process đó chạy.

