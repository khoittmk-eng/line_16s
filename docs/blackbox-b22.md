> B24 giữ format/Flash layout và công cụ CH340. CORNER chỉ bắt cue có hướng/TURN/fault, bỏ invalidPROBE không hướng; LOSS không đổi. Stop vẫn Save được tail nếu chưatrigger. Xem [speed-b24.md](speed-b24.md).

> **B23 tương thích:** format record/CSV/Flash giữ nguyên, parser/receiver này đọc cả B22 và B23. B23 thêm `flags` bit4 cho PROBE continuation có giới hạn; xem [speed-b23.md](speed-b23.md). Tài liệu B22 bên dưới vẫn mô tả quy trình Save/Export.

> Nhận log không dùng Hercules: chạy `scripts/receive-blackbox.ps1`, chọn COM rồi bấm Export trên OLED; tự lưu CSV và kiểm tra CRC. Xem [hướng dẫn nhận log](receive-log.md). Dùng với firmware B22 hiện tại, không cần nạp lại.

# B22 — Hộp đen, lưu Flash và xuất CSV qua CH340

B22 bổ sung telemetry để phân biệt giảm PWM, phanh một bánh, BRAKE và recovery. Controller B21, profile **Base 55 / Straight 60 / Max 85**, PID **12 / 0 / 0,12** giữ nguyên. Đây là công cụ đo lệnh điều khiển; chưa đo tốc độ bánh hoặc quỹ đạo thật.

## 1. Nối dây và nạp firmware

| STM32 | CH340 | Ghi chú |
| --- | --- | --- |
| PB12 | RXD | Software UART TX, mức logic 3,3 V |
| GND | GND | Bắt buộc mass chung |
| Không nối | TXD | Firmware chỉ truyền log |
| Không nối khi STM32 có nguồn riêng | VCC / 5V / 3V3 | Cấp nguồn STM32 theo cách đang dùng |

PB12 được chọn vì UART mặc định vướng motor, line hoặc nút 3. Không remap các chân đang chạy xe. Dùng CH340 có RX tương thích 3,3 V; chọn chế độ USB–TTL, không dùng tín hiệu điện RS-232 trực tiếp.

Nạp `build/linefollower-B22.hex`; menu phải hiện **B22**. Nạp firmware vẫn bằng ST-Link/Keil như trước. CH340 chỉ lấy log. Lần đầu chạy phải calibration và lấy white reference.

**Flash log không giữ calibration hay PID qua Reset.** Log lưu bản sao cấu hình của lần chạy để đọc lại; firmware không tự dùng bản sao này điều khiển xe.

## 2. Menu và cách ghi một lượt

Sáu mục Main Menu giữ thứ tự cũ. Chọn **6. System → N2: BLACK BOX**.

| Thao tác trong BLACK BOX | Chức năng |
| --- | --- |
| N1 | Di chuyển xuống |
| N3 | Di chuyển lên |
| N2 | Thực hiện dòng đang chọn |
| Giữ N2 | Về Main Menu |
| Arm next run | Xóa bản RAM hiện tại và chờ lần Start tiếp theo; không xóa Flash |
| Trigger: LOSS/CORNER | Đổi loại trigger và ARM lại; cũng xóa RAM hiện tại |
| Save RAM to Flash | Lưu bản RAM đã đóng băng; không lưu khi xe RUN |
| Export Flash CSV | Phát bản Flash mới nhất hợp lệ qua PB12 |

Sau boot, recorder mặc định **ARMED / LOSS**. ARM chỉ chuẩn bị ghi, **không chạy motor**. Chỉ Start được chấp nhận tại Run mới bắt đầu ghi.

- **LOSS:** trigger đầu tiên khi BRAKE/TURN/FAULT hoặc invalid ngoài GAP. Một PROBE do fragmented frame ở cua mềm cũng có thể trigger. Nét đứt được nhận là GAP không kích LOSS.
- **CORNER:** ngoài các điều kiện LOSS, trigger khi có corner pending hoặc valid |error| ≥ 1,5 pitch. Đây là cue, không bảo đảm đúng góc 90°.
- Khi trigger: giữ dữ liệu trước sự kiện, ghi thêm một đoạn sau rồi chuyển READY.
- Khi Stop trước khi đủ đoạn sau: đóng băng dữ liệu đã có. Nếu chưa có trigger: giữ đoạn cuối trước Stop.
- Sau READY, chạy tiếp không ghi đè RAM; muốn ghi lượt mới phải **Save nếu cần → Arm next run**.

Quy trình khuyến nghị:
1. Vào BLACK BOX, chọn **CORNER** để xem vào cua hoặc **LOSS** để xem khựng/mất line; xác nhận **ARMED**.
2. Giữ N2 về Main Menu. Vào Run và Start bằng N3 như cũ.
3. Cho xe qua đoạn cần xem, sau đó Stop.
4. Vào **6. System → N2 → Save RAM to Flash → N2**.
5. Chờ **SAVED - POWER OFF OK**. Lúc này có thể tắt nguồn, tháo STM32.
6. Cấp nguồn lại, mở BLACK BOX và Export; không cần calibration để xuất bản Flash.

