> **B09:** wiring/driver short brake/STBY nối 3V3 giữ nguyên. Owner báo nét đứt/cua nhẹ khựng và chọn Base cruise; guide memory không là encoder/odometry. B09 chưa hardware-verified; xem [cruise-gap-b09.md](cruise-gap-b09.md).

# Đặc tả giao tiếp phần cứng

Trạng thái: chủ dự án xác nhận linh kiện và wiring dưới đây vào ngày 24/10/2026.
Owner đã xác nhận Motor Test và manual calibration hoạt động. Hướng đặt hàng cảm biến và dấu sửa lái của RUN mới cần kiểm tra; mặc định polarity đã đảo theo phản hồi đen yếu/trắng mạnh.

## Danh sách phần cứng

| Thiết bị | Số lượng | Thông số / vai trò |
| --- | --- | --- |
| MCU | 1 | STM32F103C8T6; target memory: 64 KiB Flash, 20 KiB SRAM |
| Line sensor array | 1 | Tám analog output, từ A1 đến A8 |
| Motor driver module | 1 | TB6612FNG, channel A và B |
| Động cơ DC giảm tốc | 2 | GA25, thông số 620 RPM, không có encoder |
| Màn hình OLED | 1 | Owner xác nhận SSD1306; tham chiếu cấu hình cũ 128×64, I2C1 remap, 400 kHz, address 7-bit 0x3C |
| Nút nhấn | 3 | Chọn chế độ và thao tác menu |

620 RPM là thông số động cơ, không phải tốc độ bánh xe được đo trong runtime.
Phần cứng hiện tại không có wheel position feedback hoặc wheel speed feedback.

## GPIO mapping của MCU

| Chân STM32 | Tín hiệu kết nối | Chức năng / peripheral |
| --- | --- | --- |
| PA0 | Sensor A1 | Mắt dò line 1; ADC1_IN0 |
| PA1 | Sensor A2 | Mắt dò line 2; ADC1_IN1 |
| PA2 | Sensor A3 | Mắt dò line 3; ADC1_IN2 |
| PA3 | Sensor A4 | Mắt dò line 4; ADC1_IN3 |
| PA4 | Sensor A5 | Mắt dò line 5; ADC1_IN4 |
| PA5 | Sensor A6 | Mắt dò line 6; ADC1_IN5 |
| PA6 | Sensor A7 | Mắt dò line 7; ADC1_IN6 |
| PA7 | Sensor A8 | Mắt dò line 8; ADC1_IN7 |
| PA8 | TB6612 AIN2 | Motor A direction input 2 |
| PA9 | TB6612 AIN1 | Motor A direction input 1 |
| PA10 | TB6612 BIN1 | Motor B direction input 1 |
| PA11 | TB6612 BIN2 | Motor B direction input 2 |
| PB0 | BT1 | Nút nhấn 1 |
| PB1 | BT2 | Nút nhấn 2 |
| PB6 | TB6612 PWMB | Motor B PWM; TIM4_CH1 |
| PB7 | TB6612 PWMA | Motor A PWM; TIM4_CH2 |
| PB8 | OLED SCL | I2C1_SCL, remap |
| PB9 | OLED SDA | I2C1_SDA, remap |
| PB10 | BT3 | Nút nhấn 3 |

A1..A8 chỉ xác định thứ tự channel điện. Thứ tự vật lý từ trái sang phải chưa được xác nhận.
PA9/PA10 đã dành cho motor driver: không bật USART1 TX/RX mặc định trên các chân này.
PB6/PB7 đã dành cho PWM: OLED phải dùng I2C1 remap sang PB8/PB9.

## Interrupt mapping cho nút nhấn

Thiết kế đã chốt: PB0 → EXTI0, PB1 → EXTI1, PB10 → EXTI10.
NVIC quản lý các IRQ `EXTI0_IRQn`, `EXTI1_IRQn` và `EXTI15_10_IRQn`.
Dùng EXTI edge interrupt để phát hiện nhấn/nhả; debounce xử lý bên ngoài ISR.
Active level và pull resistor chưa được xác nhận. Driver EXTI/NVIC đã triển khai; cấu hình tạm active-low theo project tham chiếu, chưa thử trên mạch.

