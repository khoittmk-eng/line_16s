# Validation hiện tại — B09

Ngày 24/10/2026. Owner báo nét đứt/cua nhẹ khựng, chọn Base cruise thay boost.
B09 chưa hardware-verified.

- Keil ARMCC 5 qua `scripts/build.ps1`: **0 error, 0 warning**; Code 22.856, RO 1.188,
  RW 316, ZI 2.572 byte, trong target Flash 64 KiB / SRAM 20 KiB.
- Artifact `build/linefollower-B09.hex`, SHA256
  `DCE548C63A7A2F68CC0D9C0A42DD528BBCD837ABB0CB0D538A94E4C902F0C85F`.
- Reproduction: test đổi error 0→500/mask0x10 rồi trắng fail 30035 trên B08 trước sửa.
- **33.993 assertion pass** qua `scripts/test-control.ps1`. Test semantics B09: Base không boost lên Max,
  jitter/nhóm rộng không throttle cua nhẹ; positional sharp-curve blend và wheel bounds/reflection.
- Gap tests giữ exact L/R thay chỉ kiểm PWM dương; high-duty repeated dashes, D kick,
  central split grace, stale/edge guide rejection, span boundary và fault không restart.
- End-to-end fake frame từ actual estimator→controller→motor-request: 240 ms toàn trắng giữ hai output,
  nhận lại đen trong cùng RUN không cần N3. ADC watchdog trong GAP brake ngay và không bị callback ghi đè.
- Giữ calibration/white reference, UI/edit speeds, explicit Start/Stop, brake pulse/cancel, recovery deadline,
  direction inversion, motor driver fake-SPL register request và command lease tests.
- Driver/pulse/ngưỡng phanh không đổi; B09 thêm guide invalidation khi recovery. Timeout GAP nay 300 ms,
  không nới ADC/UI/motor watchdog. Emulation không mô phỏng quán tính/current/ma sát hoặc xác nhận motor đang quay.
- Bước tiếp: nạp B09, calibration/white reference; thử đoạn liền→nét đứt→cua nhẹ, sau đó góc 90°.
  Không yêu cầu đọc OLED lúc chạy. Xem [cruise-gap-b09.md](cruise-gap-b09.md).

## Baseline B08 (lịch sử)

Ngày 24/10/2026. Owner xác nhận phanh ở profile trước ổn; yêu cầu Base/cua/90° nhanh hơn.
Profile tốc độ B08 chưa hardware-verified.

- `scripts/build.ps1`: Keil ARMCC 5, **0 error, 0 warning**. Code 22.976, RO 1.188,
  RW 316, ZI 2.572 byte, trong target Flash 64 KiB / SRAM 20 KiB.
- Artifact `build/linefollower-B08.hex`, SHA256
  `E3F1E999FF153A46B53BA2FDB2DFA748B6D0EA618C8A50D2921E8E54BEFB3BF0`.
- `scripts/test-control.ps1`: **30.223 assertion pass**. Thêm kiểm tra default profile thực sự đạt
  Base 40, curve target 32/28, RECENTER 26, pivot 30/ramp 450 và vẫn cap theo Max thấp 15%.
- Giữ regression boost/middle-eye/jitter, output bounds/symmetry, brake pulse/cancel, gaps,
  shared recovery deadline, Start/Stop, UI và ADC/UI watchdog.
- So source với backup B07: `bsp/motor.c`, BeginRecovery/RecoveryOutput, deceleration 600,
  brake entry 40/pulse 12, loss confirm 6, GAP 200/PROBE 80 và recovery deadline 1.200 không đổi.
- Emulation và source equality kiểm tra request/policy; không mô phỏng quãng phanh/current/ma sát/góc quay thực.
- Bước tiếp: nạp B08, calibration/white reference, thử profile 40/75 và cua hai phía/nét đứt;
  xem LAST RUN V/T/reason/PWM/B. Xem [speed-b08.md](speed-b08.md).

## Baseline B07 (lịch sử)

