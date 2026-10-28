# B20 — Làm mượt lái và phanh sớm khi vào góc gấp

Profile thử: **Base 55 / Straight 60 / Max 85**, **Kp 12 / Ki 0 / Kd 0,12**. Owner xác nhận vẫn dùng gain này, cua mềm còn có cảm giác phanh và từ đoạn thẳng nhanh vào góc 90° còn vượt sang line khác. Chưa có trace để xác định mọi lần khựng là hạ PWM, phanh một bánh hay full brake.

## 1. Làm mượt lệnh lái có điều kiện

Trong TRACK, khi không corner pending/post-turn, mask không chạm mắt ngoài bit0/7, |error| <= 1,5 pitch và |planner_rate| <= 4 pitch/s, giới hạn tốc độ tăng độ lớn correction ở 800 điểm PWM/giây (khoảng 1,6 điểm mỗi frame 2 ms).

- Khi line trở về, correction cần nhỏ hơn được giảm ngay, không giữ lực lái cũ.
- Khi đổi dấu, bỏ hướng cũ rồi tăng lực theo hướng mới từ zero.
- Khi ra mép, drift nhanh, có corner cue hoặc recovery, PD lấy lại quyền sửa đầy đủ ngay.
- Không ép PWM bánh trong luôn dương: góc thật vẫn cần quyền giảm bánh trong về 0. Không cam kết mọi cua mềm sẽ hết phanh.
- Gain, derivative filter 15 ms và planner filter 40 ms giữ nguyên. Ki vẫn bằng 0.

## 2. ENTRY BRAKE — phanh sớm trước khi mất line

Đây là thay đổi policy so với B06–B19: cho phép một xung full brake khi line còn valid nhưng đã đủ dấu hiệu vào góc gấp. User đã cho phép triển khai sau khi thảo luận về phanh chủ động.

Điều kiện đồng thời:

1. Shape rộng lệch bên, chạm mép và có mắt giữa được xác nhận cùng phía ít nhất 6 ms.
2. Có lịch sử chạy gần tâm đủ 80 ms với common PWM thực đã xuất ít nhất 50%, còn trong cửa sổ 150 ms.
3. Common PWM trước khi kích phanh vẫn ít nhất 50%.
4. Không post-turn và chưa phanh entry trong episode góc hiện tại.

Pulse mục tiêu 8 ms ở control period 2 ms; interval test 1–4 ms làm thời lượng được lượng tử hóa lên tối đa dưới một interval thêm. Trạng thái riêng CONTROL_ENTRY_BRAKE / nhãn ENTRY BRAKE (ENT).

Nếu shape còn valid sau pulse, quay lại forward PD ở mức corner cap32; không pivot khi line valid. Nếu thấy line valid bình thường, hủy pulse sớm và nhận PD ngay. Nếu mất line trong pulse, chuyển phần thời gian phanh còn lại sang recovery theo phía đã nhớ, không phát thêm pulse12 ms. Thời gian đã dùng được tính vào recovery deadline1200 ms. Latch chống lặp, chỉ mở lại sau trusted center100 ms; Stop/Start reset state.

Phanh recovery12 ms cũ vẫn dùng khi mất line nhanh mà chưa kích entry. GPIO/TB6612 driver không đổi. Shape chỉ là heuristic: cua rộng ngay sau đoạn thẳng cũng có thể thỏa điều kiện; góc chỉ hiện vài mắt mà không tạo shape có thể không kích entry. Không có encoder để đo vận tốc/quãng phanh.

## 3. Bắt lại line sau recovery

Vẫn nhận valid line bằng forward PD ngay, không giữ quay mù hoặc buộc tiến theo geometry. Trong post-turn, target cap35 cho đến khi trusted center liên tục60 ms; sau đó mới về target nền. Post-turn guard có TTL200 ms như cũ nên nếu không đạt tâm, cap này không kéo dài vô hạn. Reloss trong guard vẫn dùng recovery episode/deadline cũ.

