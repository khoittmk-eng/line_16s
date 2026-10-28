# Nhật ký lỗi và bài học triển khai

Đọc file này trước khi sửa firmware. Chỉ ghi kết luận có bằng chứng; tách lỗi xác nhận
trong source, lỗi tái hiện trên phần cứng và rủi ro chưa tái hiện.
Mỗi lần sửa phải bổ sung cách kiểm tra lại để tránh lặp lỗi cũ.
Owner đã báo polarity ngược trên xe ở bản trước (ERR-007). Calibration/Motor Test và RUN ở Base 20% trên vòng tròn/cua nhẹ đã được owner xác nhận. Owner báo dừng ở cua 90°; B03 xử lý mới chưa thử trên xe.

## ERR-001 — PID bị gián đoạn bởi OLED và delay

- Nguồn: review project tham chiếu `D:/STM32-LineFollower-Robot-main`.
- Hiện tượng: có khả năng phản ứng lái không đều khi refresh OLED; xe cũ được chủ dự án báo văng khi tăng tốc.
- Bằng chứng: PID và blocking OLED transfer chạy trong cùng vòng lặp với `Delay_ms(5)`.
- Nguyên nhân trong source: control period phụ thuộc công việc foreground; chưa khẳng định đây là nguyên nhân duy nhất của hiện tượng trên xe.
- Phòng tránh: lấy mẫu bằng timer; không đặt OLED hoặc delay trong control ISR.
- Kiểm tra lại: đo control period/execution time bằng GPIO hoặc timestamp khi OLED refresh và khi OLED lỗi.
- Trạng thái: bản mới chạy estimator/control theo callback DMA từ TIM3, OLED ở foreground; chưa đo timing thực.

## ERR-002 — Nét đứt bị coi là mất đường và ra lệnh dừng

- Nguồn: review project tham chiếu và phản hồi chủ dự án.
- Bằng chứng: `dig == 0x00` gọi stop ngay, chuyển PAUSE rồi SEARCH.
- Nguyên nhân: thiếu bounded gap crossing, không xét tracking history.
- Phòng tránh: validity riêng với error; supervisor phải xét lịch sử và timeout.
- Kiểm tra lại: thử gap ngắn, off-track ở mép và timeout độc lập.
- Trạng thái: ghi nhận từ bản cũ; bản mới có bounded gap/recovery và đã kiểm thử logic bằng emulation; chưa xác nhận trên xe.

## ERR-003 — Nhảy trực tiếp từ Base lên Max

- Nguồn: review project tham chiếu.
- Bằng chứng: sau 6 mẫu đạt điều kiện, tốc độ nền chuyển trực tiếp sang Max.
- Nguyên nhân: thiếu acceleration ramp và chưa xét error variation.
- Phòng tránh: planner giới hạn tốc độ thay đổi duty, kiểm tra tracking stability theo thời gian.
- Kiểm tra lại: thử tăng tốc lặp lại trên đường thẳng và đo requested duty theo thời gian.
- Trạng thái: bản mới có acceleration ramp và điều kiện stability, emulation pass; chưa xác nhận trên xe.

## ERR-004 — Debounce tính sai do timestamp foreground cũ

- Nguồn: review khi triển khai `bsp/buttons.c` ngày 24/10/2026.
- Bằng chứng: nếu `now` được đọc trước EXTI mới nhất, phép trừ unsigned `now - edge_ms` có thể wrap và vượt debounce threshold.
- Nguyên nhân: ghép timestamp từ hai thời điểm không nhất quán.
- Cách sửa: lấy `now = Time_Millis()` trong critical section cùng lúc đọc edge state.
- Kiểm tra lại: xem lại thứ tự đọc và thử cạnh nút sát ranh giới SysTick; nhấn một lần chỉ có một short/long event.
- Trạng thái: đã sửa source, build pass; chưa thử timing này trên phần cứng.

## ERR-005 — Nhầm tên driver OLED và biểu diễn I2C address

- Nguồn: owner gọi OLED là SSD1306, nhưng source tham chiếu đặt tên SH1106 và dùng legacy initialization.
- Bằng chứng: source cũ dùng PB8/PB9, 400 kHz, 128×64, address truyền cho SPL là `0x78`.
- Nguyên nhân tiềm ẩn: coi tên function là bằng chứng model, hoặc shift address `0x78` thêm lần nữa.
- Cách xử lý: API OLED trung lập; giữ legacy profile mặc định; cấu hình address 7-bit `0x3C`, shift đúng một lần ở boundary SPL.
- Kiểm tra lại: xác nhận hình ảnh/column alignment trên màn thực, không đổi profile chỉ dựa vào tên module.
- Trạng thái: đã tích hợp source và timeout; chưa kiểm chứng màn hình repo mới.

## ERR-006 — ARM Compiler không hỗ trợ host target x86_64