Ngày 24/10/2026. Owner đã thử B06 cua mềm ổn, 90° đi được nhưng quay lâu, thẳng chậm.
B07 chưa hardware-verified.

- `scripts/build.ps1`: Keil ARMCC 5, **0 error, 0 warning**. Code 22.976, RO 1.188,
  RW 316, ZI 2.572 byte; trong target Flash 64 KiB / SRAM 20 KiB.
- Artifact `build/linefollower-B07.hex`, SHA256
  `5839819FBAF24A70A77344C034416DFB9882EE647C84B8283500A67C7A8D477C`.
- Reproduction: test mới middle-eye error=500/mask=0x10 fail 40037 trên B06 trước khi sửa.
- `scripts/test-control.ps1`: **30.217 assertion pass**. Thêm middle-eye ±0,5 pitch, jitter nhỏ,
  centered 0x3C boost, marginal spikes, sustained drift và broad-side giảm tốc; xác nhận không full brake trên valid line.
- End-to-end fake frame từ estimator → planner → PWM: một mắt giữa đạt boost vượt Base,
  V/T/reason được giữ sau Stop. UI test kiểm tra vị trí dòng planner; formatter vẫn là stub.
- Recovery: target ±24%, request đạt target với ramp mới, cap Max 15%, thấy line nhả BRAKE/pivot ngay;
  giữ regression brake pulse/deadline, repeated gaps, Start/Stop và watchdog.
- Actual `bsp/motor.c` vẫn được test với fake SPL register writes; không mô phỏng PWM waveform/motor dynamics.
- Mở rộng suite từng hết budget mà chưa test_done, không tính là pass. Completion hook được giới hạn
  tại test_done thay vì global callback mỗi instruction; giữ ARMCC -O1, timeout 60 s, budget 200 triệu instruction
  và yêu cầu test_done. Không nới timing/watchdog của firmware để chạy test.
- Bước tiếp: nạp B07, calibration/white reference, thử cùng Base/Max B06 và xem V/T/reason;
  kiểm tra cua mềm/90° hai phía/nét đứt. Xem [speed-b07.md](speed-b07.md).

## Baseline B06 (lịch sử)

Ngày 24/10/2026. B06 chưa hardware-verified.

- `scripts/build.ps1`: Keil ARMCC 5 build **0 error, 0 warning**; Code 22.412, RO 1.120,
  RW 240, ZI 2.552 byte, trong target Flash 64 KiB / SRAM 20 KiB.
- Artifact: `build/linefollower-B06.hex`, SHA256
  `B02DCD17F331649DA48A57556AC56F01E39C7F9A7A24C4544EA4377127D5D43E`.
- `scripts/test-control.ps1`: **29.256 assertion pass**, gồm baseline calibration/PD/gap/recovery,
  planner ở Max 60/90%, deceleration, cooldown, fresh D, valid-line authority, no full brake trên dashes,
  pulse deadline/cancel, RECENTER không mang speed cũ, reflection symmetry và output bounds.
- Supervisor test xác nhận BRAKE là active RUN: explicit Stop giữ brake trace và không tự restart.
- Compile actual `bsp/motor.c` với fake SPL register writes vào MMIO RAM-backed:
  kiểm tra brake HH/CCR0, coast LL/CCR=period, PWM/polarity, reversal brake và motor lease.
  Không mô phỏng timer waveform, cầu H thật, brake current hay động lực học bánh xe.
- Suite mở rộng ở -O0 hết budget sau 28.934 assertion, chưa tới test_done, không tính là pass.
  Regression chuyển ARMCC -O1, giữ timeout/instruction limit và yêu cầu hoàn tất.
  Không đổi optimization của firmware project Keil; firmware build riêng qua uVision.
- UI formatter vẫn stub trong emulation; không xác nhận format float bằng ARM runtime thật.
- Owner báo Max 40% đã bám line; bộ gain tạm 2/0/5 lắc/không qua góc. Chưa có phép đo tốc độ/quãng phanh.
- Tiếp theo: nạp B06, calibration/white reference, thử Base 30/Max 50→60 theo [high-speed.md](high-speed.md).

