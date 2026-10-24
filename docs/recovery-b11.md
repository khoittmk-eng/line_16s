# B11: chuyển tiếp giữa các góc 90° liên tục

Ngày: 24/10/2026.

Owner xác nhận B10 đi được nét đứt và góc 90° sau đường thẳng dài; chuỗi góc vuông liên tục còn văng ra ngoài rồi LINE LOST. Đây là phản hồi phần cứng cho B10 ở điều kiện owner đã thử, chưa xác định Base thực tế hoặc F/R chi tiết.

## Lỗi tái hiện trong source

1. Nhánh recovery trả về trước khi xét split cue mới. Sau khi đã nhìn thấy line trong RECENTER, gặp hướng góc tiếp theo có thể vẫn dùng turn_dir của góc trước. Test trái/phải mới fail 30036 trên B10.
2. Khi RECENTER đủ điều kiện về TRACK, code xóa corner hint nhưng chưa gọi RememberGuide. Khoảng trắng xuất hiện ngay frame kế tiếp không có guide mới; khi last_error gần 0, không có hướng tìm kiếm và có thể fault sau 6 ms. Đây là lỗ hổng chuyển trạng thái, chưa có trace để kết luận nó gây mọi lần văng trên xe.

## Thay đổi

- ConfirmSplit dùng chung cho TRACK/PROBE và recovery; vẫn yêu cầu cùng hướng 6 ms, bất đối xứng mạnh có mắt ngoài cùng.
- Trong recovery, chỉ cho đổi hướng theo split cue mới sau khi đã nhìn thấy ít nhất một frame line valid trong episode đó. Nếu chưa thấy lại line, không tự đảo hướng vì split ở phía đối diện.
- Đổi hướng không gọi BeginRecovery lần nữa, không reset recovery deadline 1.200 ms, không thêm brake pulse. Pivot ramp và giới hạn Max giữ nguyên.
- Valid line vẫn chuyển ngay sang forward PD. Khi RECENTER hoàn tất, lưu L/R mới bằng cùng điều kiện RememberGuide; không giữ lệnh quay hoặc guide trước cua. Xóa ambiguous counter và gap eligibility cũ tại handoff.
- Một lần đổi hướng hoặc một frame giữa không chứng minh đã xoay đủ 90°. Không dùng thời gian làm phép đo góc/quãng đường; không thêm minimum-spin hay ADVANCE.

PID 12 / 0 / 0,12, default Base45 / Max75, phanh, tốc độ pivot, acceleration và policy GAP 300 ms giữ nguyên. Không tự boost lên Max. B11 chưa hardware-verified.

## Thử xe

Nạp build/linefollower-B11.hex, xác nhận B11 rồi calibration và white reference lại. Giữ đúng Base/Max/PID đã dùng khi thử B10 để so sánh. Parameter vẫn RAM-only: Reset trả default Base45 / Max75; nếu trước đó đã hạ Base35 thì cần chỉnh lại 35.

Thử lại nét đứt và góc đơn đã chạy được, sau đó chuỗi góc trái–phải liên tiếp. Không cần nhìn OLED lúc xe chạy. Nếu dừng, LAST RUN và F/R vẫn xem được khi xe đứng yên.

Chưa tăng Base trong lần đối chiếu này. Khi chuỗi góc chạy lặp lại ổn định, có thể thử tăng Base từng 5 điểm phần trăm, giữ gain/Max và kiểm tra lại toàn map. Đây là quy trình tìm mức chạy ổn trên xe, không bảo đảm tốc độ tối đa; PWM không phải vận tốc đo được. Nếu tăng Base làm văng, quay về mức đã chạy ổn.

## Validation

- ARMCC Cortex-M3 emulation: 34.073 assertions pass.
- Test mới: góc ngược xuất hiện khi đang RECENTER, cập nhật hướng hai phía, guide ngay frame sau exit, nhiễu đổi bên, không đảo hướng trước reacquisition, đổi hướng nhiều lần vẫn timeout, chuỗi tám góc không chèn đường thẳng dài ở Base35 và45.
- Giữ regression GAP, valid-line authority, phanh, watchdog, Start/Stop và fault latch.
- Keil: 0 error / 0 warning; Code23160, RO1252, RW340, ZI2572 bytes.
- SHA256 HEX: E66849701A90D33671107BC3244B34F4FA9693D4E6AF4E8AE0CEF966F35E1D64.
- Không mô phỏng chuyển động, ma sát/quán tính, tải motor hoặc hình học line. Không khẳng định đã giải quyết toàn bộ hiện tượng văng.
- Backup: backups/B10-before-B11-20261024.
