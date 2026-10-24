# B12: tìm lại theo phía line gần nhất có giới hạn

Ngày: 24/10/2026. Owner gửi ảnh line liên tục dạng xung vuông; xe vẫn LINE LOST ở chuỗi góc và đề nghị rẽ theo hướng cuối bắt được. Không có F/R hoặc trace sensor thực nên chưa kết luận mọi lần dừng là GAP timeout, ambiguous timeout hay recovery timeout.

## Ý nghĩa LINE LOST

Đây là nhãn FAULT_LINE sau khi policy không tìm được line hoặc không xác định hướng trong giới hạn. Không phải mọi frame trắng đều Stop: GAP/PROBE/TURN đã có xử lý. Nếu recovery đã hết 1.200 ms thì B12 vẫn Stop. Lệnh quay không bảo đảm bắt lại đường khi xe đã văng xa hoặc hướng nhớ không còn đúng.

## Bằng chứng và thay đổi

B11: sau mẫu lệch bên rõ, vài mẫu giữa làm last_error về 0; khi GAP hết 300 ms, code fault trực tiếp, không thử lại phía đã thấy. Regression mới fail30036 trước sửa. B12 thêm last_side riêng với last_error của PD:

- Valid error có độ lớn ít nhất 1,2 pitch lưu phía lệch mới nhất. Split cue bất đối xứng đã xác nhận 6 ms cũng lưu phía khi được chấp nhận để recovery/retarget.
- Memory sống tối đa 600 ms từ quan sát đó. Timer vẫn trôi trong GAP/recovery, không tự kéo dài chỉ vì đang quay. Split cue mới được chấp nhận là quan sát mới.
- Sau 100 ms liên tục đủ điều kiện trusted center guide, xóa hướng cũ sớm; không coi một mẫu giữa là đủ để quên phía vừa thấy.
- Khi GAP hết 300 ms hoặc PROBE ambiguous hết 80 ms, nếu còn last_side mới thì chuyển sang bounded recovery thay vì fault ngay. Nhánh trắng không đủ điều kiện GAP và không còn last_error/corner hint cũng dùng fallback này sau loss confirmation 6 ms.
- Nếu không có hướng mới, vẫn Stop theo policy cũ. Hướng mới từ cảm biến được ưu tiên; không đoán cố định trái/phải theo hình map.
- Recovery dùng phanh/pivot/ramp hiện có, giới hạn 1.200 ms tính từ entry; không reset deadline khi đổi hướng. Tổng một chuỗi không thấy valid line có thể gồm GAP tối đa 300 ms rồi recovery 1.200 ms (có sai số một control frame ở ngưỡng chuyển).
- Thấy valid line thì trả forward PD ngay. Nếu đã FAULT thì không auto-restart; vẫn cần N3 Start sau khi đặt xe trên line.

Thay đổi này thay thế quy tắc cũ “GAP hết 300 ms luôn fault” bằng “GAP hết 300 ms kết thúc giữ lệnh đi tới; chỉ được thử recovery nếu còn hướng gần đây”. Không tăng thời gian GAP, không quay vô hạn và không bỏ watchdog.

## Giới hạn

Hướng cuối là heuristic: một hàng sensor không biết chắc góc tiếp theo hoặc hướng thân xe khi tất cả mắt trắng. Ảnh cho thấy đường liên tục nhưng không cung cấp sensor trace hoặc kích thước đo được. Nếu mất dấu trước khi quan sát phía góc mới, hướng cũ có thể sai. Không cam kết B12 sửa hết hiện tượng văng; không dùng kết quả test phần mềm làm bằng chứng xe chạy đúng hình xung vuông.

Giữ nguyên PID 12/0/0,12, default Base45/Max75, brake driver, ramp, pivot30 và RECENTER26. Nên giữ thông số đang test, chưa tăng Base. Reset trả mặc định nên cần nhập lại Base nếu đang dùng35.

## Nạp và đối chiếu

Nạp build/linefollower-B12.hex, xác nhận B12 rồi calibration/white reference. Thử nét đứt và góc đơn trước, sau đó chuỗi góc như ảnh ở cùng thông số cũ. Không cần đọc OLED khi chạy. Nếu xe dừng, LAST RUN và F/R có thể xem khi đứng yên; R2 là recovery hết thời gian, không phải thiếu fallback hướng.

Backup B11: backups/B11-before-B12-20261024.
Validation: 34.085 assertions pass trong ARMCC Cortex-M3 emulation; Keil 0 error/0 warning, Code23360/RO1252/RW340/ZI2580 bytes. Bao gồm last-side hai phía, hướng mới thay hướng cũ, TTL, clear khi giữa ổn định, timeout tìm kiếm, ambiguous fallback, fault latch; giữ tests nét đứt, BRAKE, ADC/UI watchdog và Start/Stop. Không mô phỏng động lực học hoặc xác nhận phần cứng.

SHA256 HEX: B09944A65FA84B2C9EA2410A2E8FC19991074BD3DCD7BB2010CD976B0DCDB03F.
