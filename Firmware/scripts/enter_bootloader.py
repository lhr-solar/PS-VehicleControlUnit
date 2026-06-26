#!/usr/bin/env python3
"""
enter_bootloader.py

Select a serial port (macOS / Windows / Linux / WSL) and send the "BOOT" magic
packet that makes the fsm_monkey_integration firmware jump into the STM32
built-in ROM (system-memory) bootloader. After that the chip can be reflashed
over the same UART with STM32CubeProgrammer or stm32flash.

Examples:
    python3 enter_bootloader.py                  # interactive port pick, sends "BOOT"
    python3 enter_bootloader.py -p COM5          # Windows
    python3 enter_bootloader.py -p /dev/ttyS3    # WSL (COM3 -> /dev/ttyS3)
    python3 enter_bootloader.py -p /dev/tty.usbserial-1420 -b 115200   # macOS
    python3 enter_bootloader.py --list           # just list ports and exit

Requires pyserial:  python3 -m pip install pyserial
"""

import argparse
import glob
import sys
import time

MAGIC_DEFAULT = "BOOT"
ACK_DEFAULT = "BOOTACK"
ACK_TIMEOUT_DEFAULT = 3.0
BAUD_DEFAULT = 115200

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    serial = None
    list_ports = None


def discover_ports():
    """Return a list of (device, description) tuples across all platforms."""
    ports = []
    seen = set()

    if list_ports is not None:
        for p in list_ports.comports():
            ports.append((p.device, p.description or ""))
            seen.add(p.device)

    # Fallback globs for *nix / WSL (list_ports can miss raw /dev nodes).
    for pattern in ("/dev/ttyUSB*", "/dev/ttyACM*", "/dev/ttyS*",
                    "/dev/tty.*", "/dev/cu.*"):
        for dev in sorted(glob.glob(pattern)):
            if dev not in seen:
                ports.append((dev, ""))
                seen.add(dev)

    return ports


def choose_port():
    ports = discover_ports()
    if not ports:
        sys.exit("No serial ports found. Plug in the VCU / UART adapter, "
                 "or pass --port explicitly.")

    print("Available serial ports:")
    for i, (dev, desc) in enumerate(ports):
        print(f"  [{i}] {dev}    {desc}".rstrip())

    while True:
        choice = input("Select port number (or paste a device path): ").strip()
        if not choice:
            continue
        if choice.isdigit() and int(choice) < len(ports):
            return ports[int(choice)][0]
        return choice  # treat as a raw device path


def main():
    ap = argparse.ArgumentParser(
        description="Send the BOOT magic packet to enter the STM32 ROM bootloader.")
    ap.add_argument("-p", "--port",
                    help="Serial port (e.g. COM5, /dev/ttyS3, /dev/tty.usbserial-XXXX)")
    ap.add_argument("-b", "--baud", type=int, default=BAUD_DEFAULT,
                    help=f"Baud rate (default {BAUD_DEFAULT})")
    ap.add_argument("-m", "--magic", default=MAGIC_DEFAULT,
                    help=f'Magic command to send (default "{MAGIC_DEFAULT}")')
    ap.add_argument("-n", "--newline", action="store_true",
                    help="Append a newline after the magic command")
    ap.add_argument("--ack", default=ACK_DEFAULT,
                    help=f'Token the board replies with (default "{ACK_DEFAULT}")')
    ap.add_argument("--ack-timeout", type=float, default=ACK_TIMEOUT_DEFAULT,
                    help=f"Seconds to wait for the ACK (default {ACK_TIMEOUT_DEFAULT})")
    ap.add_argument("--no-ack", action="store_true",
                    help="Don't wait for the board's ACK")
    ap.add_argument("-l", "--list", action="store_true",
                    help="List serial ports and exit")
    args = ap.parse_args()

    if args.list:
        for dev, desc in discover_ports():
            print(f"{dev}\t{desc}".rstrip())
        return

    if serial is None:
        sys.exit("pyserial is required: python3 -m pip install pyserial")

    port = args.port or choose_port()
    payload = args.magic.encode() + (b"\n" if args.newline else b"")

    ack_token = args.ack.encode()
    try:
        with serial.Serial(port, args.baud, timeout=0.1) as ser:
            ser.reset_input_buffer()
            ser.write(payload)
            ser.flush()
            print(f"Sent {payload!r} to {port} @ {args.baud} baud.")

            if args.no_ack:
                print("Not waiting for ACK (--no-ack).")
            else:
                deadline = time.monotonic() + args.ack_timeout
                buf = bytearray()
                got_ack = False
                while time.monotonic() < deadline:
                    chunk = ser.read(256)
                    if chunk:
                        buf += chunk
                        if ack_token in buf:
                            got_ack = True
                            break
                if not got_ack:
                    sys.exit(
                        f"ERROR: no ACK ({args.ack!r}) within {args.ack_timeout:.0f}s. "
                        "Check that the board is running the dumb-bootloader firmware "
                        "and that the port and baud are correct.")
                print(f"ACK ({args.ack!r}) received - board is rebooting into the ROM bootloader.")
    except serial.SerialException as e:
        sys.exit(f"Failed to open/write {port}: {e}")

    print("Flash example (standalone image at 0x08000000):")
    print(f"  STM32_Programmer_CLI -c port={port} br={args.baud} -w firmware.bin 0x08000000 -s")


if __name__ == "__main__":
    main()