## Nguồn và wiring của TB6612

| Tín hiệu TB6612 | Kết nối | Ý nghĩa |
| --- | --- | --- |
| VM | VBAT | Nguồn động cơ |
| VCC | 3V3 | Nguồn logic |
| GND | GND chung | Chung GND với MCU và pin |
| STBY | Nối trực tiếp 3V3 | Driver luôn được enable khi có nguồn |
| AO1 | Motor L, đầu 1 | Output động cơ trái |
| AO2 | Motor L, đầu 2 | Output động cơ trái |
| BO1 | Motor R, đầu 2 | Output động cơ phải |
| BO2 | Motor R, đầu 1 | Output động cơ phải |
| PWMA | PB7 | PWM bánh trái |
| AIN1 | PA9 | Direction bánh trái |
| AIN2 | PA8 | Direction bánh trái |
| PWMB | PB6 | PWM bánh phải |
| BIN1 | PA10 | Direction bánh phải |
| BIN2 | PA11 | Direction bánh phải |

**Không có GPIO điều khiển STBY từ firmware.** Không cấp phát chân hoặc viết logic disable driver qua STBY.
Chức năng stop sau này phải dùng direction/PWM input.
Khi triển khai driver, cần định nghĩa riêng coast và brake; cả hai không có nghĩa bánh xe dừng cơ học ngay lập tức.
Kiểm tra forward direction độc lập cho từng động cơ.

## Các chi tiết chưa xác nhận

| Thông tin | Lý do cần xác nhận |
| --- | --- |
| Điện áp VBAT, rated voltage và stall current của GA25 | Chỉ thông số RPM chưa đủ xác định giới hạn hoạt động của driver và động cơ |
| Model, nguồn và analog output range của cảm biến | Xác định tương thích điện với ADC và yêu cầu sampling |
| Black/white signal polarity và màu line | Xác định calibration và cách tính line strength |
| Hướng vật lý A1..A8 | Xác định dấu error và chiều sửa lái |
| OLED controller và init profile trên mạch mới | Owner gọi SSD1306; source cũ tên SH1106. Giữ legacy init và 128×64/0x3C từ source cũ, cần thử hiển thị |
| I2C pull-up và nguồn của pull-up | Xác định bus level và độ tin cậy |
| Button active level và pull resistor | Cấu hình input và debounce |
| Tần số thạch anh HSE | Firmware hiện giả định 8 MHz, target 72 MHz |
| LED PC13 và active level | Heartbeat hiện giả định LED active-low |
| Đường kính bánh, khoảng cách hai bánh, sensor offset/height | Phục vụ control tuning và mô hình hình học |

Owner đã xác nhận calibration và Motor Test của bản trước. Polarity đã sửa và RUN mới chưa được kiểm chứng trên xe thực tế.

## Geometry owner cung cấp cho B04

Hàng 8 mắt ở trước trục hai bánh chủ động khoảng 16 cm theo chiều dọc xe; đoạn line giữa hai góc khoảng 40 cm. Đây là kích thước gần đúng, không có wheel-speed feedback. Thời gian ADVANCE cần hiệu chỉnh trên xe, xem [square-wave.md](square-wave.md).

B05: các số 16 cm/40 cm là ước lượng có thể chưa chính xác; không dùng để tính lệnh tiến/quay trong firmware hiện tại. Xem [b05-recovery.md](b05-recovery.md).

## B22 — Cổng lấy log CH340

PB12 được dành cho Software UART TX → CH340 RXD; GND nối chung. 9600 baud, 8N1, logic 3,3 V. Không nối CH340 TXD; không nối chân nguồn CH340 khi STM32 có nguồn riêng. Chỉ Export khi stopped; không đổi GPIO của sensor/motor/OLED/nút. ADC được pause trong Save/Export và resume với frame mới trước Start. CH340 không thay ST-Link để nạp firmware. Xem [blackbox-b22.md](blackbox-b22.md).
