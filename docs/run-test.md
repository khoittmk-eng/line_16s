> **B14 hiện tại:** giảm tốc theo valid shape lệch bên, nhớ cue trước khi mất line, thêm DIAG sau Stop để khoanh vùng. Xem [B14](diagnosis-b14.md). Quy tắc cũ bên dưới là lịch sử khi bị thay thế.

# Chạy thử bám line đen

## Trạng thái

Owner đã xác nhận manual calibration và Motor Test hoạt động trên xe.
Owner báo tín hiệu line bị ngược: nền trắng mạnh hơn line đen.
B05 không dùng một giả định raw HIGH/LOW cố định làm kết luận. Sau min/max calibration, lấy white reference riêng từng mắt để map nền trắng về strength thấp và line đen về strength cao. Trước white reference, mặc định tạm là raw LOW = line. White reference được dùng làm mốc strength=0; nhiễu nền dưới 15% bị đưa về 0 cả trên OLED lẫn thuật toán.
Owner báo B03 cải thiện cua/khoảng trắng nhưng cắt chéo chuỗi góc vuông. B05 bỏ ADVANCE/timing geometry vì B04 regression; ưu tiên line feedback và quay cân hai bánh khi mất line. Xem [b05-recovery.md](b05-recovery.md).

## Thứ tự thử

1. Build `Ctrl+Shift+B`, nạp `build/linefollower-B09.hex` hoặc Download từ project Keil mới rồi Reset. Main Menu và Run phải hiện **B09** để nhận diện bản nạp.
2. Vào **Calibration** bằng N2, nhấn **N3** bắt đầu, quét đủ tám mắt qua đen/trắng, nhấn **N3** kết thúc. Khi hiện PLACE ALL ON WHITE, đặt **cả 8 mắt trên nền trắng** rồi nhấn ngắn **N2**. Phải hiện WHITE REF OK / CALIB DONE.
3. Giữ **N2** về menu, dùng N1/N3 đưa dấu > đến **Sensor Monitor**, nhấn **N2** vào: nền trắng cột thấp, line đen cột cao; CAL:1 W:1.
   Nếu thực tế vẫn ngược, giữ **N1** để đổi polarity; không đảo dây motor để sửa lỗi cảm biến.
4. Đặt line dưới giữa hàng mắt: `V:1`, error gần 0. Dịch line sang hai phía và kiểm tra error đổi dấu.
5. Vào **PID Setup**, kiểm tra default B09 Base 45/Max 75, Kp 12.0, Ki 0.00, Kd 0.12. Base là target cruise, Max chỉ cap mỗi bánh; không tự boost. Xem [cruise-gap-b09.md](cruise-gap-b09.md).
6. Tại Main Menu đưa dấu > đến **1. Run**, nhấn **N2 để vào Run**. N1 chỉ chuyển xuống mục kế tiếp, không phải Enter. Kê bánh khỏi mặt đất cho lần kiểm tra hướng lái đầu tiên.
   Đặt line dưới mắt giữa rồi nhấn/ngả **N3** để Start. Nhấn N3 lần nữa để Stop.
7. Khi line nằm bên phải xe, bánh trái phải nhận PWM cao hơn bánh phải; bên trái thì ngược lại.
   Nếu motor đã tiến đúng nhưng sửa lái bị ngược: Stop, giữ **N1 tại màn Run** để đổi `DIR:+/-`, rồi thử lại.
8. Đặt xe xuống mặt đường trống và thử ở Base/Max hiện tại. Không tăng gain và duty cùng lúc.

Không có auto-start khi cấp nguồn hoặc sau fault. Calibration và parameter chỉ lưu RAM,
phải calibration lại sau reset. Auto Calib chưa được triển khai; dùng Manual như trên.

## Thao tác RUN

- N3 nhấn ngắn khi đã vào màn Run và đang dừng: Start nếu calibration + white reference hợp lệ, line đang thấy rõ, ADC mới và OLED hoạt động.
- Khi đang chạy, N3 dừng ngay sau khi sự kiện pressed được debounce; lần nhả không khởi động lại.
- Giữ N2: Stop và về Main Menu.
- Giữ N1 khi đã Stop/Fault: đảo steering sign, không đổi chiều chạy tới của motor.
- Muốn chỉnh gain: Stop, về PID Setup, sửa rồi quay lại Run.

## Nếu nhấn N3 mà chưa chạy

Main Menu không chạy xe bằng N3. Dùng N2 vào đúng màn RUN MODE B09 trước.
Khi bị chặn Start, màn Run hiển thị nguyên nhân hiện tại:

| Thông báo | Cần làm |
| --- | --- |
| CALIBRATE FIRST | Vào Calibration, thực hiện min/max sweep |
| SET WHITE REF FIRST | Sau sweep đặt cả 8 mắt trên trắng, nhấn N2 trong màn Calibration |
| PLACE ON BLACK LINE | Đưa line đen về vùng giữa, kiểm tra chỉ nhóm mắt trên đen nhận line |
| ADC ERROR: RESET | Xem ADC status, reset sau debug halt hoặc capture fault |
| BASE/MAX IS ZERO | Đặt duty khác 0 trong PID Setup |
| READY N3 START | Đã đủ điều kiện, nhấn/ngả N3 để Start |

