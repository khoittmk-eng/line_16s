# Các quyết định kiến trúc (ADR)

## ADR-001: Bare-metal C dùng SPL

Trạng thái: đã thống nhất.
Dùng STM32F103C8T6, Standard Peripheral Library và CMSIS.
Không tự đưa HAL, Arduino hoặc RTOS vào dependency. Tách application logic khỏi BSP code.

## ADR-002: Sửa bằng VS Code, build bằng Keil toolchain đã cài

Trạng thái: đã triển khai.
Dùng `linefollower.uvprojx` làm build definition. VS Code gọi `scripts/build.ps1`.
Cùng project này mở được trong Keil. Startup, device header và SPL source được lưu trong repo.
Build thành công chỉ xác nhận compile/link; kiểm chứng phần cứng là bước riêng.

## ADR-003: STBY nối cố định lên 3V3

Trạng thái: đã thống nhất; chủ dự án xác nhận phần cứng.
Không có GPIO STBY trên STM32. Motor A là bánh trái, Motor B là bánh phải.
Hành vi stop phải thực hiện qua PWM/direction input. Firmware không thể ngắt nguồn động cơ
qua STBY; cần kiểm tra hành vi khi cấp nguồn, reset và startup trên mạch thực.

## ADR-004: Không có wheel encoder

Trạng thái: đã thống nhất; chủ dự án xác nhận phần cứng.
Hai động cơ GA25 có thông số 620 RPM và không có encoder.
Triển khai line-position feedback và duty request có giới hạn.
Không cung cấp wheel speed đo được hoặc triển khai wheel-speed PI loop khi không có feedback tương ứng.

## ADR-005: Control timing độc lập với cập nhật màn hình

Trạng thái: đã triển khai acquisition và controller theo frame TIM3 2 ms; execution timing trên board chưa được đo.
Target control period ban đầu là 2 ms; OLED không được làm trễ control deadline.
Scheduling, execution time, jitter và overrun handling cần được triển khai và đo thực tế.

## ADR-006: Triển khai navigation sau khi xác nhận tracking

Trạng thái: đã thống nhất thiết kế.
Triển khai và kiểm tra sensing, motor output, tracking, speed ramping và gap recovery có giới hạn
trước khi làm junction navigation hoặc maze memory. Không coi mẫu toàn nền là ngõ cụt.

## ADR-007: Ba nút dùng EXTI + NVIC

Trạng thái: đã triển khai driver ngày 24/10/2026; build pass, chưa kiểm chứng trên mạch.
Dùng GPIO edge interrupt qua EXTI và NVIC thay vì polling định kỳ để phát hiện nhấn/nhả.
Mapping: PB0/EXTI0, PB1/EXTI1, PB10/EXTI10; IRQ tương ứng là EXTI0, EXTI1 và EXTI15_10.
ISR chỉ ghi nhận edge/timestamp và clear pending đúng line. Debounce, nhấn ngắn/nhấn giữ
và menu event được xử lý ngoài ISR bằng state machine theo elapsed time.
Đọc lại GPIO để xác nhận debounce được phép; không delay hoặc xử lý OLED trong ISR.
Ưu tiên button interrupt thấp hơn control/acquisition. Cần xác nhận active level và pull resistor,
đồng thời kiểm tra bounce, event trùng, nhấn đồng thời và ảnh hưởng đến control timing.
Chi tiết runtime được quy định trong [runtime.md](runtime.md).

## ADR-008: Lấy mẫu theo TIM3 và công bố DMA frame

Trạng thái: đã triển khai source, build pass; chưa đo timing trên mạch.
TIM3 TRGO kích hoạt ADC1 scan PA0..PA7 mỗi 2 ms, không dùng ADC continuous mode.
DMA circular có 16 sample, chia hai nửa 8 sample; HT/TC IRQ copy frame đã hoàn tất.
Frame snapshot được chuyển sang foreground bằng critical section ngắn.
DMA error hoặc hai flag HT/TC cùng pending làm dừng acquisition để tránh xuất dữ liệu không chắc chắn.
Controller và calibration hiện chạy trong frame callback DMA; foreground chỉ nhận snapshot/UI và gửi yêu cầu.

