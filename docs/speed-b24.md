# B24 — Quay trực tiếp, bỏ xung BRAKE trước recovery

Ngày28/10/2026. Owner báo vọt ở cuối thẳng/góc90°, cảm giác phanh nhiều và yêu cầu đổi sang đánh lái ngay. Profile giữ **Base55 / Straight60 / Max85**, gains **Kp12 / Ki0 / Kd0,12**.

## 1. Log mới cho biết gì?

`logs/run-20261028-182311-164428.csv`, B23 sequence3,160sample, CRC32 **3ED4E91F** hợp lệ. Trigger ở391ms sauStart, không có BRAKE/TURN trong window. PROBE30ms giữ common31,1% xuyên các mask99/FB/FF/...; rejoin31,3%, cuốiwindow54,0%. Flagsbit4 xác nhận continuation B23 hoạt động. Đây không phải bằng chứng của BRAKE ở góc nhanh về sau.

Recorder CORNER cũ nhận mọi invalid ngoàiGAP, nên lại đóng băng ở mảng đen startup. Phân tích: [log-analysis-20261028-seq3.md](log-analysis-20261028-seq3.md). Không kết luận report của owner sai; log chỉ chưa bao phủ đúng vị trí được mô tả.

## 2. Bỏ brake-before-turn

`BeginRecovery` đi thẳng vào TURN khi đã có hướng, không đặt cả hai lệnh bánh bằng0 để giữ xung12ms. BRAKE/ENTRY enums và counters chỉ giữ để giải mã log cũ, không được phát bởi controller B24.

Explicit Stop, fault, watchdog, maintenance và Motor Test Stop vẫn dùng StopBrake. Không bỏ calibration/fresh-frame interlock, deadline recovery1200ms hoặc no-auto-restart.

## 3. Mixer quay theo common/steering

Trước đây, Pivot ramp từng bánh độc lập tới±36. Nếu hai bánh đang chạy bằng nhau trên36 và không có brake reset, hai bánh cùng giảm một lượng như nhau trước khi bắt đầu tạo thêm yaw.

B24 tách:
- common = trung bình lệnh hai bánh, tiến về0 ở600điểm phần trăm PWM/giây;
- steering = nửa chênh lệch hai bánh, tiến về±min(Max,36%) ở1800điểm phần trăm/giây;
- common được giới hạn theo steering để hai output luôn nằm trong±Max.

Như vậy, từ hai bánh chạy nhanh bằng nhau, frame TURN đầu tiên đã có chênh lệch. Xe được lệnh chuyển từ cung tiến sang counter-rotation khi cần, không có thời gian chờ dừng rồi mới tạo yaw. Đây là lệnh điện, không bảo đảm thân xe thay đổi heading tức thì; quán tính/độ bám vẫn cần thử.

Hướng mới sau visible reacquisition được retarget theo policy đã có, không renew deadline. Valid line hủy TURN ngay để nhận forward PD; không minimum-spin/white-clear/geometry ADVANCE.

## 4. Cue góc valid đánh lái mạnh hơn

Khi broad asymmetric shape được xác nhận6ms và valid centroid cùng hướng ít nhất1pitch, error không đang cải thiện theo planner_rate, thêm steering floor bằng0,8×steering_limit. Đây là shape feed-forward; output vẫn forward và trongMax. Khi error cải thiện hoặc shape biến mất, floor được nhả ngay; PD tiếp tục.

Bỏ cap32 chỉ vì valid shape. Target vẫn theo CruiseTarget: positional curve/outward-risk protection còn áp dụng khi thật sự ra mép; không tăng lênStraight trong corner guard. Góc broad ổn định có thể giữ targetBase thay vì bị buộc32. Cua mềm không có cue này vẫn dùng PD như trước.

Không ép pivot khi line valid. Floor có thể cho xe rẽ sớm/quá mạnh nếu sensor/mảng đen bị nhận nhầm shape; chỉ thử ở profile hiện tại trước khi tăng tốc.

## 5. Reversal guard theo từng bánh

`bsp/motor.c` vẫn chèn zero một update trên mỗi channel đổi dấu, nhưng chỉ zero bánh đang đảo chiều. Khi bánh trong đổi từ tiến sang lùi, bánh ngoài vẫn nhận lệnh tiến, tránh ngắt cả xe.