## Baseline B05 (lịch sử)

- Firmware ARMCC 5 / Cortex-M3 build pass, không có warning/error.
- **27.588 assertion emulation pass**, gồm white reference, gap, valid-line PD invariant,
  pivot đối chiều cùng duty, nhận lại line ngay, timeout flicker, Start/Stop, ADC/UI watchdog,
  OLED parameter tác động thực đến output và LAST RUN sau Stop.
- Test mới không khẳng định robot đã đi đúng góc 90°, không mô phỏng ma sát/tải/độ trượt/motor.
- B04 từng pass test giả lập nhưng owner báo regression trên xe. Kết quả đó không phải hardware validation.
- Owner xác nhận baseline trước bám cua nhẹ/vòng tròn, B03 cải thiện gap/cua; B04 bị xoay vòng.
- **B05 chưa thử trên xe.** Cần nạp B05, calibration/white reference và so baseline ở Base 20/Max 30.
- UI formatting được stub trong emulation; firmware dùng ARM C runtime thật.

## Lịch sử validation

# Kết quả kiểm tra B03

Ngày: 24/10/2026. Repo: `D:/stm32-linefollower-spl`.

## Kiểm thử tự động

- ARMCC 5 build firmware: **0 error, 0 warning**.
- Compile C cho Cortex-M3, chạy trên Unicorn với peripheral/timebase/motor stub cho supervisor/UI.
- Kết quả: **16.795 assertion pass**.
- Giữ regression polarity, white reference, calibration rejection, UI Start/Stop, watchdog và fault latch.
- Bổ sung cua trái/phải, edge cue trước ambiguous mask, edge flicker không thoát recovery sớm,
  repeated straight dashes không reset speed và không tăng duty trên trắng,
  strict gap/turn/ambiguous timeout, gentle curve PD và mask inversion cùng steering sign.
- Test source: `tests/test_recovery.c`, `tests/test_control.c`, `tests/test_robot.c`, `tests/test_menu.c`.
- Build/test command ở [run-test.md](run-test.md).

Test không mô phỏng động lực học GA25, ma sát, quán tính, vị trí cảm biến, ADC/I2C/TIM register;
không đo execution time thật hoặc chứng minh xe đạt tốc độ tối đa.

## Kết quả phần cứng do owner cung cấp

- Manual calibration và Motor Test hoạt động.
- B02 Base 20% bám được vòng tròn/cua nhẹ; cua 90° bị dừng.
- Chưa có F/R hoặc chuỗi ADC lúc dừng; chưa xác định fault thực tế.
- B03 Base 30/Max 45 với recovery mới chưa thử trên xe.

## B04

- Firmware build: 0 error, 0 warning; `build/linefollower-B04.hex`.
- Emulation: **16.864 assertion pass**, gồm regression của B03.
- Bổ sung: ADVANCE trước pivot khi có góc L, không release vào line cũ chưa clear,
  bắt giữa chỉ chuyển ALIGN, retry không reset deadline, chuỗi trái/phải phải qua đủ các pha.
- UI: Base/Max cùng hiện một màn, edit Max thành 40% thực sự giới hạn output về 400 permille;
  Corner Timing qua System cho phép chỉnh Advance/Align.
- Test formatter được stub bằng macro chỉ trong test build vì bare entry không khởi tạo ARM printf runtime.
  Không dùng test layout để kết luận format float trên phần cứng; firmware vẫn dùng C runtime bình thường.
- Kích thước owner: sensor–axle khoảng 16 cm, inter-corner straight khoảng 40 cm.
- B04 chưa hardware-verified. Default Advance=350 ms tại 30% là trial estimate, không phải đo quãng đường;
  phải hiệu chỉnh theo [square-wave.md](square-wave.md).

## B10 ? 24/10/2026

