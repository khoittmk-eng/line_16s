# B34 — Bánh trong có thể lùi khi còn thấy góc gắt

## Yêu cầu

Owner muốn bánh trong lùi, bánh ngoài tiến khi gặp đoạn rẽ đột ngột. B34 mở rộng mixer PD của B33 cho phép output âm riêng ở góc gắt đã xác nhận; yêu cầu này thay thế giới hạn forward-only trên mọi valid frame của các bản trước. Không thêm mode quay theo thời gian hoặc pulse cố định trên valid line.

Giữ Base45/Max85, PID nền12/0/0,12, extra P trần1,0 và rise/release0,08/ms. B32 từng được owner báo cua rộng/văng và chậm hơn B30; đây là thử nghiệm khác, không có số đo chứng minh tốt hơn B30/B33.

## Điều kiện và giới hạn

Cho phép mở rộng steering khi tất cả điều kiện sau đúng:

- TRACK có valid directional shape cùng phía, confirmed ít nhất6ms như B30.
- Error cùng phía cue và ít nhất2pitch, planner rate chưa cho thấy về tâm.
- Trend B29 chưa latch releasing; correction PD yêu cầu cùng hướng error.

Reverse allowance bằng `min(Max,20) * clamp(corner_gain,0,1)` (điểm phần trăm PWM). Steering limit mở tới `min(speed+allowance,(Max+allowance)/2)`; mixer giữ common không âm và clip mỗi bánh trong `[-allowance,Max]`. Antiwindup dùng steering limit mới. Không ép steering floor hoặc tăng Kd.

Điều này **cho phép**, không ép bánh lùi ở mọi cue. PD phải đủ lớn để vượt common. Với Base45/Max85, pure P, e=2pitch và gain đã ổn định: B33 yêu cầu85/0%, B34 yêu cầu85/−11%; e=2,5pitch có thể chạm85/−20%. Đây là PWM request, không phải tốc độ bánh đo được. Với e=1,5pitch, cua mềm, centered3C, mắt ngoài đơn hoặc cue ngược phía vẫn giữ forward-only PD. Góc thật có thể không kích hoạt nếu shape/error chưa đủ; cue này không chứng minh hình học90° hoặc nhận đúng nhánh.

Khi error xuống dưới2pitch, cue clear/đổi, rate về tâm hoặc releasing được xác nhận, quyền lùi được nhả ngay và Follow trở lại forward-only. Giữ trend6ms, plateau không rearm và6ms outward mới cho phép assist lại. Nếu D yêu cầu ngược error thì không cấp quyền lùi. Driver vẫn zero short brake một update trước đảo chiều và giữ stagger; lệnh PD đổi ngay không có nghĩa bánh thực đảo chiều tức thì.

Nếu mất line trong khi một bánh đang âm, điều kiện entry arc tiến không đạt, nên vào bounded search/pivot hiện có; không tạo lại arc28ms từ lệnh âm. Khi valid trở lại trong recovery, ALIGN nhận forward PD ngay. Với entry còn tiến, arc0,8/28ms B33 giữ nguyên. Deadline1200ms, GAP300ms, memory/dash/ambiguity, Stop/fault/watchdogs/calibration/Start và driver không đổi. Không auto-start.

## Log và kiểm tra

Output âm có thể xuất hiện ở modeTRACK/SPEED_CORNER; log int16 đã hỗ trợ output âm từ recovery. Record48/header108/CSVv1/Flash layout không đổi. Menu/metadata vẫn là gain nền, target45 không có nghĩa mean duty luôn45 khi mixer cần steering headroom. ZERO diagnostics đếm zero command, không đo lực phanh hoặc góc quay.

- Backup: `backups/B33-before-B34-20261029/B33-source.zip` gồm BSP và `linefollower-B33.hex`.
- Baseline B33 fail130035 với kỳ vọng signed corner mới; đây là tính năng owner yêu cầu, không phải defect hardware mới.
- **104.778 ARM assertions +8 parser/layout +18 receiver tests pass.** Hai phía/dt1–4ms, cue6ms,85/−11 với pureP, reverse≤20 và clipMax cùng frame, ordinary/mismatched cue không reverse, inward release/plateau/rearm, D ngược hướng, cue đổi phía, loss từ signed output không arc, valid rejoin/timeout, antiwindup và suite driver/square-chain/GAP/interlocks/watchdogs.
- Emulator300M instructions/60s hoàn tất `test_done`; không đo WCET/dynamics/waveform trên MCU.
- `scripts/build.ps1`: Keil0Error/0Warning; Code33568/RO1856/RW444/ZI10524bytes. HEX kết thúc0x08008B38, dưới log0x0800C000.
- HEX: `build/linefollower-B34.hex`; SHA256 `D1AF56B1FC9A487A93C2E612BAE660DE25F6CDD1014303C70C8843AB27FAE26D`.

## Thử xe

Chưa nạp hoặc thử B34 trên xe. Sau nạp kiểm tra OLED B34, calibration/white reference, chiều sửa lái và Stop. Giữ45/85,12/0/0,12; so với B30/B33 cùng pin/map và nhiều lượt. Lùi bánh trong có thể làm trượt, khựng do đảo chiều, cắt nhánh hoặc quá lái. Nếu regression, lưu CORNER/LOSS để kiểm tra mask/error/mode/PWM; chưa khẳng định giảm thời gian vòng. Export log cần giữ trước reflash; không full erase hoặc đổi option bytes.
