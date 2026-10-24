# B14: nhận dấu hiệu cua khi line còn valid và khoanh vùng lỗi

Ngày: 24/10/2026. Owner báo góc90° ngắn đi thẳng qua rồi LINE LOST, yêu cầu kiểm tra và sửa. Bản này giữ PID12/0/0,12, Base45/Max75, ADC period2ms, brake/pivot/watchdog của B13.

## Những gì đã xác nhận trong source/test

- Nhóm valid F8 (năm mắt bên phải, strength bằng nhau) có centroid error1,5 pitch. Với B13, CruiseTarget vẫn là Base ở ngưỡng này; ConfirmSplit chỉ xử lý invalid nên không giảm tốc theo hình dạng rộng lệch bên. Test mới fail30034 trên B13.
- Chuỗi F8/1F → một đoạn giữa ngắn → trắng có thể đủ điều kiện GAP dù chưa từng recovery. post_turn của B13 không áp dụng vì trước đó chỉ TRACK. Đây là đường đi có trong source, không phải bằng chứng đã ghi được mask thật tại góc của owner.
- TIM3 cấu hình TRGO mỗi2ms, ADC scan8 channel, DMA mỗi nửa buffer là một frame. OLED chạy foreground; control callback không delay/I2C. Có kiểm tra interval1..4ms, execution>1000us và watchdogADC10ms. Chưa đo board nên không kết luận thời gian thực đã đạt đúng500Hz hoặc loại trừ cảm biến vật lý đáp ứng chậm.
- Chưa xác nhận: mask tại góc thực, vận tốc/quán tính, vị trí sensor so với trục bánh và heading khi bắt lại line. Không có encoder/IMU để đo heading.

## Cách sửa

ObserveValidShape xét line valid có mắt giữa, chạm mép, ít nhất ba mắt trong nửa hàng phía rẽ và nhiều nhất một mắt ở nửa đối diện. Cùng phía liên tục6ms mới xác nhận. Nhóm giữa3C và mẫu đổi trái–phải không được coi là dấu hiệu này.

Khi mẫu đó đang hiện diện và được xác nhận:
- Vẫn TRACK/forward PD, không pivot khi line valid.
- Target giảm tới min(target hiện tại,28%), dùng ramp giảm cũ. Label CRN.
- Lưu corner_pending_dir trong200ms từ quan sát xác nhận mới nhất. Bám trusted center100ms clear sớm. Hướng quan sát rõ mới được cập nhật.
- Nếu mất line trong cửa sổ pending, không cấp GAP300ms. Trắng hoặc ambiguous xác nhận6ms thì tìm theo hướng còn nhớ; split mới xác nhận có ưu tiên. Hướng đã nhận được dùng trước khi BeginRecovery xóa pending.
- Không thay đổi các điều kiện Start, Stop, timeout1200ms, post-turn B13 hoặc xử lý fault latch. Không gia hạn vô hạn khi thấy line chập chờn.

Một cua mềm có nhóm đen rộng chạm mép cũng có thể làm xe giảm tốc. Nét đứt ngay sau mẫu rộng cũng có thể kích recovery sớm. Đây là heuristic giảm rủi ro đi tới quá xa, không chứng minh góc90°/giao lộ; không thêm advance theo geometry hay minimum-spin.

## DIAG sau khi dừng

Trong Run, khi xe đã Stop/FAULT, nhấn ngắn N1 lần lượt: LIVE → LAST RUN → DIAG → LIVE.
Start/Stop và sáu menu chính giữ nguyên. Không cần nhìn OLED khi xe chạy.

DIAG lưu cho lần RUN gần nhất, không bị ADC frames lúc đứng yên ghi đè; reset khi Start thành công hoặc Reset MCU:

| Hiển thị | Ý nghĩa |
| --- | --- |
| C | Số lần nhận pending từ mẫu valid rộng lệch bên, hoặc đổi phía pending; không phải số góc thật |
| G | Số lần vào GAP |
| T | Số lần vào BRAKE/TURN từ mode khác; có tính retry từ RECENTER, không phải số vòng quay |
| LAST | Mask valid mới nhất, sau chuẩn hóa DIR |
| LOSS | Mask ở cạnh chuyển valid→invalid gần nhất |
| FROM | Mode ngay trước lần chuyển valid→invalid gần nhất |
| DIR | Hướng memory lúc đó: pending, last_side rồi post_turn; không phải phép đo góc hay chắc chắn hướng đã dùng |
| dt | Interval callback lớn nhất trong RUN, đơn vịus; bình thường dự kiến gần2000us |
| ex | Thời gian xử lý callback lớn nhất đã ghi trong RUN, đơn vịus |
| F/R | Fault và line-stop reason hiện tại |

C/G/T trên màn cap999, dt/ex cap99999; giá trị đầy đủ trong Robot_GetDiagnostics. min_interval_us có trong API nhưng không hiển thị để giữ layout. Timing đến từ DWT trên board; emulator dùng clock stub nên không dùng ex trong test để chứng minh WCET. DMA fault có thể return trước cập nhật interval; F1 vẫn là bằng chứng acquisition fault.

Các số là thống kê cả RUN và loss gần nhất, không phải chuỗi trace đầy đủ. C>0 và G>0 không chứng minh GAP xảy ra ngay tại góc đó. LAST sau một lần reacquire có thể thuộc lần quan sát mới hơn LOSS nếu chưa có loss tiếp.

## Đối chiếu sau khi thử

- F1/F3 hoặc dt bất thường: kiểm tra acquisition/timing, chưa quy cho PID.
- F2/R2, T tăng: đã thử quay tìm nhưng hết deadline; cần xét hướng nhận, quán tính/heading, không chỉ tăng thời gian.
- F2/R1 hoặcR3, C=0: chưa ghi được mẫu valid rộng đáp ứng điều kiện; có thể mask thực khác, không đủ thời gian xác nhận, hoặc không có cue. Không tự kết luận ADC chậm.
- FROM=GAP không thường là cạnh valid→invalid vì GAP đã invalid; trạng thái trước fault xem LAST RUN. G tăng chỉ xác nhận GAP từng được dùng trong RUN.
- C>0 mà xe vẫn vượt: test phần mềm chưa phản ánh dynamics/map; giữ Base để so sánh, chưa tăng tốc.

## Nạp và validation

Nạp build/linefollower-B14.hex, xác nhận B14, calibration + white reference. Giữ Base đang thử (nếu35 thì nhập lại35 sau Reset; default45). Thử cua nhẹ, góc đơn, chuỗi góc ngắn và nét đứt để kiểm tra regression.

34.135 assertions pass: controller hai phía, noise/centered width/TTL, valid PD authority, stable center khôi phục GAP, actual estimator→controller→motor-request với F8→18→00, frozen diagnostics sau fault, reset diagnostics khi Start, timing fault7ms, menu LIVE/LAST RUN/DIAG. Keil0 error/0 warning; Code24576/RO1280/RW344/ZI2616 bytes. UI test kiểm tra nội dung/vị trí qua stub, chưa render trên OLED thật. Chưa hardware-verified.

SHA256 HEX: AA01A638C8F3AE81BDDDB8DD1C8314AF96A3D3DA81CA0E1F5FC8B13B982191A9.
Backup: backups/B13-before-B14-20261024.
