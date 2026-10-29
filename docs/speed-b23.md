# B23 — Giảm khựng khi line tách nhóm trong thời gian ngắn

Profile giữ **Base55 / Straight60 / Max85**, PID **12 / 0 / 0,12**. Firmware tag **B23**. Không thay calibration, polarity, estimator, motor driver, ADC period, PID gains hoặc recovery brake.

## Bằng chứng từ log B22

Owner cung cấp `logs/run-20261028-160554-654293.csv`:160sample, CRC32 **B4544906**, sequence2. Khi mask từ18 chuyển98/D9/FB/FF/... trong30ms, estimator đánh dấu invalid, controller vào PROBE với target15. Common PWM request giảm33,5→15,5%; khi valid lại chỉ15,7%, cuối cửa sổ38,4%. Không có BRAKE/TURN hoặc lệnh bánh bằng0 trong window.

Test mới tái hiện trên B22 fail **70051** tại điều kiện giữ target trong đoạn ambiguity; gain không đổi. Phân tích gốc: [log-analysis-20261028-seq2.md](log-analysis-20261028-seq2.md).

## Thay đổi controller

Trước khi đi vào một continuous loss, chỉ cấp allowance khi đủ:
- Guide hợp lệ, tuổi không quá10ms và trước đó centered tracking ít nhất40ms.
- Last valid error không quá0,8pitch, |planner_rate| không quá4pitch/s, cả hai lệnh guide dương.
- Không recovery/post-turn, pending/hint corner hoặc speed guard.
- Invalid mask vẫn chứa **cả hai mắt giữa**, không có asymmetric SplitDirection cue ngay tại entry.

Khi đủ điều kiện, PROBE có thể tiếp tục tối đa **32ms tính theo lost_ms**:
- Target tối đa **min(Base,32%,duty hiện tại)**; tuyệt đối không tăng tốc khi invalid.
- Duty thẳng cao vẫn giảm ngay theo deceleration600điểm phần trăm/giây; không giữ nguyên80% qua vùng không rõ line.
- Giữ tỷ lệ hai bánh từ guide đáng tin cuối cùng, không dùng centroid invalid và không nhân correction0,85 mỗi frame trong allowance này.
- Counter lost/ambiguous và các deadline vẫn tiến; allowance không gia hạn theo đổi mask.
- Mất hai mắt giữa, chuyển trắng hoặc hết32ms sẽ hủy allowance. Việc xuất hiện lại mask giữa trong cùng episode không cấp lại allowance.
- Confirmed split direction sau6ms và pending corner có ưu tiên trước nhánh này; TURN/BRAKE/recovery vẫn theo policy cũ.
- Nếu ambiguity kéo dài, trở lại PROBE15 và giới hạn80ms như trước. Không chọn hướng từ maskFF.
- Line valid trở lại nhận PD ngay. Muốn cấp allowance mới sau flicker phải kiếm lại centered history40ms; chuỗi góc có memory/guard không được đi tắt theo allowance này.

B18 moderate curve bridge và B09 GAP vẫn riêng biệt, giữ giới hạn cũ. Thay đổi này không biến nhiều nhóm/FF thành một centroid hợp lệ, không thêm geometry advance và không có timed pivot khi valid.

## Telemetry / công cụ PC

Giữ enumPROBE và reasonPRB. Trong cửa sổ được tiếp tục có giới hạn, `flags` bit4 (giá trị16) được bật khi PROBE; bit này bị xóa ở terminal row. Target giúp phân biệt mức tiếp tục32 với PROBE15 cũ.

Record vẫn48byte, header108byte, CSV version1 và Flash layout không đổi. Script nhận/check CRC hiện tại dùng được với cả B22/B23. Firmware tag trong CSV giúp xác định bản chạy. Export log cũ trước khi nạp lại vì full erase có thể xóa Flash log.

## Validation

- **86.667 assertions pass** từ ARMCC C chạy Cortex-M3 emulation; **8 parser/layout +18 receiver tests pass**.
- Fixture `tests/fixtures/b22-corner-seq2.h` chứa160 raw frame và calibration của log thật. Estimator phải tái tạo đúng mask/valid/error ở từng sample valid. Replay hai steering signs.
- Các frame2ms trung gian bị thiếu trong log4ms được xấp xỉ bằng last-known raw; khoảng trước sample đầu cũng dùng first-known raw để warmup. Đây là kiểm tra phản ứng phần mềm, không mô phỏng xe và không phải đo gain thời gian vòng.
- Kết quả replay xấp xỉ B23: common nhỏ nhất trong ambiguity **32,0%**, tại rejoin **32,1%**, cuối window **54,8%**. Đây là command requests trong emulator; các số15,5/15,7/38,4% là từ log B22 thật, nên không gọi đây là so sánh hai lượt hardware.
- Tests thêm: dt1..4, hai phía, ratio/bounds, tốc cao giảm ngay, tốc thấp không tăng khi invalid, trắng/anchor loss/budget expiry, stale guide/high rate/guard/pending/post-turn bypass, flicker không rearm, confirmed split ưu tiên, telemetrybit4 và Stop reset.
- Tất cả regression cũ về repeated-square, recovery deadline, GAP, Start/Stop, watchdog, Flash/serial và menu tiếp tục pass.
- Keil **0 error /0 warning**. Code33416 / RO1856 / RW440 / ZI10496byte.
- HEX kết thúc trước **0x08008A9C**, không chạm log Flash từ0x0800C000.
- Artifact: `build/linefollower-B23.hex`.
- SHA256: `5437B0D893D2FEEB214A25C52FE546DEDF9D5228E0FC705C5B7E01D680B67076`.
- Backup B22: `backups/B22-before-B23-20261028/source.zip` và HEX B22.
- Budget emulator giữ300M/60s; synthetic tests seed moving duty và earn48ms guide thay vì warmup dài không cần thiết. Suite phải hoàn tất test_done; không suy ra WCET trên board.
- Chưa hardware-verified B23.

## Thử trên xe

1. Giữ bản CSV B22 trên PC, nạp B23, Reset và calibration/white reference lại.
2. Giữ55/60/85 và12/0/0,12. Chưa tăng tốc cùng lượt đổi thuật toán.
3. Arm next run / Trigger CORNER, chạy cùng đoạn đã ghi log, Stop → Save → Export qua script CH340.
4. So sánh PROBE target, common và duration; B23 có thể vẫn PROBE nhưng không rơi15 trong điều kiện allowance đủ. Nếu cue không đủ tin cậy, PROBE15 vẫn là hành vi bảo vệ.
5. Thử lại nét đứt, góc đơn và góc vuông liên tiếp hai chiều. Ghi thời gian nhiều vòng trong điều kiện pin/mặt đường tương đương.

Một hàng sensor không phân biệt chắc chắn góc90°, giao line và mảng đen rộng. Allowance32ms là heuristic có giới hạn, có thể cho xe tiến thêm vào vùng ambiguous. Nếu sau vùng này là cua gấp chưa nhận được hướng, vẫn có thể overshoot; cần log/video tiếp để đánh giá. Không khẳng định tối ưu cả map chỉ từ replay của một sự kiện.
