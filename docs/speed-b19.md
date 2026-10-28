# B19 — Straight chỉnh riêng trên OLED

Defaults: Base55 / Straight60 / Max85, PID12/0/0,12. Owner xác nhận B18 ở55/85 ổn cả map, cua mềm cải thiện; Base60 vọt hơn và thời gian không giảm. B19 tách target đường thẳng khỏi Base, chưa hardware-verified.

## Ý nghĩa cấu hình

| Mục | Vai trò |
| --- | --- |
| Base | Nền tracking/cua mềm; cua gấp có target thấp hơn |
| Straight | Target có thể đạt khi line gần tâm đủ lâu |
| Max | Trần PWM từng bánh sau PD |

PID Setup có6 dòng Kp/Ki/Kd/Base/Straight/Max, tất cả hiện cùng màn. N1/N3 chọn dòng; N2 vào Edit; N1 tăng/N3 giảm, N2 chốt; giữ N2 Back. Mỗi bước tốc5 điểm PWM; các giá trị RAM-only, Reset về55/60/85.

Giữ Base<=Straight<=Max: Base vượt Max thì nâng Max; Max thấp hơn Base thì hạ Base; Straight luôn clamp giữa hai mức này. Chỉnh Straight riêng không đổi Base/Max. Đặt Straight=Base để bỏ tăng tốc thẳng, làm mốc đối chiếu. API controller cũng clamp phòng dữ liệu ngoài thứ tự.

## Planner

Bỏ hard-coded fast cap55, bonus cap10 và headroom15 của profile cũ. Phần tăng thêm lấy từ Straight-Base, Max vẫn được giới hạn tại output mixer để PD có thể hạ common khi cần sửa lái.

Cần80ms valid gần tâm (|error|<=0,8 pitch), không mask ngoài0xC3, không guard/pending/post-turn. Mắt giữa±0,5 vẫn đủ điều kiện; không dùng corridor0,25 từng gây starvation. Phần tăng thêm lên20/giảm100 điểm PWM/giây; output ramp tăng80/giảm600 như B18. Với55→60 cần khoảng250ms sau đủ điều kiện nếu xe đã ởBase. Không tăng thẳng lênMax.

Ra khỏi corridor thì ngừng cấp bonus và rút dần; shape/edge hoặc memory guard xóa bonus theo policy cũ. Cua mềm error1,2 ổn định vềBase55 thay vì hưởng bonus. Outward risk/curve blend/PD vẫn có quyền giảm thấp hơnBase. Cua gấp32, pivot36, RECENTER30, post-turn acceleration160 dưới42, bridge8/4ms, brake12ms và deadlines giữ B18.

Nét đứt không tăng PWM trên trắng: hold exact60ms rồi giảm phần dư vềBase theo tỷ lệ L/R. Gap ngắn bắt lại tâm giữ phần tốc đã có, nhưng không vượt Straight/Max mới. GAP tối đa300ms và recovery1200ms không đổi.

Một hàng sensor không biết chắc đường thẳng hay cua tròn đang tạm ở tâm; đây là heuristic, không nhìn trước được góc chưa tới. Nhập Straight cao hơn không bảo đảm xe bám được, cũng không bảo đảm đạt target nếu chưa đủ đoạn ổn định.

## Nạp và đối chiếu

Nạp build/linefollower-B19.hex, Reset, calibration + white reference, kiểm tra55/60/85, giữ DIR đã xác nhận rồi Run/N3. Trong PID Setup muốn thử lại nền B18 thì đặt55/55/85. Sau Stop vẫn N1: LIVE→LAST RUN→DIAG→TIME→LIVE. Chưa tăng Base60 trong lần đối chiếu đầu.

34.763 assertions pass: default straight60, mắt giữa/jitter, API Straight62 và clamp Max61/belowBase, cua vềBase, boost guard giữa góc, GAP/bridge/recovery, menu Straight độc lập/clamp/six-row layout và watchdog. Các baseline tests khai báo rõ Straight; test UI dùng OLED stub, chưa render trên màn thật.

Keil0 error/0 warning; Code26436/RO1328/RW344/ZI2696 bytes.
HEX SHA256:0D03F4EC7E403EC83917B0CE39B51115C02F7A8E86A924E8CC340CDF27616949.
Backup:backups/B18-before-B19-20261026/source.zip và linefollower-B18.hex.
