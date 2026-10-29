> **B29 hiện tại:** Giữ Base45/Max85 và PID nền12/0/0,12. ExtraP lên nhanh hơn, nhả sớm sau6ms error về tâm; xác nhận tăng lại để tránh bật/tắt do một mẫu nhiễu. Mốc ownerB28:16,3giây;55/60 từng chậm hơn45. Xem [B29](corner-return-b29.md). Chưahardware-verified; policy cũ bên dưới là lịch sử khi mâu thuẫn.

> **B28 hiện tại:** Base45/Max85, PID nền12/0/0,12. Tăng P từ từ ở confirmed visible corner trước khi mất line; search50 trên trắng,36 khi có tín hiệu, nhậnPD ngay khi valid. Giữ B27 arc/nét đứt/driver. Đã so ba candidate bằng command trace; chưa hardware-verified. Xem [B28](visible-corner-b28.md); policy cũ bên dưới là lịch sử khi mâu thuẫn.

> **B27 hiện tại:** Base45/Max85, PID12/0/0,12; rẽ tiến bằng PWM đang có trong28ms đầu recovery rồi mới pivot nếu còn mất line. Nhận line ngay, bảo vệ retry góc vuông; lọc hướng GAP và stagger đảo chiều để tránh both-wheel zero khi retarget. Xem [B27](rolling-corner-b27.md). Chưa hardware-verified; các policy cũ bên dưới là lịch sử khi mâu thuẫn.

> **B26 hiện tại:** Base45/Max85, PID12/0/0,12; TRACK cố định, P phản ứng ngay khi có line, rejoin giữ duty dưới cap40. Giữ recovery/góc vuông/nét đứt có giới hạn. Thêm cấu hình SWD rõ ràng và hướng dẫn lỗi nạp Keil. Xem [B26](corner-flow-b26.md) và [Keil/ST-Link](keil-download.md). **Chưa hardware-verified**; các policy cũ bên dưới là lịch sử khi mâu thuẫn.

> **B25 hiện tại:** TRACK dùng Base cố định, bỏ boost/Straight và steering floor B24. Mất line sau góc đã xác nhận chuyển TURN trực tiếp; sửa lỗi sai số giữa ±0,5 làm đảo hướng tìm line. Giữ GAP, RECENTER/post-turn và watchdog. Mặc định Base55/Max85, PID12/0/0,12; PID Setup có5 dòng. Xem [hướng dẫn B25](fixed-base-b25.md). **Chưa hardware-verified**; các chính sách cũ bên dưới là lịch sử khi mâu thuẫn.

> **B24 hiện tại:** bỏ xung BRAKE trước recovery; TURN trộn common/steering để tạo yaw ngay, valid corner có steering floor và bỏ cap32 do shape đơn thuần; reversal guard theo từng bánh. CORNER chờ cue có hướng/TURN/fault. Profile55/60/85, PID12/0/0,12. Xem [B24](speed-b24.md). Chưa hardware-verified; policy cũ bên dưới là lịch sử khi mâu thuẫn.

> **B23 hiện tại:** sửa PROBE15 theo log thật: continuation tối đa32ms khi có guide giữa ổn định, cap32%, không tăng duty khi invalid; cue rẽ/memory/recovery có ưu tiên. Profile55/60/85, PID12/0/0,12. Xem [B23](speed-b23.md). Chưa hardware-verified; policy cũ bên dưới là lịch sử khi mâu thuẫn.

> Nhận log không dùng Hercules: chạy `scripts/receive-blackbox.ps1`, chọn COM rồi bấm Export trên OLED; tự lưu CSV và kiểm tra CRC. Xem [hướng dẫn nhận log](receive-log.md). Dùng với firmware B22 hiện tại, không cần nạp lại.

