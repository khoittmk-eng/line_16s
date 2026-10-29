# Phân tích log CORNER — B22, sequence 2

Nguồn: `logs/run-20261028-160554-654293.csv`. Parser xác nhận160 sample, CRC32 **B4544906**, cửa sổ334627..335255ms (628ms). Profile55/60/85, PID12/0/0,12, DIR+1, polarity0. Đây là log phần cứng owner cung cấp, không phải fixture emulator.

## Kết luận có bằng chứng

Tại334941ms, controller chuyển từ TRACK sang PROBE do invalid mask0x98 (hai nhóm tách biệt), sau đó0xD9/0xFB/0xFF/...; nó không có pending/side đáng tin, không vào TURN. PROBE hạ target về15%, common request từ33,5% trước sự kiện xuống15,5% sau28ms; line valid lại ở334971ms thì common chỉ15,7%, rồi tăng lại theo ramp80điểm phần trăm/giây.

Trong cửa sổ không có modeBRAKE/TURN, không có sample L/R bằng0, fault0. Vì thế sự khựng được quan sát trong log là **hạ duty do ambiguous PROBE**, không phải nhánh full-brake12ms. Không kết luận lực phanh/tốc độ cơ học từ các lệnh này.

## Mốc chính

| Timestamp ms | Mask | Valid | Mode | Common / target PWM request |
| --- | --- | --- | --- | --- |
| 334935 | 18 | 1 | TRACK | 33,2 /57,8% |
| 334939 | 18 | 1 | TRACK | 33,5 /57,8%; error+0,751pitch |
| 334941 | 98 | 0 | PROBE | 32,3 /15,0% |
| 334953 | FF | 0 | PROBE | 25,1 /15,0% |
| 334969 | 19 | 0 | PROBE | 15,5 /15,0% |
| 334971 | 18 | 1 | TRACK | 15,7 /55,0% |
| 335255 | 18 | 1 | TRACK | 38,4 /59,1% |

Trước sự kiện334717ms có GAP2ms với mask00, giữ lệnh guide rồi TRACK; không phải lần giảm mạnh này.

## Đối chiếu source

- Estimator cho invalid khi có nhiều active groups hoặc maskFF. Invalid ở đây là không xác định được một line, không đồng nghĩa toàn nền trắng.
- `app/control.c`: invalid mask không có accepted split direction/pending → PROBE, `ForwardHold(...min(Base,15)...)`, deceleration600; correction nhân0,85 mỗi frame. Đây là nguyên nhân phần mềm trực tiếp của duty drop.
- Các mask invalid được ghi không thỏa SplitDirection yêu cầu phía ngoài ít nhất3mắt và phía đối diện không quá1mắt. Pending/side trong toàn window đều0; không có valid confirmed-corner cue.
- `app/blackbox.c`: chọn CORNER vẫn bao gồm trigger invalid ngoài GAP. Lần này trigger là invalid/PROBE, không phải xác nhận góc90°.
- Mắt7 raw đi từ2670 ở334935ms xuống1888 ở334939ms (strength467), rồi1508 ở334941ms (strength628). Nó ảnh hưởng centroid trước khi vượt active threshold500. Do đó error nhảy trước khi mask tách nhóm; không được dùng centroid stale trên FF để quyết định hướng.
- Low calibration của mắt4 (index3) là2386, white3242: usable span856count, nhỏ hơn các mắt khác. Đây là điểm cần quét/kiểm tra lại, không đủ bằng chứng kết luận sensor hỏng hoặc là nguyên nhân chính.

## Giới hạn và hướng xử lý

Start334518ms, trigger sau423ms; cửa sổ kết thúc737ms sauStart. Đây vẫn là giai đoạn tăng PWM từ startup, chưa có đoạn ổn định ởStraight60%. Không dùng log này để khẳng định lỗi overshoot khi đang chạy thẳng ở60%.

Ưu tiên nghiên cứu xử lý ambiguous pattern có hướng tin cậy và chuyển tốc độ PROBE/rejoin, thay vì tăngKp/Kd hoặc bỏ mọi phanh. Giữ phân biệt broad/crossing với mất trắng; không invent hướng khiFF. Cần bảo vệ repeated-square/gap/deadline và không lái bằng centroid invalid. Chưa sửa firmware trong lượt phân tích. Log không xác định hình học thực của vùng đen; cần đối chiếu vị trí trên map/video nếu muốn phân biệt góc/giao nhau/mảng đen.
