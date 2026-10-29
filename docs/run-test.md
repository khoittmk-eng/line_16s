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