Keil 0 error/warning; Code23064, RO1252, RW340, ZI2572 bytes. 34.008 assertions pass, bao g?m split corner hai ph?a v? fault UI. Regression tr??c s?a fail30041. HEX v? SHA256 t?i [B10](recovery-b10.md). Ch?a th? ph?n c?ng.

## B11 — 24/10/2026

34.073 assertions pass, Keil 0 error/warning. Code23160 / RO1252 / RW340 / ZI2572 bytes. Test mới và SHA256 tại [B11](recovery-b11.md). Owner xác nhận B10 nét đứt/góc đơn, chưa xác nhận B11. Test không mô phỏng động lực học xe.

## B12 — 24/10/2026

34.085 assertions pass; Keil0 error/warning. Code23360/RO1252/RW340/ZI2580 bytes. Regression last-side before fix fail30036; new TTL/clear/opposite direction/timeout/fault tests pass. HEX hash và giới hạn tại [B12](recovery-b12.md). Chưa thử xe.

## B13 — 24/10/2026

34.106 assertions pass; Keil0 error/warning, Code23516/RO1252/RW340/ZI2588. Regression fail30040 trước fix trên profile B12. SHA256 và phạm vi tại [B13](recovery-b13.md). Chưa thử phần cứng.

## B14 — 24/10/2026

34.135 assertions pass; Keil0 error/warning, Code24576/RO1280/RW344/ZI2616. Regression on B13 fail30034. Estimator masks→controller→output and DIAG persistence/menu/timingfault tested. Hardware timing/heading not verified. See [B14](diagnosis-b14.md) for HEX hash and scope.

## B15 — 26/10/2026

34.709 assertions pass; Keil0 error/0 warning (Code25316/RO1280/RW344/ZI2632). Bổ sung bonus/cua mềm/guard/GAP dài giảm đúng tỷ lệ/GAP ngắn không giật. Owner xác nhận B14 45/75 chạy map; B15 chưa thử phần cứng. Chi tiết và HEX hash tại [speed-b15.md](speed-b15.md).

## B16 — Profile50/80

34.715 assertions pass; Keil0 error/0 warning. Test defaults50/80, target thẳng55/cua mềm54/GAP50/shape28 và menu adjustment. Giữ baseline45/75 và watchdog/recovery. Chưa thử xe; xem [profile-b16.md](profile-b16.md).

## B17

34.734 assertions pass; Keil0 error/0 warning, Code25484/RO1280/RW344/ZI2632. Stable curve/outward risk và high-profile repeated turns tested. Owner báo hết map nhanh hơn khoảng1s nhưng cua mềm còn khựng;60/90 phanh nhiều hơn. Không có trace hoặc phép đo quãng phanh. Chi tiết speed-b17.md.

## B18

34.754 assertions pass; Keil0 error/0 warning, Code26348/RO1328/RW344/ZI2688. New curve bridge/expiry/white/outward/handoff, post-turn ramp, mode durations/freeze/reset và4-page menu. UI stub không là OLED render; emulation không là hardware dynamics. Chưa có phản hồi xe B18. Xem speed-b18.md.

## B19

34.763 assertions pass; Keil0 error/0 warning; Code26436/RO1328/RW344/ZI2696. Thêm Straight config/controller/menu clamps và6-row layout; giữ quantization/GAP/bridge/recovery/watchdog tests. Owner xác nhận B18 ổn55/85; B19 chưa hardware-verified. HEX hash và hướng dẫn tại speed-b19.md.

## B20

34.861 assertions pass; Keil0 error/0 warning; Code27472/RO1360/RW352/ZI2720. Entry-brake two-sided confirmation/pulse/cancel/rearm/remainder/deadline; slew release/edge bypass; rejoin cap; supervisor raw masks, frozen ZERO/ENTRY counters, watchdog during entry và five-page menu. So backup: motor/ADC/estimator unchanged. Test instruction budget300M/timeout60s, completion required; không là hardware timing validation. B20 chưa thử xe; chi tiết/hash tại speed-b20.md.
