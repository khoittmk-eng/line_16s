> B24 giữ format/Flash layout và công cụ CH340. CORNER chỉ bắt cue có hướng/TURN/fault, bỏ invalidPROBE không hướng; LOSS không đổi. Stop vẫn Save được tail nếu chưatrigger. Xem [speed-b24.md](speed-b24.md).

> **B23 tương thích:** format record/CSV/Flash giữ nguyên, parser/receiver này đọc cả B22 và B23. B23 thêm `flags` bit4 cho PROBE continuation có giới hạn; xem [speed-b23.md](speed-b23.md). Tài liệu B22 bên dưới vẫn mô tả quy trình Save/Export.

# Nhận log CH340 trực tiếp trên PC

Dùng được ngay với firmware **B22**, không nạp lại STM32. Script nhận qua COM, lưu file mới và kiểm tra CRC32/số mẫu/schema. Không in các dòng sensor ra màn hình.

## Thao tác mỗi lần xuất

1. Trên xe, bảo đảm log đã **Save RAM to Flash → SAVED**.
2. Nối **PB12 → CH340 RXD**, **GND → GND**, cấp nguồn STM32. CH340 RX phải tương thích logic3,3V; nếu STM32 có nguồn riêng thì không nối chân nguồn CH340.
3. **Đóng Hercules** và Serial Monitor khác đang giữ COM.
4. Trong terminal của project, chạy:

```powershell
cd D:\stm32-linefollower-spl
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\receive-blackbox.ps1
```

5. Script liệt kê COM. Nhập số thứ tự của dòng **USB-SERIAL CH340** hoặc tên như **COM5**, rồi Enter. Không tự chọn Intel AMT/SOL nếu đó không phải adapter của xe.
6. Đợi **READY**. Sau đó trên OLED chọn **6. System → N2: BLACK BOX → Export Flash CSV → N2**. Chỉ nhấn Export một lần.
7. Chờ **PASS**. Script in đường dẫn `GUI FILE NAY`; gửi nguyên file CSV đó cho mình. Không copy cửa sổ terminal.

Lần đầu, launcher tự cài **pySerial3.5** vào `build/serial-deps`, cần có Python3/pip và internet. Các lần sau dùng bản đã cài. Không sửa global Python environment. Parser/tóm tắt chỉ dùng Python standard library.

Ví dụ kết quả:

```text
PASS: CRC32 ..., 160 mau, firmware B22.
GUI FILE NAY: D:\stm32-linefollower-spl\logs\run-20261028-153000-123456.csv
Tom tat:      D:\stm32-linefollower-spl\logs\run-20261028-153000-123456.summary.json
```

Tên gồm thời điểm nhận và microseconds; script tạo file theo chế độ exclusive, không ghi đè lượt trước. `logs/` được Git ignore để tránh tự push dữ liệu chạy thử. File JSON giúp xem profile, chuyển mode và thời lượng lệnh xấp xỉ; **CSV là file chính cần gửi**.

## Khi nhận lỗi

- **FAIL / CRC32 mismatch:** truyền hoặc copy bị sai; chỉ giữ `.partial.log`, không công bố CSV hợp lệ. Export lại cùng bản Flash vào lượt nhận mới.
- **Không có header:** có thể đã bấm Export trước READY, nhầm COM hoặc sai dây/nguồn. Chạy script lại, đợi READY rồi Export.
- **Bị ngắt trước END:** giữ raw data và báo lỗi sau5giây không có byte; kiểm tra adapter/nguồn và xuất lại.
- **Port busy/access denied:** đóng Hercules/Serial Monitor; mỗi lần chỉ một chương trình mở COM.
- **Chưa thấy CH340:** kiểm tra USB/driver và chạy lại. Tên COM có thể đổi sau khi cắm sang cổng USB khác.
- **Ctrl+C:** hủy nhận, đóng COM; dữ liệu chưa xác nhận được giữ dưới dạng `.partial.log`.
- Không cần ARM hoặc chạy xe lại để Export lại bản đã Saved. ARM dùng để chuẩn bị lượt ghi trên xe mới.

Script mặc định chờ header120giây; khi đã có header, giới hạn transfer90giây và dung lượng128KiB. Nó không gửi byte/lệnh nào xuống xe. DTR/RTS đặt false trước khi mở; chỉ nối RXD/GND như hướng dẫn.

## Lệnh tùy chọn

```powershell
# Chỉ liệt kê COM
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\receive-blackbox.ps1 -ListPorts

# Chọn sẵn COM và thư mục nhận
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\receive-blackbox.ps1 -Port COM5 -OutputDir D:\logs-xe

# Chạy Python trực tiếp sau khi dependency đã có
python scripts/receive-blackbox.py --port COM5 --wait 180
```

Kiểm tra lại một file đã nhận:

```powershell
python scripts/read-blackbox.py logs/run-20261028-153000-123456.csv
```

## Validation

18 receiver tests pass với gói serial giả lập và CSV do C/ARM emulator xuất: split header/END, các cỡ USB chunk, read rỗng, timeout, disconnect, Ctrl+C, CRC lỗi, file collision và đóng COM. 8 parser/layout tests tiếp tục pass. PowerShell launcher đã thử cài dependency local và liệt kê cổng Windows. Chưa nhận trọn một bản qua CH340 thật trong phiên này; cần bạn thử nối adapter và Export.

Định dạng và giới hạn cửa sổ STM32: [blackbox-b22.md](blackbox-b22.md). API serial tham chiếu [pySerial3.5](https://pyserial.readthedocs.io/en/latest/pyserial_api.html).
