> **Baseline B08:** owner báo nét đứt/cua nhẹ khựng, chọn Base cruise. B09 hiện tại xem [cruise-gap-b09.md](cruise-gap-b09.md); speed planner/cap/GAP cũ bên dưới là lịch sử.

# B08: tăng profile tốc độ, giữ policy phanh

Owner báo Base 30% và cua còn chậm, muốn tăng cả tracking lẫn góc 90°; owner xác nhận phanh ổn trên xe đã thử.
B08 tăng duty profile, không thay thuật toán/phần cứng phanh. Các target mới chưa hardware-verified.

## Thông số

| Mục | B07 | B08 |
| --- | --- | --- |
| Base mặc định | 30% | 40% |
| Max mặc định | 60% | 75% |
| Acceleration ramp | 65 điểm %/giây | 80 điểm %/giây |
| Target cua thường khi error >1 hoặc nhóm rộng ra bên | min(Base × 0,8; 25%) | min(Base × 0,8; 32%) |
| Target cua gắt khi error >2 | min(Base × 0,7; 22%) | min(Base × 0,7; 28%) |
| Pivot recovery | ±24% | ±30%, cap theo Max |
| Pivot ramp | 350 điểm %/giây | 450 điểm %/giây |
| RECENTER khi bắt lại line | min(Base; 20%) | min(Base; 26%) |
| Kp / Ki / Kd | 12 / 0 / 0,12 | Giữ nguyên |

Với default B08, cua thường target 32%, cua gắt 28%, RECENTER 26%.
Tăng Base một mình ở B07 vẫn bị cap 25/22/20%; B08 nâng đồng bộ các cap này.
Các mức trên là common PWM request, không là vận tốc đo được hoặc trần từng bánh.
Wheel mixing/headroom có thể làm L/R khác common PWM. Target pivot là duty hai bánh ngược chiều,
không phải cam kết đã quay đúng 90° hoặc nhanh hơn một tỷ lệ cơ học cụ thể.

## Phần giữ nguyên

- Driver TB6612: explicit short brake ở duty=0; STBY vẫn nối trực tiếp 3V3.
- Entry recovery PWM cao >40% dùng BRAKE tối đa 12 ms, valid line hủy pulse ngay.
- Deceleration ramp 600 điểm %/giây, loss confirm 6 ms, GAP 200 ms, PROBE 80 ms.
- BRAKE + recovery + RECENTER chung deadline 1.200 ms; Reacq 12 ms, không thêm minimum-spin/ADVANCE.
- Planner B07: corridor 0,65 pitch, rate filter riêng 40 ms, confirm thẳng 180 ms,
  cooldown 150 ms, risk vừa xác nhận 8 ms; valid line luôn forward PD.
- PD filter 15 ms, gain không đổi; calibration/white reference, Start/Stop và watchdog giữ nguyên.
- Menu và V/T/reason/LAST RUN giữ nguyên; Base/Max sửa trên OLED, RAM-only.

Đã so source với backup: `bsp/motor.c`, hàm BeginRecovery/RecoveryOutput và các constant phanh/deceleration/
line-loss/recovery deadline không thay đổi. Profile nhanh hơn có động lực học khác; cần thử lại trên map thực.

## Nạp và thử

1. Nạp `build/linefollower-B08.hex`, Reset, kiểm tra OLED B08.
2. Calibration + white reference lại; xác nhận PID Setup Base 40/Max 75, gain 12/0/0,12.
3. N3 Start trong Run. Thử đường thẳng, cua mềm, 90° trái/phải và nét đứt với profile mới.
4. Nếu còn chậm, Stop rồi N1 xem LAST RUN: V/T/label phân biệt BASE/BST/CURV/WAIT/REC/BRK.
   V/T là PWM request/target; đọc thêm PWM L/R, không suy ra tốc độ thực khi xe chưa có encoder.
5. Ghi lại label và V/T ở đoạn còn chậm hoặc văng; chưa có dữ liệu để bảo đảm Max 90% phù hợp map này.

Build Keil 0 error/warning; 30.223 assertion emulation pass. Có test Base 40, curve target 32/28,
RECENTER 26, pivot 30%, low-Max cap, pulse cancel, dashes, Start/Stop và watchdog.
Emulation không mô phỏng quán tính, current, ma sát hoặc wheel slip.

Backup source/artifact B07: `backups/B07-before-B08-20261024`.
Nền tảng planner và ý nghĩa telemetry xem [speed-b07.md](speed-b07.md); B07 là lịch sử profile.
