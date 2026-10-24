> **Baseline B05:** bản hiện tại B07 xem [speed-b07.md](speed-b07.md), baseline phanh B06 ở [high-speed.md](high-speed.md). Các default/timing tốc độ B05 bên dưới là lịch sử.

# B05: sửa regression bám line và quay vòng của B04

## Phản hồi owner và giới hạn kết luận

Owner xác nhận B04 tệ hơn B03: cua vuông xoay vòng không đi tiếp, cua nhẹ văng line;
quan sát một bánh đứng và bánh còn lại quay. Owner không đọc được mode/F/R lúc xe đang chạy.
Không thể kết luận toàn bộ do PID hoặc kích thước owner cung cấp.

Các nhánh source B04 có thể gây regression:

- CornerCue kích hoạt TURN/ADVANCE khi vẫn đang có một line hợp lệ, gồm cả error lớn ở cua thường.
- ADVANCE 350 ms không có wheel speed feedback nên không chứng minh đã tiến đúng 16 cm.
- TURN yêu cầu từng thấy toàn trắng, thời gian tối thiểu và center hẹp; có thể bỏ qua line đã bắt lại.
- ALIGN có thể trở về pivot thay vì ưu tiên dữ liệu line hiện tại.
- Duty pivot bất đối xứng: bánh ngoài 35%, bánh trong lùi 18% tại Base 30%; mức lùi có thể không thắng tải.
  Đây là khả năng cần đo, chưa phải bằng chứng motor cụ thể đã stall.

B05 loại bỏ các ràng buộc geometry/timer này. Không khẳng định đã sửa thành công trên xe trước khi owner thử.

## Chính sách B05

1. Khi line đơn hợp lệ, PD điều khiển hai bánh chạy tới. Error lớn/cột ngoài chỉ ghi nhớ hướng,
   không tự gọi một routine góc cố định. Cua nhẹ và cua rộng không bị chuyển sang pivot chỉ sau 6 ms cue.
2. Toàn trắng ngay sau line gần tâm được GAP tối đa 200 ms, không tăng tốc trên trắng.
3. Mất line ở mép được xác nhận trong 6 ms rồi mới pivot theo hướng cuối.
4. Khi thật sự recovery, target hai bánh bằng nhau và ngược chiều, **+20%/-20%**, không vượt Max.
   Output vẫn có ramp và driver vẫn có một zero command khi đảo dấu.
5. Bắt lại một line hợp lệ thì chuyển ngay sang **RECENTER**: PD chạy tới ở duty thấp, không tiếp tục ép pivot
   để chờ hết thời gian hoặc chờ thấy toàn trắng.
6. Line gần giữa ổn định mặc định 12 ms thì về TRACK, reset D để tránh giật khi kết thúc recovery.
7. Recovery/RECENTER tổng cộng không quá 1.200 ms; mẫu line chập chờn không reset deadline.
8. Line rộng/nhiều nhóm chưa rõ: PROBE tối đa 80 ms; có hướng đáng tin thì recovery, không có thì Stop.
9. ADC, timing, UI watchdog, motor command lease và nút Stop giữ nguyên.

`advance_ms` không có tác dụng trong controller B05, kể cả giá trị cũ khác 0.
System → N3 mở Recovery: Advance hiển thị DISABLED; chỉ chỉnh Reacq 4..40 ms, bước 2 ms.
Các enum legacy giữ để tương thích giao diện, nhưng ADVANCE không được phát sinh trong RUN.
Không dùng kích thước 16 cm/40 cm để tính lệnh di chuyển trong B05.

## Parameter để đối chiếu

- Kp=12, Ki=0, Kd=0.08: giữ nguyên, không đồng thời tăng gain với sửa state machine.
- Base=20%, Max=30%: trở về mức thử chậm để so với bản từng bám vòng tròn/cua nhẹ.
- PID Setup vẫn hiển thị cả năm dòng; N2 edit, N1 tăng, N3 giảm, N2 xác nhận.
- Parameter và calibration lưu RAM; Reset trở về mặc định và cần calibration/white reference lại.
- Duty là PWM request, không phải RPM đo được.

Một bánh có PWM request 0 khi PD sửa lệch mạnh có thể là lệnh có chủ đích.
Nếu PWM request hai bánh khác 0 mà một bánh thực tế đứng, cần kiểm tra chiều quay, tải, nguồn và ngưỡng thắng ma sát;
không thể suy ra từ phần mềm rằng cả hai bánh thật sự đã quay vì không có encoder.

## LAST RUN: đọc sau khi xe đã dừng

Trong màn Run, nhấn N3 để Stop. Sau khi đã dừng, nhấn ngắn N1 để xem LAST RUN:

- Mode ở frame hoạt động gần nhất: TRACK, TURN hoặc RECENTER.
- Error và sensor mask của frame đó.
- PWM request L/R trước Stop.
- SAT: thời gian error lớn tại mép và một output bằng 0, dùng chẩn đoán, không tự kích hoạt pivot.
- F là fault hiện tại nếu có.

Nhấn ngắn N1 lại về Live. N3 vẫn Start một lượt mới theo điều kiện calibration/ADC/line.
Giữ N1 vẫn là đổi steering DIR khi dừng; không nhầm với nhấn ngắn để xem snapshot.
Snapshot không phải đo wheel speed và không phải đầy đủ lịch sử chuyển động.

## Quy trình chạy thử

1. Nạp `build/linefollower-B05.hex`, Reset, thấy B05 trên OLED.
2. Calibration + white reference; xác nhận line đen mạnh, nền trắng yếu, CAL:1 W:1.
3. Kê bánh, dùng Run kiểm tra line ở bên phải làm PWM bánh trái lớn hơn bánh phải; ngược lại nếu line ở trái.
   Nếu sửa hướng sai, Stop và giữ N1 đổi DIR trước khi chạy dưới đất.
4. Giữ Base 20/Max 30 để thử cua nhẹ trước, rồi góc riêng, cuối cùng mới chuỗi góc/nét đứt.
5. Nếu vẫn một bánh đứng/quay vòng: N3 Stop → N1 ngắn LAST RUN, ghi mode, error, mask, L/R và F.
   Dùng dữ liệu này để phân biệt PD saturation, recovery hoặc motor không thực hiện PWM request.
6. Chỉ tuning gain sau khi xác nhận polarity, chiều steering, frame mới và state transition đúng.

B05 không bảo đảm thân xe bám vuông đúng hình học ở mọi tốc độ. Ưu tiên lấy lại baseline bám line ổn định;
độ chính xác góc/quãng đường cần thêm measurement thực tế, không thể chứng minh từ synthetic mask tests.