- Nguồn: thử compile line estimator để kiểm tra khả năng chạy native test trên Windows.
- Bằng chứng: `armclang --target=x86_64-pc-windows-msvc` báo `ARM Compiler does not support`.
- Root cause: Keil ARM Compiler không phải bản Clang đa target cho host Windows.
- Cách xử lý: tiếp tục build target Cortex-M3 bằng ARMCC 5; không coi build là functional test.
- Phòng tránh: muốn chạy host unit test phải dùng host compiler riêng hoặc emulator; không lặp lại lệnh trên với Keil ARM Compiler.
- Trạng thái: đã xác nhận giới hạn môi trường; native functional test chưa chạy.

## ERR-007 — Line đen yếu, nền trắng mạnh sau calibration

- Nguồn: owner tái hiện trên OLED sau calibration của bản mới.
- Hiện tượng: đưa cảm biến lên trắng thì cột mạnh hơn trên đen.
- Phân tích trước: suy đoán mặc định raw LOW sai và đổi sang HIGH. Owner tiếp tục báo ngược; không có số raw đo trên màu đã biết nên kết luận cũ chưa có đủ bằng chứng.
- Cách sửa hiện tại B02: mặc định tạm LOW; sau sweep lấy white reference từng mắt trên nền trắng để xác định chiều mapping thực tế. Không coi test raw HIGH là bằng chứng màu đen thực.
- Kiểm tra lại: C/ARM emulation xác nhận raw cao → strength 1000/bit 1, raw thấp → strength 0/bit 0; cần owner thử lại trên OLED.
- Trạng thái: source-fixed, chưa hardware-verified cho bản sửa.

## ERR-008 — Test harness hết instruction budget trước khi hoàn tất

- Nguồn: lần chạy emulation đầu cho các vòng kiểm tra output bounds.
- Bằng chứng: 4.710 assertion đã qua, không có failure line nhưng chưa tới test_done.
- Root cause: giới hạn 10 triệu instruction không đủ cho soft-float ARMCC ở mức -O0.
- Cách sửa: tăng budget lên 100 triệu, giữ timeout 30 giây và yêu cầu chạm test_done.
- Kiểm tra lại: bộ test hoàn tất, không coi việc timeout giữa chừng là pass.
- Trạng thái: đã sửa test harness.

## ERR-009 — Emulator chưa khởi tạo RW execution section

- Nguồn: test supervisor báo không xuất PWM dù Start thành công.
- Bằng chứng: ELF scatter layout có RW init bytes trong RO load segment; SystemCoreClock trong RAM bằng 0 nếu chỉ nạp PT_LOAD.
- Root cause: test runner đi thẳng vào C, bỏ qua __main/scatter loader của ARM runtime.
- Cách sửa: nạp các SHF_ALLOC section theo execution address; RAM map zero cho NOBITS.
- Kiểm tra lại: test Start/Stop, ADC/UI watchdog và sticky capture fault đều pass.
- Trạng thái: test-harness-fixed; không phải lỗi firmware startup trên STM32.

## ERR-010 — Stop khi giữ N3 có thể nuốt lần Start tiếp theo

- Nguồn: review logic mới chuyển Stop từ short-release sang pressed event.
- Root cause: suppress_run_release còn giữ sau long press, trong khi release sau long press không phát short event.
- Cách sửa: clear suppression khi long_press N3 được phát; vẫn chặn short-release ngay sau Stop.
- Kiểm tra lại: thử Stop bằng nhấn ngắn và giữ lâu; lần nhấn mới khi đã dừng phải Start bình thường.
- Trạng thái: source-fixed; chưa thử nút này trên xe với RUN mới.

## ERR-011 — Polarity vẫn ngược sau lần sửa bằng giả định

- Nguồn: owner xác nhận đen không có tín hiệu, trắng lại mạnh ở bản trước.
- Root cause xác nhận trong thiết kế: min/max sweep không chứa nhãn màu nên không thể tự suy ra endpoint nào là đen/trắng. Raw HIGH/LOW từng được chọn bằng phỏng đoán.
- Cách sửa B02: white reference có nhãn màu rõ ràng, từng mắt; Start cần reference hợp lệ. Có firmware tag để đối chiếu artifact.
- Kiểm tra lại: C emulation xác nhận white LOW/black HIGH và white HIGH/black LOW đều cho trắng 0/đen 1000; mid-range reference bị từ chối.
- Trạng thái: source-fixed; cần owner thử lại B02, chưa hardware-verified.

## ERR-012 — Không rõ N1 chọn mục và N2 Enter; Start bị chặn không rõ lý do

- Nguồn: owner nhấn N1 rồi N3 và báo không chạy.
- Bằng chứng source: N1 ở Main Menu chỉ đổi mainIndex; N3 ngoài Run chỉ điều hướng. Start còn yêu cầu calibration, line valid và ADC mới.
- Chưa xác định chính xác điều kiện nào chặn Start trên mạch trong lần báo lỗi.
- Cách sửa: Main Menu hiện N2:OK, Run hiển thị lý do thiếu CAL/WHITE/LINE/ADC/duty thay cho thông báo chung; tag B02 xác nhận bản nạp.
- Kiểm tra lại: emulation UI thực dùng app/menu.c xác nhận N1+N2 vào Sensor, N2 vào Run, N3 Start/Stop, không tự restart khi nhả Stop; flow calibration → white reference → Start pass.
- Trạng thái: source-fixed, chưa thử UI B02 trên xe.