> **B22 hiện tại:** hộp đen RAM → Save Flash khi dừng → Export CSV qua PB12/CH340, 9600 8N1. Vào **6. System → N2: BLACK BOX**. Controller B21 giữ nguyên; không tự tối ưu PID. Xem [hướng dẫn B22](blackbox-b22.md). Chưa hardware-verified; nội dung cũ bên dưới là lịch sử khi mâu thuẫn.

> **B21 hiện tại:** bỏ ENTRY_BRAKE khi line valid; tiếp tục forward PID và giảm PWM mượt ở cua mềm, giữ phanh khi mất line nhanh và bảo vệ chuỗi góc vuông. Profile 55/60/85, PID 12/0/0,12. Xem [B21](speed-b21.md). Các policy cũ bên dưới là lịch sử khi mâu thuẫn. Chưa hardware-verified.

> **B20 hiện tại:** 55/60/85, PID12/0/0,12; làm mượt correction có điều kiện, ENTRY BRAKE8ms khi góc đã xác nhận sau thẳng nhanh, cap35 khi xác nhận lại tâm60ms. Thêm STEER sau TIME. Xem [B20](speed-b20.md). Chưa hardware-verified; policy cũ bên dưới là lịch sử khi mâu thuẫn.

> **B19 hiện tại:** Base55 / Straight60 / Max85, thêm Straight vào PID Setup. Chỉ tăng thêm khi gần tâm đủ80ms, giữ recovery/bridge/phanh B18. Xem [B19](speed-b19.md). Chưa thử phần cứng; cấu hình cũ bên dưới là lịch sử khi mâu thuẫn.

> **B18 hiện tại:** giữ55/85, short curve bridge8ms/white4ms, tăng ramp thoát cua khi line giữa, thêm TIME sau Stop (N1: LIVE/LAST/DIAG/TIME). Xem [B18](speed-b18.md). Chưa thử phần cứng; policy cũ bên dưới là lịch sử khi mâu thuẫn.

> **B17 hiện tại:** defaults55/85, cua mềm giảm ít hơn khi ổn định, outward risk giảm sớm, corner32/pivot36/RECENTER30. Owner báo hết map, giảm khoảng1s;60/90 chậm hơn do phanh nhiều. Xem [B17](speed-b17.md). Các policy cũ bên dưới là lịch sử khi mâu thuẫn.

> **B16 hiện tại:** defaults Base50/Max80, giữ thuật toán/PID B15 và trần bonus55. Xem [profile B16](profile-b16.md). Chưa thử phần cứng; các cấu hình mặc định cũ bên dưới là lịch sử.

> **B15 hiện tại:** Base45/Max75, bonus có điều kiện lên55, cua mềm nhanh hơn và GAP giảm phần tốc dư. Xem [B15](speed-b15.md). B14 đã được owner xác nhận chạy map; B15 chưa thử phần cứng. Các policy cũ bên dưới là lịch sử khi mâu thuẫn.

> **B14 hiện tại:** giảm tốc theo valid shape lệch bên, nhớ cue trước khi mất line, thêm DIAG sau Stop để khoanh vùng. Xem [B14](diagnosis-b14.md). Quy tắc cũ bên dưới là lịch sử khi bị thay thế.

# Tài liệu kỹ thuật

Thư mục này là tài liệu tham chiếu chung cho người phát triển và AI coding agent.
Trước khi sửa firmware, hãy đọc mục lục này, tài liệu phần cứng và trạng thái triển khai.

