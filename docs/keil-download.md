# Nạp bằng Keil/ST-Link sau khi chạy xe

## Bằng chứng hiện có

Ngày 29/10/2026, owner xác nhận lỗi Cortex-M3/Flash Download xảy ra sau RUN/Stop, **chưa bấm Save RAM to Flash**, và chưa nối NRST. Log chỉ ghi Flash khi thao tác Save gọi `Robot_LogSave` → `Blackbox_Save` → `LogStorage_Begin`; RUN/Stop không gọi erase/program. Source không ghi option bytes hoặc bật readout protection. `FLASH_Lock()` chỉ khóa thao tác ghi Flash của application, không khóa quyền SWD như readout protection.

Hiện chưa có log lỗi đầy đủ của Keil hoặc thử nạp trên board trong phiên. Chưa kết luận nguyên nhân duy nhất và chưa xác nhận lỗi download đã hết.

## Thay đổi B26

`Board_RemapOLEDAndKeepSWD()` ghi cấu hình SWD enabled rõ ràng ở startup và khi remap I2C1 OLED. Hàm bỏ giá trị đọc của trường `SWJ_CFG` trước khi ghi lại `AFIO_MAPR`, chọn `JTAG disabled / SWD enabled`, giữ I2C1 trên PB8/PB9 và các remap khác. Đây là hardening theo RM0008 vì `SWJ_CFG` là write-only, readback không xác định; không phải bằng chứng driver OLED cũ đã tắt SWD trên board này. Không sửa vendor SPL, log storage hoặc option bytes.

Test dùng code board thật với MMIO giả kiểm tra mọi tổ hợp readback SWJ, remap, tính lặp lại và motor GPIO an toàn. Nó không chứng minh kết nối debugger vật lý thành công.

## Dây và cài đặt Keil

| ST-Link | STM32F103C8T6 |
| --- | --- |
| SWDIO / DIO | PA13 |
| SWCLK / CLK | PA14 |
| GND | GND chung với STM32 |
| NRST / RST của giao tiếp STM32 | Chân R / NRST — cần khi dùng Connect under Reset |

Chỉ DIO/CLK mà không có đường GND chung thì không đủ. STM32 cần nguồn ổn định. Dây NRST phụ thuộc đúng pinout probe; không dùng nhầm chân reset của giao tiếp SWIM. Nếu board đã có nguồn riêng, không tự nối thêm đường cấp nguồn thứ hai từ probe.

1. Dừng xe; đóng hoặc Disconnect STM32 ST-LINK Utility/CubeProgrammer để nhả probe. Mở đúng `D:\stm32-linefollower-spl\linefollower.uvprojx`.
2. `Options for Target → Debug → Use: ST-Link Debugger → Settings`: chọn **SWD**, clock thử **100–200 kHz**. Chưa có NRST: dùng **Normal**, reset **SYSRESETREQ** nếu driver cho chọn. Không chọn Under Reset khi thiếu đường NRST.
3. Sau khi nối NRST: có thể chọn **Connect under Reset**, reset qua chân phần cứng. Chế độ này cho debugger kết nối trước khi application chạy.
4. `Utilities`: chọn **Use Target Driver for Flash Programming / Use Debug Driver**. Source project đã dùng chung debug driver; chuỗi UL2CM3 dự phòng trong project không tự chứng minh Keil đang dùng nhầm ULINK.
5. `Settings → Flash Download`: có algorithm **STM32F10x 128kB Flash**, base `0x08000000`, Program/Verify; dùng **Erase Sectors** để tránh xóa log ngoài vùng firmware. Tên algorithm 128kB không có nghĩa được dùng thêm Flash ngoài 64KiB của C8. Firmware chỉ chiếm vùng đầu48KiB, log nằm16KiB cuối.
6. Build rồi Download. Nếu chip đang không cho kết nối mà chưa có NRST từ probe: thử giữ nút RESET trên board khi bắt đầu kết nối/download rồi thả. Cách ổn định hơn là nối đúng NRST. Không mặc định Full Chip Erase hay thay option bytes để chữa lỗi chưa rõ.
7. Sau lần nạp B26 đầu, Reset, vào menu và thử nạp lại trước RUN, rồi RUN/Stop và nạp lại khi **chưa Save**. Sau đó mới thử Save/Export và nạp lại. Ghi đúng bước đầu tiên thất bại và toàn bộ dòng Output của Keil.

Chạy kiểm tra cấu hình/file trên PC, không tác động board:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File "D:\stm32-linefollower-spl\scripts\diagnose-keil.ps1"
```

Nếu DLL/algorithm tồn tại và B26 vẫn lỗi sau RUN, cần đối chiếu SWD clock/reset/GND/nguồn và phiên bản driver/firmware ST-Link; không coi thêm code Flash là nguyên nhân khi chưa có erase/program.

Nguồn: [ST RM0008 — AFIO_MAPR/SWJ_CFG](https://www.st.com/resource/de/reference_manual/cd00171190.pdf), [ST UM0892 — connect under reset và dây reset](https://www.st.com/resource/en/user_manual/cd00262073-stm32-st-link-utility-software-description-stmicroelectronics.pdf), [Keil Flash Programming](https://www.keil.com/support/man/docs/uv4cl/uv4cl_flash_programming.htm).
