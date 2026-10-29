# B30 — Tăng chênh lệch hai bánh ở góc gắt

## Yêu cầu và thay đổi

Owner yêu cầu tăng chênh lệch hai bánh khi cua gắt, chấp nhận xe dễ văng hơn. Giữ Base45/Max85 và PID nền12/0/0,12 để so với mốc B28 được owner báo16,3giây. Chưa có kết quả hardware hoặc thời gian vòng cho B29/B30.

Chỉ đổi ba hằng số extra P trong `app/control_tuning.h`:

| Tham số | B29 | B30 |
| --- | --- | --- |
| Trần extra gain | 0,6 | 1,0 |
| Tốc độ tăng gain mỗi ms | 0,05 | 0,08 |
| Tốc độ nhả gain mỗi ms | 0,05 | 0,08 |

Luật vẫn là `extra_target = gain_max * clamp(abs(e)-1,0,1)`; Kp effective bằng Kp nền nhân `(1+extra_gain)`. Với Kp12, trần effective tăng19,2→24. Thời gian tăng từ0 đến toàn trần ở target tối đa khoảng12→12,5ms sau confirmed cue; fade từ toàn trần cũng khoảng12,5ms nếu releasing và cue/planner vẫn cho phép fade.

Giữ eligibility: TRACK, valid directional shape confirmed6ms, error cùng hướng ít nhất1pitch, planner rate chưa cải thiện. Giữ B29: error mới giảm rõ liên tiếp6ms latches releasing; plateau không bật lại gain, cần6ms outward rõ để rearm. Khi mất điều kiện hỗ trợ, extra về0 như trước. Không thêm floor ép correction hay đổi Kd.

## Ví dụ PWM request

Base45/Max85, pure P, error giữ ổn định và cue đủ lâu để gain đạt target:

| abs(error), pitch | B29: bánh ngoài / trong | B30: bánh ngoài / trong |
| --- | --- | --- |
| 1,5 | 68,4 / 21,6% | 72 / 18% |
| 2,0 | 83,4 / 6,6% | 85 / 0% |

Hai phía đối xứng. Đây là PWM request, không phải tốc độ bánh hay góc quay đo được; thành phần D thực có thể làm output khác. Tại cue6ms/e1,5, correction đầu tăng19,8→20,88%, không nhảy ngay tới27%.

Mixer vẫn forward-only khi valid, steering limit=min(speed,Max/2), clipping và antiwindup dùng effective P. Khi đã chạm85/0 thì tăng gain không tăng thêm chênh lệch. Không cấp lệnh bánh trong lùi trên valid line.

Giữ search50/36, arc28ms hệ số0,8, GAP/ambiguity, rejoin40, memory/deadlines, driver reversal guard, D filter15ms/planner40ms, calibration/Start/watchdogs và Flash/CSV record48/header108. Menu/CSV vẫn ghi gain nền; cần đọc firmware tagB30 khi phân tích log.

## Validation

- Backup trước sửa: `backups/B29-before-B30-20261029/B29-source.zip` và `linefollower-B29.hex`; HEX B29 rebuild từ source gốc,0Error/0Warning và layout pass.
- **104.155 ARM assertions +8 parser/layout +18 receiver tests pass.** Kiểm tra hai phía, hỗ trợ sau confirmed cue, chênh lệch54điểm phần trăm ởe1,5, saturation85/0 ởe2 với gain nền, nhả/rearm, antiwindup/Max/dt1–4ms cùng suite square-chain/GAP/driver/watchdogs. Test kỳ vọng mới chạy với B29 dừng ở140030; đây là thay đổi profile có chủ ý, không phải defect hardware mới.
- Emulator vẫn300M instructions/60s và hoàn tất `test_done`; không chứng minh WCET trênSTM32.
- `scripts/build.ps1`: Keil0Error/0Warning; Code33240/RO1856/RW440/ZI10520bytes. HEX kết thúc0x080089EC, không chạm vùng log0x0800C000.
- HEX: `build/linefollower-B30.hex`, SHA256 `7D3F6C6B575E025A4A4BB49FFCA4326F64240A8CCE600BD1A3FB7DEA7799405F`.

Môi trường ban đầu chỉ có alias Python không chạy được. Portable Python3.12.10 và dependency emulator unicorn2.1.4/pyelftools0.33 nằm riêng trong `build`, không đổi Python hệ thống. Khi chạy lại script, thêm `build/python` vào PATH của phiên PowerShell trước khi chạy `scripts/build.ps1` hoặc `scripts/test-control.ps1`.

## Thử xe

Nạp HEX, kiểm tra OLED B30, calibration và white reference lại; thử45/85,12/0/0,12. Giữ cùng điều kiện pin/map/cách đo, chạy nhiều lượt và ghi cả thời gian lẫn số lần mất line/cắt nhánh. Log CORNER/LOSS đối chiếu mask→mode→PWM, không đo vận tốc thực.

Gain mạnh hơn có thể làm xe văng, trượt hoặc cắt nhánh; đây là đánh đổi owner đã chấp nhận. Chưa nạp board và chưa hardware-verified; chưa khẳng định cải thiện thành tích.
