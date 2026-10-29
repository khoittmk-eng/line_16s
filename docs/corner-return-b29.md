# B29 — Lên lực hỗ trợ nhanh hơn, nhả sớm khi line về tâm

## Mốc thử

Owner báo B28 đạt16,3giây ởBase45, cải thiện từ17,3giây của lượt trước. Owner tiếp tục xác nhận Base55/60 từng chậm hơn45. Giữ **Base45/Max85**, PID nền **12/0/0,12** để thử thay đổi cách vào/thoát cua, không tăng target đường thẳng.

Đây là số đo owner cung cấp. **B29 chưa chạy thử trên xe**, chưa có thời gian vòng hoặc quỹ đạo mới được đo.

## Hai thay đổi

1. Tốc độ tăng extra corner gain từ0,04 lên **0,05/ms**, giữ trần extra0,6 và các điều kiện cue củaB28. Ở input pureP e1,5, correction tại frame đầu được hỗ trợ từ19,44 lên19,8%; sau cue ổn định vẫn23,4%. Không tăng Kp/Kd nền hoặc lực hỗ trợ tối đa.
2. Bổ sung quan sát error mới theo frame để nhả extra P sớm hơn rate đã lọc. Khi cùng một cue đã confirmed, error còn cùng phía và trên1pitch, **abs(error) giảm hơn0,01pitch mỗi frame liên tiếp ít nhất6ms** thì đặt trạng thái releasing. Với frame2ms là3frame liên tiếp.

Khi releasing mà planner rate vẫn còn báo đang ra ngoài, extra gain giảm dần về0 ở0,05/ms. Nếu điều kiện B28 đã không hợp lệ hoặc planner rate đã đổi dấu về tâm, extra về0 như trước. P nền, D filter và quyền sửa lái vẫn hoạt động; không thả mất điều khiển khi nhả hỗ trợ.

Một frame phẳng/nhiễu nhỏ làm reset bộ đếm liên tiếp. Sau khi đã xác nhận releasing, trạng thái giữ qua đoạn error đứng yên; chỉ được cho phép tăng hỗ trợ lại sau ít nhất6ms error tăng rõ liên tiếp, hoặc được reset khi cue/mode đổi. Cách này tránh gain tăng lại chỉ vì planner rate cũ vẫn dương hoặc vì một mẫu nhiễu ra ngoài.

Trend chỉ tác động extra P. Không xóa pending/last-side/post-turn memory, không đổi mode/góc quay và không gia hạn loss/recovery deadline. Invalid, Stop và vào recovery đều reset trend. Cua mềm không có confirmed directional shape tiếp tục PID nền.

## So sánh ba candidate

`scripts/compare-corner-exit.py` compile và chạy code ARM với cùng64frame synthetic pureP. Không mô phỏng ma sát, motor, thân xe hoặc thời gian vòng.

| Candidate | Correction lúc bắt đầu hỗ trợ | Correction khi cue ổn định | Extra gain sau6ms error bắt đầu về tâm |
| --- | --- | --- | --- |
| B28 gốc | 19,44% | 23,4% | 0,264 |
| Chỉ tăng tốc lên gain | 19,8% | 23,4% | 0,264 |
| B29 — tăng nhanh + nhả sớm | 19,8% | 23,4% | 0,176 |

Extra gain là hệ số nhân thêm vào Kp nền, không phải PWM:0,264 nghĩa Kp_effective bằng1,264 lầnKp. Trace xác nhận B29 giảm phần hỗ trợ sớm hơn nhưng không đổi mức ổn định; không dự đoán lap time.

Chạy lại sau release:

```powershell
python scripts/compare-corner-exit.py
```

Script đọc B28 source/header/tuning từ ZIP backup local, xuất CSV vào `build/corner-exit-comparison`. Có `--baseline-source` để dùng trước release. Cần dependency emulator đã dùng trong suite, không kết nối/nạp MCU.

## Giới hạn và kiểm thử

- SourceB28 fail regression **140035**: sau ba frame error giảm, planner vẫn hướng ra ngoài và extra gain chưa nhả sớm. Đây là hạn chế source được tái hiện, chưa chứng minh nó là phần tốn thời gian nhất trên map.
- **104.149 ARM assertions +8 parser/layout +18 receiver tests** pass; candidate comparison pass và không compiler warning.
- Tests gồm trend hai phía, plateau không bật lại gain, một mẫu ra ngoài không rearm, xác nhận tăng lại, nhiễu xen kẽ, đổi góc, invalid/Stop reset và dt1–4ms. Bộ bounds/antiwindup, nét đứt, chuỗi góc vuông, quét, driver và watchdog cũ vẫn pass.
- Emulator giữ300M instructions/60s, không phải đo WCET trên board. D filter15ms, planner rate40ms, search50/36, arc28ms, GAP300ms và recovery1200ms giữ cấu hìnhB28.
- Keil0Error/0Warning; Code33240 / RO1856 / RW440 / ZI10520bytes. HEX kết thúc `0x080089EC`, dưới vùng log `0x0800C000`.
- SHA256 HEX: `D4F13408D7FCCA575C8CBAEA7EB62ED6A7B1981982A3AE1B85A9F385EEEBD204`.

Nhả hỗ trợ sớm có thể làm xe thiếu lực ôm cua nếu tín hiệu giảm error chỉ là nhiễu hoặc đi qua nhánh khác. Gain lên nhanh hơn có thể tăng nhạy ở cue giả. Cần thử xe ở45/85; chưa tăng Base trong lượt này. Nếu còn lỗi, log CORNER/LOSS giúp đối chiếu actual PWM/mode nhưng không đo vận tốc.

Nạp `build/linefollower-B29.hex`, kiểm tra OLED B29 và calibration/white reference lại. Giữ gain nền để so với16,3giây; dùng cùng cách tính thời gian. Backup trước sửa: `backups/B28-before-B29-20261029`. Flash/CSV schema không đổi; không push GitHub hoặc tự nạp board trong phiên.