Điều này hạn chế lao nhanh ngay khi vừa thấy một đoạn line, không xác nhận được danh tính line. Hai đường sát nhau có thể cùng tạo mẫu sensor hợp lệ; không bảo đảm loại bỏ mọi trường hợp bắt nhầm line khác. Tốc ổn định qua chuỗi góc có thể đổi do bước xác nhận này, cần so sánh map thật.

## 4. STEER và TIME sau Stop

N1 khi dừng: **LIVE → LAST RUN → DIAG → TIME → STEER → LIVE**. N3 Start/Stop như cũ; sáu mục menu chính và PID Setup không đổi.

| STEER | Ý nghĩa |
| --- | --- |
| ZERO count | Số lần vào tình huống TRACK có đúng một bánh được yêu cầu PWM0, bánh kia tiến |
| ZERO ms | Tổng command-frame duration của tình huống trên |
| ENTRY count | Số lần kích phanh sớm |
| ENTRY ms | Tổng command-frame duration trong ENTRY BRAKE |

ZERO không phải số lần bánh thực sự dừng; có thể bao gồm startup lệch tâm và góc gắt hợp lệ. Counts không định vị phần map. Cần xem cùng thời gian/cảm giác chuyển động và TIME để kết luận. Mỗi frame được tính 2 ms; không đo wall-clock hay động lực học.

BRK trên TIME vẫn là BRAKE recovery. ENTRY có bộ đếm riêng trên STEER và được cộng vào ALL. Nếu mất line trong entry pulse, phần tiếp theo được tính BRK; tổng full-brake command time là BRK + ENTRY. Phanh khi Stop, watchdog hoặc zero update do driver đảo chiều không được tính theo hai mode này.

Diagnostics giữ sau Stop/fault, reset khi Start thành công. Display ms cap99999, API uint32 saturating; count uint16 saturating. Inner-zero dùng command state; không suy ra ADC noise hoặc bánh bị khóa thực sự chỉ từ số này.

## Validation và nạp

**34.861 assertions pass** trên Cortex-M3 emulation:
- Shape đủ6 ms mới entry; trái/phải; pulse bounded8 ms; không lặp trên shape kéo dài; re-arm sau center.
- Valid bình thường hủy entry; mất line chuyển phần pulse còn lại, không double brake; timeout episode.
- Cua thường không kích entry chỉ vì error; hết fast-history thì shape không tự phanh entry.
- Lái mềm giới hạn build-up nhưng nhả correction ngay sau reacquisition; edge bypass.
- Cap35 trong trusted-center confirmation60 ms, sau đó target về Base.
- Raw estimator → supervisor → entry output; ZERO counts, reset/freeze, watchdog khi entry active.
- Existing GAP/bridge/square/recovery/menu/driver tests.

Harness tăng instruction budget toàn suite từ200 lên300 triệu do mở rộng control/test paths; timeout60 s vẫn giữ. Test phải tới test_done, không xem lần hết budget là pass. Đây không là kiểm chứng hardware execution deadline.

Keil: **0 error / 0 warning**; Code27472 / RO1360 / RW352 / ZI2720 bytes. So backup xác nhận motor driver, ADC driver và estimator không đổi. UI dùng stub, chưa kiểm tra OLED vật lý; B20 chưa hardware-verified.

HEX: `build/linefollower-B20.hex`.
SHA256: `7C8185C7DB5BE3167706349D52A4AA1615F2D805369356E4C33AD9A3302E989D`.
Backup: `backups/B19-before-B20-20261026/source.zip` và `linefollower-B19.hex`.

Nạp, Reset, calibration + white reference rồi thử55/60/85 và gain12/0/0,12. So sánh cùng map; ghi thời gian và sau Stop xem TIME/STEER nếu còn khựng/vượt. Đừng tăng đồng thời Kp và tốc trong lần đối chiếu đầu. Gain/calibration vẫn RAM-only.
