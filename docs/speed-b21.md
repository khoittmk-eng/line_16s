# B21 — Vừa đánh lái vừa giảm tốc khi vào cua

Ngày: 28/10/2026. Profile giữ **Base 55 / Straight 60 / Max 85**, PID **12 / 0 / 0,12**.

## Thay đổi
Owner báo mất thời gian ở chuỗi tăng tốc → phanh → chạy lại và yêu cầu chuyển hướng mượt. Source B20 có ENTRY_BRAKE đưa cả hai PWM về 0 trong 8 ms ngay khi xác nhận broad shape sau đoạn thẳng nhanh. Đây là hành vi xác định trong source; chưa có trace để quy mọi lần khựng thực tế cho nhánh này.

B21 bỏ trigger, state và timing của ENTRY_BRAKE. Khi line valid, controller luôn tiếp tục forward PID, kể cả confirmed shape: hai bánh nhận correction ngay và common duty tiến về target theo ramp, không đặt cả hai bánh về 0 chỉ vì nhận góc. Góc broad vẫn cap target 32%; không giữ nguyên tốc độ thẳng qua mọi góc 90°.

Ở moderate track (không pending/post-turn, không outer bit, |error| ≤ 1,5 pitch, |planner_rate| ≤ 4 pitch/s), nếu target ≥ Base, deceleration giới hạn 120 điểm phần trăm PWM/s thay vì 600. Acceleration vẫn 80. Khi line đi ra mép, outward nhanh, target xuống dưới Base hoặc có corner/recovery, dùng lại deceleration 600 ngay. Giữ steering build-up slew 800, nhả correction ngay khi cần và bypass slew ở mép/góc.

Đây là chuyển tiếp lệnh bánh theo sensor feedback, **không phải bộ lập quỹ đạo parabol**. Xe không có encoder/IMU và chưa xác nhận geometry; không tính quãng đường hay bán kính cua từ thời gian. PWM không phải tốc độ đo được. Khi correction lớn, bánh trong vẫn có thể về 0 để rẽ gấp; không ép minimum PWM làm mất quyền lái.

## Những giới hạn giữ nguyên
- Shape confirm 6 ms, pending direction 200 ms, post-turn protection và cap 35 trong giai đoạn xác nhận center 60 ms.
- Chuỗi cua không được cấp GAP dài ngay sau turn; hướng mới phải được quan sát, không chạy tiếp theo hướng cũ vô hạn.
- Nét đứt: giữ exact guide 60 ms đầu, giới hạn GAP 300 ms, sau đó recovery có giới hạn.
- Mất line thật ở duty cao vẫn có BRAKE 12 ms; valid line hủy brake/pivot và nhận lại forward PID.
- Recovery deadline 1.200 ms, explicit Start/Stop, calibration, ADC/UI watchdog, fixed frame 2 ms giữ nguyên.
- Enum ENTRY_BRAKE và ô diagnostics ENTRY được giữ tương thích layout; B21 không phát mode này, counters luôn 0. Không dùng số 0 này để kết luận không có phanh cơ học.

## Validation
- 37.251 assertions pass trên Cortex-M3 emulation của C biên dịch ARMCC.
- Regression mới: broad valid hai phía ở dt 1–4 ms không full brake, lệnh bánh có hướng và trong Max; đổi hướng góc liên tiếp rồi loss giữ hướng mới; ramp soft và edge bypass; raw estimator → supervisor không ENTRY; watchdog vẫn dừng.
- Các test cũ về GAP, hồi phục, deadline, menu, driver và interlock tiếp tục pass.
- Keil: 0 error / 0 warning; Code 27096, RO 1360, RW 352, ZI 2712 bytes.
- HEX: `build/linefollower-B21.hex`.
- SHA256: `0A13F5FE8D04DD3896F1DC78A3CC409AEDD64AD4AAB2AA14BD36C2BF41F7453E`.
- Backup: `backups/B20-before-B21-20261028/source.zip` và HEX B20.
- Chưa hardware-verified; test không mô phỏng quán tính, độ bám bánh hay khoảng cách phanh.

## Thử trên xe
1. Nạp B21, Reset, calibration và white reference như cũ; kiểm tra tag B21.
2. Giữ 55/60/85 và 12/0/0,12 để so sánh riêng thuật toán.
3. Thử cua mềm, thẳng → góc 90°, chuỗi góc vuông hai chiều và nét đứt. Ghi thời gian nhiều vòng với pin/mặt đường tương đương.
4. Sau Stop, xem TIME (BRK/TRN/REC) và STEER (ZERO). ENTRY phải bằng 0; BRK vẫn có thể tăng nếu mất line.
5. Chỉ tăng Straight từng bước sau khi cả map ổn định. Nếu vọt sang line khác, giữ/tạm giảm Straight và ghi vị trí xảy ra; một hàng sensor không bảo đảm xác định đúng nhánh line.
