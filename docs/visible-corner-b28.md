# B28 — Tăng lực lái khi còn thấy line và quay tìm hai mức

## Mốc so sánh và trạng thái

Owner báo B27 ở Base45 đạt17,30giây, nét đứt đã ổn, nhưng thời gian cua chưa khác nhiều bản trước. Đây là kết quả owner cung cấp, không phải phép đo lặp lại của phiên này. Owner yêu cầu kết hợp các đề xuất gain scheduling/nhận góc sớm và thử nhiều phương án.

B28 đã build/test source. **Chưa chạy B28 trên xe; chưa có thời gian vòng hoặc quỹ đạo đo được.** Mặc định vẫn Base45/Max85, Kp12/Ki0/Kd0,12.

## Tác động trước khi mất line

B27 vẫn dùng P cố định khi line valid; forward arc chỉ bắt đầu sau loss có hướng. B28 thêm extra P khi đồng thời:

- TRACK đang có line valid, có shape lệch một bên vươn tới mắt ngoài và đã xác nhận6ms;
- error cùng phía shape và ít nhất1pitch;
- planner rate chưa cho thấy error đang trở về tâm.

Đây là cue heuristic, không chứng minh hình học90°. Một cua rộng cũng có thể tạo shape này; không dùng một mắt ngoài đơn lẻ để ép quay.

Luật thử nghiệm, với error theo pitch:

```text
extra_target = 0.6 × clamp(|e| - 1, 0, 1)
extra_gain tiến dần tới extra_target, tối đa0.04 mỗi ms
Kp_effective = Kp × (1 + extra_gain)
correction = Kp_effective × e + Ki × integral + Kd × derivative
```

Giới hạn steering/Max và anti-windup vẫn áp dụng. D vẫn low-pass15ms, không tăng Kd đột ngột. Nếu cue không còn hợp lệ, đang RECENTER, hoặc planner rate cho thấy quay về tâm thì extra gain được nhả; góc đối diện phải xác nhận lại. Sai số trái/phải giữ đúng dấu, không dùng e² trực tiếp.

Ví dụ pure P với e1,5: B27 correction18%; B28 tiến dần tới23,4%, tương đương Kp_effective15,6 khi Kp menu là12. Không có floor ép thẳng lên0,8steering_limit của B24. Với e2, mức extra tối đa0,6 và Kp_effective19,2, trước các giới hạn output. Khi steering đã saturation, tăng gain không tạo thêm lực lái ngoài trầnPWM.

PID Setup hiển thị **gain nền**; metadata CSV cũng giữ gain nền. Phân tích log B28 phải tính tới scheduling theo firmware revision, không hiểu Kp12 là Kp_effective luôn cố định.

## Ba phương án đã so sánh

Chạy code ARMCC thật trên cùng chuỗi64frame giả lập, pure P, Base45/Max85. Không mô phỏng motor, quán tính, ma sát hoặc vị trí xe.

| Phương án | Extra correction tại cue6ms | Bước extra lớn nhất khi vào cue | Correction sau cue ổn định |
| --- | --- | --- | --- |
| Controller B27 | 0% | 0% | 18% |
| Gain đổi tức thì | 5,4% | 5,4% | 23,4% |
| Gain tăng dần — chọn cho B28 | 1,44% | 1,44% | 23,4% |

Phương án tăng dần giữ nguyên output ở đoạn cua mềm không có cue và giảm bước tăng lực lái ban đầu so với đổi gain tức thì. Đây là lý do chọn về continuity/output bounds, **không phải bằng chứng nhanh hơn trên sân**. Source B27 fail test mới130035 vì chưa có extra steering ở visible cue.

Chạy lại sau release:

```powershell
python scripts/compare-corner.py
```

Script đọc controller B27 từ ZIP backup local, compile ba candidate và xuất CSV vào `build/corner-comparison`. Có thể chỉ định `--baseline-source` trước khi release. Không nạp hoặc tác động MCU. Dependency dùng `build/test-deps` như suite ARM hiện có.

## Quay tìm hai mức

- Giữ forward arc28ms của B27 trong episode phù hợp.
- Khi cần pivot và toàn bộ mask trắng: request min(Max,50%) mỗi bánh đối dấu, thay36%.
- Khi có bất kỳ tín hiệu đen nhưng estimator chưa valid: request min(Max,36%), không tiếp tục quét mạnh qua tín hiệu mới.
- Valid line ở bất kỳ mắt nào: nhận forward PD/RECENTER ngay, không chờ mắt giữa mới kết thúc pivot. Rejoin sau pivot vẫn cap40; arc thành công vẫn dùngBase.
- Không thêm xung âm cực mạnh20–50ms, không minimum-spin/geometryADVANCE. Post-turn memory, split confirmation, GAP300ms và recovery deadline1200ms không được gia hạn.

## Validation

- **104.022 ARM assertions +8 parser/layout +18 receiver tests** pass. Candidate comparison pass, không compiler warning.
- Test extra P hai phía, ramp trước loss, cue release/opposite, soft curve/centered3C/outer-eye không false-select, anti-windup/Max/dt, search50→36→PD, deadline và toàn bộ suite cũ.
- Emulator vẫn300M instructions/60s; không phải WCET đo trên STM32. Record48/header108bytes và Flash layout không đổi. Motor/SWD/estimator/ADC/supervisor không sửa.
- Keil0Error/0Warning; Code32956 / RO1856 / RW440 / ZI10520bytes. HEX kết thúc `0x080088D0`, dưới vùng log `0x0800C000`.
- SHA256: `824DEF484B0028BB7029E80B02EE208D9D16C408249D28F38276B5BCA3BF9185`.

## Thử trên xe

Nạp `build/linefollower-B28.hex`, kiểm tra OLED B28 rồi calibration/white reference lại. Giữ45/85 và gain nền12/0/0,12 trong lượt đầu để so với mốc17,30giây. Quan sát góc đầu sau thẳng, chuỗi góc vuông và khả năng bắt lại sau quét nhanh; extra steering có thể làm quay sớm/cắt nhánh, và search50 có thể quét quá line. Cần log CORNER hoặc LOSS để xác định nếu có regression; chưa tăng Base trong lượt này.

Backup source/HEX B27: `backups/B27-before-B28-20261029`. Không push GitHub hoặc tự nạp board trong phiên.