`CAL:1` chỉ là min/max calibration đủ contrast; cần `W:1` xác nhận white reference.
Nếu white reference báo SWEEP/WHITE RETRY, đưa cả hàng ra nền trắng hoàn toàn và nhấn N2 lại;
nếu vẫn lỗi, làm lại sweep đủ cả đen và trắng cho từng mắt.

## Hệ số và đơn vị

| Parameter | Mặc định | Ý nghĩa |
| --- | --- | --- |
| Kp | 12.0 | % PWM trên mỗi khoảng cách một mắt |
| Ki | 0.00 | % PWM trên tích phân error theo giây; mặc định tắt |
| Kd | 0.12 | % PWM trên tốc độ thay đổi error, đơn vị khoảng cách mắt/giây |
| Base | 45% | Target cruise khi thẳng/cua nhẹ; giảm liên tục ở error lớn |
| Max | 75% | Chỉ giới hạn PWM từng bánh, không tự thành target |

Estimator xuất error -3500..3500, một khoảng mắt = 1000 unit.
Controller dùng error/1000 và dt = 0.002 s. Không dùng lại gain ESP32 hoặc project STM32 cũ.
D được low-pass với time constant 15 ms; khởi tạo lại khi Start và khi bắt lại line.
Integral contribution giới hạn ±10% PWM và có conditional integration; Ki=0 vẫn là PD.
Gain/parameter trên menu có tác dụng thực trong RUN, không còn chỉ là UI storage.
Các mặc định chỉ để bắt đầu thử, chưa được tuning trên GA25 thực tế.

## Planner và mất line

B09 valid line luôn PD; target Base khi abs(error)<=1,5 pitch, từ 1,5→3 giảm tuyến tính về min(Base;28%).
Không boost/cooldown. Ramp khởi hành 80, giảm 600 điểm/giây. RECENTER min(Base;26%).
GAP tối đa 300 ms giữ đúng L/R từ guide gần tâm mới nhất, không hạ Base/full brake mỗi nét.
Split mask giữa hàng sensor grace 12 ms tính chung vào span. Trong GAP thấy đen là nhận lại PD ngay,
không cần Start; quá allowance gây fault cần Start rõ ràng, không tự khởi động lại.
Mất line phía đã biết xác nhận 6 ms; entry PWM cao thì BRAKE tối đa 12 ms trước pivot cân hai bánh 30%, cap Max.
Có line trở lại nhận RECENTER ngay; không buộc white-clear/min-turn. BRAKE/recovery/RECENTER deadline 1.200 ms.
Xem [cruise-gap-b09.md](cruise-gap-b09.md).

## Hiển thị và fault

Run hiển thị READY, TRACK, GAP, BRAKE, SEARCH, TURN, RECENTER, PROBE hoặc FAULT; error, `F`, `R`, `DIR`, PWM request hai bánh và B (số full brake pulse). Thêm V/T/label để xem common PWM request/target và nguyên nhân giữ tốc; xem [speed-b07.md](speed-b07.md). Khi F=2, xem R để biết gap timeout, turn timeout hoặc ambiguous timeout; bảng chi tiết ở [recovery.md](recovery.md).

| F | Ý nghĩa | Xử lý |
| --- | --- | --- |
| 0 | Không có fault | READY chờ N3 |
| 1 | ADC stale / DMA error | Kiểm tra `debug_frame`; lỗi DMA/capture cần Reset |
| 2 | Không tìm lại line hoặc line ambiguous | Đặt lại trên line rõ, N3 Start lại |
| 3 | Acquisition interval sai hoặc control execution quá lâu | Xem timing_errors, reset sau debug halt và kiểm tra timing |
| 4 | Foreground/UI không phục vụ trên 100 ms | Kiểm tra code foreground; không tự Start lại |
| 5 | OLED lỗi khi RUN | Kiểm tra I2C, Reset để khởi tạo lại màn |

SysTick dừng output khi không có frame mới trên 10 ms hoặc UI mất heartbeat trên 100 ms.
Motor output còn có lease 20 ms: nếu chủ output không refresh command thì brake.
Fault được kiểm tra lại ngay trước khi ghi PWM, tránh ISR cấp thấp ghi đè lệnh Stop của watchdog.
`Robot_GetStatus()` cung cấp frames, max_execution_us và timing_errors để đo thực trên board.
Halt debugger có thể gây fault timing/DMA; Reset để chạy thử lại bình thường.

## Kiểm thử tự động

Build firmware: `powershell -File scripts/build.ps1`.
Kiểm tra thuật toán và supervisor bằng ARMCC + Unicorn (không mô phỏng peripheral):

```powershell
python -m pip install --target build/test-deps unicorn==2.1.4 pyelftools==0.33
powershell -File scripts/test-control.ps1
```

Test chạy C đã biên dịch cho Cortex-M3, bao gồm polarity, calibration rejection,
centroid validity, acceleration ramp, output bounds, gap/reacquisition, search timeout,
Start/Stop và watchdog ADC/UI. Test không thay thế kiểm tra motor, ADC hay I2C trên mạch.

Sau Stop, nhấn ngắn N1 trong Run để xem LAST RUN: mode, error, mask, PWM request và B trước dừng.
B09 không chạy theo kích thước map 16 cm/40 cm. Parameter vẫn RAM-only. Snapshot giữ V/T/reason; xem [cruise-gap-b09.md](cruise-gap-b09.md).
