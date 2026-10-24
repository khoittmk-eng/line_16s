> **Baseline B06:** owner đã thử cua mềm ổn nhưng đường thẳng chậm và góc 90° quay lâu. B07 hiện tại xem [speed-b07.md](speed-b07.md); các threshold/ramp/pivot B06 bên dưới là lịch sử.

# B06: speed planner và short brake

## Trạng thái và default

Owner báo Max 40% bám line được; tăng tốc làm văng ở góc 90°. Bộ gain tạm 2/0/5 trong source
được owner xác nhận xe lắc, không qua góc. Chưa có trace đồng bộ trên xe để kết luận một nguyên nhân duy nhất.
B06 là bản thử mới, chưa hardware-verified; không cam kết Max 60% hoặc 90% qua mọi góc.

| Parameter | Default B06 | Đơn vị / ý nghĩa |
| --- | --- | --- |
| Kp | 12.0 | % PWM / sensor pitch |
| Ki | 0.00 | Tắt integral khi tune; không dùng I để xử lý quán tính |
| Kd | 0.12 | % PWM / (sensor pitch/giây), D filter 15 ms |
| Base | 30% | Target khi chưa cho phép boost; không là tốc độ tối thiểu |
| Max | 60% | Target đường thẳng đã xác nhận và trần từng wheel output |

Đây là trial gains, không phải tuning đã đo trên GA25. Error đổi 0,2 pitch/2 ms tạo D filtered
ban đầu khoảng 11,8 pitch/s; Kd=5 tạo riêng D gần 59% PWM trước giới hạn lái, có thể gây saturation/lắc.
Không chép gain từ code dùng D là error difference không chia dt. Ki giữ 0 để đánh giá PD.
PID Setup chỉnh như cũ, RAM-only; System N1 Reset Defaults lấy bộ B06.

## Planner

- Cập nhật D trước planner trong cùng frame; risk tăng theo |error| từ 0,25 đến 1 pitch,
  |D| từ 3 đến 12 pitch/s; nhóm valid rộng từ 4 mắt cũng yêu cầu giảm tốc.
- Boost cần 300 ms gần tâm, D nhỏ, không có nhóm rộng. Risk >=0,5 hoặc invalid hủy boost;
  cooldown 250 ms và cần xác nhận thẳng lại sau cua.
- Khi boost còn được phép, target nội suy Base..Max theo risk; nếu không dùng Base.
- |error| >1 hoặc nhóm rộng: target min(Base × 0,8; 25%). |error| >2: min(Base × 0,7; 22%).
- Ramp tăng 40, giảm 600 điểm phần trăm PWM/giây. Command 90→21 cần khoảng 115 ms
  nếu điều kiện giảm duy trì, thay vì 690 ms ở ramp cũ. Đây không là thời gian dừng cơ học.
- Có line valid: forward PD tiếp tục lái, không quay/phanh hai bánh theo timer.

Target nói trên là common PWM; bánh ngoài có thể cao hơn khi PD lái. Wheel mixing có headroom,
giới hạn từng bánh theo Max. Bánh trong về 0 ở cua gắt là short brake, không đo được bánh đã đứng.

## Phanh TB6612

Theo [Toshiba datasheet, H-SW Control Function trang 4](https://toshiba.semicon-storage.com/info/datasheet_en_20141001.pdf?did=10660):

| Request | IN1 / IN2 | PWM | STBY |
| --- | --- | --- | --- |
| Drive / PWM short brake | Hai mức đối nhau | High drive, low short brake | High cố định |
| Full short brake | High / High | Low | High cố định |
| Coast | Low / Low | High | High cố định |

Driver duty=0 dùng explicit High/High, thay tổ hợp Low/Low/PWM Low không được liệt kê rõ trong bảng.
Giảm PWM khi vẫn thấy line tăng phần short brake trong chu kỳ 20 kHz, vẫn giữ PD authority.
Khi đã mất line phía đã biết và đủ điều kiện recovery, nếu common request hoặc trung bình output
còn >40%, BRAKE xuất duty=0 hai bánh tối đa 12 ms, rồi pivot đối xứng +20%/-20% theo hướng đã biết.
Nếu entry duty thấp thì bỏ full brake pulse. Valid line trong pulse hủy phanh ngay → RECENTER
bằng forward PD common target <=20%. Không blocking delay, không đảo motor ở PWM cao để phanh.
Chuyển dấu vẫn có một update brake trong motor driver. Pulse là trial parameter, không là phép đo dừng bánh.
STBY nối 3V3; không có STBY GPIO. Chưa đo current, wheel slip hoặc quãng phanh trên xe.

## GAP và recovery

GAP sau mẫu gần tâm vẫn tối đa 200 ms, chỉ giảm về Base, không tăng duty/full brake mỗi nét đứt.
Mỗi lần invalid hủy boost. Ambiguous PROBE tối đa 80 ms.
BRAKE + TURN + RECENTER chung deadline 1.200 ms, flicker không reset deadline.
Valid line nhận ngay PD, không white-clear/minimum-turn hoặc ADVANCE B04.
Đồng bộ biến common PWM khi pivot; RECENTER không mang biến speed cao của đường thẳng vào lái lại.

Một hàng sensor không phân biệt chắc chắn gap và góc cụt khi lịch sử sensor giống nhau.
Mất line đột ngột từ giữa vẫn áp dụng GAP; planner không nhìn trước ngoài hàng sensor,
không có wheel speed/heading/position feedback. Cần thử tốc độ phù hợp trên map thật.

## Thứ tự thử

1. Nạp `build/linefollower-B06.hex`, Reset, kiểm tra OLED B06; calibration + white reference lại.
2. Kiểm tra sửa lái/Stop ở duty thấp; có thể đặt Base 25/Max 40 để so baseline đã chạy được.
3. Thử Base 30/Max 50 rồi Max 60, Kp 12/Ki 0/Kd 0,12; thử cả cua trái/phải và nét đứt.
4. Chỉ tăng Max từng 5% sau khi cùng góc qua được nhiều lần. Chưa nhảy thẳng 90%.
5. Lắc khi còn thấy line: cố định speed để tune PD. Lao quá góc sau đoạn nhanh: giảm Max,
   đối chiếu planner/phanh và LAST RUN, không chỉ tăng Kp.
6. Stop rồi nhấn ngắn N1 ở Run xem LAST RUN. B là số full brake pulse đã yêu cầu,
   không phải lực/quãng phanh; B=0 vẫn có PWM drive/brake và không phải mọi cua đều cần full pulse.

Emulation kiểm tra logic và register request, không mô phỏng động lực học GA25, ma sát hay sensor thực.
