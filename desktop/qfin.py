#!/usr/bin/env python3
"""qFin CLI — talk to a FinOS device over USB serial (no Electron required)."""
import argparse, sys, time, glob, json

try:
    import serial
except ImportError:
    print("pip install pyserial", file=sys.stderr)
    sys.exit(1)


def crc8(data: bytes) -> int:
    c = 0
    for b in data:
        c ^= b
        for _ in range(8):
            c = ((c << 1) ^ 0x07) & 0xFF if c & 0x80 else (c << 1) & 0xFF
    return c


def frame(cmd: int, payload: bytes = b"") -> bytes:
    n = len(payload)
    hdr = bytes([0x54, 0x46, 0x01, cmd, n & 0xFF, (n >> 8) & 0xFF])
    cr = crc8(bytes([cmd])) ^ crc8(bytes([n & 0xFF, (n >> 8) & 0xFF])) ^ (crc8(payload) if n else 0)
    return hdr + payload + bytes([cr])


def decode(buf: bytes):
    i = 0
    out = []
    while i + 7 <= len(buf):
        if buf[i:i+3] != b"TF\x01":
            i += 1
            continue
        cmd = buf[i+3]
        n = buf[i+4] | (buf[i+5] << 8)
        if i + 7 + n > len(buf):
            break
        payload = buf[i+6:i+6+n]
        out.append((cmd, payload))
        i += 7 + n
    return out


def guess_port():
    cands = glob.glob("/dev/ttyACM*") + glob.glob("/dev/ttyUSB*") + glob.glob("/dev/cu.usb*")
    return cands[0] if cands else None


def xfer(ser, cmd, payload=b"", wait=0.6):
    ser.reset_input_buffer()
    ser.write(frame(cmd, payload))
    ser.flush()
    time.sleep(wait)
    raw = ser.read(ser.in_waiting or 1)
    time.sleep(0.05)
    raw += ser.read(ser.in_waiting or 0)
    return decode(raw)


def main():
    ap = argparse.ArgumentParser(description="qFin CLI")
    ap.add_argument("-p", "--port", default=None)
    ap.add_argument("cmd", nargs="?", default="info",
                    help="info | ping | ls [path] | install file.tapp | screenshot | reboot")
    ap.add_argument("arg", nargs="?")
    args = ap.parse_args()
    port = args.port or guess_port()
    if not port:
        sys.exit("no serial port")
    ser = serial.Serial(port, 115200, timeout=0.3)
    time.sleep(0.2)
    cmd = args.cmd
    if cmd in ("info", "ping"):
        frames = xfer(ser, 0x02 if cmd == "info" else 0x01)
        for c, p in frames:
            print(p.decode("utf-8", "replace"))
        if not frames:
            ser.write(b"info\n")
            time.sleep(0.2)
            print(ser.read(4096).decode("utf-8", "replace"))
    elif cmd == "ls":
        path = (args.arg or "/ext").encode()
        for c, p in xfer(ser, 0x03, path):
            print(p.decode())
    elif cmd == "install":
        data = open(args.arg, "rb").read()
        for c, p in xfer(ser, 0x08, data, wait=1.2):
            print(p.decode())
    elif cmd == "reboot":
        xfer(ser, 0x0A)
    elif cmd == "screenshot":
        frames = xfer(ser, 0x0B, wait=0.8)
        for c, p in frames:
            open("screenshot.bin", "wb").write(p)
            print("wrote screenshot.bin", len(p))
    else:
        ser.write((cmd + "\n").encode())
        time.sleep(0.3)
        print(ser.read(4096).decode("utf-8", "replace"))


if __name__ == "__main__":
    main()