## ERR-013 — B02 dừng tại cua 90° dù bám vòng tròn/cua nhẹ

- Nguồn: owner thử thực tế ở Base 20%, báo xe tự ngắt khi cua 90°.
- Bằng chứng source B02: invalid mask khác 0 Stop ngay; gap chỉ 80 ms, cần stability 100 ms;
  SEARCH dựa error cuối, error gần 0 thì Stop. Chưa có F và chuỗi mask lúc dừng nên chưa chốt nhánh fault thực tế.
- Root cause trong thiết kế: thiếu corner memory và trạng thái quay giữ hướng đến khi thấy vùng giữa ổn định;
  khoảng trắng/nét đứt bị đánh giá quá chặt cho đường đua có góc gấp.
- Cách sửa B03: cue 6 ms, nhớ phía 250 ms, TURN/SEARCH giới hạn 1.400 ms, reacquire vùng giữa 12 ms;
  GAP 200 ms không cần 100 ms ổn định mỗi nét; ambiguous có PROBE 80 ms thay vì Stop ngay.
- Bổ sung: Base 30/Max 45, R reason trên OLED, steering inversion áp dụng cả error và mask.
- Kiểm tra lại: emulation cua trái/phải, edge flicker, full-width sau cue, dashes lặp,
  timeout và hướng sensor đảo. Test logical pass; không chứng minh xe qua mọi góc ở tốc độ mới.
- Trạng thái: source-fixed, B03 chưa hardware-verified. Khi thử cần ghi F/R nếu còn dừng.

## ERR-014 — Cắt chéo trên chuỗi góc vuông dù cua/gap đã cải thiện

- Nguồn: owner xác nhận cả quay sớm và thoát cua khi thân còn chéo; offset sensor–axle khoảng 16 cm, đoạn nối hai góc khoảng 40 cm.
- Bằng chứng source B03: cue tạo TURN ngay, chưa có phase bù offset; center 12 ms + turn tối thiểu 30 ms đã cho phép về TRACK.
- Giới hạn quan sát: một hàng sensor ở giữa line chưa chứng minh heading thân xe đã đúng.
- Cách sửa B04: ADVANCE có giới hạn với góc L rõ, yêu cầu clear line cũ, TURN min 120 ms / center 24 ms,
  rồi ALIGN chậm và kiểm tra ổn định trước TRACK. ADC/UI/motor watchdog giữ nguyên.
- Thời gian 350 ms ở duty tham chiếu 30% chỉ là giá trị trial; không có phép đo vận tốc để khẳng định tiến đúng 16 cm.
- Kiểm tra lại: phase sequence, không skip ADVANCE/ALIGN, old-line rejection, timeout/retry và chuỗi trái/phải;
  sau đó cần thử thực tế và chỉnh Advance/Align trên OLED.
- Trạng thái: B04 có emulation pass nhưng owner xác nhận regression thực tế. Không coi đây là đã khắc phục; được thay bằng B05 tại ERR-017.

## ERR-015 — Base/Max bị khuất trong danh sách PID cuộn

- Nguồn: owner hỏi tốc độ có đang cố định không và yêu cầu chỉnh trên OLED.
- Bằng chứng source: đã có setter và parameter RAM, nhưng PID Setup chỉ vẽ ba dòng nên Base/Max nằm dưới.
- Cách sửa B04: hiển thị cả năm dòng cùng màn, giữ cách bấm N1/N3 chọn/tăng giảm, N2 edit/xác nhận.
- Kiểm tra lại: UI test thấy cả Base/Max; Max chỉnh thành 40% thực sự giới hạn RUN output ở 400 permille.
- Trạng thái: source-fixed; thông số chưa lưu Flash nên Reset vẫn về mặc định.

## ERR-016 — UI emulation gọi printf khi chưa khởi tạo ARM runtime

- Nguồn: thêm test render màn PID/Speed vào harness đi thẳng vào C.
- Bằng chứng: invalid read tại address 0 trong `_printf_fp_dec_real`; ARM stdio gọi `__2snprintf` thay vì symbol `snprintf` tự stub.
- Root cause: test entry bỏ qua C runtime/locale init; firmware thực vẫn đi qua __main bình thường.
- Cách sửa: test-only macro `-Dsnprintf=TestSnprintf` và formatter stub chỉ cho test layout/event. ARMCC ưu tiên system stdio header nên fake header không đủ để chặn __2snprintf. Formatter của firmware không bị thay đổi.
- Kiểm tra lại: rerun suite; ghi rõ test không kiểm chứng format số float bằng C runtime thật.
- Trạng thái: sửa test harness, không phải kết luận lỗi printf trên board.

## ERR-017 — B04 regression: cua thường văng line, cua vuông xoay vòng

