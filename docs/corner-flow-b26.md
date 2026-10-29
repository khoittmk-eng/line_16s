# B26 — Base 45 và phản ứng cua liên tục

## Mục tiêu và trạng thái

Owner báo Base45 ổn định hơn55, cua mềm bị chậm và muốn phản ứng ngay khi thấy line. B26 ưu tiên thời gian trong cua thay vì boost đường thẳng. Đã kiểm thử source/build; **chưa chạy B26 trên xe**, chưa có số đo lap time mới.

## Thay đổi điều khiển

- Default **Base45 / Max85**, PID **12 / 0 / 0,12**; `straight_percent=45` chỉ để tương thích log. TRACK giữ target Base, không có boost.
- Bỏ slew thứ hai áp lên toàn bộ correction trong cua mềm. P tác động ngay trong frame có line hợp lệ; D vẫn low-pass15ms, integral vẫn anti-windup. Ví dụ riêng P, error0,5pitch và Kp12: correction6% ngay frame2ms thay vì chỉ1,6% ở B25.
- Cua có line hợp lệ dùng forward PD: bánh ngoài tăng, bánh trong giảm. Không thêm xung brake hay ép Pivot theo timer. Ví dụ Base45 với correction6: L/R51/39%. Có thể chạm zero khi correction thực sự lớn; không ép sàn bánh trong vì điều đó giới hạn khả năng rẽ gắt.
- RECENTER target ceiling từ30 lên40%; lúc bắt lại line không reset duty cố định xuống30 mà giữ command magnitude trước đó, cap theo Base/40. Sau Pivot36%, mẫu valid đầu tiên yêu cầu khoảng36,16%, rồi ramp tiếp; nếu trước đó thấp hơn thì không nhảy thẳng lên40.
- Sau RECENTER, target ceiling40 thay cho35 trong thời gian xác nhận tâm60ms; sau đó vềBase45. Giữ memory/deadline của chuỗi góc vuông. Đây là mức thử cần đánh giá trên xe.

Quay ngược bánh vẫn có trong recovery khi mất line đã xác nhận. Việc giữ forward PD lúc còn nhìn thấy line không cho phép khẳng định có thể chạy mọi góc90° ở cùng tốc độ. GAP300ms, short curve bridge8ms/white4ms, bounded ambiguity và deadline recovery1200ms giữ nguyên. Không dùng khoảng cách16cm để tính lệnh tiến hoặc thời gian quay.

Estimator/ADC, driver TB6612, reversal guard, Flash logger và điều kiện Start/Stop/watchdog không thay đổi. Vì chưa có log cua mềm B25, không thay threshold validity hoặc tự tạo centroid từ mask nhiều nhánh.

## SWD và lỗi nạp Keil

Owner xác nhận chưa Save RAM to Flash mà vẫn lỗi download; chưa có NRST. B26 thêm `Board_RemapOLEDAndKeepSWD`: ghi SWD enabled rõ ràng cùng remap OLED, bỏ readback write-only `SWJ_CFG`. Đây là hardening, **chưa chứng minh đã chữa lỗi nạp trên board**. Không sửa option bytes và không tắt chức năng log Flash.

Xem [hướng dẫn Keil/ST-Link](keil-download.md), gồm dây DIO/CLK/GND/NRST, chế độ Normal khi chưa có NRST và Connect under Reset khi đã nối NRST. Chạy `scripts/diagnose-keil.ps1` để kiểm tra cấu hình/file; script không tác động MCU.

## Validation

- Test mới chạy với controller B25 gốc fail **100031**: correction P chỉ1,6 thay vì6 ở frame đầu. Đây là bằng chứng trễ lệnh điều khiển, không phải số đo thời gian thân xe quay.
- B26 **101.895 ARM assertions**, **8 parser/layout +18 receiver tests** pass. Bộ test55/85 được giữ bằng preset riêng, ngoài default45/85 mới.
- Kiểm tra cua mềm liên tục hai phía: cả hai PWM tiến, không BRAKE/TURN; rejoin có PD ngay, giữ duty dưới cap40, góc ngược khi ALIGN chưa xong vẫn retarget; GAP/timeout/watchdog/driver/log tests pass.
- Test board dùng code thật và MMIO giả: SWD field luôn ghi010, I2C remap giữPB8/PB9, các remap khác giữ nguyên, motor GPIO startup an toàn. Không mô phỏng physical debug link.
- Keil0Error/0Warning; Code32112 / RO1856 / RW440 / ZI10496 bytes. HEX kết thúc `0x08008580`, dưới vùng log `0x0800C000`.
- SHA256: `6516AB071574C1FC341B5D8863002C1D2A8B7E085140C97FABFDDE8918AD6972`.

## Thử trên xe

Nạp `build/linefollower-B26.hex`, kiểm tra OLED B26, calibration và white reference lại. Giữ Base45/Max85 và gain12/0/0,12 trong lượt so sánh đầu. Thử cua mềm trước, rồi góc đơn, chuỗi góc vuông và nét đứt. Không tăng Base cùng lượt này; ghi log CORNER mới nếu còn khựng/văng để biết mode gây ra.

Backup source và HEX trước sửa: `backups/B25-before-B26-20261029`. Chưa push GitHub hoặc nạp MCU tự động.
