# B17 — Cua mềm và recovery nhanh hơn

Profile defaults55/85, PID12/0/0,12, trần bonus55 như B16. Đổi curve blend bắt đầu từ2 pitch thay1,5; kết thúc3 pitch, tight target32 thay28. Shape gấp xác nhận vẫn dùng guard cũ nhưng cap32. Không đồng nhất mọi error lệch với mất line: outward filtered rate trên4 pitch/s, tăng risk đến16 pitch/s, nhân mức lệch từ0,8 đến2 pitch để giảm target liên tục về tight target. Rate cao giữa các mắt tâm không kích nhánh này. Valid line luôn forward PD, không full brake/pivot.

Pivot36 thay30, ramp600 thay450 điểm PWM/giây, RECENTER30 thay26. Brake entry40/pulse12ms, recovery deadline1200ms, pending/post-turn, GAP300ms/hold60ms và watchdog giữ nguyên. Không có delay quay tối thiểu; valid line hủy phanh/quay ngay. Tốc thực/quãng phanh chưa đo.

Owner báo chạy ổn định hết map, giảm khoảng1s và phanh90° chủ động hơn; cua mềm vẫn có cảm giác phanh rồi đi, chuỗi góc còn chậm. Thử60/90 phanh nhiều và tốn thời gian hơn. Chưa có trace để phân biệt full brake/recovery với hạ PWM. Đây là phản hồi thực của owner, không là đo động lực học bởi agent.

Validation:34.734 assertions pass; thêm outward movement so với stable moderate curve, chuỗi góc hai hướng ở55/85, ramp quay36 và RECENTER30. Test ramp phải tính cả bánh còn chạy tiến khi vào TURN; không mặc định bắt đầu từ0. Keil0 error/0 warning; Code25484/RO1280/RW344/ZI2632.
HEX:build/linefollower-B17.hex.
SHA256:5A69FDC4897F27373E22AE532D0F8669578B9F03649840B8D96E75F463CF417D.
Backup:backups/B16-before-B17-20261026/source.zip và linefollower-B16.hex.

Bước nghiên cứu tiếp: đo thời gian TRACK/CURVE/PROBE/BRAKE/TURN/RECENTER và số lần vào recovery, để phân biệt cua mềm invalid thoáng qua với giảm target quá mạnh. Giữ55/85 để so sánh. Chưa triển khai thêm thay đổi sau phản hồi này.