## ADR-009: Giữ OLED profile tham chiếu đã chạy

Trạng thái: đã triển khai source, chưa thử màn thực của repo mới.
Giữ I2C1 remap PB8/PB9, 400 kHz, 128×64, address 7-bit 0x3C và legacy init của project cũ.
Source cũ tên SH1106 nhưng owner xác nhận SSD1306; không dùng tên function để suy ra controller.
Có thể chọn SSD1306 init tiêu chuẩn qua cấu hình, nhưng không tự thay profile đang tham chiếu.
Mỗi lần gửi một page ở foreground, transaction timeout 6 ms; lỗi bus ngừng refresh đến reset.

## ADR-010: Giữ bố cục và thao tác menu cũ

Trạng thái: triển khai source theo yêu cầu owner; build pass, chưa thử menu mới trên xe.
Giữ thứ tự Run, Sensor Monitor, PID Setup, Calibration, Motor Test, System.
N1/N3 điều hướng hoặc tăng/giảm; N2 chọn/edit; giữ N2 dừng và quay lại.
Không mang theo polling, blocking Auto Calib, gain cũ hoặc PID timing của project tham chiếu.
Motor Test chạy riêng từng bánh ở 20% PWM, timeout 2 giây. RUN đã tích hợp; Auto Calib vẫn chưa triển khai.

## ADR-011: Black-line polarity và RUN theo frame

Trạng thái: source đã triển khai theo phản hồi owner; build/emulation pass, RUN chưa thử trên xe.
Owner xác nhận Motor Test hoạt động, nhưng line đen yếu/trắng mạnh ở bản trước.
Đảo mặc định thành raw HIGH = line strength cao. Giữ shortcut đổi polarity để đối chiếu thực tế.
Controller chạy sau frame hoàn chỉnh trong DMA IRQ, dùng dt=2 ms, D filter 15 ms và acceleration ramp.
Gain mặc định Kp=12, Ki=0, Kd=0.08, Base=20%, Max=30%; không kế thừa gain scaling cũ.
N3 Start/Stop trong Run; Start yêu cầu calibration, line valid và frame mới.
Không auto-start sau boot/fault. SysTick giám sát ADC/UI, output có lease 20 ms.
Có bounded gap/search; ambiguous line dừng vì chưa có junction policy.

## ADR-012: White reference rõ ràng và firmware identity B02

Trạng thái: triển khai source, build và emulation pass; chưa xác nhận B02 trên mạch.
Thay giả định raw polarity toàn hàng bằng white reference riêng từng mắt, lấy sau sweep min/max.
N2 nhấn ngắn sau khi N3 kết thúc calibration: đặt cả 8 mắt trên trắng để xác nhận endpoint.
Start chỉ cho phép khi calibration + reference hợp lệ. Run ghi rõ nguyên nhân chưa Start được.
Giữ mapping cũ: N1/N3 chọn mục, N2 Enter, N3 Start/Stop trong Run.
OLED hiện tag B02; build tạo thêm linefollower-B02.hex và SHA256 để tránh nạp nhầm artifact.
ADR-011 và mô tả raw HIGH trước đó là lịch sử, không còn là chính sách polarity hiện tại.

## ADR-013: B03 phân biệt corner recovery, straight gap và ambiguous probe

Trạng thái: triển khai source, build/emulation pass; chưa xác nhận B03 trên xe.
Owner đã chạy được vòng tròn/cua nhẹ với Base 20% nhưng dừng ở cua 90°.
Giữ PD cho tracking thông thường, thêm TURN theo corner cue và direction memory.
Chỉ thoát TURN/SEARCH khi vùng giữa ổn định; partial edge không reset timeout.
Straight GAP giữ duty không tăng, không reset speed mỗi nét; ambiguous PROBE có thời hạn.
Giữ watchdog và giới hạn recovery. Mặc định Base 30%, Max 45%, không tăng gain mặc định.
Hướng sensor đảo phải đảo cả mask và error. OLED thêm R reason, firmware tag B03.

