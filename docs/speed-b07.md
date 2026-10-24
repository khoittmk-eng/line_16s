> **Baseline B07:** B08 tăng profile tốc độ, giữ planner/phanh; xem [speed-b08.md](speed-b08.md). Các giá trị Base/Max/cap/pivot B07 bên dưới là lịch sử.

# B07: sửa điều kiện boost và tăng tốc recovery

## Phản hồi và bằng chứng

Owner đã thử B06: cua mềm ổn, góc 90° đi được nhưng quay lâu, đường thẳng vẫn chậm.
Chưa có trace thực để xác định tất cả nguyên nhân trên xe. Review và regression tìm thấy:

- B06 chỉ xác nhận thẳng khi |error| <0,25 pitch trong 300 ms. Một mắt giữa đơn lẻ cho error ±0,5,
  nên line vẫn valid mà boost không bao giờ được bật, dù Max lớn hơn Base.
- D của PD được dùng trực tiếp cho quyết định boost. Jitter giữa mắt có thể reset điều kiện liên tục.
- B06 coi mọi nhóm từ 4 mắt là cua, kể cả nhóm 0x3C nằm đối xứng giữa hàng sensor.
- Test mới giữ error=500/mask=0x10 tái hiện B06 không boost, fail 40037 trước khi sửa.

Nhánh BRAKE không hoạt động khi line valid. Điểm đã xác nhận là planner giữ tốc thấp,
không phải hai tác vụ độc lập cùng lúc ghi phanh và tăng tốc. Motor chỉ nhận output cuối từ controller.

## Thay đổi B07

| Mục | B06 | B07 |
| --- | --- | --- |
| Base / Max mặc định | 30 / 60% | 30 / 60%, giữ để so cùng mức |
| Kp / Ki / Kd | 12 / 0 / 0,12 | Giữ nguyên |
| Corridor nhận thẳng | abs(error) <0,25 pitch | abs(error) <=0,65 pitch |
| D dùng cho planner | D của PD | D qua thêm low-pass 40 ms, chỉ dùng cho planner |
| Xác nhận thẳng | 300 ms liên tục | 180 ms; sai lệch nhẹ làm giảm counter thay vì xóa ngay |
| Cooldown sau risk/invalid | 250 ms | 150 ms |
| Risk vừa | Hủy boost ngay | Xác nhận 8 ms; risk giảm thì counter giảm nhanh |
| Error >1 hoặc nhóm rộng ra bên | Giảm tốc | Giảm ngay, không đợi 8 ms |
| Nhóm rộng cần giảm tốc | Mọi nhóm >=4 mắt | >=4 mắt và chạm ít nhất một mắt thuộc mask 0xC3 |
| Ramp tăng / giảm | 40 / 600 điểm %/giây | 65 / 600 điểm %/giây |
| Pivot target | ±20% | ±24%, vẫn cap theo Max |
| Pivot ramp | 250 điểm %/giây | 350 điểm %/giây |

PD vẫn dùng derivative filter 15 ms và gain cũ; không dùng filter planner để làm chậm phản ứng lái.
Risk vị trí bắt đầu trên 0,65 pitch, đạt 1 ở 1 pitch; risk rate dùng |D planner| từ 3 đến 12 pitch/s.
Ngưỡng hủy boost là risk >=0,5 được xác nhận; error >1 hoặc nhóm rộng ra bên là hard condition.
Khi boost được phép, target nội suy Base..Max theo risk. |error| >1 hoặc nhóm rộng ra bên vẫn
min(Base × 0,8; 25%); |error| >2 vẫn min(Base × 0,7; 22%). Đây là common target, không là trần từng bánh.

Nhịp BRAKE 12 ms khi entry recovery PWM cao, điều kiện nhả ngay khi line valid, GAP 200 ms,
PROBE 80 ms, RECENTER 20% common target/Reacq 12 ms và recovery deadline 1.200 ms giữ nguyên.
Không geometry ADVANCE, không kéo dài minimum-turn và không ép quay khi line valid.
Pivot duty cao hơn là trial parameter; không cam kết góc/quãng đường/thời gian cơ học chính xác.

## OLED và LAST RUN

Giữ sáu menu và thao tác nút cũ. Run thêm dòng `Vxx Txx LABEL`:

- `V`: biến common PWM request hiện tại, **không phải vận tốc đo được**. Headroom/mixing có thể làm
  trung bình L/R thấp hơn V; nhìn thêm PWM từng bánh.
- `T`: common target mà planner đang hướng tới.
- `LABEL`: nguyên nhân chọn target, theo bảng dưới.
- `B`: số full brake pulse đã yêu cầu; không đếm phần brake trong mỗi chu kỳ PWM.

| Label | Ý nghĩa |
| --- | --- |
| OFF | Đã Stop/fault |
| BASE | Chưa đủ xác nhận thẳng, target Base |
| BST | Boost được phép; target có thể tới Max khi risk nhỏ |
| CURV | Giảm vì error lớn hoặc nhóm line rộng ra bên |
| WAIT | Đang cooldown, target Base |
| GAP / PRB | Đang qua khoảng trắng / ambiguous probe |
| REC / BRK | Recovery/RECENTER / full brake pulse |

Sau Stop nhấn ngắn N1 xem LAST RUN: dòng đầu như `TRK BST V60 T60`, còn error/mask/PWM/SAT/B/F.
Mode viết ngắn TRK=TRACK, TRN=TURN, REC=RECENTER, BRK=BRAKE. Snapshot mới giữ cả V/T/label.
N3 Start/Stop, giữ N2 Back và giữ N1 đổi DIR khi đã Stop vẫn hoạt động như trước.

## Chạy thử

1. Nạp `build/linefollower-B07.hex`, Reset và xác nhận OLED B07. Calibration + white reference lại.
2. Thử cùng Base/Max đã dùng ở B06; default vẫn 30/60, gain vẫn 12/0/0,12.
3. Kiểm tra đoạn thẳng có BST, T tới Max, V tăng dần; không cần tăng Base để ép vượt planner.
4. Thử cua mềm, 90° hai phía và nét đứt. Pivot target tăng lên 24%; line xuất hiện phải nhận lại PD ngay.
5. Nếu Max 60 ổn qua nhiều vòng, tăng Max từng 5% rồi thử lại; chưa mặc định 90%.
6. Nếu còn chậm, dừng và xem LAST RUN V/T/label cùng PWM/B. Không thể suy ra vận tốc hoặc quãng phanh chỉ từ PWM.

Backup source/artifact B06 tại `backups/B06-before-B07-20261024`.
Regression kiểm tra logic; B07 cần thử trên xe, không mô phỏng quán tính, heading hoặc wheel slip.
