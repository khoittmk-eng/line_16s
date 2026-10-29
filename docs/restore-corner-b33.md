# B33 — Khôi phục cách cua B30 sau phản hồi B32 văng/cua rộng

## Bằng chứng và lựa chọn

Owner báo B32 chạy lâu hơn B30 và xác nhận biểu hiện chính: **cua rộng hoặc văng rồi mới bắt lại line**. Chưa có số vòng, thời gian cụ thể, điều kiện pin/Base/Max hoặc log để tách nguyên nhân vật lý.

B33 khôi phục hai chính sách của B30: zero command của mỗi bánh dùng short brake và arc dùng hệ số0,8 thay1,0. Đây là trở lại mốc owner đánh giá tốt hơn, không phải bằng chứng một thuật toán mới nhanh hơn B30. Giữ PID nền12/0/0,12, Base45/Max85, extra P trần1,0 và rise/release0,08/ms. Chưa thay ngưỡng cue/gain hoặc thời gian tiến mù.

## Hành vi

- `Motor_SetDuty` với một bánh0: riêng bánh đó short brake, bánh còn lại vẫn nhận drive. Motor Test một bánh cũng trở lại chính sách này.
- Arc sau confirmed loss, entrymean>=35: correction0,8common, tối đa28ms trong episode hiện có; tại entry45/Max85 yêu cầu81/9%, mean45. Khi entry42,5 từ valid85/0, yêu cầu76,5/8,5%, mean42,5 như B30. Không coi việc giữ nguyên chênh lệch85 qua loss là tiêu chí bắt buộc nữa.
- Giữ forward PD ngay khi line valid, fallback search50/36, post-turn retry không cấp thêm arc, memory/deadlines và GAP/ambiguity/dash.
- Giữ phần bảo vệ B32: `Motor_StopBrake` riêng, atomic; command lease20ms theo dõi mọi drive command, explicit StopCoast hủy lease. Guard đảo chiều/stagger dùng zero brake như trước B32. Stop/fault/watchdog/maintenance vẫn brake; không auto-start.
- STBY vẫn3V3; không sửa wiring, estimator, ADC, PWM nonzero, Flash/CSV48/108 hoặc schema.

Giả thuyết coast làm bánh trong giữ đà và khiến cua mở rộng phù hợp phản hồi, nhưng chưa được xác nhận độc lập. Arc1,0 cũng là một thay đổi đồng thời. Việc hoàn nguyên cả hai giúp có mốc chạy lại; không phân lập được ảnh hưởng của từng thay đổi.

## Validation

- Backup trước sửa: `backups/B32-before-B33-20261029/B32-source.zip` gồm BSP và `linefollower-B32.hex`.
- Source comparison: controller/header/tuning khớp B30 khi bỏ comment/whitespace; SetWheel và SetDuty khớp driver trước coast khi bỏ phần flag lease B32 được giữ lại. Đối chiếu hai archive B30/B31 trước sửa.
- Test mới B33 fail120036 trên B32 vì arc còn bánh trong0. Sau hoàn nguyên: **104.172 ARM assertions +8 parser/layout +18 receiver tests pass**. Kiểm tra arc có hai PWM dương, mean/differential tương ứng B30, expiry/rejoin/Max/dt, zero brake từng channel/hai chiều, Stop/coast riêng và watchdog, cùng suite controller/square-chain/GAP/driver/interlocks.
- Emulator300M instructions/60s hoàn tất `test_done`; không là đo dynamics hoặc WCET trên board.
- `scripts/build.ps1`: Keil0Error/0Warning; Code33316/RO1856/RW444/ZI10524bytes. HEX kết thúc0x08008A3C, không chạm vùng log0x0800C000.
- Artifact: `build/linefollower-B33.hex`; SHA256 `C88EA07A3D9DE87D168BF431F6ADF4876762DD0A2F83F7C3F9BD51F825E53705`.

## Chạy lại

Chưa nạp hoặc chạy B33 trên xe. Sau nạp kiểm tra OLED B33, calibration/white reference; dùng45/85 và gain nền như B30. So nhiều lượt với B30/B32 trong cùng điều kiện, ghi cua rộng/văng/mất line và thời gian vòng. Nếu cần giữ log, Export trước reflash; không full erase hoặc đổi option bytes. B33 chưa được chứng minh nhanh hơn B30 hoặc khắc phục hoàn toàn regression.
