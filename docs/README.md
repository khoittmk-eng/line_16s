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