## ADR-014: B04 giữ menu chính và bổ sung tuning theo geometry

Trạng thái: source triển khai, build/emulation pass; chưa hardware-verified.
Giữ sáu mục chính. PID Setup hiển thị đồng thời năm dòng, System N3 mở Corner Timing.
Base/Max và Advance/Align đều sửa được trên OLED, lưu RAM. Không thêm Save to Flash ngầm.
Owner mô tả sensor offset khoảng 16 cm, đoạn nối góc 40 cm và xác nhận cả rẽ sớm/thoát sớm.
ADVANCE có giới hạn trước pivot cho góc L rõ; ALIGN sau reacquisition để không coi một lần thấy giữa là hoàn tất cua.
Default Advance 350 ms tại 30% là estimate cần hiệu chỉnh thực tế, không phải tính góc/quãng đường chính xác.

## ADR-015: B05 thay thiết kế B04 sau regression trên xe

Trạng thái: source và test logic đã thực hiện; cần thử phần cứng B05.
Owner báo cua vuông xoay vòng một bánh, cua nhẹ văng line. Không quy toàn bộ cho geometry hoặc gain.
Bỏ ADVANCE 350 ms và mọi điều kiện bắt buộc thấy trắng/minimum-turn trước khi nhận lại line.
Valid line giữ PD authority; chỉ pivot khi mất line được xác nhận. Pivot target hai bánh đối chiều cùng duty 20%.
Reacquire valid line chuyển ngay RECENTER bằng forward PD; deadline 1.200 ms không reset bởi flicker.
Default Base 20/Max 30 để so baseline; giữ OLED speed edit và thêm LAST RUN.
Các ADR-013/014 là lịch sử bị thay thế ở phần điều khiển góc. Không coi test mask giả lập là chứng minh góc thực.

## ADR-016: B06 speed planner và short brake có giới hạn

Owner báo Max 40% chạy được, duty cao làm văng ở góc; bộ gain tạm 2/0/5 gây lắc và không qua góc.
Thay gain tạm bằng trial 12/0/0,12, Base 30/Max 60; Ki giữ 0, chưa tuning trên xe.
D per second cập nhật trước planner; risk từ error/rate/nhóm rộng hạ target trước khi mất line.
Ramp tăng 40, giảm 600 điểm/giây; boost cần 300 ms thẳng, cooldown 250 ms sau risk/invalid.
Valid line giữ PD; drive/brake PWM giảm tốc, không cưỡng bức quay/phanh hai bánh theo timer.
Recovery entry PWM cao >40%: BRAKE tối đa 12 ms với duty=0/IN1=IN2=High;
valid line hủy pulse ngay. Deadline bao gồm pulse, watchdog/Start interlock giữ nguyên.
Đồng bộ magnitude PWM khi pivot, RECENTER <=20% common, không dùng speed cũ của đường thẳng.
GAP không full brake mỗi nét; một hàng sensor không phân biệt hoàn toàn gap/góc cụt.
Bổ sung B pulse counter ở LAST RUN. Test logic/register request không là mô hình cơ học.
Xem [high-speed.md](high-speed.md), [error-log.md](error-log.md).

## ADR-017: B07 tách noise handling của planner khỏi PD