- Nguồn: owner báo B04 tệ hơn bản trước, một bánh đứng và bánh kia quay; chưa đọc được mode/F/R.
- Bằng chứng source: valid edge cue có thể gọi ADVANCE/TURN; ADVANCE dựa timer chưa đo vận tốc;
  TURN yêu cầu clear trắng/min turn/center chặt và ALIGN retry có thể bỏ qua line đã bắt lại.
- Chưa xác nhận phần nào xảy ra thực tế: PD saturation hoặc PWM lùi chưa thắng tải cũng có thể tạo một bánh đứng.
- Cách sửa B05: xóa geometry-based advance; valid line luôn PD, corner hint chỉ là hướng;
  mất line mới pivot đối chiều đối xứng 20%; valid trở lại nhận ngay RECENTER rồi TRACK;
  giữ deadline và watchdog, hạ default Base/Max về 20/30 để so baseline.
- Bổ sung LAST RUN: sau Stop xem mode/error/mask/PWM/SAT trên OLED thay vì đọc khi xe đang chuyển động.
- Test mới kiểm tra invariant valid line không tự vào pivot dù error ở mép hoặc legacy Advance=800;
  cân hai output pivot, nhận lại line ngay, deadline flicker, dashes và snapshot sau Stop.
- Trạng thái: source-fixed/tested; B05 chưa hardware-verified. Không quy lỗi cho kích thước owner trước khi có measurement.

## ERR-018 — D còn lớn khi kết thúc RECENTER

- Nguồn: regression test B05 yêu cầu derivative không kick khi vừa nhận line giữa sau error ở mép.
- Bằng chứng: test failure 30053; filter còn giữ D từ bước thay đổi error lớn dù state đã về TRACK.
- Cách sửa: khởi tạo last_error và reset D/I trước Follow ở frame đã đủ Reacq, không chỉ reset ở lần đầu thấy line.
- Kiểm tra lại: test reacquisition trái/phải và chuyển về tracking phải có D=0 với error giữa không đổi.
- Trạng thái: source-fixed, kiểm tra logic pass; chưa đo trên xe.

## ERR-019 — Tăng duty đường thẳng gây văng góc, gain tạm gây lắc

- Ngày / revision: 24/10/2026, B05 → B06.
- Nguồn: owner báo Max 40% bám được; tăng duty văng ở góc. Bộ source tạm 2/0/5, Base 40/Max 80 bị owner xác nhận lắc/không qua góc.
- Bằng chứng source: ramp giảm chỉ 100 điểm/giây, boost sau 200 ms gần tâm; 90→20 mất 700 ms cho biến common PWM.
  D tính theo giây: Kd=5 có thể gây correction saturation từ thay đổi error nhỏ.
- Root cause trong thiết kế: giảm PWM chậm và thiếu risk/cooldown; chưa có trace để quy toàn bộ lỗi thực cho một nhánh hoặc một gain.
- Sửa B06: planner error/D mới cập nhật/nhóm rộng, ramp giảm 600, boost confirm 300 ms/cooldown 250 ms;
  high-duty recovery có BRAKE tối đa 12 ms, valid line nhận PD ngay; defaults thử 12/0/0,12, Base 30/Max 60.
- Kiểm tra: regression high-duty 60/90%, giảm target, pulse deadline/cancel, dashes, symmetry, output bounds, Stop/watchdog.
- Trạng thái: source-fixed, 29.256 assertion pass; chưa hardware-verified. Không khẳng định Max 60/90 phù hợp mọi map.

## ERR-020 — Duty zero chưa dùng tổ hợp short brake được datasheet liệt kê rõ

- Ngày / revision: 24/10/2026, B06.
- Nguồn: review `bsp/motor.c` khi thêm phanh.
- Bằng chứng: duty=0 trước đây xuất IN1=IN2=Low/PWM Low;
  bảng Toshiba H-SW trang 4 liệt kê High/High/PWM bất kỳ là short brake, Low/Low/PWM High là coast.
- Root cause: dựa vào tổ hợp không liệt kê rõ; chưa đo để kết luận trạng thái output cũ trên module.
- Sửa: duty=0 dùng explicit High/High/PWM Low; giữ coast Low/Low/PWM High và STBY cố định High.
- Kiểm tra: compile actual `bsp/motor.c`, fake SPL register writes; kiểm tra HH/CCR0 khi brake,
  LL/CCR=period khi coast, polarity/PWM hai channel, reversal brake và command lease.
- Trạng thái: source-fixed/register-request-tested; cần kiểm tra waveform/quãng phanh thực tế.

## ERR-021 — Biến speed cũ có thể làm tăng lại PWM khi RECENTER

- Ngày / revision: 24/10/2026, B06.
- Nguồn: review high-duty recovery.
- Bằng chứng source B05: Pivot sửa left/right nhưng không đồng bộ speed; Follow khi reacquire có thể dùng speed cũ
  của đường thẳng dù pivot target chỉ 20%. Chưa có trace xác nhận đây là nguyên nhân lần văng thực tế.
