# B15 — Tăng tốc có giới hạn trên nền B14

Ngày 26/10/2026. Owner xác nhận B14 Base45/Max75 chạy cả map; Base60/Max80–90 vượt line và dừng ở khoảng trắng. Chưa có trace xác định mọi nhánh dừng. B15 được owner yêu cầu, chưa hardware-verified.

## Cấu hình và hành vi

Giữ Base45, Max75, Kp12, Ki0, Kd0,12. Max vẫn là wheel-output ceiling. Sau80ms tracking đủ điều kiện, bonus tăng dần tối đa10 điểm PWM, target tối đa55%; cấp bonus phải còn15 điểm dưới Max. Nếu tự đặt Base trên55 thì không nhận bonus nhưng cũng không bị tự hạ về55: lần thử đầu cần giữ45/75.

Bonus tăng20/giảm100 điểm PWM mỗi giây; output ramp nền vẫn tăng80/giảm600. Mắt giữa ±0,5 pitch và jitter nhỏ vẫn được tăng tốc. Cua ổn định error1,2 pitch có target khoảng53% thay45%; ra xa tâm hơn thì bonus giảm liên tục. Filtered outward rate ngoài vùng giữa0,8 pitch cũng giảm bonus, không thay derivative PID.

Giữ curve blend B14: error1,5→3 pitch giảm nền dần về28%. Shape rộng lệch bên xác nhận6ms vẫn cap28, giữ forward PD. Sau shape gấp, error từ2,4 pitch hoặc recovery, chặn bonus300ms từ lần quan sát cuối. Đây chỉ là chặn bonus, không ép28% suốt300ms. Pivot30%, RECENTER26%, brake12ms và recovery deadline1200ms giữ nguyên.

## Nét đứt

Giữ điều kiện trusted guide/tuổi80ms, pending/post-turn guard, GAP tối đa300ms và central split grace12ms.

- 60ms đầu giữ exact L/R đã ghi, không tăng PWM trên trắng.
- Sau60ms chỉ giảm common duty dư trên Base với60 điểm PWM/giây, giữ tỷ lệ L/R. Không hạ lệnh vốn đã thấp hơn Base.
- Với55→45, khoảng227ms từ đầu GAP thì về45. Bắt lại line giữa sau gap ngắn giữ tốc đã có, tránh target tụt mỗi mép đen. Sau gap dài đã về45 thì xác nhận tracking lại trước khi tăng.
- Quá300ms vẫn recovery có giới hạn hoặc fault theo memory cũ. Không auto-start hoặc chạy mù vô hạn. Nét đứt ngay sau cua vẫn có thể kích recovery theo heuristic B14.

## Validation và nạp

34.709 assertions pass trên Cortex-M3 emulation: target55, central-eye/jitter, cua mềm53, guard giữa góc, chuỗi gap ngắn không sụt duty, gap dài giảm đúng tỷ lệ về45, output bounds, recovery/deadline, estimator/supervisor, menu, watchdog và motor register tests.

Keil build: 0 error/0 warning; Code25316, RO1280, RW344, ZI2632 bytes.
HEX SHA256: A3AF4C2F55FF7146775A45DA4775E18BA09AD77AA104F7C38D7EEDC845A29117.

Nạp `build/linefollower-B15.hex`, Reset, calibration + white reference, giữ DIR đã kiểm chứng, Run/N3 Start/Stop. Gain/calibration vẫn RAM-only. Backup B14: `backups/B14-before-B15-20261026/source.zip` và `linefollower-B14.hex`.

So sánh thời gian cùng map và số lần trượt ở45/75 trước khi tăng tiếp. Không cần đọc OLED khi chạy; DIAG xem sau Stop. Test logic không đo quãng phanh/vận tốc. Một hàng sensor không nhìn được góc chưa tới; chưa bảo đảm bám map ở tốc độ cao hơn.
