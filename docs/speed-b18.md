# B18 — Giảm khựng ngắn ở cua mềm, đo thời gian trạng thái

Giữ defaults55/85, PID12/0/0,12, profile curve/outward risk và pivot36/ramp600 của B17. Owner báo B17 ổn cả map, nhanh hơn khoảng1s nhưng cua mềm và chuỗi góc còn tốn thời gian;60/90 kém hơn. Chưa có trace để xác nhận nhánh gây khựng trên xe.

## Short curve bridge

B17 có nhánh valid error1,9 pitch ổn định → mask fragmented24: không có straight guide, nên PROBE hạ target15 ngay. B18 lưu lệnh L/R riêng cho moderate curve nếu error không quá2 pitch, filtered rate không quá4 pitch/s, hai bánh tiến, không mắt ngoài bit0/7, không pending/post-turn/corner hint.

Khi tín hiệu invalid, snapshot còn mới và không có cue góc: giữ exact L/R tối đa8ms với fragmented mask không chạm mép, tối đa4ms với toàn trắng. Mode vẫn PROBE, không coi centroid invalid là dữ liệu PD. Snapshot age tối đa10ms, không cập nhật khi invalid. Counters loss/ambiguous tiếp tục tích lũy, không làm mới allowance. Góc/split có outer edge, pending/post-turn hoặc outward drift bỏ qua bridge. Hết allowance dùng recovery/PROBE cũ; không mở rộng GAP300ms hay recovery1200ms.

Bắt lại valid line nhận PD ngay và reset D history như cũ. Đây chỉ là bảo vệ vài frame, không cho xe tiếp tục cua mù dài. Chưa biết mức cải thiện thực trên map.

## Sau góc vuông

Sau recovery, khi đang post-turn và line ở giữa (error không quá1 pitch), không corner pending: acceleration160 thay80 điểm PWM/giây trong vùng speed dưới42%. Qua khoảng42 thì trở lại80. Target vẫn do planner cũ quyết định, không phải ép chạy42 hay bỏ guard. Shape32, RECENTER30, pivot36, pulse phanh12ms và các deadline giữ nguyên. Không thêm delay hoặc timed advance.

## TIME trên OLED

Sau Stop/fault, N1 lần lượt LIVE → LAST RUN → DIAG → TIME → LIVE. N3 Start/Stop như cũ.

| Nhãn | Giá trị ms |
| --- | --- |
| TRK | Thời gian command thuộc TRACK |
| CV | Phần trong TRK có speed reason CURVE; không phải đo cua thật |
| GAP | Thời gian GAP |
| PRB | PROBE, gồm short curve bridge và nhánh target15 |
| BRK | BRAKE do controller yêu cầu trong RUN |
| TRN | TURN |
| REC | RECENTER/ALIGN |
| ALL | Tổng thời gian command active qua các frame RUN |

Các bộ đếm cộng2ms theo frame control, không phải phép đo wall-clock/thời gian phanh cơ học. BRK không bao gồm Stop/phanh do watchdog hoặc command zero khi đảo chiều. CV nằm trong TRK, không cộng hai số này với nhau. Giá trị trên OLED cap99999; API giữ uint32 saturating. Tổng chỉ mô tả cả lượt, không định vị thời điểm/phần map gây chậm. Dữ liệu giữ sau Stop/fault, reset khi Start thành công. PRB cao chưa phân biệt bridge với PROBE15; cần xem thêm trace nếu còn khó xác định.

## Validation và thử xe

34.754 assertions pass: exact bridge8ms, valid handoff không giật D, allowance hết chuyển PROBE15, white4ms rồi recovery6ms, outward rejection, square pattern hai phía, post-turn acceleration160, watchdog/deadline, diagnostics duration/reset/freeze và menu4 trang. Keil0 error/0 warning; Code26348/RO1328/RW344/ZI2688 bytes. UI tests dùng OLED stub, chưa kiểm tra màn hình thật. Chưa hardware-verified.

HEX:build/linefollower-B18.hex.
SHA256:402AD5A51B2865992683F6559581756205E8837693CF41A36456ABEB5B358911.
Backup:backups/B17-before-B18-20261026/source.zip và linefollower-B17.hex.

Nạp B18, Reset, calibration + white reference, giữ55/85 để so sánh cùng map. Sau Stop xem TIME, ghi thời gian hoàn thành và còn khựng/cắt góc hay không. Chưa tăng60/90 trong lần đối chiếu đầu. Có thể nạp lại B17 để so sánh; calibration/gain RAM-only cần nhập lại sau Reset.