- Sửa: Pivot đồng bộ magnitude request, entry RECENTER dùng common target <=20%; reset D/I như baseline.
- Kiểm tra: test từ 90% → BRAKE → pivot 20% → center request 20%, và valid line xuất hiện trong pulse.
- Trạng thái: source-fixed/emulation-tested, chưa hardware-verified.

## ERR-022 — B06 boost starvation trên đường thẳng

- Ngày / revision: 24/10/2026, B06 → B07.
- Nguồn: owner thử B06 cua mềm ổn, 90° đi được nhưng quay lâu; thẳng chậm, nghi phanh/tăng tốc xung đột.
- Bằng chứng: B06 chỉ tích lũy stable khi abs(error)<0,25 pitch và abs(D)<3 trong 300 ms;
  một mắt giữa đơn lẻ cho error ±0,5 nên không thể bật boost. Nhóm giữa 0x3C cũng bị giới hạn như cua.
  Regression mới giữ error=500/mask=0x10 fail 40037 trên B06; không có BRAKE khi line valid.
- Root cause xác nhận trong source/test: corridor quá hẹp so với centroid, dùng D của PD trực tiếp cho speed gate,
  reset stable ngay bởi jitter, heuristic width không phân biệt nhóm giữa. Chưa có trace thực để kết luận mọi nguyên nhân trên xe.
- Sửa B07: corridor 0,65 pitch/180 ms, planner-only rate filter 40 ms, risk vừa xác nhận 8 ms;
  hard condition giảm ngay, stable decay khi sai lệch nhẹ, cooldown 150 ms, broad cần vươn ra bên.
  Ramp tăng 65, giảm vẫn 600; gain/base/max và PD filter giữ B06.
- Theo yêu cầu quay nhanh hơn: pivot target 20→24%, ramp 250→350; cap Max, nhận line ngay,
  không tăng thời gian quay bắt buộc. Đây là trial parameter, chưa đo tốc độ góc.
- Bổ sung V/T/reason trong status/LAST RUN/OLED để phân biệt BASE/BST/CURV/WAIT/GAP/REC/BRK/PRB/OFF.
- Kiểm tra: 30.217 assertion pass, gồm middle-eye hai phía, jitter/nhóm giữa bốn mắt, sustained drift,
  edge/broad giảm tốc, recovery ramp/Max cap, estimator→planner→PWM boost, trace sau Stop và UI layout.
  Giữ regression watchdog/brake/dashes. Build Keil 0 error/warning.
- Harness: giới hạn completion hook tại test_done thay global Python callback từng instruction;
  suite mở rộng hoàn tất trong cùng timeout 60 s/instruction budget 200 triệu. Không tính lần chưa tới test_done là pass.
- Trạng thái: source-fixed/emulation-tested; B07 chưa hardware-verified. Cua mềm B06 ổn là phản hồi owner, không là chứng minh mọi tốc độ/map.

## ERR-023 — Profile cua thấp hơn Base khiến tăng Base chưa đủ nhanh

- Ngày / revision: 24/10/2026, B07 → B08.
- Nguồn: owner báo Base 30%/cua/90° còn chậm, muốn tăng tốc; xác nhận phanh đang ổn.
- Bằng chứng source: B07 curve targets bị cap 25/22%, RECENTER 20%, pivot 24% độc lập với Base.
  Vì vậy chỉ tăng Base vẫn không nâng được các cap này. Đây là giới hạn profile trước, không là kết luận lỗi phanh.
- Sửa B08: default Base 40/Max 75, acceleration 80, curve caps 32/28, pivot 30/ramp 450,
  RECENTER min(Base; 26%). Giữ gain 12/0/0,12 và planner B07, không thêm delay/minimum-spin.
- Bảo toàn phanh: so backup xác nhận driver motor, BeginRecovery/RecoveryOutput và các constant
  brake threshold/pulse, deceleration, loss/GAP/PROBE/recovery deadlines không đổi.
- Validation: build Keil 0 error/warning, 30.223 assertion pass; kiểm tra profile đi tới target,
  cap Max thấp, valid-line authority, brake cancel/deadline, gaps, UI/Start/Stop và watchdog.
- Trạng thái: source-tested; owner chưa thử profile B08. PWM cao hơn không là phép đo vận tốc/quãng phanh/góc quay.

## ERR-024 — Mép nét đứt bị loại khỏi GAP bởi D-kick, hold chưa giữ lệnh

- Ngày / revision: 24/10/2026, B08 → B09.
- Nguồn: owner báo trắng thì khựng, đưa đen trở lại thì chạy; cua nhẹ cũng khựng. Không có mode/V/T thực.
  Owner chọn bỏ tự tăng tốc lên Max và chạy Base cao cố định để mượt.
- Bằng chứng source/test: B08 GAP gate dùng abs(D PD)<12; đổi error 0→0,5 pitch trong 2 ms
  tạo filtered D khoảng 29,4, từ chối GAP ngay dù là mắt giữa. Test mới fail 30035 trên B08.
  ForwardHold hạ về Base, ambiguous PROBE có thể hạ về 15%. Test trước chỉ assert PWM>0,
  không chứng minh GA25 còn quay hoặc lệnh trước gap được giữ.
