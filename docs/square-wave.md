> **Tài liệu lịch sử. B07 hiện tại xem [speed-b07.md](speed-b07.md) và [runtime.md](runtime.md). Không áp dụng lại ADVANCE/timing B04 để sửa regression.**

# B04: line xung vuông liên tiếp và chỉnh tốc độ trên OLED

## Thông tin owner cung cấp

- B03 cải thiện cua gấp và khoảng trắng, nhưng ở chuỗi góc vuông xe rẽ sớm và thoát cua khi thân còn chéo.
- Khoảng cách hàng 8 mắt đến trục hai bánh chủ động theo chiều dọc xe: khoảng **16 cm**.
- Đoạn line thẳng giữa hai góc: khoảng **40 cm**.
- Đây là số gần đúng do owner cung cấp, chưa phải phép đo odometry trên firmware.

Khi cảm biến vừa thấy góc, trục bánh vẫn có thể ở phía sau góc 16 cm.
B03 TURN ngay khi có cue và thoát khi thấy vùng giữa trong 12 ms; cách đó chưa xét đầy đủ offset
hoặc việc thân xe có thể còn chéo dù một hàng mắt đã thấy line ở giữa.
Không có encoder/IMU nên không thể khẳng định góc quay thực là 90° chỉ bằng một timestamp.

## Thay đổi B04

```text
TRACK → góc L rõ → ADVANCE → TURN → ALIGN → TRACK
                        timeout / lỗi → FAULT
```

- Chỉ góc có chữ ký L rõ mới có ADVANCE: cue một phía, có mắt giữa và ít nhất ba mắt ở nửa phía rẽ.
  Generic edge-loss SEARCH/TURN vẫn giữ cách phục hồi có giới hạn, không tự tiến dài khi chỉ thấy một mắt ngoài.
- ADVANCE giữ hướng rẽ đã chọn, đưa hai bánh về cùng duty không quá 30%, tiến thêm trước khi pivot.
- Sau đó TURN dùng logic quay có ramp như B03. Phải từng thấy mask=0 để xác nhận đã rời line cũ;
  chỉ nhìn lại line cũ ngay sau cue không đủ điều kiện thoát TURN.
- TURN tối thiểu 120 ms; vùng giữa phải ổn định 24 ms, error không quá 0.55 khoảng mắt,
  mask không có mắt ngoài và không rộng bất thường so với line đã học khi tracking ổn định.
- Bắt được giữa chuyển sang ALIGN, chưa chuyển thẳng về TRACK.
- ALIGN chạy chậm tối đa 20%, dùng PD nhỏ; chỉ tính thời gian xác nhận khi hai bánh đã chuyển về tiến,
  output gần target, error nhỏ và ít biến thiên. Hết Align mới về TRACK và tăng tốc lại.
- ALIGN mất line hoặc lệch lớn 12 ms thì quay lại TURN, không reset timeout recovery.
- TURN + ALIGN dùng chung giới hạn 1.400 ms; ADVANCE tối đa 800 ms.
- ADC/UI/motor watchdog và N3 Stop vẫn hoạt động trong tất cả các pha.
- GAP thẳng 200 ms của B03 được giữ, không có ADVANCE trên nét đứt thông thường.

Đây là cải tiến state machine theo hình học đã mô tả; chưa chứng minh xe giữ đúng mọi cạnh/góc trên đường thực.

## Chỉnh Base/Max — không phải giá trị cố định

**Main Menu → 3. PID Setup → N2**.
B04 hiển thị đồng thời cả năm dòng Kp, Ki, Kd, Base và Max, không cần cuộn để tìm tốc độ.

1. N1/N3 chọn dòng Base hoặc Max.
2. N2 bật edit: dấu `>` đổi thành `*`.
3. N1 tăng, N3 giảm, mỗi bước 5 điểm phần trăm PWM.
4. N2 xác nhận; giữ N2 về Main Menu.
5. Vào Run bằng N2, N3 Start để thử giá trị mới.

Base không vượt Max. Mặc định vẫn Base 30%, Max 45%.
Parameter đang lưu RAM: Reset/tắt nguồn trở về mặc định; chưa có Save to Flash.
Đây là PWM duty, không phải RPM/tốc độ thực do không có encoder.

## Chỉnh góc trên OLED

**Main Menu → 6. System → N2 → nhấn N3: Corner**.
Sáu mục chính giữ nguyên thứ tự cũ. Submenu mới có:

| Parameter | Mặc định | Phạm vi / bước | Ý nghĩa |
| --- | --- | --- | --- |
| Advance | 350 ms | 0..800 ms, bước 10 ms | Thời gian tiến tham chiếu tại duty 30% trước khi pivot góc L |
| Align | 80 ms | 20..300 ms, bước 10 ms | Thời gian error/output ổn định sau khi hai bánh đã tiến trở lại |

N1/N3 chọn dòng; N2 edit/xác nhận; N1 tăng, N3 giảm khi edit; giữ N2 về Main Menu.

Advance=350 ms chỉ là điểm khởi đầu thử cho offset 16 cm, **không được suy ra như một phép đo tốc độ**.
ADVANCE dùng duty min(Base, Max, 30%). Khi duty dưới 30%, firmware tăng thời gian theo tỉ lệ duty tham chiếu,
nhưng vẫn cap 800 ms. Tỉ lệ này chỉ là xấp xỉ; tốc độ motor không tuyến tính hoàn toàn theo duty.
Advance=0 tắt pha tiến, nhưng vẫn giữ kiểm tra TURN/ALIGN mới.
System N1 Reset PID Default cũng đưa Advance/Align về mặc định.

## Cách hiệu chỉnh thực tế

1. Nạp `build/linefollower-B04.hex`, xác nhận OLED B04, calibration + white reference lại như cũ.
2. Thử từng góc trước, rồi mới chuỗi xung vuông 40 cm. Có thể đặt Base=Max=30% khi hiệu chỉnh để giảm biến số.
3. Quan sát trục hai bánh khi bắt đầu pivot: nếu còn phía sau góc, tăng Advance từng 10–20 ms;
   nếu đã vượt góc, giảm Advance. Không thay gain cùng lúc để dễ đánh giá nguyên nhân.
4. Khi điểm pivot đã hợp lý nhưng thoát cua còn chéo, tăng Align từng 10–20 ms và kiểm tra line/error sau quay.
5. Nếu bị FAULT, ghi F/R và pha ADVANCE/TURN/ALIGN trước đó; không tăng timeout vô hạn.
6. Khi hình vuông đã tốt mới tăng Base/Max từng bước, rồi kiểm tra lại corner timing vì tốc độ thực đã đổi.

Một hàng sensor chỉ cho biết vị trí line tại hàng đó, không đo trực tiếp heading của thân xe.
Nếu cần độ chính xác góc/quãng đường lặp lại cao ở nhiều tốc độ, phải bổ sung thông tin chuyển động phù hợp.

## Kiểm thử phần mềm

Test bổ sung: không skip ADVANCE, không thoát TURN vào line cũ chưa clear,
không vào TRACK chỉ bằng vài mẫu giữa, ALIGN retry không reset deadline,
chuỗi trái/phải liên tiếp phải đi qua đầy đủ các pha.
UI test kiểm tra Base/Max đều có mặt trên màn, thay Max trên menu thực sự giới hạn output khi RUN,
và chỉnh Advance/Align được qua System.
Test thuật toán/supervisor/UI không mô phỏng động lực học xe hoặc xác nhận thời gian 350 ms là đúng cho mạch thực.
