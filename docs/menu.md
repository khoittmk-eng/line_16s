> **B34 hiện tại:** Cho phép bánh trong lùi theo PD khi TRACK có cue valid cùng phía6ms, error≥2pitch và chưa về tâm; lùi tối đa20% theo gain và không vượtMax. Cua nhẹ/ALIGN giữ forward PD, driver giữ zero brake/guard, Base45/Max85 và PID nền12/0/0,12. 104.778 ARM assertions +26 Python tests pass; Keil 0 lỗi/cảnh báo. Chưa nạp/thử xe. Xem [B34](signed-corner-b34.md); policy cũ bên dưới là lịch sử khi mâu thuẫn.

> **B33 hiện tại:** Owner báo B32 cua rộng/văng rồi bắt lại line, chậm hơn B30. B33 khôi phục zero brake và arc0,8 của B30, giữ bảo vệ Stop/watchdog B32; Base45/Max85, PID nền12/0/0,12. 104.172 ARM assertions +26 Python tests pass; Keil 0 lỗi/cảnh báo. Chưa nạp/thử B33 hoặc chứng minh nhanh hơn B30. Xem [B33](restore-corner-b33.md); policy cũ bên dưới là lịch sử khi mâu thuẫn.

> **B32 hiện tại:** Lệnh motor 0% trong drive/turn chuyển sang coast từng bánh. Stop/fault/watchdog/maintenance vẫn brake; lease 20ms bao gồm cả lệnh0/0 coast. Giữ controller B31, Base45/Max85 và PID nền12/0/0,12. 104.172 ARM assertions +26 Python tests pass; Keil 0 lỗi/cảnh báo. Chưa nạp/thử xe. Xem [B32](coast-zero-b32.md); policy cũ bên dưới là lịch sử khi mâu thuẫn.

> **B31 hiện tại:** Arc steering tăng 0,8→1,0 trong tối đa 28ms sau loss có hướng, tránh giảm chênh lệch đã đạt ở valid85/0. Giữ B30, Base45/Max85 và PID nền12/0/0,12; common bị giới hạn42,5% khi output85/0. 104.160 ARM assertions +26 Python tests pass; Keil 0 lỗi/cảnh báo. Chưa nhận probe USB, chưa nạp/thử xe. Xem [B31](sharp-arc-b31.md); policy cũ bên dưới là lịch sử khi mâu thuẫn.

> **B30 hiện tại:** Owner chấp nhận xe dễ văng hơn để tăng chênh lệch hai bánh ở góc gắt. Extra P tối đa +100%, tăng/nhả 0,08/ms; giữ xác nhận góc và nhả sớm B29, Base45/Max85, PID nền12/0/0,12. 104.155 ARM assertions +26 Python tests pass; Keil 0 lỗi/cảnh báo. Chưa thử xe. Xem [B30](stronger-corner-b30.md); các policy cũ bên dưới là lịch sử khi mâu thuẫn.

> **B29 hiện tại:** Giữ Base45/Max85 và PID nền12/0/0,12. ExtraP lên nhanh hơn, nhả sớm sau6ms error về tâm; xác nhận tăng lại để tránh bật/tắt do một mẫu nhiễu. Mốc ownerB28:16,3giây;55/60 từng chậm hơn45. Xem [B29](corner-return-b29.md). Chưahardware-verified; policy cũ bên dưới là lịch sử khi mâu thuẫn.

> **B28 hiện tại:** Base45/Max85, PID nền12/0/0,12. Tăng P từ từ ở confirmed visible corner trước khi mất line; search50 trên trắng,36 khi có tín hiệu, nhậnPD ngay khi valid. Giữ B27 arc/nét đứt/driver. Đã so ba candidate bằng command trace; chưa hardware-verified. Xem [B28](visible-corner-b28.md); policy cũ bên dưới là lịch sử khi mâu thuẫn.

> **B27 hiện tại:** Base45/Max85, PID12/0/0,12; rẽ tiến bằng PWM đang có trong28ms đầu recovery rồi mới pivot nếu còn mất line. Nhận line ngay, bảo vệ retry góc vuông; lọc hướng GAP và stagger đảo chiều để tránh both-wheel zero khi retarget. Xem [B27](rolling-corner-b27.md). Chưa hardware-verified; các policy cũ bên dưới là lịch sử khi mâu thuẫn.

> **B26 hiện tại:** Base45/Max85, PID12/0/0,12; TRACK cố định, P phản ứng ngay khi có line, rejoin giữ duty dưới cap40. Giữ recovery/góc vuông/nét đứt có giới hạn. Thêm cấu hình SWD rõ ràng và hướng dẫn lỗi nạp Keil. Xem [B26](corner-flow-b26.md) và [Keil/ST-Link](keil-download.md). **Chưa hardware-verified**; các policy cũ bên dưới là lịch sử khi mâu thuẫn.

> **B25 hiện tại:** TRACK dùng Base cố định, bỏ boost/Straight và steering floor B24. Mất line sau góc đã xác nhận chuyển TURN trực tiếp; sửa lỗi sai số giữa ±0,5 làm đảo hướng tìm line. Giữ GAP, RECENTER/post-turn và watchdog. Mặc định Base55/Max85, PID12/0/0,12; PID Setup có5 dòng. Xem [hướng dẫn B25](fixed-base-b25.md). **Chưa hardware-verified**; các chính sách cũ bên dưới là lịch sử khi mâu thuẫn.

> **B24 hiện tại:** bỏ xung BRAKE trước recovery; TURN trộn common/steering để tạo yaw ngay, valid corner có steering floor và bỏ cap32 do shape đơn thuần; reversal guard theo từng bánh. CORNER chờ cue có hướng/TURN/fault. Profile55/60/85, PID12/0/0,12. Xem [B24](speed-b24.md). Chưa hardware-verified; policy cũ bên dưới là lịch sử khi mâu thuẫn.

> **B23 hiện tại:** sửa PROBE15 theo log thật: continuation tối đa32ms khi có guide giữa ổn định, cap32%, không tăng duty khi invalid; cue rẽ/memory/recovery có ưu tiên. Profile55/60/85, PID12/0/0,12. Xem [B23](speed-b23.md). Chưa hardware-verified; policy cũ bên dưới là lịch sử khi mâu thuẫn.

> **B22 hiện tại:** hộp đen RAM → Save Flash khi dừng → Export CSV qua PB12/CH340, 9600 8N1. Vào **6. System → N2: BLACK BOX**. Controller B21 giữ nguyên; không tự tối ưu PID. Xem [hướng dẫn B22](blackbox-b22.md). Chưa hardware-verified; nội dung cũ bên dưới là lịch sử khi mâu thuẫn.

> **B21 hiện tại:** bỏ ENTRY_BRAKE khi line valid; tiếp tục forward PID và giảm PWM mượt ở cua mềm, giữ phanh khi mất line nhanh và bảo vệ chuỗi góc vuông. Profile 55/60/85, PID 12/0/0,12. Xem [B21](speed-b21.md). Các policy cũ bên dưới là lịch sử khi mâu thuẫn. Chưa hardware-verified.

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