Owner thử B06: cua mềm ổn, quay 90° lâu, thẳng chậm. Regression tái hiện static middle-eye error 0,5 pitch
không boost vì corridor B06 quá hẹp; jitter D và nhóm giữa bốn mắt cũng có thể giữ tốc thấp.
B07 giữ gain/PD derivative filter và curve cap, thêm planner-only rate filter 40 ms;
nhận thẳng trong corridor 0,65 pitch/180 ms, cooldown 150 ms và xác nhận risk vừa 8 ms.
Error >1/nhóm rộng ra ngoài vẫn giảm ngay; centered 0x3C không tự bị coi là góc.
Ramp tăng 65, giảm giữ 600; pivot target 24%/ramp 350, không thêm thời gian quay cưỡng bức.
Base/Max giữ 30/60 để so cùng duty; không tăng Max để che boost starvation.
Thêm V/T/reason trong status, LAST RUN và OLED; giữ Start/Stop/watchdog/calibration và brake/GAP policy.
Xem [speed-b07.md](speed-b07.md). Test logic không là xác nhận tốc độ/góc cơ học trên xe.

## ADR-018: B08 tăng duty profile, giữ phanh theo phản hồi owner

Owner xác nhận phanh ổn nhưng Base 30%/cua/90° còn chậm, yêu cầu tăng tốc.
Tăng default Base/Max 40/75%, ramp tăng 80; curve caps 32/28%, pivot 30%/ramp 450,
RECENTER min(Base; 26%). Nâng đồng bộ cap vì tăng Base một mình vẫn bị cap profile cũ.
Giữ gain 12/0/0,12, planner B07, loss/GAP/recovery deadlines, brake threshold/pulse/deceleration và driver motor.
Valid line vẫn PD, không thêm delay hoặc thời gian quay bắt buộc. Không thay watchdog/Start interlock.
Đây là trial duty profile; source equality và regression không chứng minh quãng phanh/góc cơ học ở tốc độ mới.
Xem [speed-b08.md](speed-b08.md).

## ADR-019: B09 Base cruise và bounded guide-memory GAP

Owner báo nét đứt/cua nhẹ khựng, không đọc được OLED khi chạy và chọn bỏ boost, dùng Base cao để mượt.
B09 target Base 45/Max ceiling 75; bỏ stability/risk/cooldown gates của tốc độ, giữ PD gain/filter.
abs(error)<=1,5 dùng Base, 1,5→3 giảm liên tục về min(Base;28%) thay các cap dạng bước cũ.
GAP lưu/giữ exact recent trusted forward L/R; filtered rate chỉ đánh giá guide, không dùng D-kick tức thời để loại GAP.
Guide TTL entry 80 ms, edge/drift invalidate, span loss 300 ms, central split grace 12 ms trong cùng deadline.
Valid line nhận PD ngay; timeout là fault cần explicit Start. Không kéo dài vô hạn theo flicker invalid.
Giữ brake driver/pulse/threshold, recovery deadline, ADC/UI/motor watchdog và Start interlocks.
Tests phải kiểm tra lệnh được giữ, không suy từ PWM dương ra bánh còn quay. Xem [cruise-gap-b09.md](cruise-gap-b09.md).

## ADR-020 ? B10: d?ng h??ng t? split mask c? x?c nh?n

Invalid centroid kh?ng ??ng ngh?a mask kh?ng ch?a th?ng tin h??ng. Ch? d?ng asymmetry m?nh c? outer edge li?n t?c 6 ms cho recovery; kh?ng d?ng centroid invalid cho PD, kh?ng ch?n nh?nh giao l?. Gi? timeout v? valid-line authority. Xem [B10](recovery-b10.md).

## ADR-021 — Cập nhật hướng trong recovery và handoff guide

B11 nhận split cue mới trong episode recovery chỉ sau khi đã thấy line valid; không reset deadline khi retarget. Khi RECENTER hoàn tất, lưu guide từ lệnh forward hiện tại theo điều kiện cũ để không có frame mất history. Không đổi gain/tốc độ hoặc tự tăng Base. Xem [B11](recovery-b11.md).

## ADR-022 — B12: last-side fallback trước line fault

