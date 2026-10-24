# B13: lưu trạng thái vừa thoát cua và tìm lại sớm

Ngày: 24/10/2026. Owner báo chuỗi góc vuông liên tục vượt line rồi LINE LOST, yêu cầu sửa tiếp từ bản cuối và ghi error-log.

## Bằng chứng

B12 đã có last_side, không phải hoàn toàn không lưu hướng. Nhưng sau RECENTER gần giữa12ms, controller chuyển TRACK và tạo guide ngay. Nếu trắng xuất hiện frame tiếp theo, code có thể vào GAP đi tới300ms rồi mới tìm theo last_side. Test mới dùng PID12/0/0,12, Base35/Max75 tái hiện: fail30040 trước sửa. Không có trace thực nên chưa khẳng định đây là toàn bộ nguyên nhân văng trên xe.

Lúc bắt đầu đọc source, defaults là10/0/3, Base45/Max90, khác HEX B12 đã phát hành. Đã sao lưu nguyên trạng trước sửa. Trong lúc làm, source được đổi tiếp về12/0/0,12; đã đọc lại trước patch, giữ giá trị đó và đưa Max về75 để khớp profile B12 đã build. Test ban đầu fail80 do default Max khác kỳ vọng, không phải bằng chứng lỗi recovery. Chỉ lần fail30040 sau khi thống nhất profile là regression hành vi mới.

## Policy B13

- Khi RECENTER đạt điều kiện cũ và trở lại TRACK, lưu post_turn_dir và cửa sổ post_turn_ms=200ms. PID vẫn nhận valid line bình thường; không giữ quay cưỡng bức và không thêm delay.
- Trong cửa sổ này, mất line không được cấp quyền GAP300ms. Trắng được xác nhận6ms, sau đó tìm lại theo last_error/corner hint mới, rồi last_side còn hạn, cuối cùng hướng vừa thoát cua.
- Split cue mới vẫn có ưu tiên qua nhánh xác nhận6ms; không ép hướng góc cũ lên tín hiệu mới.
- Bám giữa đủ điều kiện trusted guide100ms xóa guard sớm và bật lại GAP thẳng như B12. Guard cũng hết hạn sau200ms; đây là giới hạn heuristic, không đo góc thân xe.
- Giữ recovery_ms qua lần thoát cua ngắn. Nếu mất lại trong guard, không cấp mới1200ms. Trắng/giữa chập chờn không được gia hạn tìm vô hạn. Khi tracking ổn định hoặc guard hết hạn, episode được kết thúc.
- BRAKE driver, pivot30, ramp, RECENTER26, watchdog và explicit Start/Stop giữ nguyên. Mọi FAULT vẫn latch.

## Giới hạn và cách thử

Một khoảng trắng thật ngay sau cua có thể bị policy này coi là mất bám để quay tìm; chỉ một hàng sensor không phân biệt chắc chắn hai tình huống. Vì vậy cần thử cả chuỗi góc liên tục lẫn nét đứt sau cua. GAP trên đường thẳng ổn định vẫn giữ exact L/R tối đa300ms. Không khẳng định giữ history sẽ chữa hết văng do quán tính hoặc PID.

Default B13: Kp12, Ki0, Kd0,12, Base45, Max75. Giữ Base đã dùng để so sánh; nếu đang thử35 thì nhập lại35 sau Reset. Chưa tăng tốc.

Nạp build/linefollower-B13.hex, xác nhận B13, calibration + white reference lại. Thử góc đơn, chuỗi góc hai phía, nét đứt thẳng và nét đứt ngay sau cua. Không cần xem OLED khi đang chạy; nếu dừng, LAST RUN/F/R vẫn xem được khi xe đứng.

## Validation

34.106 assertions pass trong ARMCC Cortex-M3 emulation. Có regression reloss sau exit ở cả hai phía, không gia hạn deadline qua nhiều center windows, hướng mới thay hướng cũ, nhớ turn sau khi last_side TTL hết, guard hết hạn, stable center khôi phục GAP240ms giữ exactL/R. Các tests BRAKE/Start/Stop/watchdog/valid-line authority giữ nguyên.

Keil0 error/0 warning; Code23516/RO1252/RW340/ZI2588 bytes.
SHA256 HEX: 966362446E995B0B7251255E9480579A834880410F054E4090E7A559A55FD08E.
Backup source owner và HEX B12: backups/B12-owner-before-B13-20261024.
Trạng thái: source-fixed/emulation-tested; chưa hardware-verified, không mô phỏng động lực học xe.
