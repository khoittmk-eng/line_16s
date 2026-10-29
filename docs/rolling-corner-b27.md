# B27 — Rẽ tiến trước khi quay tìm, Base45

## Mục tiêu và bằng chứng

Owner báo B26 đã cải thiện, các đoạn còn lại ổn định ở Base45 nhưng nét đứt hơi lắc và vẫn cảm giác dừng hai bánh rồi cua. Owner chấp nhận rủi ro thân xe văng thêm để giảm thời gian cua. Không có log B26 mới hoặc số đo thời gian vòng để khẳng định nguyên nhân duy nhất.

Source B26 chuyển thẳng sang counter-rotation khi mất line có hướng. Ngoài ra driver đặt cả hai PWM về zero cùng update nếu cả hai đang đổi chiều khi đổi hướng pivot. Test driver mới tái hiện lệnh zero này; chưa chứng minh bánh xe dừng cơ học vì lệnh ngắn đó.

## Rẽ bằng mức PWM đang có

Default giữ **Base45 / Max85**, PID **12 / 0 / 0,12**. TRACK vẫn dùng PID liên tục và không boost trên đường thẳng.

Khi mất line đã xác nhận hướng và command trước đó vẫn tiến, trung bình ít nhất35%:

1. TURN tạo **forward arc ngay** từ trung bình hai lệnh bánh lúc vào cua, không có pha brake trước đó. Correction bằng0,8 lần common PWM. Nếu vào ở45%, yêu cầu gần **81/9%** hoặc **9/81%**: trung bình45% khi Max đủ headroom.
2. Arc chỉ được phép trong **28ms đầu của recovery episode**, tính chung vào deadline1200ms. Nếu Max thấp, common giảm để bánh ngoài không vượtMax; không đảm bảo giữ tốc độ cơ học.
3. Line valid xuất hiện ở bất kỳ frame nào: chuyển forward PID ngay, không chờ hết28ms. Arc thành công giữ targetBase trong ALIGN/post-turn thay vì cap40 riêng của B26.
4. Nếu sau28ms vẫn chưa thấy line: chuyển pivot giới hạn36%, không chạy tiến mù vô hạn. Khi đã pivot, valid rejoin vẫn giữ chính sách40% của B26.
5. Reloss trong post-turn không được cấp lại arc28ms; giữ bounded pivot retry để hạn chế lao thẳng ở chuỗi góc vuông. Line flicker không gia hạn recovery deadline. Entry thấp hơn35% giữ fallback pivot như cũ.

Đây là một đoạn rẽ có giới hạn, không phải ADVANCE theo16cm hay lệnh chờ trước khi quay. Cả hai PWM tiến không đảm bảo cả hai bánh đều lăn đúng tốc độ, nhất là bánh trong ở duty thấp. Arc có thể làm thân xe đi ra ngoài/cắt nhánh nhiều hơn; đây là tradeoff thử nghiệm đã được owner chấp nhận, chưa được hardware-verified.

## Driver khi đổi hướng quay tìm

Khi hai bánh cùng đổi dấu để đổi hướng pivot, driver B27 stagger reversal guard: cho bánh tiến mới về zero trước, giữ bánh tiến cũ một update trong giới hạn yêu cầu mới; update tiếp theo guard bánh còn lại. Mỗi bánh vẫn đi qua zero trước đổi dấu, nhưng không chèn both-wheel zero trong tình huống này.

Explicit Stop/fault vẫn yêu cầu brake cả hai ngay. Đổi cả hai bánh từ tiến sang lùi thẳng vẫn dùng guard cũ. Không chuyển sang coast và không bỏ bảo vệ đảo chiều.

## Giảm lắc qua nét đứt

B26 lưu trực tiếp correction có cả D kick của một mẫu mắt giữa vào guide rồi giữ qua trắng. B27 giữ guide gốc cho ambiguity, nhưng lưu thêm correction đã lọc riêng cho GAP, hệ số0,04 mỗi1ms (nominal25ms). Common PWM giữ theo guide, hướng qua trắng được làm dịu; TRACK PID vẫn phản ứng ngay như B26.

GAP vẫn giới hạn300ms, giữ cặp lệnh đã lọc cố định trong một khoảng trắng, không tự tính centroid trên trắng. Nhiễu nhỏ hơn nửa một PWM permille được đưa vềzero; clip correction để không tạo PWM âm hoặc vượtMax. Khi lại có line valid, nhận PID ngay và reset D tại lần rejoin như trước. Không thay threshold estimator hoặc Flash logger.

## Validation và thử xe

- Baseline controller B26 fail **120032**: lost entry yêu cầu bánh âm thay vì forward arc. Baseline driver fail **50056**: retarget pivot chèn PWM zero cả hai.
- **102.331 ARM assertions +8 parser/layout +18 receiver tests** pass; test arc first-frame, mọi thời điểm rejoin, expiry28ms, Max/dt, post-turn không renew, deadline, dash kick, stagger driver, Stop/watchdog.
- Emulator giữ giới hạn300M instructions/60s; không coi đó là WCET trên MCU. Flash/CSV record/header vẫn48/108bytes.
- Keil0Error/0Warning, Code32724 / RO1856 / RW440 / ZI10512bytes. HEX kết thúc `0x080087E8`, dưới vùng log `0x0800C000`. SHA256 `F21387210846145B99728119524519AEA330AA85A87DCFE8B70C27245E0F95A4`. Chưa chạy xe B27 và chưa có kết quả lap time.

Nạp `build/linefollower-B27.hex`, kiểm tra OLED B27 rồi calibration lại. Giữ45/85 và PID hiện tại trong lần so sánh đầu; chú ý góc đầu sau thẳng, góc nối tiếp và nét đứt. BLACK BOX CORNER có thể dùng để kiểm tra `mode=TURN` với `reason=SPEED_CORNER` trong arc, sau đó `SPEED_RECOVERY` nếu cần pivot. Đây là PWM request, không phải vận tốc đo được.

Backup trước sửa: `backups/B26-before-B27-20261029`. Chưa push GitHub hoặc tự nạp board.