Theo đề nghị owner, lưu phía line rõ gần nhất có TTL600ms, clear sau trusted center100ms. GAP vẫn kết thúc giữ lệnh ở300ms; khi còn hướng mới được chuyển recovery1200ms thay vì fault ngay. Đây là heuristic tìm lại, không nhận dạng map hay auto-restart. Chi tiết [B12](recovery-b12.md).

## ADR-023 — B13: phân biệt tracking ổn định với vừa thoát recovery

Lưu hướng và guard200ms sau RECENTER; center tin cậy100ms clear sớm. Reloss trong guard không dùng GAP300ms, tiếp tục recovery có deadline chung. Không chờ bằng delay hoặc đo góc bằng timer. Đây là heuristic có tradeoff với nét đứt ngay sau cua. Xem [B13](recovery-b13.md).

## ADR-024 — B14: cue từ valid shape và diagnostics sau Stop

Centroid valid không đủ phân biệt hình dạng. Mẫu rộng chạm mép lệch bên xác nhận6ms giảm target nhưng giữ PD; pending200ms chặn GAP khi mất dấu sớm. Không pivot trước khi invalid. Thêm DIAG sau Stop để phân biệt nhận dạng/GAP/recovery/timing thay vì suy đoán từ LINE LOST. Chi tiết và tradeoff tại [B14](diagnosis-b14.md).

## ADR-025 — B15: bonus trên nền Base thay tăng Base toàn map

Owner cho phép adaptive speed trở lại. Giữ Base45/Max75/PID đã chạy ổn; thêm bonus tối đa10 có điều kiện, cap55 và steering headroom15. Cua mềm có thể nhận bonus; shape gấp/recovery chặn bonus300ms. GAP giữ exact60ms rồi giảm riêng phần dư vềBase, giữ tỷ lệ bánh; short reacquisition giữ duty. Hệ quả: thay policy fixed cruise/exact300ms của B09; tham số heuristic cần thử xe. Không sửa watchdog, brake hardware hoặc recovery deadline. Xem [B15](speed-b15.md).

## ADR-026 — Tăng defaults theo từng nấc, B16 chỉ đổi50/80

Theo yêu cầu owner, đổi defaults sang50/80, giữ gain và controller B15 để đối chiếu riêng ảnh hưởng profile. Không triển khai gain scheduling hay tăng trần bonus trong lần này. Trần bonus vẫn55; mức GAP nền nâng50. Kết quả trên xe quyết định nấc tiếp theo. Xem [profile-b16.md](profile-b16.md).

## ADR-027 — B17: phân biệt stable curve và outward drift, tăng tốc recovery

Blend từ2→3 pitch, tight/shape32, outward rate4→16 tạo giảm target theo risk; valid PD giữ authority. Pivot36/ramp600 và RECENTER30. Giữ brake12ms/recovery1200ms và memory guards. Defaults55/85; không gain scheduling trong revision này. Owner báo cải thiện1s,60/90 kém hơn; ưu tiên giảm thời gian recovery/khựng trước tăng Base. Xem speed-b17.md.

## ADR-028 — B18: bounded curve snapshot và command-state durations

Owner cho phép tiếp tục tối ưu. Fragmented moderate curve được giữ L/R8ms, white4ms với snapshot age10ms và guards loại corner/edge/rate. Không mở rộng straight GAP hay dùng centroid invalid. Thoát recovery giữa line dùng ramp160 dưới42, sau đó80. TIME cộng frame2ms, CV subset TRACK, không là wall-clock hay brake distance. Giữ55/85/PID/phanh/pivot36. Chi tiết speed-b18.md.

## ADR-029 — Tách Straight khỏi Base và wheel ceiling Max

B19 thêm Straight RAM-only trên OLED; defaults55/60/85, clamp Base<=Straight<=Max. Bỏ hard-coded cap55/bonus10/headroom15; dùng Straight-Base và mixer giới hạn Max. Straight cần corridor0,8 pitch/80ms, guards góc giữ B18. Cua mềm ngoài corridor vềBase, không kéo thêm tốc thẳng theo cua. Heuristic không là geometry lookahead; test xe quyết định mức tiếp. Xem speed-b19.md.