- Root cause tái hiện: quyết định từ một derivative sample và thiếu trusted command snapshot.
  Chưa có trace để xác nhận toàn bộ trạng thái xe owner đã gặp; fault thật cũng không tự resume khi thấy đen.
- Sửa B09: RememberGuide lưu L/R valid gần tâm với filtered rate; HoldGuide phát lại exact commands,
  clamp Max; TTL entry 80 ms, edge/drift invalidate. GAP span 300 ms, central split grace 12 ms tính chung deadline.
  Thấy valid nhận PD ngay; quá span fault latch, không tự Start lại. Driver/pulse phanh và watchdog giữ nguyên.
- Theo lựa chọn owner: Base cruise 45%, Max chỉ ceiling 75%, bỏ boost/cooldown; cua nhẹ không bị gate tốc độ,
  cua gắt chuyển target liên tục từ error 1,5→3 thay cap bước. Gain 12/0/0,12 và pivot30/RECENTER26 giữ trước.
- Validation: 33.993 assertion pass; giữ exact L/R 240 ms trắng từ estimator→controller→motor-request,
  nhận đen không Start mới, D-kick, split grace, stale/edge rejection, timeout boundary, watchdog trong GAP và fault latch.
  Build Keil 0 error/warning. Không kiểm chứng chuyển động dưới tải, quãng phanh hoặc map thực bằng emulation.
- Trạng thái: source-fixed/tested; B09 chưa hardware-verified. Mở rộng GAP là bound thời gian, không là đo chiều dài nét đứt.

## Mẫu ghi lỗi mới

```text
## ERR-NNN — Tên lỗi
- Ngày / firmware revision:
- Nguồn và điều kiện tái hiện:
- Hiện tượng:
- Bằng chứng / log:
- Root cause:
- Cách sửa:
- Cách kiểm tra lại và kết quả:
- Trạng thái: open / source-fixed / hardware-verified.
```

## ERR-025 ? Cua d?ng, Start readiness che nh?n fault

- Ng?y / revision: 24/10/2026, B09 ? B10.
- Owner: cua 90? tr??c ?n nay d?ng v? hi?n PLACE ON BLACK LINE. Ch?a c? trace th?c ?? x?c nh?n nh?nh d?ng.
- Evidence: menu d?ng START_NO_LINE khi ?ang FAULT; regression split mask 0xE8/0x17 t? tracking gi?a fail 30041 tr?n B09, kh?ng t?o h??ng recovery.
- Fix: fault label ?u ti?n Start readiness; split b?t ??i x?ng c? outer edge x?c nh?n c?ng h??ng 6 ms m?i recovery. Gi? valid PD authority, phanh, GAP, deadlines, watchdog v? explicit Start. Kh?ng thay gain/t?c ??.
- Validation: 34.008 assertions pass; Keil 0 error/warning. Chi ti?t v? gi?i h?n heuristic t?i recovery-b10.md.
- Status: source-fixed/emulation-tested, ch?a hardware-verified; ch?a kh?ng ??nh s?a h?t l?i cua th?c t?.

## ERR-026 — Mất hướng mới trong recovery và thiếu guide tại handoff

- Ngày / revision: 24/10/2026, B10 → B11.
- Owner: nét đứt/góc đơn sau thẳng dài ổn; góc vuông liên tục văng và LINE LOST. Không có trace F/R thực.
- Evidence: recovering return trước split cue; regression hướng ngược trong RECENTER fail 30036 trên B10. Exit RECENTER chưa RememberGuide, có thể dừng sau 6 ms trắng ngay sau exit.
- Fix: ConfirmSplit dùng chung; chỉ retarget sau khi episode đã thấy valid line, không reset deadline. Hoàn tất RECENTER lưu guide mới theo điều kiện hiện có và xóa ambiguous/gap flag cũ. Giữ tốc độ/gain/phanh/watchdog/explicit Start.
- Validation: 34.073 assertions pass; Keil 0 error/warning. Xem recovery-b11.md.
- Status: source-fixed/emulation-tested, chưa hardware-verified. B10 được owner xác nhận riêng nét đứt/góc đơn; không suy ra toàn map đã ổn.

## ERR-027 — GAP timeout không thử lại phía line gần nhất

- Ngày / revision: 24/10/2026, B11 → B12.
- Owner: ảnh line liên tục xung vuông, chuỗi góc vẫn LINE LOST; đề nghị nhớ hướng cuối. Chưa có trace/F/R để biết nhánh dừng thực.
- Evidence: lệch bên rõ → giữa ngắn → trắng quá 300 ms; B11 fault trực tiếp, regression mới fail30036. Không kết luận đây là mọi nguyên nhân trên xe.
- Fix: last_side độc lập PD error, TTL600ms; valid |error|>=1,2 pitch hoặc split cue được chấp nhận cập nhật phía. Trusted center100ms xóa sớm. Khi GAP/ambiguous hết allowance hoặc thiếu hướng tức thời, thử recovery nếu còn memory. Không mở rộng GAP, không reset recovery1200ms, không bypass watchdog/fault latch.
- Validation: 34.085 assertions pass; Keil 0 error/warning; chi tiết tại recovery-b12.md.
- Status: source-fixed/emulation-tested; chưa hardware-verified. R2 timeout vẫn Stop, không khẳng định chữa hết văng.

