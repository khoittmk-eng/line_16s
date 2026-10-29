# B25 — Fixed Base và hướng quay trong chuỗi góc vuông

## Trạng thái

Owner báo B24 văng line khi hết đoạn thẳng và ở các góc vuông liên tục. B25 được sửa theo yêu cầu chạy Base cố định, không tự boost trên đường thẳng. Đã build/test source; **chưa chạy thử B25 trên xe**. Không khẳng định hết văng line hoặc giảm thời gian vòng.

## Hành vi mới

| Tình huống | B25 |
| --- | --- |
| TRACK thẳng, cua mềm hoặc line lệch ra mép | Target bằng Base, giới hạn bởi Max; bỏ Straight bonus, curve/risk target scheduling |
| Vừa Start | Vẫn ramp lên Base ở 80 điểm phần trăm/giây, không cấp ngay duty lớn |
| Cua có line hợp lệ | PID tính lại mỗi frame 2 ms; bỏ steering floor 0,8 của B24, không ép quay theo timer |
| Góc đã xác nhận, sau đó mất line | Chuyển TURN ngay khi không có cue mới mâu thuẫn; không thêm thời gian tiến hoặc xung BRAKE trước TURN |
| TURN | Request hai bánh đối dấu, mỗi bánh min(Max, 36%); bỏ ramp common tiến của B24 |
| Cue split mới ngược hướng pending | Chờ xác nhận liên tiếp 6 ms như trước, sau đó ưu tiên hướng mới; không lấy centroid từ mask invalid |
| Line xuất hiện khi đang TURN | Nhận forward PD ngay trong RECENTER, target min(Base, 30%); không minimum-spin/white-clear |
| Vừa hoàn tất RECENTER | Giữ cap 35% cho tới khi tâm ổn định 60 ms, trong cửa sổ bảo vệ post-turn; không boost giữa hai góc |
| Nét đứt sau bám tâm | Giữ đúng lệnh bánh gần nhất trong GAP có giới hạn 300 ms; không tự Start lại sau fault |
| Invalid ambiguous không biết hướng | Giữ cơ chế bridge/PROBE có giới hạn của B23; không chạy Base cao mù qua mọi vùng trắng/đen rộng |

**Fixed Base áp dụng cho TRACK bình thường, không có nghĩa hai bánh luôn cùng PWM hoặc mọi mode đều dùng Base.** Mixer vẫn ưu tiên correction: `common = min(speed, Max - abs(correction))`, rồi chia ra `common ± correction`. Vì vậy khi rẽ gắt hoặc chạm Max, trung bình PWM có thể thấp hơn Base. Đây là điều kiện tạo chênh lệch hai bánh, không phải chu kỳ boost rồi phanh. Xe không có encoder nên duty không phải tốc độ đo được.

Per-wheel reversal guard của motor driver giữ nguyên: bánh đổi dấu có một update PWM zero; bánh còn lại tiếp tục. Stop/fault/watchdog vẫn brake. Request quay ngay không làm quán tính cơ học biến mất.

## Sửa hướng quay ở các góc liên tiếp

B24 ưu tiên dấu `last_error` khi độ lệch lớn hơn 0,35 pitch. Một mắt giữa có thể cho ±0,5 pitch: sau khi đã xác nhận góc phải, một mẫu giữa lệch trái có thể làm xe tìm line sang trái. Trong ALIGN cũng có nhánh ghi đè hướng tương tự.

B25 giữ hướng pending hoặc hướng vừa quay trước sai số yếu gần tâm. Quan sát mạnh từ 1,2 pitch hoặc split đã xác nhận vẫn có thể đổi hướng theo góc kế tiếp. Khi có split ngược hướng pending, phải cho bộ xác nhận 6 ms hoàn tất; không vào TURN hướng cũ ở frame đầu rồi xóa lịch sử cue mới. Giữ recovery deadline 1.200 ms, không gia hạn vô hạn bằng line flicker.

## Cấu hình và thao tác

- OLED phải hiện **B25**. PID Setup còn **Kp, Ki, Kd, Base, Max**; bỏ dòng Straight. N1/N3 chọn hoặc giảm/tăng, N2 chọn chỉnh như trước.
- Mặc định **Base 55%, Max 85%, Kp 12, Ki 0, Kd 0,12**. Không tăng gain cùng lúc để phân biệt ảnh hưởng thuật toán.
- `straight_percent` vẫn tồn tại để giữ compatibility của CSV/Flash, được menu đồng bộ bằng Base, controller không dùng nó để boost.
- Source Keil: `D:\stm32-linefollower-spl\linefollower.uvprojx`. Firmware: `build/linefollower-B25.hex`.
- Sau nạp/reset: calibration min/max, white reference, kiểm tra Sensor rồi Run → N3 Start/Stop. Gain/calibration vẫn RAM-only.
- Export log cũ trước khi reflash nếu cần giữ lại. Cho lần chạy mới: System → BLACK BOX → Trigger CORNER → Arm next run; sau Stop chọn Save RAM to Flash rồi Export bằng script PC.

Thử cùng Base 55/Max 85 trên nét đứt, cua mềm, góc đơn và chuỗi góc vuông. Sau khi ổn định mới tăng riêng Base thêm 5 điểm và so sánh thời gian/văng line; không có cam kết Base 60 hoặc cao hơn sẽ bám được chỉ nhờ đổi thuật toán.

## Validation

- Regression đầu tiên chạy với controller B24 gốc: fail **90048**, confirmed corner → mẫu giữa trái dấu → trắng 6 ms, quay sai hướng.
- B25: **97.520 assertions** từ C compile ARMCC trên Cortex-M3 emulation; **8 parser/layout + 18 receiver tests** pass.
- Có test hai phía, góc kế tiếp ngược hướng, ALIGN/post-turn reloss, pending/split conflict, fixed Base với Straight cũ tùy ý, bỏ floor, Max/dt, timeout, GAP, bridge, menu và watchdog.
- Keil: **0 Error, 0 Warning**; Code 32.312 / RO 1.856 / RW 440 / ZI 10.496 bytes. HEX kết thúc `0x0800864C`, dưới vùng log Flash `0x0800C000`.
- SHA256 HEX: `04DA54E352EB2A420335E4B2276238D0DEC1E2C09526A20BF859103619D52BE6`.
- Không sửa ADC/estimator, motor driver, blackbox format, Flash layout hay supervisor. Tests không mô phỏng ma sát, quán tính, dòng motor hoặc nhận đúng nhánh line ngoài thực tế.

Backup source/HEX trước sửa nằm trong `backups/B24-before-B25-20261028`. Tài liệu B04 có ADVANCE theo geometry và từng bị báo regression; B25 không khôi phục logic đó.