**READY chưa có nghĩa đã lưu Flash.** Nếu tắt nguồn trước Saved, RAM mới sẽ mất, bản Flash cũ vẫn là bản cũ. Nếu có RAM mới chưa Save, Export bị chặn với “SAVE RAM FIRST” để tránh gửi nhầm log trước.

## 3. Nhận bằng Hercules

1. Nối CH340 như bảng trên và cấp nguồn STM32.
2. Hercules → **Serial**, chọn đúng COM; **9600 baud, 8 data bits, parity None, 1 stop bit**, Handshake OFF, Mode Free.
3. Chọn **Open** trước khi bấm Export trên OLED. Dùng text mode, không bật HEX display hoặc chèn timestamp vào nội dung nhận.
4. Trong vùng Received data, dùng menu chuột phải **Log to File**, chọn tên file và bật ghi log (tên mục có thể khác theo phiên bản). Hoặc copy nguyên khối text nhận được sang file UTF-8.
5. Trên OLED chọn **Export Flash CSV → N2**. Màn hiện EXPORTING CSV; lúc này motor đã dừng và UI chờ hoàn tất truyền.
6. Chờ tới dòng **# END,count,...,crc32,...** và OLED **EXPORT DONE**, rồi kết thúc ghi file. Một bản tối đa có thể mất khoảng 15–30 giây ở 9600 baud; đây là ước tính theo lượng dữ liệu, chưa đo thực tế.
7. Gửi file `run-01.csv` hoặc `.txt` chứa đúng một lần xuất, kèm mô tả vị trí xe khựng. Giữ các dòng metadata bắt đầu bằng `#`.

Nút Send của Hercules gửi từ PC xuống; không cần dùng trong phương án TX-only này. Nếu chưa nhận đủ, có thể Export lại cùng bản Flash vào file mới.

Để kiểm tra file trên PC:

```powershell
python scripts/read-blackbox.py C:\logs\run-01.csv
```

Script kiểm tra CRC32, số dòng và schema, sau đó in JSON: profile, cửa sổ thời gian, chuyển mode/reason và thời lượng lệnh xấp xỉ. File bị cắt hoặc sai ký tự bị từ chối. LF/CRLF đều đọc được. Không cần cài thêm thư viện Python cho parser.

Nếu không nhận: kiểm tra COM, PB12→RXD, GND, nguồn STM32, 9600/8N1 và đã bấm Export. Nếu ký tự rác/CRC lỗi: kiểm tra mức logic, dây/mass và xuất lại; không dùng log lỗi để kết luận PID.

## 4. Ghi bao lâu và ghi gì?

Buffer **160 sample × 48 byte = 7.680 byte RAM**. Bình thường ghi mỗi 4 ms từ frame control 2 ms; frame chuyển mode hoặc trigger được ghi sớm hơn. Trước trigger giữ tối đa 80 sample; từ trigger ghi tối đa 80 sample gồm chính sample trigger. Với nhịp 4 ms, cửa sổ đầy khoảng **636 ms**, gồm khoảng 320 ms trước và 316 ms sau. Chuyển mode nhiều sẽ làm cửa sổ ngắn hơn.

Đây là **một đoạn quanh sự kiện đầu tiên**, không phải toàn bộ map. Nếu sự kiện cần xem nằm sau sự kiện đầu: thử Start ở đoạn dẫn tới góc đó, hoặc đổi LOSS/CORNER phù hợp. Nếu cần toàn bộ vòng/mọi góc, cần thiết kế dung lượng/ghi khác.

| CSV | Ý nghĩa / đơn vị |
| --- | --- |
| ms | Timestamp SysTick unsigned 32-bit; không phải thời gian tuyệt đối trên PC |
| raw0..raw7 | ADC 0..4095, thứ tự điện PA0..PA7 |
| error | Error sau steering sign; 1000 đơn vị = một khoảng mắt; invalid giữ error cũ |
| mask, valid | Mask canonical sau steering sign, dạng số decimal; validity 0/1 |
| left, right | Lệnh controller signed permille, ±1000 = ±100% PWM |
| common_x10, target_x10, correction_x10 | Phần trăm PWM nhân 10 |
| derivative_x10, planner_x10 | Rate đã filter, pitch/second nhân 10; saturate int16 |
| lost_ms, recovery_ms | Counter controller, saturate uint16 |
| mode, reason | Enum controller hiện tại; script giải mã tên |
| fault, stop_reason | Mã fault và lý do line stop |
| side | Hướng pending/last-side/post-turn tại frame, -1/0/+1 |
| flags | bit0 pending corner; bit1 post-turn; bit2 terminal; bit3 trigger |

Metadata ghi firmware, sequence, Base/Straight/Max, gains, DIR, polarity mask, align_ms và low/high/white calibration của tám mắt. Các giá trị này thuộc lúc Start.