## ERR-028 — Vừa thoát cua đã cấp GAP dài khi mất lại line

- Ngày / revision: 24/10/2026, B12 → B13.
- Owner: chuỗi góc vuông vượt line, LINE LOST; nghi không nhớ trạng thái trước đó. Không có trace F/R để xác nhận tất cả nguyên nhân.
- Evidence: B12 có last_side nhưng RECENTER exit tạo guide, trắng tiếp theo có thể giữ tiến300ms. Regression mới với profile B12 fail30040. Test đầu fail80 do owner đổi defaults, tách riêng khỏi regression recovery.
- Fix: post-turn memory200ms; trusted center100ms xóa sớm. Reloss không cấp GAP, xác nhận6ms rồi recovery theo hướng mới/side/turn lưu. Giữ deadline episode qua exit ngắn, không gia hạn bằng center flicker. Valid PD, brake và watchdog giữ nguyên.
- Profile: backup source10/0/3,45/90; source đổi giữa phiên về12/0/0,12 được giữ. Max về75 theo profile HEX B12; Base45 không tăng.
- Validation:34.106 assertions pass; Keil0 error/warning. Xem recovery-b13.md, gồm tradeoff nét đứt ngay sau cua có thể kích recovery.
- Status: source-fixed/emulation-tested, chưa hardware-verified; không tuyên bố giải quyết toàn bộ văng.

## ERR-029 — Mẫu valid rộng lệch bên chưa giảm tốc và mất dấu trước recovery

- Ngày / revision:24/10/2026, B13→B14.
- Owner: góc90° ngắn đi thẳng qua; cần khoanh vùng ADC hay nhận dạng/control.
- Source/test: F8/1F valid equal-strength centroid±1,5 còn targetBase; test mới fail30034 trên B13. Broad→center ngắn→white có đường vào GAP dù chưa TURN, nên guard sau recovery không áp dụng. Chưa có trace mask thật trên xe.
- Fix: confirmed valid shape6ms có center+outer edge/asymmetry giảm target cap28 nhưng giữ forward PD. Pending200ms, trusted center100ms clear; loss6ms tìm theo hướng, không đi GAP dài. Không đổi gain/Base/ADC500Hz/brake/watchdog.
- Khoanh vùng: thêm DIAG lưu C/G/T, mask/mode/direction tại loss và callback timing; N1 khi dừng chuyển LIVE/LAST RUN/DIAG. Không dùng counters làm bằng chứng thời điểm sự kiện chính xác.
- Validation:34.135 assertions pass; Keil0 error/warning; actual estimator/raw mask và frozen diagnostics/timingfault tested. Xem diagnosis-b14.md.
- Status: source-fixed/emulation-tested; timing thực, sensor response, heading/quán tính và map còn cần đo. Cua rộng/nét đứt sau cua có thể bị giảm tốc/recovery sớm.

## ERR-030 — Tăng Base toàn bộ khiến vào cua và GAP quá nhanh

- Ngày/revision:26/10/2026, B14→B15.
- Owner:45/75 chạy cả map;60/80–90 vọt line, trắng dừng; muốn nhanh cả thẳng/cua mềm.
- Evidence source: B14 tăng Base làm tăng nền cả thẳng/cua; GAP giữ exact duty300ms, không phân biệt phần tốc dư. Chưa có trace thực để kết luận mọi lần dừng là GAP timeout hay phanh muộn.
- Fix: bonus giới hạn10/cap55/headroom15; tăng dần theo tracking, rút theo error/outward rate; guard300ms sau cua gấp; GAP60ms exact rồi giảm phần dư vềBase theo tỷ lệ L/R. Giữ PID/góc/recovery/watchdog. Gap ngắn bắt lại giữa không giật vềBase.
- Validation:34.709 assertions pass, Keil0 error/warning; xem speed-b15.md. Bổ sung test cua mềm, bonus đạt target, quantization, guard, GAP ngắn/dài và estimator integration.
- Status: source-tested, chưa hardware-verified. Backup B14 được giữ để đối chiếu; chưa khẳng định hết văng thực tế.

## Theo dõi thử tốc B16 — chưa ghi nhận lỗi mới

Owner báo B15 cải thiện khoảng0,5s và yêu cầu profile50/80. B16 đổi defaults, giữ thuật toán/PID/trần bonus55; GAP dài hiện về50 nên cần kiểm tra thực. 34.715 assertions pass; Keil0 error/warning. Chưa có hardware validation cho B16. Backup/source/HEX và quy trình thử tại [profile-b16.md](profile-b16.md).

## ERR-031 — Cua mềm bị giảm tốc và recovery/cua gấp tốn thời gian