## ADR-030 — B20: entry brake có điều kiện và giới hạn tăng correction

Owner cho phép phanh chủ động khi vào góc. B20 thêm ENTRY_BRAKE8ms trước loss với confirmed shape6ms, fast center memory150ms và mean duty>=50. Đây là ngoại lệ có kiểm soát cho policy cũ không full brake khi valid. Valid thường hủy ngay; invalid dùng phần pulse còn lại và giữ recovery deadline; latch chỉ mở sau trusted center. Soft steering giới hạn build-up800 trong vùng moderate, giải phóng correction ngay, không ép bánh trong luôn tiến. Post-turn cap35 tới60ms center (TTL200ms), không xác định được danh tính line. Giữ55/60/85/gain12/0/0,12 để đối chiếu. Diagnostics phân biệt ZERO/ENTRY; xem speed-b20.md.

## ADR-031 — Forward PID liên tục thay ENTRY_BRAKE khi line valid (B21)

Owner yêu cầu giảm chuỗi phanh/chạy lại. B21 thay ADR-030 ở phần entry pulse: bỏ phanh hai bánh chỉ vì valid corner cue; giữ PID và giảm common duty có ramp. Moderate track dùng deceleration120 khi target≥Base, edge/corner giữ600. Không tạo quỹ đạo parabol giả định. Recovery brake, corner memory, post-turn bounds và watchdog giữ nguyên. Enum/diagnostic ENTRY chỉ giữ tương thích, không phát ở B21. Chi tiết và tradeoff: [speed-b21.md](speed-b21.md); chưa hardware-verified.

## ADR-032 — Hộp đen B22, Flash hai slot và TX-only PB12

Owner đồng ý CH340 và cần giữ log sau khi tháo nguồn. Chọn ring RAM 160×48 byte, cadence4ms với mode/trigger bypass; capture80 sample trước/80 từ trigger. Chọn LOSS hoặc CORNER trên OLED; freeze đến ARM tiếp theo. Lưu Flash thủ công khi dừng vào hai slot8KiB ở 0x0800C000/0x0800E000, CRC32 và commit cuối; firmware bị giới hạn48KiB bởi scatter và HEX checker. Không ghi Flash trong RUN.

Giữ sáu main items; System N2 vào BLACK BOX, N1/N3 chọn, N2 ARM/Trigger/Save/Export. PB12 Software UART TX9600/8N1, stopped-only; không đổi dây đang chạy xe. Pause ADC khi Flash/Export và yêu cầu frame mới trước Start, giữ sticky capture fault. CSV có transport CRC32, profile/calibration và parser trên PC. RAM mới chưa Save chặn Export bản Flash cũ. Giới hạn: một cửa sổ ngắn, PWM request không là đo tốc độ, UI chờ lúc Export, logging có overhead cần đo. Chi tiết: [blackbox-b22.md](blackbox-b22.md).

## ADR-033 — PROBE có giới hạn theo trusted guide cho ambiguity ngắn (B23)

Owner yêu cầu sửa theo raw trace CRC B4544906. B23 thay policy mọi unknown nonzero mask đều lập tức target15 trong một trường hợp hẹp: center history40ms, guide≤10ms, error≤0,8/rate≤4, hai mắt giữa vẫn đen, không corner/recovery/guard hoặc entry SplitDirection. Giữ trusted wheel ratio trong≤32ms, cap32 và không tăng duty, vẫn decel600 nếu đang nhanh. Sau giới hạn hoặc mất anchor dùng policy cũ; confirmed direction/pending có ưu tiên, invalid centroid không được dùng, counters không reset. Valid line tiếp tục PID ngay. Gains/defaults55/60/85, brake/GAP/memory/watchdogs không đổi. Rủi ro: có thể tiến thêm vào intersection/đen rộng; một trace startup không chứng minh whole-map stability. Xem [speed-b23.md](speed-b23.md).

