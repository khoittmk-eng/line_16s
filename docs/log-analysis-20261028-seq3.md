# Phân tích B23 sequence3 — 28/10/2026

File `logs/run-20261028-182311-164428.csv`, CRC32 **3ED4E91F**,160sample. Profile55/60/85, PID12/0/0,12, DIR+1, polarity0.

Start124389ms, trigger124780ms (391ms sauStart), kết thúc125094ms. Không BRAKE/TURN/fault trongwindow. TRACK602ms/PROBE30ms xấp xỉ theo các sample.

| Timestamp | Mask | Mode | Common request | Target | Flags |
| --- | --- | --- | --- | --- | --- |
| 124778 | 18 | TRACK |31,1%|60,0%|0|
| 124780 | 99 | PROBE |31,1%|31,1%|24|
| 124788..124800 | FF | PROBE |31,1%|31,1%|16|
| 124808 | 98 | PROBE |31,1%|31,1%|16|
| 124810 | 18 | TRACK |31,3%|55,0%|0|
| 125094 | 18 | TRACK |54,0%|59,1%|0|

Flags16 xác nhận allowance B23. B23 đã tránh collapse15 trong sự kiện này. Không có lệnh bánh về0, error lớn nhất0,372pitch; đây chưa phải góc90° ở tốcStraight60. Feedback owner về overshoot/phanh nhiều ở các vị trí khác không được phủ bởiwindow.

Root telemetry: CORNER trigger B23 bao gồm invalidPROBE nên đóng băng sớm ở startup ambiguity. B24 đổi sang cue có hướng/TURN/fault để tăng khả năng bắt được đoạn cua có ích. Đồng thời, theo yêu cầu owner, B24 bỏ recovery brakepulse và thay mixer để thêm yaw ngay; đó là thay đổi chủ động theo yêu cầu và review source, không phải kết luận từ log này rằng BRAKE đã xảy ra.

Xem [speed-b24.md](speed-b24.md). Log nguyên gốc giữ tronglogs, không sửa.
