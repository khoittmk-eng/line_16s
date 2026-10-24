> **B14 hiện tại:** giảm tốc theo valid shape lệch bên, nhớ cue trước khi mất line, thêm DIAG sau Stop để khoanh vùng. Xem [B14](docs/diagnosis-b14.md). Quy tắc cũ bên dưới là lịch sử khi bị thay thế.

# STM32 Line Follower — B09 / SPL

STM32F103C8T6, line analog 8 mắt, TB6612, hai GA25 620 RPM không encoder,
OLED I2C và ba nút EXTI/NVIC. Sửa bằng VS Code, build bằng Keil ARMCC 5.

## Trạng thái hiện tại

B09 chạy Base cruise, bỏ tự boost/cooldown; giữ lệnh L/R đáng tin qua nét đứt tối đa 300 ms.
Có line valid thì forward PD lái; không dùng ADVANCE/geometry timing hoặc ép quay theo map.
Giữ calibration + white reference, ADC/DMA 2 ms, OLED menu, Motor Test và watchdog.
Mặc định thử Base 45%, Max 75% (trần từng bánh), Kp 12, Ki 0, Kd 0.12; chỉnh trên OLED, chưa lưu Flash.
Owner báo nét đứt/cua nhẹ khựng và chọn chạy Base cố định; B09 cần thử lại trên xe thực.

Đọc [hướng dẫn B09](docs/cruise-gap-b09.md), [menu](docs/menu.md) và
[nhật ký lỗi](docs/error-log.md) trước khi chạy hoặc sửa firmware.
Không coi test giả lập là xác nhận xe chạy tốt trên phần cứng.

## Build và nạp

- VS Code: Ctrl+Shift+B gọi `scripts/build.ps1`; hoặc mở `linefollower.uvprojx` trong Keil.
- CLI không cần mở uVision: `powershell -File scripts/build-headless.ps1`.
- Cả hai dùng ARMCC 5, target Cortex-M3, Flash 64 KiB và RAM 20 KiB.
- Nạp `build/linefollower-B09.hex` hoặc Download AXF từ Keil; kiểm tra OLED hiện B09.
- Build task không tự flash. Source/artifact B08 giữ tại `backups/B08-before-B09-20261024`.

Sau nạp: Calibration N3 start/end → cả 8 mắt lên trắng → N2 WHITE REF.
N2 vào Run, N3 Start/Stop. Kiểm tra hướng sửa lái ở PWM thấp trước khi tăng tốc.
Sau Stop nhấn ngắn N1 để xem LAST RUN (mode/V/T/reason/error/mask/PWM/B trước dừng).

## Cấu trúc

| Folder | Vai trò |
| --- | --- |
| `app/` | Estimator, controller, supervisor và menu |
| `bsp/` | ADC/DMA, TB6612, EXTI buttons, OLED, timebase |
| `config/` | Board pin/configuration |
| `startup/`, `vendor/` | Startup, SPL và CMSIS; giữ license/provenance |
| `scripts/` | Build và emulation tests |
| `docs/` | Kiến trúc, phần cứng, hướng dẫn và error log |
| `tests/` | Regression tests; không được link vào firmware |

Xem [mục lục tài liệu](docs/README.md). STBY nối trực tiếp 3V3, không có STBY GPIO.