## ADR-034 — Direct turn B24 theo yêu cầu owner

Owner yêu cầu bỏ phanh rồi mới rẽ. B24 supersedes policy giữ recoveryBRAKE ở các ADR/AGENTS cũ: không phát BRAKE/ENTRY trong controlRUN, giữ explicitStop/faultwatchdogBrake. Pivot tách common decel600 và steering slew1800 để thêm yaw ởframeđầu, target±min(Max,36). Valid confirmedasymmetricshape thêm steering floor0,8 khi cùng hướng/e không cải thiện, bỏ cap32 riêng do shape; PD/risk planner giữ authority khi cue clear/improving. Reversal guard theo từng wheel để outerwheel không bị ngắt khi chỉ inner đảo dấu. Giữ bounds/deadlines/squarememory/whiteGAP và không chọn hướng từFF. CORNER recorder đổi trigger để bỏstartupinvalidkhônghướng. Chi tiết/rủiro/validation tại [speed-b24.md](speed-b24.md), chưahardware-verified.

## ADR-035 — Fixed Base và hướng recovery theo bằng chứng quan sát (B25)

- Context: owner báo B24 văng line ở thẳng/góc liên tiếp, yêu cầu bỏ boost rồi giảm tốc. Không có log B24 mới để kết luận động lực học.
- Decision: TRACK target=Base, bỏ Straight bonus/curve-risk target scheduling/shape floor; giữ PD mỗi2ms và startup ramp. Pending loss không chờ thêm6ms nếu không có cue split ngược; TURN request counter-rotation ngay, không ramp tiến. Giữ hướng đã xác nhận trước sai số giữa yếu; hướng mới mạnh/split confirmed vẫn ưu tiên. Xem [B25](fixed-base-b25.md).
- Exceptions: ALIGN30/post-turn35, bounded ambiguity, GAP300ms, recovery1200ms, Stop/fault/watchdog giữ nguyên. Fixed Base không đảm bảo common PWM hoặc tốc độ cơ học cố định khi mixer đạt giới hạn.
- UI:5 dòng Kp/Ki/Kd/Base/Max; legacy straight_percent đồng bộ Base, không dùng boost; CSV/Flash schema không đổi.
- Evidence: baselineB24 fail90048; B25 97520 C assertions +26 Python tests, Keil0errors/0warnings. Cần chạy xe để đánh giá ổn định/thời gian vòng; không khôi phục ADVANCE/minimum-spin B04.

## ADR-036 — Base45, forward PD phản ứng ngay và SWD cấu hình rõ ràng (B26)

- Context: owner báo45 ổn định hơn55, cần giảm thời gian cua; lỗi Keil xảy ra dù chưa Save và thiếuNRST.
- Decision: giữ fixed Base, default45/85; bỏ slew toànPD vì trễ P, giữ D filter15ms. Rejoin giữ command magnitude có giới hạn40 thay vì reset30, post-turn cap40. Không sửa estimator/driver/Flash logger khi chưa có trace chứng minh lỗi ở đó.
- Hardware interface: dùng helper board ghi SWD-enabled field cùng I2C remap, bỏ SWJ_CFG readback write-only. Giữ các thay đổi DBGMCU/main đã có của owner. Hướng dẫn Normal khi thiếuNRST và UnderReset khi đã nốiNRST; không viết option bytes.
- Consequences: valid line vẫn chia PWM hai bánh ngay, quay ngược chỉ ở confirmed loss. Giảm độ trễ có thể tăng nhạy với noise; rejoin nhanh hơn phải thử square-chain. Không có cam kết Base45 luôn giữ được40 trong mọi góc do mixer/Max.
- Validation:101895ARM+26Python tests, Keil0errors/warnings; baselineB25 fail100031. Chưa chạy xe/nạp board. Chi tiết [B26](corner-flow-b26.md), [Keil](keil-download.md), ERR-044/045.

