# B10: recovery cho split mask lệch bên

B10 sửa trường hợp invalid mask 0xE8/0x17 không tạo corner hint, dẫn tới PROBE rồi dừng. Yêu cầu ít nhất ba mắt ở nửa phía rẽ, nhiều nhất một mắt phía đối diện và mắt ngoài cùng; xác nhận cùng hướng 6 ms trước recovery. Full-width/đối xứng không tạo hướng mới. Valid line nhường ngay về PD; không thay PID, Base45/Max75, phanh hoặc watchdog.

OLED ưu tiên nhãn fault thay vì dùng Start readiness che thành PLACE ON BLACK LINE. Fault vẫn cần explicit Start.

Validation lúc tạo: regression fail30041 trước sửa; 34.008 assertions pass sau sửa, Keil 0 error/warning. SHA256 HEX B10: B9AFF9C79D0721A93FA9538D272BD3E522CB4C0498CA2F464479E69C294A5302. Backup B09: backups/B09-before-B10-20261024.

Phản hồi owner sau B10: nét đứt và góc 90° sau đường thẳng dài đã ổn, chuỗi góc liên tục còn văng/LINE LOST. B11 xử lý chuyển tiếp recovery, xem [recovery-b11.md](recovery-b11.md). Test phần mềm không chứng minh chuyển động thực hoặc mọi map.