- Owner thử55/85: hơi vọt lúc vào90°, cua mềm cảm giác phanh nhiều, chuỗi góc ổn nhưng chậm.
- Evidence source: blend cũ từ1,5 pitch giảm cả error ổn định; cap28/RECENTER26/pivot30 cố định. Chưa có trace xác định full brake ở cua mềm.
- Fix B17: blend2→3, cap32, outward rate risk riêng; pivot36/ramp600, RECENTER30. Giữ gain, brake và deadlines. Backup B16 trước sửa.
- Validation:34.734 assertions pass, Keil0 error/warning; xem speed-b17.md. Test đủ ramp đảo chiều từ lệnh forward, không giả định mọi TURN bắt đầu từzero.
- Hardware feedback owner: hết map, giảm khoảng1s, phanh90° tốt hơn; cua mềm/chuỗi góc vẫn tốn thời gian.60/90 phanh nhiều và chậm hơn. Trạng thái: cải thiện được owner báo, tối ưu cua còn open; chưa xác định nhánh gây khựng thực.

## ERR-032 — Fragmented frame ở cua mềm hạ ngay về PROBE15

- Nguồn: owner báo B17 còn khựng ở cua mềm; chưa có trace thực xác định full brake hay hạ PWM.
- Source/reproduction: stable valid error1,9/mask60 không có straight guide; invalid24 chuyển target15 ngay. Không kết luận đây là mọi lần khựng trên xe.
- Fix B18: snapshot moderate curve, exact hold tối đa8ms fragmented/4ms white, tuổi10ms; edge/pending/post-turn/hint/outward bị loại. Không reset loss/ambiguous/deadline. Valid nhận lại PD ngay. Tăng post-turn central ramp160 dưới42; giữ55/85 và B17 gains/pivot/phanh.
- Đo thêm: TIME lưu command duration các mode, CV subset TRACK; không đo phanh cơ học, không thay thế temporal trace.
- Validation:34.754 assertions pass; Keil0 error/warning. Thử expiry, handoff, white6ms recovery, square/memory/watchdog, diagnostic freeze/reset/menu. Xem speed-b18.md.
- Status: source-fixed/emulation-tested, chưa hardware-verified. Backup B17 giữ nguyên.

## ERR-033 — Trần đường thẳng cố định55 gây nhầm với Max

- Owner: B18 55/85 ổn cả map, tăng Base60 vọt nhưng không tiết kiệm thời gian; yêu cầu chỉnh tốc thẳng riêng.
- Evidence source: FAST_CAP55/bonus10/headroom15 giới hạn tự tăng; Max chỉ trần bánh, Base60 vượt cap bonus vì nâng cả nền.
- Fix B19: straight_percent mặc định60, OLED6 dòng; clamp Base<=Straight<=Max. Chỉ cấp phần tăng thêm trong corridor0,8 pitch đủ80ms và không guard. Bỏ cap/bonus/headroom cố định cũ; giữ B18 phanh/recovery/bridge/GAP policy. Cua ổn định ngoài corridor vềBase.
- Validation:34.763 assertions pass, Keil0 error/warning. Test Straight62, clampMax61/belowBase, menu độc lập/clamp/layout, central quantization và recovery/watchdog regressions. Xem speed-b19.md.
- Status: source-fixed/emulation-tested, chưa thử xe B19. Backup B18 đã lưu.

## ERR-034 — Phanh vào góc muộn và correction có thể làm cua mềm khựng

- Owner xác nhận PID12/0/0,12; B19 cải thiện ít, cua mềm còn cảm giác phanh; thẳng nhanh vào90° có thể vọt tới line khác. Chưa có trace xác định đầy đủ nguyên nhân.
- Evidence source: full brake chỉ bắt đầu trong recovery sau invalid; valid shape chỉ giảm target. PD correction có thể kéo một bánh về0, driver dùng short brake tại0; chưa chứng minh đây là mọi lần khựng với gain mặc định. Valid reacquisition không phân biệt danh tính line.
- B20: slew build-up correction trong vùng moderate, nhả correction ngay; ENTRY BRAKE8ms cho confirmed shape sau fast-center history, latch chống lặp và chuyển phần pulse còn lại sang recovery nếu invalid. Post-turn cap35 khi xác nhận center60ms; valid PD không bị khóa. Không tuyên bố nhận đúng mọi line cạnh nhau.
- Instrumentation: STEER lưu ZERO command events/ms và ENTRY events/ms; không là đo bánh dừng cơ học. Bổ sung mode vào arrays/telemetry; năm trang N1.
- Validation:34.861 assertions pass, Keil0 error/warning. Hai phía, dt1..4, pulse/cancel/rearm/no-double-brake/deadline, soft steering release/edge bypass, cap rejoin, raw-mask supervisor, diagnostics/watchdog/menu. Driver motor/ADC/estimator unchanged so với backup B19.
- Harness mở rộng300M instructions/60s toàn suite; hoàn tất test_done. Không chứng minh execution time thực.
- Status: implemented/emulation-tested, chưa hardware-verified. Backup và chi tiết tại speed-b20.md.