Nếu cả hai channel đảo dấu cùng lúc, cả hai vẫn có thể zero trong một update. Bánh trong về0 vẫn là short brake điện ở channel đó. Vì vậy “bỏ phanh” ở đây là bỏ pulse trước cua và tránh dừng bánh ngoài không cần thiết, không phải mọi output0 hoặc mọi giảm duty bị cấm.

TB6612 polarity/PWM/STBY/wiring không đổi.

## 6. CORNER trigger có ý nghĩa rõ hơn

CORNER chỉ trigger với:
- pending corner hoặc valid |error|≥1,5pitch;
- TURN/recovery có hướng;
- fault.

Invalid PROBE không có hướng và GAP đơn thuần không trigger CORNER nữa; chúng vẫn được giữ trong prehistory vòng. Nếu chưa có cue, khi Stop vẫn đóng băng đoạn cuối để Save. LOSS giữ cách bắt invalid ngoàiGAP để chẩn đoán mảng đen/khựng.

CORNER vẫn là cue, không chứng minh hình học90°; cua mềm lệch đủ lớn cũng có thể trigger. Cửa sổ vẫn160sample, khoảng0,64s, không ghi cảmap. Nếu cần góc cụ thể, chọn vị tríStart phù hợp.

Record48/header108/Flash layout/CSVv1 giữ nguyên. Script CH340 hiện tại đọc đượcB24. Gain/PID không tự tune theo log.

## 7. Validation

- Regression mới trên B23 fail80023: high-duty loss vẫn BRAKE; B24 chuyểnTURN và tạo differential ngay.
- **92.268 assertions pass**, **8 parser/layout +18 receiver tests pass**.
- Tests mới kiểm tra dt1..4/Max20,55,85/hai hướng; first-frame yaw, steady pivot, đổiMax khiđangTURN, validline authority và faultStop; valid corner floor/release; per-wheel và simultaneous reversal guards; CORNER bỏ qua startup ambiguity nhưng bắt cue/fault.
- Các regression repeated-square, pending/side memory/deadline, GAP/bridge, startup/Stop/watchdog, Flash và PC tools vẫn pass.
- Raw replay B22 seq2 vẫn xấp xỉ min32,0/rejoin32,1/end54,8% common request; không dùng replay này để kết luận độ bám cua B24.
- Keil **0 error /0 warning**, Code33468 /RO1856 /RW440 /ZI10496byte.
- HEX kết thúc trước **0x08008AD0**, không chạm log Flash0x0800C000.
- Artifact: `build/linefollower-B24.hex`.
- SHA256: `5B05781C3E7814AD2E46E5D81EE96CB0A5FDD6699E848F78AA4531F9749592EB`.
- Backup: `backups/B23-before-B24-20261028/source.zip` và HEXB23.
- Chưa hardware-verified B24; test không mô phỏng quán tính, lực kéo hay xác định góc thân xe.

## 8. Thử xe

1. Giữ log đã Export trên PC, nạp B24, Reset và calibration/white reference lại.
2. Giữ55/60/85 và12/0/0,12. Kiểm tra Motor Test/Stop trước khi chạy map vì reversal guard đã đổi theo từng channel.
3. Arm next run / Trigger CORNER, Start trước đoạn thẳng dẫn tới góc cần xem. Stop → Save → Export bằng scriptCH340 hiện có.
4. Xem log có TRACK/CORNER vàTURN với differential sớm; BRAKE không phải mode được controller phát. Khi cònvalidshape, target không còn bị cap32 chỉ vìshape.
5. Thử riêng góc trái/phải và chuỗi góc vuông liên tiếp trước khi tăng Straight. Ghi thời gian nhiều vòng và vị trí overshoot nếu có.

B24 chủ động bỏ pulse nên xe có thể tiến xa hơn nếu chưa nhận đủ hướng hoặc lực quay không đủ. Không giữ nguyên tốc độ thẳng qua mọi góc bằng mọi giá. Không chọn trái/phải từFF hoặc centroidinvalid; khi góc chưa có hướng đáng tin, boundedPROBE/GAP/recovery vẫn cần thiết.
