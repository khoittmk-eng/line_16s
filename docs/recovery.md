> **Tài liệu lịch sử. B07 hiện tại xem [speed-b07.md](speed-b07.md). Không áp dụng lại ADVANCE/timing B04 để sửa regression.**

# B03: cua gấp và nét đứt đường thẳng

## Phản hồi thực tế và nguyên nhân trong source

Owner xác nhận B02 ở Base 20% bám được vòng tròn/cua nhẹ, nhưng dừng khi gặp cua 90°.
Chưa có F/R hoặc chuỗi sensor thực tại thời điểm dừng nên chưa khẳng định nhánh nào đã xảy ra.
Review B02 cho thấy: invalid mask khác 0 bị Stop ngay; gap chỉ 80 ms và cần 100 ms ổn định;
SEARCH dùng error cuối, nếu gần 0 thì dừng. Những điều kiện này không phù hợp nhiều mẫu cua/nét đứt.
B03 thay chính sách recovery, giữ white reference, PD frame 2 ms và các watchdog.

## Mặc định chạy thử

- Base 30%, Max 45%; Kp 12, Ki 0, Kd 0.08.
- Acceleration vẫn 30 điểm phần trăm PWM/giây, deceleration 100 điểm phần trăm/giây.
- Firmware tag: B03. Nạp `build/linefollower-B03.hex`, Reset rồi calibration + white reference lại.
- Sau boot không tự chạy; N2 vào Run rồi N3 Start/Stop như cũ.
- Motor Test vẫn 20%, không đổi theo Base.

## Nhận dạng và xử lý góc gấp

Controller dùng mask đã chuẩn hóa về chiều steering: bit 0 bên trái, bit 7 bên phải.
Nếu DIR bị đảo, supervisor đảo cả error và thứ tự mask; không chỉ đảo dấu PD.

1. Line chạm mép một phía và phía đó có nhiều mắt hơn, hoặc valid error lệch ít nhất 2.4 khoảng mắt, tạo corner cue.
2. Cue cùng hướng trong 6 ms và xe đã có motion history sẽ chuyển TURN.
3. Nhớ phía cue 250 ms: nếu sau đó thấy toàn trắng hoặc full-width, vẫn giữ phía quay.
4. TURN dùng bánh ngoài tiến, bánh trong lùi; duty ngoài khoảng 20..35%, trong khoảng 12..20%, đều không vượt Max.
   Với Base 30/Max 45, target là ngoài 35%, trong lùi 18%.
5. Output khi vào TURN thay đổi có ramp 250 điểm phần trăm/giây, không nhảy ngay sang target đảo chiều.
6. Giữ TURN qua các mẫu mất line hoặc chỉ thấy mắt ngoài. Chỉ về TRACK khi thấy line giữa, mép ngoài đã clear,
   error không quá 0.8 khoảng mắt trong 12 ms, và TURN đã chạy ít nhất 30 ms.
7. Timeout TURN/SEARCH là 1.400 ms, tính liên tục; một mẫu line ở mép không reset timeout.

Đây là phản ứng theo sensor, không phải đo góc 90° bằng encoder/IMU.
Timing và duty quay cần xác nhận trên GA25 thực; không bảo đảm mọi hình học góc chỉ từ test logic.
Motor driver vẫn chèn một command zero khi đổi dấu, nên chuyển tiến/lùi có thể có khoảng brake rất ngắn.

## Nét đứt đen–trắng trên đường thẳng

- Nếu mẫu line cuối gần tâm, D thấp và đã có history: vào GAP khi không thấy line, không cần thêm 100 ms ổn định mỗi đoạn.
- GAP được giữ tối đa 200 ms. Không tăng duty khi toàn trắng; chỉ giảm dần về Base nếu trước đó cao hơn.
- Correction cũ được giảm dần để tiếp tục gần hướng thẳng thay vì quay theo nhiễu.
- Khi bắt lại đoạn đen: reset D history, giữ forward speed và stability history; không reset tốc độ về 0 mỗi nét.
- Đoạn trắng quá 200 ms: nếu còn error chỉ hướng đáng tin cậy thì SEARCH có giới hạn; nếu không có hướng thì Stop.
- Nét đứt liên tiếp không được bỏ qua ADC/UI watchdog hoặc cho phép chạy mù vô hạn.

Gap time là giới hạn thời gian, không phải chiều dài gap đo được. Không có encoder nên cần thử theo đường thật.

## Line rộng hoặc nhiều nhóm mắt

Estimator vẫn đánh dấu invalid để centroid không bị hiểu nhầm là giữa line.
Controller không Stop ngay chỉ vì invalid mask khác 0:

- Có corner hint: TURN theo phía đã nhớ.
- Chưa có hướng: PROBE tiến chậm tối đa 80 ms, không tăng tốc, chờ hình dạng rõ hơn.
- Không rõ nhánh sau giới hạn: Stop; chưa có maze/junction route selection.

## Mã dừng trên OLED

Fault F giữ ý nghĩa cũ. Khi F=2, xem thêm R:

| R | Nguyên nhân |
| --- | --- |
| 0 | Không có line-stop reason |
| 1 | Khoảng trắng quá giới hạn, không có hướng recovery đáng tin |
| 2 | TURN/SEARCH quá 1.400 ms chưa bắt được vùng giữa ổn định |
| 3 | Line rộng/nhiều nhóm không rõ nhánh quá 80 ms |
| 4 | Chưa có tracking history để phục hồi |

Nếu xe dừng, ghi F/R và mode trước khi nhấn Start lại. F=1/3/4/5 là ADC/timing/UI/OLED,
không được bỏ watchdog để xử lý lỗi hình học line.

## Tham số và kiểm chứng

Timing nằm tại `app/control_tuning.h`; không nằm trong menu PID Setup để giữ bố cục cũ.
Test bổ sung ở `tests/test_recovery.c` và supervisor test:

- Cua phải/trái với nửa hàng đen → toàn trắng → bắt lại giữa.
- Một edge cue ngắn trước full-width vẫn giữ hướng.
- Line ở mép chập chờn không thoát TURN sớm hoặc kéo dài timeout vô hạn.
- Hai mươi chuỗi black 40 ms / white 120 ms không reset về Stop hoặc tăng tốc trên trắng.
- GAP quá giới hạn, TURN timeout và ambiguous timeout vẫn Stop.
- Gentle curve vẫn dùng PD chạy tới; steering DIR đảo thì corner mask cũng đảo.

Build và test algorithm đã pass; B03 chưa được thử phần cứng.
