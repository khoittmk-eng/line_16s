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

> **Cập nhật B09:** owner báo nét đứt/cua nhẹ khựng, chọn Base cố định. Default Base 45/Max 75 (trần), giữ guide L/R qua GAP 300 ms, giảm liên tục ở cua gắt; phanh/recovery giữ policy trước. Xem [cruise-gap-b09.md](cruise-gap-b09.md), [run-test.md](run-test.md), [runtime.md](runtime.md). B09 chưa hardware-verified; nội dung phiên bản cũ bên dưới là lịch sử.

# Bring-up phần cứng và trạng thái hiện tại

Owner đã xác nhận manual calibration và Motor Test hoạt động trên xe.
Owner tiếp tục báo polarity ngược ở bản RUN trước. B02 mặc định tạm raw LOW, bổ sung white reference sau sweep để xác nhận polarity riêng từng mắt thay vì suy đoán.
Owner đã thử RUN bám vòng tròn/cua nhẹ ở Base 20%; cua 90° bị dừng. B03 đã cải thiện nhưng còn cắt chéo chuỗi góc; B04 thêm ADVANCE/ALIGN và tuning UI theo offset sensor 16 cm, xem [square-wave.md](square-wave.md). Bước chạy thử xem [run-test.md](run-test.md).
Manual calibration, gain và steering sign chỉ lưu RAM, mất khi Reset/tắt nguồn.

## Module hiện có

| Module | Source | Hoạt động |
| --- | --- | --- |
| Timebase | `bsp/timebase.c` | SysTick 1 ms, DWT, watchdog hook |
| Motor | `bsp/motor.c` | TIM4 20 kHz, duty permille, brake/coast, command lease |
| ADC | `bsp/line_adc.c` | TIM3 TRGO 2 ms, ADC1 scan, DMA hai nửa frame |
| Buttons | `bsp/buttons.c` | EXTI hai cạnh, debounce 25 ms, long press 600 ms |
| OLED | `bsp/oled.c` | Legacy init, page refresh foreground, transaction timeout |
| Estimator | `app/line_estimator.c` | Calibration min/max, strength 0..1000, weighted average và validity |
| Controller | `app/control.c` | PD/PID giới hạn, derivative filter, ramp và bounded gap/recovery |
| Supervisor | `app/robot.c` | Frame callback, Start/Stop, snapshot, watchdog và timing metrics |
| Menu | `app/menu.c` | Menu cũ, calibration, PID Setup, Motor Test và Run |

## Data flow

```text
TIM3 mỗi 2 ms → ADC scan PA0..PA7 → DMA HT/TC IRQ
    → copy frame hoàn chỉnh → Robot_OnFrame → estimator → controller → PWM

Foreground → EXTI event/debounce → menu → yêu cầu robot/parameter
Foreground → snapshot → OLED page transfer
SysTick → ADC/UI watchdog + motor output lease
```

NVIC: SysTick=0, DMA1 channel 1=1, button IRQ=3, grouping 4.
DMA buffer có 16 sample, chia hai frame 8 sample; callback chỉ đọc nửa đã hoàn tất.
HT/TC cùng pending hoặc DMA error làm dừng acquisition. Lỗi capture cần Reset.
Không có I2C, formatted printing hoặc delay trong control callback.
Snapshot foreground và thay đổi calibration/parameter dùng critical section ngắn.

## OLED và input

- I2C1 remap PB8/PB9, 400 kHz, 128×64, address 7-bit `0x3C` (SPL nhận `0x78`).
- Giữ legacy init của project cũ, column offset 0; profile cấu hình trong `config/board.h`.
- Mỗi lần gửi một page, cách nhau ít nhất 10 ms; timeout tổng mỗi transaction là 6 ms.
- OLED lỗi sẽ ngừng refresh đến Reset; RUN dừng nếu phát hiện lỗi màn hình.
- Nút cấu hình pull-up, active-low kế thừa project cũ.
- Manual calibration: mục 4, N3 bắt đầu/kết thúc sweep; sau đó đặt cả 8 mắt trên nền trắng, nhấn ngắn N2 lấy white reference. Chỉ Run khi CAL:1 W:1.
- Yêu cầu calibration: tối thiểu 50 frame và contrast mỗi mắt ít nhất 200 ADC count.
- Sensor Monitor: giữ N1 đổi polarity nếu cần; Run: giữ N1 khi Stop đổi steering sign.

## Checklist trên mạch

1. Xác nhận đen mạnh/trắng yếu sau calibration; line giữa cho validity và error gần 0.
2. Xác nhận Motor Test tiến đúng cho từng bánh, không tự chuyển sang RUN.
3. Kê bánh và kiểm tra dấu sửa lái ở Base 30%, Max 45% trước khi chạy dưới đất.
4. Kiểm tra N3 Stop và giữ N2 Stop/Back.
5. Đo frame period và `max_execution_us`, thử OLED refresh không làm trễ control.
6. Thử mất line, gap ngắn, timeout và ADC/OLED fault độc lập.
7. Ghi revision và kết quả vào [error-log.md](error-log.md).

Debugger xem `debug_frame`, `debug_line`, `adc_ready`, `adc_stale`, `oled_ready`,
`button_short_count`, `button_long_count` và `Robot_GetStatus()` snapshot.
Halt debugger có thể gây timing/DMA fault; Reset sau debug halt khi kiểm tra chạy thực.
