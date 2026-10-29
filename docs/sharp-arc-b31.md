# B31 — Giữ lực lái khi chuyển sang rẽ tiến ở góc

## Lựa chọn

Owner yêu cầu chọn hướng tối ưu góc vuông và nạp thử, tiếp tục chấp nhận nguy cơ văng. Chọn tăng chênh lệch ngay trong pha rẽ tiến sau confirmed loss, giữ Base45/Max85 và PID nền12/0/0,12 cùng extra P của B30.

Lý do từ source: khi B30 đang valid với lệnh85/0%, mean42,5%, loss có hướng vào arc dùng correction0,8mean làm output giảm về76,5/8,5%. Chênh lệch giảm85→68 điểm phần trăm tại đúng lúc đang mất line. Regression mới chạy với B30 fail120036. Đây là hành vi command tái hiện được; chưa chứng minh nó là phần chiếm nhiều thời gian nhất trên map.

## Thay đổi duy nhất về điều khiển

`CONTROL_RECOVERY_ARC_STEERING` tăng0,8→1,0. Bánh trong yêu cầu0, bánh ngoài dùng `2*min(entry_mean,Max/2)`; cả hai không âm trong arc. Hệ số1,0 là mức lớn nhất của mixer này mà không yêu cầu bánh trong lùi. Không đổi driver: duty0 vẫn dùng hành vi brake riêng channel đã có, không có pulse brake cả hai trước cua. Lệnh0 không chứng minh bánh đã dừng cơ học.

| Entry mean / Max | B30 ngoài/trong | B31 ngoài/trong | Mean B31 |
| --- | --- | --- | --- |
| 45 / 85% | 81 / 9% | 85 / 0% | 42,5% |
| 42,5 / 85% sau valid85/0 | 76,5 / 8,5% | 85 / 0% | 42,5% |

Mục tiêu là xoay theo góc nhanh hơn để bắt lại line trước pivot; không phải tăng tốc tiến trung bình. Mức common giảm khi cần headroom cho steering. Nếu motor/bánh trượt hoặc quét lố, hiệu quả thực có thể kém hơn. Không có plant model hay lap time mới để khẳng định lựa chọn nhanh nhất.

Giữ arc tối đa28ms trong recovery episode, entry tiến mean>=35, không cấp lại arc khi post-turn retry, valid frame nhận PD ngay ở bất kỳ thời điểm nào. Fallback search50 trên trắng/36 khi có đen invalid, rejoin policy, deadline1200ms, GAP300ms, memory/ambiguity/dash, D15ms/planner40ms, Start/calibration/watchdogs và Flash48/108 giữ nguyên. Không kéo dài tiến mù hoặc dùng geometry16cm để canh thời gian.

## Validation và artifact

- BackupB30: `backups/B30-before-B31-20261029/B30-source.zip` và `linefollower-B30.hex`.
- **104.160 ARM assertions +8 parser/layout +18 receiver tests pass**. Regression hai phía kiểm tra valid85/0→loss giữ85/0, expiry28ms, rejoin mọi frame, post-turn không renew, Max thay đổi/dt1–4ms, cùng suite square-chain/GAP/driver/watchdogs. Các test trước yêu cầu cả hai PWM dương được đổi sang arc có bánh trong0, vẫn kiểm tra hướng và giới hạn.
- Emulator300M instructions/60s hoàn tất `test_done`; không là WCET trên board.
- `scripts/build.ps1`: Keil0Error/0Warning; Code33236/RO1856/RW440/ZI10520bytes. HEX kết thúc0x080089E8, dưới log0x0800C000.
- Artifact: `build/linefollower-B31.hex`; SHA256 `5008CC86E1435D6B78D331B4E0C1F7D673AADEBA7C13E22FDE8B7F2315D3EFA5`.

## Nạp và thử xe

Khi chuẩn bị nạp, truy vấn USB hiện diện không thấy ST-Link/STM32 VID0483. Máy có Keil/ST-Link DLL nhưng options local hiện chọnUL2CM3, không thể coi là cấu hìnhST-Link đã đúng. Chưa có thao tác program/erase trên board; không full erase, không đổi option bytes hoặc tự chạy xe. Cần kết nối probe/board để tiếp tục nạp bằng SWD và sector erase, giữ vùng Flash log.

Sau nạp, xác nhận OLED B31, calibration/white reference và chạy45/85,12/0/0,12. Đo nhiều lượt cùng pin/map, ghi thời gian và số lần văng/mất line. Chưa hardware/lap validated; xem [Keil/ST-Link](keil-download.md) và [B30](stronger-corner-b30.md).
