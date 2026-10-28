> **B20 hiện tại:** 55/60/85, PID12/0/0,12; làm mượt correction có điều kiện, ENTRY BRAKE8ms khi góc đã xác nhận sau thẳng nhanh, cap35 khi xác nhận lại tâm60ms. Thêm STEER sau TIME. Xem [B20](speed-b20.md). Chưa hardware-verified; policy cũ bên dưới là lịch sử khi mâu thuẫn.

> **B19 hiện tại:** Base55 / Straight60 / Max85, thêm Straight vào PID Setup. Chỉ tăng thêm khi gần tâm đủ80ms, giữ recovery/bridge/phanh B18. Xem [B19](speed-b19.md). Chưa thử phần cứng; cấu hình cũ bên dưới là lịch sử khi mâu thuẫn.

> **B18 hiện tại:** giữ55/85, short curve bridge8ms/white4ms, tăng ramp thoát cua khi line giữa, thêm TIME sau Stop (N1: LIVE/LAST/DIAG/TIME). Xem [B18](speed-b18.md). Chưa thử phần cứng; policy cũ bên dưới là lịch sử khi mâu thuẫn.

> **B17 hiện tại:** defaults55/85, cua mềm giảm ít hơn khi ổn định, outward risk giảm sớm, corner32/pivot36/RECENTER30. Owner báo hết map, giảm khoảng1s;60/90 chậm hơn do phanh nhiều. Xem [B17](speed-b17.md). Các policy cũ bên dưới là lịch sử khi mâu thuẫn.

> **B16 hiện tại:** defaults Base50/Max80, giữ thuật toán/PID B15 và trần bonus55. Xem [profile B16](profile-b16.md). Chưa thử phần cứng; các cấu hình mặc định cũ bên dưới là lịch sử.

> **B15 hiện tại:** Base45/Max75, bonus có điều kiện lên55, cua mềm nhanh hơn và GAP giảm phần tốc dư. Xem [B15](speed-b15.md). B14 đã được owner xác nhận chạy map; B15 chưa thử phần cứng. Các policy cũ bên dưới là lịch sử khi mâu thuẫn.

> **B14 hiện tại:** giảm tốc theo valid shape lệch bên, nhớ cue trước khi mất line, thêm DIAG sau Stop để khoanh vùng. Xem [B14](diagnosis-b14.md). Quy tắc cũ bên dưới là lịch sử khi bị thay thế.

# Menu OLED hiện tại — B09

Giữ sáu mục chính theo project cũ: Run, Sensor Monitor, PID Setup, Calibration, Motor Test, System.
N1/N3 chọn dòng; N2 Enter; giữ N2 Stop và về Main Menu.

| Menu | N1 | N2 | N3 |
| --- | --- | --- | --- |
| Main Menu | Mục tiếp | Enter | Mục trước |
| PID Setup | Chọn tiếp / tăng khi edit | Edit / xác nhận | Chọn trước / giảm khi edit |
| Calibration | Auto chưa hỗ trợ | Sau sweep lấy white reference; giữ để thoát | Bắt đầu/kết thúc manual sweep |
| Motor Test | Test tiếp, dừng test cũ | Start/Stop | Test trước, dừng test cũ |
| Run, đang dừng | Nhấn ngắn: LAST RUN/Live; giữ: đổi DIR | Giữ để thoát | Start |
| Run, đang chạy | Không đổi parameter | Giữ để Stop/Back | Stop khi pressed đã debounce |
| Sensor Monitor | Giữ đổi polarity thủ công | Giữ để thoát | Chưa gán |
| System | Reset parameter | Giữ để thoát | Mở Recovery |
| Recovery | Tăng Reacq khi edit | Edit/xác nhận; giữ để thoát | Giảm Reacq khi edit |

PID Setup hiện đồng thời Kp, Ki, Kd, Base, Max. Không cuộn ẩn tốc độ.
Mặc định thử B09 Kp=12, Ki=0, Kd=0.12, Base=45%, Max=75%; xem [cruise-gap-b09.md](cruise-gap-b09.md).
Base là target cruise trên đường thẳng/cua nhẹ; Max chỉ cap từng bánh, không tự boost lên Max.
Base/Max chỉnh mỗi bước 5 điểm phần trăm PWM, Base không vượt Max; giá trị có tác dụng trong RUN.
Parameter hiện RAM-only, Reset không giữ giá trị đã chỉnh.

System → N3: Recovery hiển thị Advance DISABLED. B09 không chạy tiến mù theo geometry.
Chỉ chỉnh Reacq mặc định 12 ms, phạm vi 4..40 ms, bước 2 ms.
Motor Test vẫn 20% PWM, timeout 2 giây, không tự chạy sau boot.

Calibration: N3 sweep start/end → đặt cả 8 mắt trên trắng → N2 lấy white reference.
Start yêu cầu calibration/reference hợp lệ, line valid, ADC mới và OLED hoạt động.
Mode Run gồm READY, TRACK, GAP, BRAKE, TURN/SEARCH, PROBE, RECENTER, FAULT.
B trên Run/LAST RUN là số full brake pulse đã yêu cầu; không là lực phanh hoặc tốc độ đo được.
Run giữ V/T/label (common PWM request/target và nguyên nhân BASE/CURV/GAP/REC/BRK/PRB/OFF).
BST/WAIT là enum legacy, controller B09 không phát chúng.
LAST RUN giữ cả V/T/label, mode viết ngắn TRK/TRN/REC/BRK; giữ error/mask/PWM/SAT/B/F.
Nút điều khiển không đổi. Xem [cruise-gap-b09.md](cruise-gap-b09.md).
