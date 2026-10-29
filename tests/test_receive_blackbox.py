"""Receiver tests use serial-like chunks and the real C-generated CSV fixture."""
import importlib.util
import io
import json
from pathlib import Path
from types import SimpleNamespace
import tempfile
import types
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('receiver', ROOT / 'scripts/receive-blackbox.py')
receiver = importlib.util.module_from_spec(spec)
spec.loader.exec_module(receiver)


class FakePort:
    def __init__(self, chunks):
        self.chunks = list(chunks)
        self.now = 0

    @property
    def in_waiting(self):
        return len(self.chunks[0]) if self.chunks else 0

    def read(self, size):
        self.now += 0.01
        if not self.chunks:
            return b''
        chunk = self.chunks.pop(0)
        if len(chunk) > size:
            self.chunks.insert(0, chunk[size:])
        return chunk[:size]


class ReceiverTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.frame = (ROOT / 'build/control-tests/blackbox-export.csv').read_bytes()

    def receive(self, chunks, **kwargs):
        port, raw = FakePort(chunks), io.BytesIO()
        frame = receiver.receive(port, raw, clock=lambda: port.now, **kwargs)
        return frame, raw.getvalue()

    def test_usb_chunks_split_header_end_and_rows(self):
        source = b'Ignored preamble\r\n' + self.frame
        for width in (1, 7, 63, 4096, len(source)):
            chunks = [source[i:i + width] for i in range(0, len(source), width)]
            frame, raw = self.receive(chunks)
            self.assertEqual(frame, self.frame)
            self.assertEqual(raw, source)

    def test_empty_serial_reads_do_not_lose_partial_lines(self):
        chunks = []
        for i in range(0, len(self.frame), 100):
            chunks.extend([self.frame[i:i + 100], b''])
        frame, _ = self.receive(chunks)
        self.assertEqual(frame, self.frame)

    def test_validation_and_unique_outputs(self):
        with tempfile.TemporaryDirectory() as directory:
            base = Path(directory) / 'run-test'
            csv_path, json_path, report = receiver.save_validated(self.frame, base)
            self.assertEqual(csv_path.read_bytes(), self.frame)
            self.assertEqual(json.loads(json_path.read_text())['crc32'], report['crc32'])
            with self.assertRaises(FileExistsError):
                receiver.save_validated(self.frame, base)
            self.assertEqual(csv_path.read_bytes(), self.frame)

    def test_existing_summary_is_not_overwritten(self):
        with tempfile.TemporaryDirectory() as directory:
            base = Path(directory) / 'run-test'
            base.with_suffix('.summary.json').write_text('keep')
            with self.assertRaises(FileExistsError):
                receiver.save_validated(self.frame, base)
            self.assertEqual(base.with_suffix('.summary.json').read_text(), 'keep')
            self.assertFalse(base.with_suffix('.csv').exists())

    def test_corrupt_or_truncated_data_never_publishes_csv(self):
        for frame in (self.frame.replace(b'12000', b'12001', 1), self.frame.split(b'# END')[0]):
            with tempfile.TemporaryDirectory() as directory:
                with self.assertRaises(ValueError):
                    receiver.save_validated(frame, Path(directory) / 'invalid')
                self.assertEqual(list(Path(directory).iterdir()), [])

    def test_wait_header_timeout(self):
        with self.assertRaisesRegex(receiver.CaptureError, 'cho header'):
            self.receive([b'old truncated sample\n'], wait_seconds=0.1)

    def test_truncated_transfer_idle_timeout_preserves_bytes(self):
        port, raw = FakePort([self.frame[:250]]), io.BytesIO()
        with self.assertRaisesRegex(receiver.CaptureError, 'bi ngat'):
            receiver.receive(port, raw, clock=lambda: port.now, idle_seconds=0.05)
        self.assertEqual(raw.getvalue(), self.frame[:250])

    def test_total_transfer_deadline(self):
        with self.assertRaisesRegex(receiver.CaptureError, 'Qua thoi gian'):
            self.receive([self.frame[:100]] + [b'x'] * 100, transfer_seconds=0.05)

    def test_repeated_header_is_rejected(self):
        with self.assertRaisesRegex(receiver.CaptureError, 'header thu hai'):
            self.receive([self.frame[:self.frame.index(b'\n') + 1]] * 2)

    def test_size_limit(self):
        with mock.patch.object(receiver, 'MAX_BYTES', 64):
            with self.assertRaisesRegex(receiver.CaptureError, '128 KiB'):
                self.receive([b'x' * 100])

    def test_port_selection_never_assumes_first_device(self):
        devices = [SimpleNamespace(device='COM3', description='other'),
                   SimpleNamespace(device='COM5', description='CH340')]
        self.assertEqual(receiver.choose_port(devices, ask=lambda _: '2', tell=lambda _: None), 'COM5')
        self.assertEqual(receiver.choose_port([], 'COM12'), 'COM12')
        with self.assertRaises(receiver.CaptureError):
            receiver.choose_port(devices, ask=lambda _: '9', tell=lambda _: None)

    def test_serial_settings_no_transmit_and_no_modem_reset(self):
        port = mock.Mock()
        serial = SimpleNamespace(Serial=mock.Mock(return_value=port), EIGHTBITS=8,
                                 PARITY_NONE='N', STOPBITS_ONE=1)
        opened = receiver.open_port(serial, 'COM5')
        self.assertIs(opened, port)
        serial.Serial.assert_called_once_with(port=None, baudrate=9600, bytesize=8, parity='N',
                                              stopbits=1, timeout=0.2, xonxoff=False,
                                              rtscts=False, dsrdtr=False)
        self.assertFalse(port.dtr)
        self.assertFalse(port.rts)
        self.assertEqual(port.port, 'COM5')
        port.open.assert_called_once()
        port.reset_input_buffer.assert_called_once()
        port.write.assert_not_called()

    def test_open_failure_closes_port(self):
        port = mock.Mock()
        port.open.side_effect = OSError('busy')
        serial = SimpleNamespace(Serial=lambda **_: port, EIGHTBITS=8, PARITY_NONE='N', STOPBITS_ONE=1)
        with self.assertRaises(OSError):
            receiver.open_port(serial, 'COM5')
        port.close.assert_called_once()

    def cli_capture(self, frame, failure=None):
        port = FakePort([frame])
        port.open = mock.Mock()
        port.close = mock.Mock()
        port.reset_input_buffer = mock.Mock()
        if failure is not None:
            real_read = port.read

            def read(size):
                if not port.chunks:
                    raise failure
                return real_read(size)
            port.read = read
        serial = types.ModuleType('serial')
        serial.Serial = mock.Mock(return_value=port)
        serial.SerialException = OSError
        serial.EIGHTBITS = 8
        serial.PARITY_NONE = 'N'
        serial.STOPBITS_ONE = 1
        tools = types.ModuleType('serial.tools')
        tools.list_ports = SimpleNamespace(comports=lambda: [])
        with tempfile.TemporaryDirectory() as directory:
            with mock.patch.dict('sys.modules', {'serial': serial, 'serial.tools': tools}), \
                    mock.patch('sys.stdout', new=io.StringIO()), \
                    mock.patch('sys.stderr', new=io.StringIO()):
                code = receiver.main(['--port', 'COM5', '--out-dir', directory])
            files = {p.name.split('.', 1)[1]: p.read_bytes() for p in Path(directory).iterdir()}
        self.assertTrue(port.close.called)
        return code, files

    def test_cli_success_publishes_only_verified_csv_and_summary(self):
        code, files = self.cli_capture(self.frame)
        self.assertEqual(code, 0)
        self.assertEqual(set(files), {'csv', 'summary.json'})
        self.assertEqual(files['csv'], self.frame)

    def test_cli_disconnect_preserves_partial_without_csv(self):
        code, files = self.cli_capture(self.frame[:100], OSError('device disconnected'))
        self.assertEqual(code, 1)
        self.assertEqual(files, {'partial.log': self.frame[:100]})

    def test_cli_crc_error_keeps_unverified_raw(self):
        broken = self.frame.replace(b'12000', b'12001', 1)
        code, files = self.cli_capture(broken)
        self.assertEqual(code, 1)
        self.assertEqual(files, {'partial.log': broken})

    def test_cli_cancel_keeps_partial_and_closes_com(self):
        code, files = self.cli_capture(self.frame[:100], KeyboardInterrupt())
        self.assertEqual(code, 130)
        self.assertEqual(files, {'partial.log': self.frame[:100]})

    def test_interrupt_while_publishing_cannot_leave_partial_csv(self):
        with tempfile.TemporaryDirectory() as directory, \
                mock.patch.object(receiver.json, 'dump', side_effect=KeyboardInterrupt):
            with self.assertRaises(KeyboardInterrupt):
                receiver.save_validated(self.frame, Path(directory) / 'run')
            self.assertEqual(list(Path(directory).iterdir()), [])


if __name__ == '__main__':
    unittest.main()
