# B16 — Profile thử Base50/Max80

Owner yêu cầu đổi profile để thử từng nấc. B15 được báo cải thiện khoảng0,5s; chưa có số vòng/điều kiện đo để kết luận độ ổn định. B16 chỉ đổi defaults45/75 thành50/80 và firmware tag. Controller, PID12/0/0,12, ADC, motor driver, brake và recovery giữ B15.

Trần bonus vẫn55: đường thẳng ổn định target55 (bonus tối đa5 ở Base50); cua mềm error1,2 pitch khoảng54; góc rộng xác nhận cap28, pivot30 và RECENTER26 như cũ. GAP dài giảm phần dư về50 thay45. Vì vậy Base cao hơn cũng nâng duty tiếp tục tiến trên trắng; chưa xác nhận mức này ổn trên xe.

Nạp `build/linefollower-B16.hex`, Reset, calibration + white reference, kiểm tra OLED Base50/Max80 rồi Run/N3. Không cần tăng thêm trước lần đối chiếu này. Thử cùng map ít nhất3 lượt, ghi thời gian từng lượt và có mất line/khựng/cắt góc hay không. Nếu ổn mới thử55/85; nếu lỗi, ghi vị trí trước khi đổi gain. Trên B15/B16, tăng Base quá55 vẫn nâng nền, nhưng không có bonus; muốn nâng trần tự tăng cần thay profile có kiểm chứng riêng. Mục tiêu là thời gian hết map nhanh và lặp lại được, không mặc định phải đạt PWM100%.

Validation:34.715 assertions pass, gồm profile50/80 (thẳng55/cua mềm54/GAP50/shape28), baseline45/75, menu edit/clamp, recovery/deadline/watchdog. Keil0 error/0 warning; Code25316/RO1280/RW344/ZI2632 bytes. Chưa hardware-verified.
SHA256 HEX:07DD597A6EACFC1EA264EACF9F17BFB0DC9D4453A43D0E37F3CF75577CBD60DE.
Backup trước sửa: `backups/B15-before-B16-20261026/source.zip` và `linefollower-B15.hex`.