Terminal row do Stop/watchdog lặp lại sensor gần nhất và đặt lệnh bánh bằng 0, có bit2 để phân biệt với ADC mới. Sau khi READY, sự kiện xảy ra ngoài cửa sổ không được thêm vào. Thời lượng parser tính từ các sample là xấp xỉ trong cửa sổ, không là thời gian cả vòng.

Driver motor vẫn có thể chèn zero update khi đảo chiều, nên lệnh controller không phải bằng chứng chính xác của từng PWM register update. Không suy ra tốc độ bánh, quãng đường phanh hoặc quỹ đạo từ duty.

## 5. Flash và runtime

| Vùng | Địa chỉ | Dung lượng |
| --- | --- | --- |
| Firmware | 0x08000000..0x0800BFFF | 48 KiB |
| Slot A | 0x0800C000..0x0800DFFF | 8 KiB |
| Slot B | 0x0800E000..0x0800FFFF | 8 KiB |

Scatter file `config/linefollower.sct` giới hạn firmware. Build script còn kiểm tra HEX không chạm vùng log. Mỗi slot gồm header 108 byte, tối đa 7.680 byte sample và commit marker tại cuối slot; có CRC32 cho header/payload.

Save chỉ được thực hiện khi stopped/fault, motor đã nhận StopBrake. ADC/TIM3/DMA được tạm ngưng khi erase/program để không tích cả HT/TC pending. Sau Save/Export, DMA bắt đầu lại từ buffer đầu và phải có frame mới trước Start; capture fault cũ vẫn yêu cầu Reset. Save/Export không tự Start.

Mỗi lần Save mới ghi slot còn lại, commit cuối cùng, kiểm tra CRC trước khi báo Saved. Lần lưu bị cắt điện chưa commit không thay thế slot cũ hợp lệ. Bản mới nhất hợp lệ được chọn khi boot. Lưu lại cùng RAM đã Saved không erase lần nữa.

Flash có số chu kỳ erase hữu hạn; chỉ Save khi cần. **Nạp lại firmware, Full Chip Erase hoặc quay về firmware cũ chưa reserve vùng log có thể xóa/đè dữ liệu. Luôn Export trước khi nạp lại.** Không thay đổi option bytes/write protection.

Software UART PB12 dùng DWT để định thời 9600/8N1, khóa IRQ khoảng một byte 1,04 ms, cho IRQ chạy giữa các byte. ADC tạm ngưng và motor dừng suốt Export. Save/Export có thể làm SysTick bỏ tick; timestamp chỉ dùng cho đoạn RUN, không đo khoảng thời gian bảo trì. Không Flash write, formatting hoặc UART trong callback điều khiển.

Thiết kế Flash tham chiếu [ST PM0075](https://www.st.com/content/ccc/resource/technical/document/programming_manual/10/98/e8/d4/2b/51/4b/f5/CD00283419.pdf/files/CD00283419.pdf/jcr%3Acontent/translations/en.CD00283419.pdf). Bản này chưa thử thao tác Flash và baud trên xe thực.

## 6. Kiểm chứng và bước thử đầu

- Cortex-M3 emulation: ring wrap/pre-post, manual Stop, trigger LOSS/CORNER, freeze, CRC, torn save ở nhiều vị trí kể cả commit, fallback slot, reboot, chặn thao tác khi RUN, bắt buộc frame mới sau maintenance và menu.
- Compile actual Flash/serial drivers với fake peripheral/clock: kiểm tra phạm vi slot, lỗi program/erase, PB12 start/data/stop bits, 9600 clock schedule và DWT wrap.
- Parser đọc CSV do C thật xuất trong emulator; thử thiếu END, mất dòng, đổi ký tự, nhiều lần xuất và đổi line ending. Layout checker thử boundary/checksum.
- Không có mô phỏng động lực học, Flash endurance hoặc đo baud vật lý. ADC pause/resume cần kiểm tra trên board; không được bỏ watchdog nếu có fault.
- Firmware build, số assertion và hash bản cuối được ghi trong validation/error-log.
- Backup trước sửa: `backups/B21-before-B22-20261028/source.zip` và HEX B21.

Thử đầu tiên khi motor không kéo xe: tạo một lần RUN ngắn, Stop → Save → tắt/bật nguồn → Export → parser phải đọc được. Sau đó calibration nếu vừa Reset, thử lại Start/Stop để xác nhận acquisition trở lại. Khi đã qua bước này mới chạy map để lấy log thực. Gửi log kèm video/vị trí lỗi sẽ giúp đối chiếu, nhưng log không tự chứng minh đã tối ưu xe.

Kết quả bản cuối: **84.890 assertions + 8 Python tests**, Keil **0 error / 0 warning**; HEX SHA256 `9A5CC6490BA319D6DF4E9AE73A3774222EB1E595ED7F48EBB607A2B2C5DB17B7`. Chi tiết [validation.md](validation.md).
