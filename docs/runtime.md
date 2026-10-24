> **B14 hiện tại:** giảm tốc theo valid shape lệch bên, nhớ cue trước khi mất line, thêm DIAG sau Stop để khoanh vùng. Xem [B14](diagnosis-b14.md). Quy tắc cũ bên dưới là lịch sử khi bị thay thế.

# Runtime B09

## Execution và ownership

TIM3 tạo ADC frame mỗi 2 ms; DMA HT/TC copy frame hoàn chỉnh rồi gọi Robot_OnFrame.
Estimator/calibration/controller thuộc callback; UI đọc snapshot và gửi request qua API robot.
Foreground xử lý EXTI event/debounce và OLED, không tính PD.
SysTick 1 ms giám sát ADC/UI và lease motor. Không có I2C/printf/delay trong callback control.

| Tác vụ | Cơ chế |
| --- | --- |
| Acquisition | TIM3 → ADC1 scan 8 channel → DMA hai nửa frame |
| Control | Estimator → PD/PID → bounded wheel PWM theo frame |
| Buttons | EXTI/NVIC, debounce ngoài ISR 25 ms, long press 600 ms |
| OLED | Foreground, mỗi lần một page có timeout |
| Stop | Fault latch được kiểm tra lại ngay trước khi ghi PWM |

NVIC: SysTick 0, DMA1 channel 1=1, buttons=3. Snapshot/parameter update dùng critical section ngắn.
ADC stale 10 ms, UI heartbeat 100 ms, motor command lease 20 ms; DMA error/capture fault cần Reset.
Acquisition interval RUN phải trong 1..4 ms; execution control quá 1.000 us Stop với timing fault.
Không auto-start sau boot/fault. Start kiểm tra calibration, white reference, line validity và duty khác 0.

## Sensor

Calibration min/max mỗi mắt, white reference có nhãn màu để xác định polarity từng channel.
Strength 0..1000, nền reference=0, black endpoint=1000, noise deadband dưới 15%.
Centroid cho error -3500..3500; controller dùng error/1000.
Một nhóm liên tục là valid; toàn nền, full-width hoặc hai nhóm tách biệt là invalid.
Steering DIR đảo thì đảo cả error và mask. Không đổi sensor polarity để sửa sai chiều motor.

## Controller

B09 giữ Kp 12, Ki 0, Kd 0.12; default Base 45/Max 75, chưa hardware-verified.
D filter time constant 15 ms; integral contribution giới hạn ±10% và conditional integration.
Tracking dùng forward-only mixing và headroom, ramp tăng 80 điểm phần trăm/giây, giảm giữ 600 điểm/giây.
Không boost/cooldown: target Base khi abs(error)<=1,5 pitch; từ 1,5 đến 3 pitch giảm tuyến tính
về min(Base; 28%). Max chỉ là trần từng bánh, không tự là target đường thẳng.
PD giữ D filter 15 ms. Rate qua filter 40 ms chỉ dùng đánh giá guide memory, không gate tốc độ cruise.
Khi line valid, kể cả error lớn hoặc nhiều mắt về mép, PD vẫn lái; không chuyển sang góc theo timer.
SAT counter chỉ chẩn đoán, không tự quay khi line còn valid.

## Recovery

- Guide lưu L/R từ frame valid gần tâm/hai bánh tiến; TTL entry 80 ms, invalidate ngay khi edge/drift rõ.
- GAP tối đa 300 ms giữ nguyên lệnh guide L/R mỗi frame, cap Max; không giảm Base, không tăng tốc/full brake.
- Split mask giữa hàng sensor có grace 12 ms tính trong span; ambiguous kéo dài trở lại PROBE.
- Hết GAP allowance là line fault cần Start lại, không quay dựa vào dấu error nhỏ của một mắt giữa.
- Mất line ở mép xác nhận 6 ms rồi pivot target hai bánh bằng nhau và ngược chiều 30%, cap Max; ramp 450 điểm/giây.
- Nếu entry recovery còn common request hoặc trung bình output >40%, BRAKE tối đa 12 ms trước pivot.
- Driver duty=0 dùng IN1=IN2=High/PWM Low để short brake rõ ràng; STBY vẫn nối 3V3.
- Valid line trong BRAKE hủy pulse ngay, nhận RECENTER bằng PD common target min(Base; 26%).
- Có line valid trở lại: RECENTER ngay bằng PD chạy tới, không tiếp tục ép pivot vì điều kiện clear/min-turn cũ.
- Reacq mặc định 12 ms ở vùng giữa → TRACK, reset derivative để tránh kick.
- BRAKE + recovery + RECENTER cùng deadline 1.200 ms, không reset bởi flicker.
- Ambiguous mask: PROBE tối đa 80 ms, sau đó recovery nếu có hướng hoặc Stop nếu không.
- B04 ADVANCE và geometry-to-time không được dùng, kể cả parameter cũ khác 0.

Run/LAST RUN thêm V/T/reason (common PWM request/target và nguyên nhân); không đo được vận tốc.
LAST RUN giữ B là số full brake pulse; không đo được lực/quãng phanh hoặc tốc độ thực.
Pivot đồng bộ biến speed với magnitude PWM request, RECENTER không mang speed cao của đường thẳng vào lái lại.
Trong GAP hợp lệ, thấy valid line nhận TRACK ngay/reset D/I, không cần Start lại.
Một hàng sensor không phân biệt hoàn toàn gap/góc cụt. Không dùng delay blocking hoặc odometry giả.
Xem [B09](cruise-gap-b09.md); [B08](speed-b08.md) và các bản trước là lịch sử baseline/thử nghiệm.