## ADR-037 — Bounded forward arc, stagger pivot reversal và lọc riêng guide nét đứt (B27)

- Owner chấp nhận rủi ro văng thân xe để rẽ ngay bằng duty đang có; báoBase45/B26 cải thiện.
- Decision: forward arc28ms đầu recovery khi entrymean>=35, correction0,8common, clipMax; valid rejoin ngay ởBase, fallbackpivot khi chưa thấy line. Không renew khi post-turn reloss. Không geometryADVANCE/minspin; vẫn deadline1200ms.
- Driver: stagger hai reversal guard khi đổi hướng pivot để giữ ít nhất một PWM tiến; giữ zero mỗi channel và Stop ngay. GAP dùng correction lọc riêng nominal25ms, giữ guide gốc cho ambiguity và livePID.
- Tradeoff: arc có thể trượt/cắt nhánh; cap40 không áp cho arc rejoin thành công. Post-turn retry vẫnpivot để không tiến mù ở chuỗi góc vuông. PWM không đo vận tốc.
- Evidence: baselines fail120032/50056;102331ARM+26Python tests. Hardware/lap time chưa xác nhận; xem [B27](rolling-corner-b27.md), ERR-046/047/048.

## ADR-038 — Visible-corner extra P tăng dần và search hai mức (B28)

- Context: owner báo B27 Base45 đạt17,30giây, yêu cầu kết hợp nhận góc sớm/gain scheduling/quay tìm nhanh và thử nhiều hướng.
- Chọn extraP mượt khi có confirmed valid directional shape, tăng theo abs(error), giới hạn0,6; không đổiKd, không floor ép correction và không geometryADVANCE. Khi cải thiện theo planner/cue không còn thì nhả; PID nền của cua mềm giữ nguyên.
- So sánh ba candidate ARM command-only cho thấy gain mượt có bước extra1,44 thay5,4 của gain đổi tức thì ở input pureP đã chọn. Không có plant model hay dự báo lap time.
- Search50 trên trắng,36 khi có đen chưa valid, forwardPD ngay khi valid. Giữ45/85, B27 arc/dash/driver, rejoin40 và deadlines/interlocks.
- Consequences: metadataKp là gain nền, cần đọc firmwareB28 để hiểuP effective. Có nguy cơ nhận nhầm cue và quét lố; không khẳng định90deg hoặc nhanh hơn trước khi thử xe.
- Validation:104022ARM+26Python, candidate comparison, Keil0Error/0Warning. Chi tiết [B28](visible-corner-b28.md), ERR-049/050. BackupB27.

## ADR-039 — Fresh inward trend nhả extraP trước rate lọc (B29)

- Context: B28 được owner báo16,3giây ở45, cao hơn55/60 không có lợi trênmap. Giữ45/85 làm mốc.
- Decision: tănggain-rise0,04→0,05/ms, giữtrần0,6; xác nhận6ms errorgiảm rõ đểlatch fadeextra sớm. Plateau khôngrebuildgain, cần6msoutward đểrearm; PID nền/memory/deadlines không bị reset bởi trend.
- Evidence: B28 fail140035. So sánhB28/rise-only/B29 cùng synthetic trace: entry19,44/19,8/19,8; steady23,4 cảba; extra sau6msreturn0,264/0,264/0,176. ChọnB29 theo hành vi lệnh, không dự báo thành tích.
- Tradeoff: rise nhanh/return fade có thể làm phản ứng với cue giả hoặc nhả thiếu lực, cần thửxe. Gain metadata vẫn làgain nền, Flash/CSV khôngđổi. Không tăngBase/Max hoặcđổi driver/SWD trong lượt này.
- Validation:104149ARM+26Python+candidatecomparison, Keil0errors/warnings. Chi tiết [B29](corner-return-b29.md), ERR-051; backupB28.
