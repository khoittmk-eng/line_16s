> **B20 hiện tại:** 55/60/85, PID12/0/0,12; làm mượt correction có điều kiện, ENTRY BRAKE8ms khi góc đã xác nhận sau thẳng nhanh, cap35 khi xác nhận lại tâm60ms. Thêm STEER sau TIME. Xem [B20](speed-b20.md). Chưa hardware-verified; policy cũ bên dưới là lịch sử khi mâu thuẫn.

> **B19 hiện tại:** Base55 / Straight60 / Max85, thêm Straight vào PID Setup. Chỉ tăng thêm khi gần tâm đủ80ms, giữ recovery/bridge/phanh B18. Xem [B19](speed-b19.md). Chưa thử phần cứng; cấu hình cũ bên dưới là lịch sử khi mâu thuẫn.

> **B18 hiện tại:** giữ55/85, short curve bridge8ms/white4ms, tăng ramp thoát cua khi line giữa, thêm TIME sau Stop (N1: LIVE/LAST/DIAG/TIME). Xem [B18](speed-b18.md). Chưa thử phần cứng; policy cũ bên dưới là lịch sử khi mâu thuẫn.

> **B17 hiện tại:** defaults55/85, cua mềm giảm ít hơn khi ổn định, outward risk giảm sớm, corner32/pivot36/RECENTER30. Owner báo hết map, giảm khoảng1s;60/90 chậm hơn do phanh nhiều. Xem [B17](speed-b17.md). Các policy cũ bên dưới là lịch sử khi mâu thuẫn.

> **B16 hiện tại:** defaults Base50/Max80, giữ thuật toán/PID B15 và trần bonus55. Xem [profile B16](profile-b16.md). Chưa thử phần cứng; các cấu hình mặc định cũ bên dưới là lịch sử.

> **B15 hiện tại:** Base45/Max75, bonus có điều kiện lên55, cua mềm nhanh hơn và GAP giảm phần tốc dư. Xem [B15](speed-b15.md). B14 đã được owner xác nhận chạy map; B15 chưa thử phần cứng. Các policy cũ bên dưới là lịch sử khi mâu thuẫn.

> **B14 hiện tại:** giảm tốc theo valid shape lệch bên, nhớ cue trước khi mất line, thêm DIAG sau Stop để khoanh vùng. Xem [B14](diagnosis-b14.md). Quy tắc cũ bên dưới là lịch sử khi bị thay thế.

> **Cập nhật B09:** Base cruise và guide-memory GAP theo lựa chọn owner, xem [cruise-gap-b09.md](cruise-gap-b09.md). Bước tiếp: thử đoạn liền→nét đứt→cua nhẹ, sau đó 90° hai phía; không yêu cầu đọc OLED lúc chạy. Test logic không là xác nhận động lực học; nội dung phiên bản cũ bên dưới là lịch sử.

# Roadmap triển khai và kiểm chứng

## Trạng thái hiện tại

Đã triển khai source cho timebase, TB6612 output, ADC/DMA frame 2 ms,
buttons EXTI/NVIC, OLED và manual calibration/line estimator.
Application đã có menu cũ, Motor Test, PD/PID chu kỳ frame 2 ms, acceleration ramp, RUN và bounded gap/recovery. Owner xác nhận calibration và Motor Test; polarity mặc định đã sửa theo phản hồi. Owner xác nhận RUN ở Base 20% bám vòng tròn/cua nhẹ, chưa qua cua 90°. Owner xác nhận B03 cải thiện cua/gap, nhưng cắt chéo chuỗi góc vuông. B04 thêm ADVANCE/ALIGN, speed UI đầy đủ và Corner Timing theo geometry 16 cm/40 cm; chưa thử B04 trên xe. Auto Calibration chưa triển khai.
Build và emulation test đã pass; calibration/Motor Test có xác nhận owner, các tiêu chí RUN/timing bên dưới vẫn cần đo trên xe.
Xem [bringup.md](bringup.md) và ghi lỗi/kết quả vào [error-log.md](error-log.md).

## Các mốc triển khai

| Thứ tự | Phạm vi | Bằng chứng hoàn thành |
| --- | --- | --- |
| 1 | Xác nhận oscillator, LED, sensor level, button polarity và OLED model/address | Ghi kết quả đo, cập nhật tài liệu phần cứng |
| 2 | TB6612 GPIO/TIM4 driver với stop mode rõ ràng | Thử riêng từng bánh; xác nhận forward polarity và hành vi stop |
| 3 | ADC/DMA frame capture và sensor monitor | Tám channel ổn định; kiểm chứng cơ chế công bố frame đầy đủ |
| 4 | Manual calibration và line estimation | Đủ contrast ở từng channel; error đơn điệu và đúng dấu |
| 5 | PD chu kỳ cố định ở duty thấp | Đo period, execution time, jitter; xác nhận đúng chiều sửa lái |
| 6 | Acceleration ramp và target duty theo tracking quality | Lặp lại thử tăng tốc đường thẳng, không mất line |
| 7 | Gap crossing và recovery có giới hạn | Thử lặp lại nét đứt, off-track recovery và timeout |
| 8 | Nút nhấn EXTI/NVIC, debounce state machine, OLED menu và chỉnh parameter | Mỗi lần nhấn chỉ tạo một event; nhấn giữ/nhả đúng; bounce không gây interrupt starvation; OLED/nút không làm trễ control; stop hoạt động ở mọi mode |
| 9 | Junction navigation và route memory nếu cần | Chốt yêu cầu và chiến lược nhận diện riêng trước khi triển khai |

Triển khai từng mốc; ghi firmware revision và điều kiện thử phần cứng.
Đọc [hardware.md](hardware.md) trước khi cấp phát peripheral pin.