| Tài liệu | Nội dung |
| --- | --- |
| [Phần cứng](hardware.md) | Phần cứng đã xác nhận, GPIO mapping và các thông số điện còn cần kiểm tra |
| [Kiến trúc C4](architecture.md) | System Context, Container, Component và cấu trúc Code hiện tại |
| [Thiết kế runtime](runtime.md) | Timing, data flow, calibration, điều khiển và recovery |
| [Quyết định kiến trúc](decisions.md) | Các Architecture Decision Record (ADR) và hệ quả |
| [Roadmap](roadmap.md) | Thứ tự triển khai và tiêu chí kiểm tra trên phần cứng |
| [Bring-up](bringup.md) | Module hiện có và checklist thử mạch |
| [Menu OLED](menu.md) | Bố cục/cách bấm kế thừa project cũ và trạng thái từng chức năng |
| [Chạy thử](run-test.md) | Polarity line đen, PD/RUN, fault code và tuning ban đầu |
| [Recovery B03](recovery.md) | Nền tảng recovery, nét đứt và mã R |
| [B09 hiện tại](cruise-gap-b09.md) | Base cruise, guide memory và giữ lệnh qua nét đứt |
| [Baseline B08](speed-b08.md) | Profile Base 40/Max 75, cua và recovery nhanh hơn, giữ policy phanh |
| [Baseline B07](speed-b07.md) | Sửa boost starvation, tăng tốc recovery và V/T/reason trên OLED |
| [Baseline B06](high-speed.md) | Speed planner, short brake, gain và thứ tự thử tốc độ cao |
| [Baseline B05](b05-recovery.md) | Lịch sử sửa regression quay vòng, sensor-based recovery, LAST RUN |
| [Xung vuông B04](square-wave.md) | Lịch sử thử nghiệm đã bị thay thế sau regression |
| [Nhật ký lỗi](error-log.md) | Lỗi đã biết, root cause, cách sửa và kiểm tra lại |
| [Validation](validation.md) | Phạm vi test tự động và phản hồi phần cứng |

## Quy ước về trạng thái và bằng chứng

- **Đã xác nhận phần cứng (Confirmed hardware)**: chủ dự án đã cung cấp thông tin; chưa đồng nghĩa đã đo kiểm trên mạch.
- **Đã triển khai (Implemented)**: có trong application source của repo này.
- **Dự kiến (Planned)**: thiết kế định hướng, chưa triển khai.
- **Chưa xác nhận (Unconfirmed)**: cần đo kiểm, xác định linh kiện hoặc hỏi chủ dự án.

Trạng thái triển khai được đối chiếu với `app/main.c`, `bsp/board.c` và `config/board.h`.
Có vendor driver trong repo không có nghĩa tính năng tương ứng đã hoạt động.
Project cũ tại `D:/STM32-LineFollower-Robot-main` chỉ dùng để tham khảo;
Bố cục menu đã được kế thừa; controller/recovery mới có implementation riêng, không sao chép PID timing hoặc gain của project cũ.

## Thiết kế I/O đã chốt

- Line 8 mắt: ADC scan + DMA, frame hoàn chỉnh mỗi 2 ms; đã triển khai, chưa kiểm chứng trên mạch.
- Ba nút: EXTI + NVIC, debounce ngoài ISR theo elapsed time; đã triển khai, chưa kiểm chứng trên mạch.
- Quyết định nút nhấn được ghi tại ADR-007 trong [decisions.md](decisions.md).

## Quy tắc cập nhật tài liệu

1. Đọc [error-log.md](error-log.md) trước khi sửa code; cập nhật khi có lỗi mới và ghi rõ bằng chứng.
2. Cập nhật tài liệu phần cứng khi thay đổi wiring; không tự suy đoán GPIO mapping.
3. Phân biệt rõ hành vi đã triển khai và hành vi dự kiến.
4. Ghi rõ đơn vị và timing cho từng control parameter và trường telemetry khi bổ sung.
5. Ghi thay đổi kiến trúc vào `decisions.md`.
6. Build thành công chỉ xác nhận compile/link, không thay thế kiểm tra phần cứng.
7. Không tự thêm encoder feedback, GPIO cho STBY, model OLED controller, sensor polarity hoặc motor forward polarity khi chưa có bằng chứng.
8. Viết tài liệu bằng tiếng Việt có dấu; giữ thuật ngữ kỹ thuật chuẩn bằng tiếng Anh, chẳng hạn firmware, GPIO, PWM, ADC/DMA, PID, calibration, debounce, runtime và C4. Giữ nguyên identifier, tên API, đường dẫn và lệnh.
