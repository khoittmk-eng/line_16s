# B09: Base cruise và giữ lệnh qua nét đứt

Owner báo xe khựng trên khoảng trắng, đưa lại lên đen thì chạy; cua nhẹ cũng khựng.
Owner không đọc được OLED khi xe chạy và chọn bỏ tăng tốc tự động, dùng Base cao để đi mượt.
B09 thực hiện lựa chọn đó. Chưa có trace thực nên không quy toàn bộ hiện tượng cho một nhánh duy nhất.

## Cruise

| Parameter | B09 |
| --- | --- |
| Base | 45% common PWM target trên đường thẳng và cua nhẹ |
| Max | 75%, chỉ là trần PWM từng bánh, không tự trở thành target |
| Kp / Ki / Kd | 12 / 0 / 0,12, giữ gain trước |
| Ramp khởi hành / giảm | 80 / 600 điểm phần trăm/giây |
| Pivot / ramp quay | ±30% / 450 điểm phần trăm/giây, cap Max |
| RECENTER | min(Base; 26%), Reacq 12 ms |

Không có boost/cooldown/stability gate cho tốc độ. Ramp khởi hành vẫn có để tránh nhảy PWM từ 0 lên Base.
Khi abs(error)<=1,5 pitch, target luôn là Base, kể cả nhóm line valid rộng hoặc D có jitter.
Từ 1,5 đến 3 pitch, target giảm tuyến tính từ Base đến min(Base; 28%); trên 3 pitch giữ mức đó.
Với default: error 1,5 →45%, error 2 →khoảng 39,3%, error 2,5 →khoảng 33,7%, error 3 →28%.
Valid line vẫn forward PD; không thêm thời gian chờ trước cua. Max/headroom có thể làm trung bình
PWM hai bánh thấp hơn common target khi lái mạnh. Các % không là vận tốc đo được.

## Lỗi đã tái hiện ở B08

GAP cũ yêu cầu abs(D PD)<12 ngay lúc mất line. Một thay đổi sang mắt giữa error=0,5 pitch
tạo D đủ lớn để từ chối GAP, dù line trước đó vẫn ở gần tâm. Test mới fail 30035 trên B08.
Nhánh ForwardHold cũng giảm PWM về Base; ambiguous PROBE có thể giảm tới 15%.
Test cũ chỉ kiểm tra PWM dương, không chứng minh motor GA25 còn quay ở duty thấp đó.
B09 kiểm tra giữ nguyên lệnh hai bánh trước gap, không chỉ nonzero.

## Guide memory và GAP

Sau mỗi frame valid gần tâm, controller lưu lệnh L/R đã tính nếu:

- abs(error)<=0,8 pitch, cả hai PWM đang tiến;
- mask không chạm hai mắt ngoài mỗi phía (mask & 0xC3 bằng 0);
- rate qua filter 40 ms không vượt 12 pitch/s. D filter 15 ms của PD giữ nguyên.

Guide có tuổi tối đa 80 ms khi bắt đầu mất line. Error >1,2 pitch, mask vươn ra bên hoặc rate lớn
hủy guide ngay để không lấy một góc thật làm đường thẳng. Khi eligible và toàn trắng:

1. Vào GAP, phát lại L/R của guide mỗi frame 2 ms, vẫn clamp theo Max.
2. Không hạ về Base, không tăng tốc, không full brake hay chờ đến hết timer.
3. Thấy line valid thì PD nhận lại ngay trong cùng RUN, reset D/I để tránh kick; không cần N3 lần nữa.
4. Span invalid liên tục tối đa 300 ms. Hết allowance gây FAULT_LINE/R=GAP_TIMEOUT, output brake và cần Start rõ ràng.

Một split mask ngắn nằm giữa hàng sensor được grace tối đa 12 ms bằng cùng lệnh guide;
thời gian đó tính chung vào span 300 ms. Full-width/outer-edge không dùng grace này.
Ambiguous kéo dài hơn grace hủy guide, trở về PROBE tối đa 80 ms theo policy cũ.
Không dùng timer 300 ms để quy đổi thành độ dài đoạn trắng. Một hàng sensor không phân biệt chắc chắn
gap với đường cụt nếu lịch sử giống nhau; không encoder nên không đo được quãng đường/heading.
Nếu PWM được nhớ vốn quá thấp thì giữ request không bảo đảm bánh quay dưới mọi tải.

## Phanh và supervisor

Driver motor và pulse 12 ms/ngưỡng entry >40% giữ nguyên. Guide bị hủy khi vào recovery.
Mất line phía đã biết vẫn confirm 6 ms, pivot target 30%; valid line trong BRAKE/TURN nhận RECENTER ngay.
BRAKE/recovery/RECENTER chung deadline 1.200 ms. ADC/UI watchdog, motor lease, Stop và Start interlock giữ nguyên.
FAULT không tự được xóa chỉ bởi cảm biến thấy đen. Nối lại đen trong allowance GAP là tiếp tục RUN, không là auto-restart sau fault.

## Thử

Nạp `build/linefollower-B09.hex`, Reset và calibration/white reference lại.
Default PID Setup Base 45/Max 75, gain 12/0/0,12. N3 Start như cũ.
Thử một đoạn liền → nét đứt → cua nhẹ; sau đó mới đối chiếu góc 90° hai phía.
Không cần đọc OLED khi xe đang di chuyển. LAST RUN vẫn có khi Stop; B09 không phát nhãn BST/WAIT nữa.

Build Keil 0 error/warning; 33.993 assertion pass, gồm actual estimator→controller→motor-request
giữ L/R 240 ms trắng rồi nhận lại đen không Start; transient mask, D kick, stale/edge guide rejection,
white-span boundary, watchdog trong GAP và fault không tự restart. Không mô phỏng tải/ma sát/current/quán tính.
Backup B08 ở `backups/B08-before-B09-20261024`. B09 vẫn cần thử trên xe thực.
