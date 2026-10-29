# B32 — Thả trôi khi lệnh bánh bằng 0, giữ phanh an toàn

> **Sau đó:** Owner xác nhận cua rộng hoặc văng rồi mới bắt lại line. B33 hoàn nguyên zero brake và arc0,8 của B30; giữ bảo vệ Stop/watchdog B32. Xem [B33](restore-corner-b33.md). Chưa có phép đo tách riêng ảnh hưởng coast và arc.

> **Phản hồi owner sau thử xe (29/10/2026):** Bản hiện tại B32 chạy lâu hơn B30. Chưa có thời gian vòng cụ thể, số lượt lặp lại hoặc log B32 để xác định nhánh gây chậm. Các kết quả emulation/build bên dưới chỉ là kiểm tra source, không chứng minh tối ưu thời gian vòng. Cần dùng B30 làm mốc đối chiếu, tách ảnh hưởng arc1,0 của B31 và zero-coast của B32; không tăng thêm lực quay hoặc Base chỉ từ PWM request.

## Phạm vi owner chấp nhận

Owner yêu cầu áp dụng đề xuất bỏ phanh bánh trong lúc bám line/cua, giữ phanh khi Stop, fault và watchdog. B32 đổi chính sách zero command của driver; controller B31, Base45/Max85, PID nền12/0/0,12 và arc28ms không đổi.

`Motor_SetDuty(left,right)` coi mỗi giá trị 0 là coast cho riêng channel đó. `Motor_StopBrake()` là đường riêng, trực tiếp phanh cả hai channel trong critical section; boot, menu Stop/Motor Test timeout, fault, watchdog và maintenance Save/Export tiếp tục gọi API này.

| Lệnh | B31 | B32 |
| --- | --- | --- |
| 850/0 (permille) | Bánh ngoài tiến85%, bánh trong brake | Bánh ngoài tiến85%, bánh trong coast |
| 0/0 qua Motor_SetDuty | Brake cả hai | Coast cả hai, vẫn có command lease20ms |
| Motor_StopBrake | Brake cả hai | Brake cả hai ngay |
| Guard đảo chiều | Zero ngắn với brake | Zero ngắn với coast; vẫn giữ guard/stagger |

Motor Test một bánh cũng dùng `Motor_SetDuty`, nên bánh không được chạy sẽ coast; explicit Stop và timeout Motor Test vẫn brake. Không đổi menu hoặc tự Start.

## Yêu cầu điện và giới hạn

Theo [datasheet Toshiba TB6612FNG, bảng H-SW Control Function](https://toshiba.semicon-storage.com/info/docget.jsp?did=10660): coast dùng IN1=IN2=Low, PWM=High, STBY=High; short brake dùng IN1=IN2=High, PWM=Low. STBY vẫn nối3V3, không thêm GPIO. Với TIM4 PWM1, CCR=period (ARR+1) giữ PWM High cho zero coast; đây không phải lệnh motor chạy100% vì hai direction input đều Low.

B32 chỉ bỏ short brake chủ động của lệnh zero trong lúc drive/turn. PWM khi chạy duty khác0 vẫn dùng cách điều chế cũ; trạng thái PWM Low của TB6612 vẫn có short brake. Không tuyên bố loại bỏ mọi khoảng phanh điện hoặc switching transient; không đổi PWM pin, driver topology hay estimator.

`command_leased` theo dõi cả lệnh0/0, thay vì dùng `previous!=0` để quyết định watchdog. Nếu drive command không được refresh trong hơn20ms, kể cả lúc cả hai bánh đang coast, `Motor_SafetyTick` vẫn gọi brake. Explicit `Motor_StopCoast()` giữ nghĩa coast độc lập và hủy drive lease. Đảo chiều vẫn phải qua một update zero, giữ stagger khi đổi hướng pivot; coast không chứng minh motor đã dừng trước khi nhận chiều mới.

PWM request và log vẫn ghi0 khi coast; đọc firmware tagB32 để phân biệt với brake của bản cũ. ZERO diagnostics vẫn đếm zero command, không đo lực phanh. Record48/header108, CSVv1 và vùng Flash log giữ nguyên.

## Validation

- Backup: `backups/B31-before-B32-20261029/B31-source.zip` (gồm BSP) và `linefollower-B31.hex`.
- Test zero-coast mới fail50048 trên B31; đây là khác biệt policy owner yêu cầu, không phải defect hardware được phát hiện.
- **104.172 ARM assertions +8 parser/layout +18 receiver tests pass.** Driver thật với fake MMIO xác nhận coast hai channel/hai chiều, bánh ngoài không mất drive, explicit brake sau coast, boot brake, reversal guard/stagger và ceiling, lease20/21ms kể cả0/0, explicit coast không tự bị brake. Suite controller/recovery/square-chain/Stop/fault/maintenance/watchdogs vẫn pass.
- Emulator300M instructions/60s hoàn tất `test_done`; không xác nhận waveform, dòng motor, stopping distance hoặc WCET thực.
- `scripts/build.ps1`: Keil0Error/0Warning; Code33336/RO1856/RW444/ZI10524bytes. Layout kết thúc0x08008A4C, dưới vùng log0x0800C000.
- HEX: `build/linefollower-B32.hex`; SHA256 `958833CA144BCC703F5C2D82D68EC8FB939E5F24A4D31E4EF43F26580308E6FA`.

## Thử xe

Chưa nạp hoặc thử B32 trên board. Sau nạp kiểm tra OLED B32, calibration/white reference và Stop, rồi thử45/85 với gain nền giữ nguyên. Coast có thể giảm ghì bánh trong nhưng làm đường cua rộng hơn hoặc văng; chưa có lap time chứng minh lợi ích. Giữ cùng pin/map, ghi nhiều lượt và log CORNER/LOSS nếu có regression. Export log cần giữ trước reflash; không full erase hoặc đổi option bytes.
