#!/usr/bin/env python3
"""
cantui.py — Combined RX monitor + TX preset panel in a single split-screen TUI.

  ┌──────────────────────────────┬──────────────────────────┐
  │  RX MONITOR (left)           │  TX PANEL (right)         │
  │  one row per message,        │  preset messages you can  │
  │  values + age update live;   │  adjust and broadcast to  │
  │  Enter a row for full detail │  simulate other nodes     │
  └──────────────────────────────┴──────────────────────────┘

Tab switches focus between the panes. Only the focused pane's selection is
highlighted, and the help bar shows that pane's keys.

RX keys (focused):
  up/down / j/k   select message       Enter / d   expand full message (scroll)
  s   cycle sort      r   record         c   clear      p / space   pause
  /   filter          [ / ]  shrink / grow the RX pane
  (in expanded view: up/down + PgUp/PgDn scroll signals; Enter/Esc/d collapses)

TX keys (focused):
  up/down / j/k   select control        left/right / h/l / -/+   change value
  space   send once    e   toggle periodic broadcast
  a / A   all on / off  [ / ]  faster / slower broadcast rate

Global:  Tab switch focus    q quit

Run:
  python3 cantui.py -c can0 CarCAN.dbc MotorCAN.dbc SteeringCAN.dbc BPSCAN.dbc
  python3 cantui.py -c can0 '../Embedded-Sharepoint/can/dbc/HighNoon/*.dbc' --all-on
  python3 cantui.py -c can0 CarCAN.dbc --dry-run     # no TX bus; preview only
  candump -L can0 | python3 cantui.py CarCAN.dbc     # piped RX input

NOTE: do not rename this file to cantools.py — that shadows the cantools library.
"""

import argparse
import curses
import glob
import logging
import os
import re
import socket
import struct
import subprocess
import sys
import threading
import time
from datetime import datetime

try:
    import cantools
except ImportError:
    sys.exit("Missing dependency 'cantools'.  Install with: pip install cantools")


# ===========================================================================
# SHARED: DBC loading + frame parsing + decoding
# ===========================================================================

def expand_dbc_paths(patterns):
    paths, seen = [], set()
    for pat in patterns:
        if os.path.isdir(pat):
            matched = sorted(glob.glob(os.path.join(pat, "**", "*.dbc"), recursive=True))
        elif any(ch in pat for ch in "*?["):
            matched = sorted(glob.glob(pat, recursive=True))
        else:
            matched = [pat]
        if not matched:
            print(f"Warning: no DBC files matched '{pat}'", file=sys.stderr)
        for p in matched:
            rp = os.path.realpath(p)
            if rp not in seen:
                seen.add(rp)
                paths.append(p)
    return paths


def _demojibake(text):
    """Repair double-encoded UTF-8 (e.g. a degree sign stored as 'Ã‚Â°')."""
    if not text:
        return text
    fixed = text
    for _ in range(4):
        if not any(ord(ch) > 0x7F for ch in fixed):
            break
        changed = False
        for enc in ("latin-1", "cp1252"):
            try:
                candidate = fixed.encode(enc).decode("utf-8")
            except (UnicodeEncodeError, UnicodeDecodeError):
                continue
            if candidate != fixed:
                fixed, changed = candidate, True
                break
        if not changed:
            break
    return fixed


def load_database(patterns, quiet=False):
    logging.getLogger("cantools").setLevel(logging.ERROR)
    paths = expand_dbc_paths(patterns)
    if not paths:
        sys.exit("No DBC files found for the given path(s)/pattern(s).")
    db = cantools.database.Database()
    owner = {}
    loaded = 0
    for p in paths:
        try:
            single = cantools.database.load_file(p)
        except Exception as exc:
            print(f"Warning: could not load '{p}': {exc}", file=sys.stderr)
            continue
        base = os.path.basename(p)
        for m in single.messages:
            key = (m.frame_id, m.is_extended_frame)
            if key in owner and not quiet:
                print(f"Note: 0x{m.frame_id:X} '{m.name}' in {base} overrides {owner[key]}",
                      file=sys.stderr)
            owner[key] = f"'{m.name}' [{base}]"
        try:
            db.add_dbc_file(p)
            loaded += 1
            if not quiet:
                print(f"  loaded {p} ({len(single.messages)} msgs)", file=sys.stderr)
        except Exception as exc:
            print(f"Warning: could not merge '{p}': {exc}", file=sys.stderr)
    if not db.messages:
        sys.exit("No messages loaded from the given DBC file(s).")
    if not quiet:
        print(f"Total: {len(db.messages)} unique messages from {loaded} DBC file(s).",
              file=sys.stderr)
    return db


LOG_RE = re.compile(
    r"^\((?P<ts>\d+\.\d+)\)\s+(?P<ch>\S+)\s+"
    r"(?P<id>[0-9A-Fa-f]+)(?P<sep>#{1,2})(?P<rest>[0-9A-Fa-fR]*)\s*$"
)
DEFAULT_RE = re.compile(
    r"(?P<ch>\S+)\s+(?P<id>[0-9A-Fa-f]+)\s+\[(?P<dlc>\d+)\]\s+(?P<data>(?:[0-9A-Fa-f]{2}\s*)*)"
)
TS_RE = re.compile(r"^\s*\((\d+\.\d+)\)")


class Frame:
    __slots__ = ("channel", "frame_id", "is_extended", "data", "remote")

    def __init__(self, channel, frame_id, is_extended, data, remote=False):
        self.channel = channel
        self.frame_id = frame_id
        self.is_extended = is_extended
        self.data = data
        self.remote = remote


def parse_line(line):
    line = line.rstrip("\n")
    if not line.strip():
        return None
    if "#" in line:
        m = LOG_RE.match(line.strip())
        if not m:
            return None
        id_str = m.group("id")
        is_ext = len(id_str) > 3
        frame_id = int(id_str, 16)
        rest = m.group("rest")
        if rest.startswith("R"):
            return Frame(m.group("ch"), frame_id, is_ext, b"", remote=True)
        if m.group("sep") == "##":
            rest = rest[1:]
        try:
            data = bytes.fromhex(rest)
        except ValueError:
            return None
        return Frame(m.group("ch"), frame_id, is_ext, data)
    m = DEFAULT_RE.search(line)
    if not m:
        return None
    id_str = m.group("id")
    is_ext = len(id_str) > 3
    frame_id = int(id_str, 16)
    try:
        data = bytes.fromhex(m.group("data").replace(" ", ""))
    except ValueError:
        return None
    return Frame(m.group("ch"), frame_id, is_ext, data)


def decode_frame(db, frame):
    try:
        msg = db.get_message_by_frame_id(frame.frame_id)
    except KeyError:
        return None, None, None
    if frame.remote:
        return msg, {}, None
    try:
        decoded = msg.decode(frame.data, decode_choices=True, allow_truncated=True)
    except TypeError:
        try:
            decoded = msg.decode(frame.data, decode_choices=True)
        except Exception as exc:
            return msg, None, str(exc)
    except Exception as exc:
        return msg, None, str(exc)
    return msg, decoded, None


def format_value(value):
    if isinstance(value, float):
        if value == int(value):
            return str(int(value))
        return f"{value:.4g}"
    return _demojibake(str(value))


# ===========================================================================
# SHARED: SocketCAN (TX)
# ===========================================================================

CAN_EFF_FLAG = 0x80000000
CAN_RTR_FLAG = 0x40000000
CAN_SFF_MASK = 0x000007FF
CAN_RAW_FD_FRAMES = 5
CLASSIC_FMT = "=IB3x8s"
FD_FMT = "=IBB2x64s"
FD_LENGTHS = [0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64]


class CanSocket:
    def __init__(self, channel, fd=False):
        self.fd = fd
        self.channel = channel
        try:
            self.sock = socket.socket(socket.AF_CAN, socket.SOCK_RAW, socket.CAN_RAW)
            if fd:
                self.sock.setsockopt(socket.SOL_CAN_RAW, CAN_RAW_FD_FRAMES, 1)
            self.sock.bind((channel,))
        except OSError as exc:
            raise SystemExit(
                f"Could not open '{channel}': {exc}\n"
                f"Is the interface up?  Try: sudo ip link set {channel} up type can bitrate 250000"
            )
        except AttributeError:
            raise SystemExit("SocketCAN is unavailable (Linux-only).")

    def send(self, can_id, data, extended=False, remote=False):
        flags = 0
        if extended:
            flags |= CAN_EFF_FLAG
        if remote:
            flags |= CAN_RTR_FLAG
        id_field = (can_id & (0x1FFFFFFF if extended else CAN_SFF_MASK)) | flags
        if self.fd:
            length = len(data)
            if length not in FD_LENGTHS:
                length = next(n for n in FD_LENGTHS if n >= length)
            frame = struct.pack(FD_FMT, id_field, length, 0, data.ljust(length, b"\x00")[:64])
        else:
            if len(data) > 8:
                raise SystemExit("Classic CAN payload exceeds 8 bytes — use --fd.")
            frame = struct.pack(CLASSIC_FMT, id_field, len(data), data.ljust(8, b"\x00"))
        self.sock.send(frame)

    def close(self):
        self.sock.close()


# ===========================================================================
# RX: State + Recorder + Reader
# ===========================================================================

SORT_KEYS = ["id", "name", "count", "age"]


class RxEntry:
    __slots__ = ("frame_id", "is_extended", "channel", "name", "msg",
                 "data", "decoded", "error", "count", "last_mono", "last_wall", "period_ms")

    def __init__(self, frame_id, is_extended):
        self.frame_id = frame_id
        self.is_extended = is_extended
        self.channel = ""
        self.name = None
        self.msg = None
        self.data = b""
        self.decoded = None
        self.error = None
        self.count = 0
        self.last_mono = None
        self.last_wall = 0.0
        self.period_ms = None

    def id_hex(self):
        return f"{self.frame_id:08X}" if self.is_extended else f"{self.frame_id:03X}"


class RxState:
    def __init__(self, db):
        self.db = db
        self.entries = {}
        self.total = 0
        self.lock = threading.Lock()

    def update(self, frame, ts):
        key = (frame.frame_id, frame.is_extended)
        now = time.monotonic()
        with self.lock:
            e = self.entries.get(key)
            if e is None:
                e = RxEntry(frame.frame_id, frame.is_extended)
                self.entries[key] = e
            e.channel = frame.channel
            if e.last_mono is not None:
                e.period_ms = (now - e.last_mono) * 1000.0
            e.last_mono = now
            e.last_wall = ts
            e.data = frame.data
            e.count += 1
            self.total += 1
            msg, decoded, error = decode_frame(self.db, frame)
            e.msg = msg
            e.name = msg.name if msg else None
            e.decoded = decoded
            e.error = error

    def get(self, key):
        with self.lock:
            return self.entries.get(key)

    def clear(self):
        with self.lock:
            self.entries.clear()
            self.total = 0

    def snapshot(self, sort_key, filt):
        now = time.monotonic()
        with self.lock:
            items = list(self.entries.values())
        if filt:
            f = filt.lower()
            items = [e for e in items if f in (e.name or e.id_hex()).lower()]
        if sort_key == "name":
            items.sort(key=lambda e: (e.name or "~" + e.id_hex()).lower())
        elif sort_key == "count":
            items.sort(key=lambda e: -e.count)
        elif sort_key == "age":
            items.sort(key=lambda e: (now - e.last_mono) if e.last_mono else 1e9)
        else:
            items.sort(key=lambda e: (e.frame_id, e.is_extended))
        return items, now


class Recorder:
    def __init__(self):
        self.fh = None
        self.path = None
        self.count = 0
        self.lock = threading.Lock()

    @property
    def active(self):
        return self.fh is not None

    def start(self, path=None):
        with self.lock:
            if self.fh:
                return
            if not path:
                path = datetime.now().strftime("candump_%Y%m%d_%H%M%S.log")
            self.fh = open(path, "w", buffering=1)
            self.path = path
            self.count = 0

    def stop(self):
        with self.lock:
            if self.fh:
                self.fh.close()
                self.fh = None

    def toggle(self, default_path=None):
        if self.active:
            self.stop()
        else:
            self.start(default_path)

    def write(self, channel, frame_id, is_extended, data, ts):
        with self.lock:
            if not self.fh:
                return
            ids = f"{frame_id:08X}" if is_extended else f"{frame_id:03X}"
            self.fh.write(f"({ts:.6f}) {channel or 'can0'} {ids}#{data.hex().upper()}\n")
            self.count += 1


def rx_reader_loop(source, rx_state, recorder, stop_event):
    for line in source:
        if stop_event.is_set():
            break
        m = TS_RE.match(line)
        ts = float(m.group(1)) if m else time.time()
        frame = parse_line(line)
        if frame is None:
            continue
        rx_state.update(frame, ts)
        if recorder.active:
            recorder.write(frame.channel, frame.frame_id, frame.is_extended, frame.data, ts)


def make_rx_source(args):
    if args.channel:
        cmd = ["candump", "-L", args.channel]
        try:
            proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, text=True, bufsize=1)
        except FileNotFoundError:
            sys.exit("`candump` not found (install can-utils), or pipe candump -L into stdin.")
        return proc.stdout, proc
    if sys.stdin.isatty():
        return None, None
    pipe_fd = os.dup(0)
    src = os.fdopen(pipe_fd, "r", buffering=1)
    try:
        tty = open("/dev/tty")
        os.dup2(tty.fileno(), 0)
    except OSError:
        pass
    return src, None


# ===========================================================================
# TX: Controls + SimMessage + Scheduler
# ===========================================================================

def _bar(val, lo, hi, width=12):
    if hi <= lo:
        return ""
    frac = min(1.0, max(0.0, (val - lo) / (hi - lo)))
    n = int(round(frac * width))
    return "[" + "#" * n + "-" * (width - n) + "]"


class SimMessage:
    def __init__(self, msg, interval_ms, defaults=None, counter=None, enabled=False):
        self.msg = msg
        self.frame_id = msg.frame_id
        self.is_extended = msg.is_extended_frame
        self.interval_ms = interval_ms
        self.enabled = enabled
        self.counter_signal = counter
        self.counter_max = 2 ** msg.get_signal_by_name(counter).length if counter else None
        self.state = {s.name: 0 for s in msg.signals}
        if defaults:
            self.state.update(defaults)
        self.controls = []
        self.next_send = 0.0
        self.send_count = 0
        self.last_hex = ""
        self.error = None

    def encode(self):
        return self.msg.encode(self.state, padding=True, strict=False)

    def bump_counter(self):
        if self.counter_signal:
            self.state[self.counter_signal] = (self.state[self.counter_signal] + 1) % self.counter_max


class ChoiceControl:
    def __init__(self, sim, label, options, index=0):
        self.sim, self.label, self.options, self.index = sim, label, options, index
        self.apply()

    def apply(self):
        for sig, val in self.options[self.index][1].items():
            self.sim.state[sig] = val

    def left(self):
        self.index = (self.index - 1) % len(self.options); self.apply()

    def right(self):
        self.index = (self.index + 1) % len(self.options); self.apply()

    def display(self):
        return f"< {self.options[self.index][0]} >"


class ValueControl:
    def __init__(self, sim, label, signals, vmin, vmax, step, unit="", value=0, bar=False):
        self.sim, self.label, self.signals = sim, label, signals
        self.vmin, self.vmax, self.step = vmin, vmax, step
        self.unit, self.bar, self.value = unit, bar, value
        self.apply()

    def apply(self):
        for s in self.signals:
            self.sim.state[s] = self.value

    def left(self):
        self.value = max(self.vmin, round(self.value - self.step, 4)); self.apply()

    def right(self):
        self.value = min(self.vmax, round(self.value + self.step, 4)); self.apply()

    def display(self):
        v = f"{self.value:g}{(' ' + self.unit) if self.unit else ''}"
        return f"{v:>10} {_bar(self.value, self.vmin, self.vmax)}" if self.bar else v


class ToggleControl:
    def __init__(self, sim, label, signals, on_val=1, off_val=0, on="ON", off="OFF", value=False):
        self.sim, self.label, self.signals = sim, label, signals
        self.on_val, self.off_val, self.on, self.off, self.value = on_val, off_val, on, off, value
        self.apply()

    def apply(self):
        v = self.on_val if self.value else self.off_val
        for s in self.signals:
            self.sim.state[s] = v

    def left(self):
        self.value = not self.value; self.apply()

    right = left

    def display(self):
        return f"[{self.on if self.value else self.off}]"


def _has(sim, signame):
    return signame in {s.name for s in sim.msg.signals}


def _signal_exists(db, msgname, signame):
    try:
        m = db.get_message_by_name(msgname)
    except KeyError:
        return False
    return signame in {s.name for s in m.signals}


def build_simulation(db):
    sims = []

    def add(name, interval, defaults=None, counter=None):
        try:
            msg = db.get_message_by_name(name)
        except KeyError:
            print(f"Note: '{name}' not in loaded DBCs — skipping.", file=sys.stderr)
            return None
        sim = SimMessage(msg, interval, defaults=defaults, counter=counter)
        sims.append(sim)
        return sim

    def has(sim, *sigs):
        names = {s.name for s in sim.msg.signals}
        return all(s in names for s in sigs)

    # --- Ignition + gear ---
    di = add("Driver_Input_Status", 100)
    if di:
        if has(di, "Ignition_Off", "Ignition_Array", "Ignition_Motor"):
            di.controls.append(ChoiceControl(di, "Ignition", [
                ("Off",   {"Ignition_Off": 1, "Ignition_Array": 0, "Ignition_Motor": 0}),
                ("Array", {"Ignition_Off": 0, "Ignition_Array": 1, "Ignition_Motor": 0}),
                ("Motor", {"Ignition_Off": 0, "Ignition_Array": 0, "Ignition_Motor": 1}),
            ]))
        if has(di, "Gear_Neutral", "Gear_Forward", "Gear_Reverse"):
            di.controls.append(ChoiceControl(di, "Gear", [
                ("Neutral", {"Gear_Neutral": 1, "Gear_Forward": 0, "Gear_Reverse": 0}),
                ("Forward", {"Gear_Neutral": 0, "Gear_Forward": 1, "Gear_Reverse": 0}),
                ("Reverse", {"Gear_Neutral": 0, "Gear_Forward": 0, "Gear_Reverse": 1}),
            ]))

    # --- Pedals ---
    ped = add("Pedal_Status", 50,
              counter="FrameID_Pedals" if _signal_exists(db, "Pedal_Status", "FrameID_Pedals") else None)
    if ped:
        if has(ped, "AccelPedal_Main_Pos"):
            sigs = [s for s in ("AccelPedal_Main_Pos", "AccelPedal_Redundant_Pos") if _has(ped, s)]
            ped.controls.append(ValueControl(ped, "Accel", sigs, 0, 100, 5, "%", bar=True))
        if has(ped, "BrakePedal_Main_Pos"):
            sigs = [s for s in ("BrakePedal_Main_Pos", "BrakePedal_Redundant_Pos") if _has(ped, s)]
            ped.controls.append(ValueControl(ped, "Brake", sigs, 0, 100, 5, "%", bar=True))

    # --- Brake pressure sensors (two redundant sensors — set independently to
    #     test sensor-disagreement handling in your code) ---
    for mname in ("Brake_Pressure_1", "Brake_Pressure_2"):
        bp = add(mname, 50,
                 counter="FrameID_Pedals" if _signal_exists(db, mname, "FrameID_Pedals") else None)
        if bp and _has(bp, "Brake_Pressure"):
            sig = bp.msg.get_signal_by_name("Brake_Pressure")
            vmax = int(sig.maximum) if sig.maximum is not None else 3000
            bp.controls.append(ValueControl(bp, "Pressure", ["Brake_Pressure"], 0, vmax, 50, "PSI", bar=True))

    # --- Steering ---
    lws = add("LWS_Standard", 50,
              counter="LWS_MSG_CNT" if _signal_exists(db, "LWS_Standard", "LWS_MSG_CNT") else None)
    if lws and _has(lws, "LWS_Angle"):
        sig = lws.msg.get_signal_by_name("LWS_Angle")
        vmin = int(sig.minimum) if sig.minimum is not None else -780
        vmax = int(sig.maximum) if sig.maximum is not None else 780
        lws.controls.append(ValueControl(lws, "Steering Angle", ["LWS_Angle"], vmin, vmax, 10, "deg", bar=True))

    # --- BPS status (healthy defaults) ---
    contactors = ["HV_Plus_Contactor_State", "HV_Minus_Contactor_State",
                  "Array_Contactor_State", "Array_Precharge_Contactor_State"]
    bps = add("BPS_Status", 100, defaults={
        "BPS_Charge_OK": 1, "BPS_Regen_OK": 1,
        "Main_Battery_Voltage": 120, "Main_Battery_Avg_Temperature": 25,
    })
    if bps:
        if _has(bps, "BPS_Fault"):
            fault = bps.msg.get_signal_by_name("BPS_Fault")
            if fault.choices:
                opts = [(str(v), {"BPS_Fault": k}) for k, v in sorted(fault.choices.items())]
                bps.controls.append(ChoiceControl(bps, "BPS Fault", opts))
        present = [c for c in contactors if _has(bps, c)]
        if present:
            bps.controls.append(ToggleControl(bps, "Contactors", present, 1, 0, "Closed", "Open", value=True))

    # --- Motor controller status (fault injector) ---
    mcs = add("MC_Status", 200)
    if mcs:
        faults = [s.name for s in mcs.msg.signals
                  if s.name.startswith("MC_FAULT_") and "Reserved" not in s.name]
        if faults:
            opts = [("Healthy", {f: 0 for f in faults})]
            for f in faults:
                d = {x: 0 for x in faults}; d[f] = 1
                opts.append((f.replace("MC_FAULT_", ""), d))
            mcs.controls.append(ChoiceControl(mcs, "Inject Fault", opts))

    # --- Velocity ---
    vel = add("MC_VelocityMeasurement", 200)
    if vel:
        if _has(vel, "MC_VehicleVelocity"):
            vel.controls.append(ValueControl(vel, "Vehicle Velocity", ["MC_VehicleVelocity"], 0, 40, 1, "m/s", bar=True))
        if _has(vel, "MC_MotorVelocity"):
            vel.controls.append(ValueControl(vel, "Motor Velocity", ["MC_MotorVelocity"], 0, 5000, 100, "rpm", bar=True))

    return [s for s in sims if s.controls]


def tx_send_once(sim, bus):
    try:
        data = sim.encode()
        if bus is not None:
            bus.send(sim.frame_id, data, extended=sim.is_extended)
        sim.send_count += 1
        sim.last_hex = data.hex().upper()
        sim.error = None
        sim.bump_counter()
    except Exception as exc:
        sim.error = str(exc)
        sim.enabled = False


def tx_scheduler(sims, bus, lock, stop):
    while not stop.is_set():
        now = time.monotonic()
        with lock:
            for sim in sims:
                if sim.enabled and now >= sim.next_send:
                    tx_send_once(sim, bus)
                    sim.next_send = now + sim.interval_ms / 1000.0
        time.sleep(0.002)


# ===========================================================================
# Curses helpers + rendering
# ===========================================================================

def safe_addstr(win, y, x, text, attr=0):
    h, w = win.getmaxyx()
    if y < 0 or y >= h or x >= w:
        return
    try:
        win.addstr(y, x, text[: max(0, w - 1 - x)], attr)
    except curses.error:
        pass


def text_prompt(stdscr, label):
    h, w = stdscr.getmaxyx()
    curses.curs_set(1)
    stdscr.nodelay(False)
    buf = ""
    while True:
        safe_addstr(stdscr, h - 1, 0, (label + buf).ljust(w - 1), curses.A_REVERSE)
        stdscr.move(h - 1, min(len(label) + len(buf), w - 1))
        ch = stdscr.getch()
        if ch in (10, 13):
            result = buf; break
        if ch == 27:
            result = None; break
        if ch in (curses.KEY_BACKSPACE, 127, 8):
            buf = buf[:-1]
        elif 32 <= ch < 127:
            buf += chr(ch)
    curses.curs_set(0)
    stdscr.nodelay(True)
    return result


def fmt_age(age):
    if age < 10:
        return f"{age:4.2f}"
    if age < 100:
        return f"{age:4.1f}"
    return " 99+"


def fmt_period(ms):
    if ms is None:
        return "     -"
    if ms < 10000:
        return f"{ms:6.1f}"
    return " >10s "


def inline_signals(entry):
    if entry.error:
        return "<decode error>"
    if entry.decoded is None:
        return "(no DBC match)" if entry.name is None else ""
    return " ".join(f"{k}={format_value(v)}" for k, v in entry.decoded.items())


def rx_row_text(entry, now, sig_width):
    age = (now - entry.last_mono) if entry.last_mono else 0.0
    name = (entry.name or "—")[:20]
    sig = inline_signals(entry)[:sig_width]
    return (f"{entry.id_hex():<8} {name:<20} {entry.count:>6} "
            f"{fmt_period(entry.period_ms):>7} {fmt_age(age):>5} {sig}")


def message_detail_lines(entry):
    """Full, one-per-line view of a message's signals (used in the expand view)."""
    lines = []
    if entry.msg and entry.decoded is not None:
        for s in entry.msg.signals:
            if s.name not in entry.decoded:
                continue
            val = format_value(entry.decoded[s.name])
            unit = f" {_demojibake(s.unit)}" if s.unit else ""
            lines.append(f"{s.name:<30} {val}{unit}")
    elif entry.error:
        lines.append(f"decode error: {entry.error}")
    else:
        lines.append(f"raw: {entry.data.hex(' ').upper()}")
    return lines


# ===========================================================================
# Combined TUI
# ===========================================================================

def run_ui(stdscr, rx_state, recorder, tx_sims, tx_lock, bus, args):
    curses.curs_set(0)
    stdscr.nodelay(True)
    stdscr.timeout(80)
    stdscr.keypad(True)
    if curses.has_colors():
        curses.start_color()
        curses.use_default_colors()
        curses.init_pair(1, curses.COLOR_CYAN, -1)
        curses.init_pair(2, curses.COLOR_GREEN, -1)
        curses.init_pair(3, curses.COLOR_YELLOW, -1)
        curses.init_pair(4, curses.COLOR_RED, -1)
        curses.init_pair(5, curses.COLOR_MAGENTA, -1)
    C1, C2, C3, C4, C5 = (curses.color_pair(i) for i in range(1, 6))

    focus = "rx"
    rx_div = 0.55

    # RX state
    rx_sort_idx = SORT_KEYS.index(args.sort) if args.sort in SORT_KEYS else 0
    rx_selected = rx_top = 0
    rx_paused = False
    rx_filt = ""
    rx_expanded = False
    rx_exp_key = None        # pinned (frame_id, is_extended) while expanded
    rx_exp_scroll = 0
    prev_items = []
    last_total = 0
    last_fps_t = time.monotonic()
    fps = 0.0

    # TX state — flatten controls once
    tx_controls, tx_ctrl_sim = [], []
    for sim in tx_sims:
        for ctrl in sim.controls:
            tx_controls.append(ctrl)
            tx_ctrl_sim.append(sim)
    tx_selected = tx_top = 0

    while True:
        h, w = stdscr.getmaxyx()
        rx_w = max(30, int(w * rx_div))
        tx_x = rx_w + 1
        tx_w = max(10, w - tx_x)
        rx_focus = (focus == "rx")

        # ---- RX data ----
        if rx_paused and prev_items:
            rx_items, now = prev_items, time.monotonic()
        else:
            rx_items, now = rx_state.snapshot(SORT_KEYS[rx_sort_idx], rx_filt)
            prev_items = rx_items
        if rx_selected >= len(rx_items):
            rx_selected = max(0, len(rx_items) - 1)
        sel_entry = rx_items[rx_selected] if rx_items else None
        if now - last_fps_t >= 0.5:
            fps = (rx_state.total - last_total) / max(0.001, now - last_fps_t)
            last_total = rx_state.total
            last_fps_t = now

        # ---- TX line buffer (record line index of each control for scrolling) ----
        tx_lines = []
        tx_ctrl_line = {}
        ci = 0
        tx_focus = not rx_focus
        with tx_lock:
            for sim in tx_sims:
                mark = "●" if sim.enabled else "○"
                hcol = C2 if sim.enabled else curses.A_DIM
                tx_lines.append((f"{mark} {sim.msg.name:<22} 0x{sim.frame_id:<4X} "
                                 f"{sim.interval_ms:>4}ms {sim.send_count:<5} {sim.last_hex}",
                                 hcol | curses.A_BOLD))
                if sim.error:
                    tx_lines.append(("  ! " + sim.error, C4))
                for ctrl in sim.controls:
                    sel = (ci == tx_selected)
                    tx_ctrl_line[ci] = len(tx_lines)
                    if sel:
                        attr = curses.A_REVERSE if tx_focus else curses.A_BOLD | C3
                    else:
                        attr = C3
                    tx_lines.append((f"  {ctrl.label:<18} {ctrl.display()}", attr))
                    ci += 1
                tx_lines.append(("", 0))
        tx_body_h = h - 2
        tx_sel_line = tx_ctrl_line.get(tx_selected, 0)
        if tx_sel_line < tx_top:
            tx_top = tx_sel_line
        elif tx_sel_line >= tx_top + tx_body_h:
            tx_top = tx_sel_line - tx_body_h + 1

        # ===== DRAW =====
        stdscr.erase()

        # divider column
        for row in range(1, h - 1):
            safe_addstr(stdscr, row, rx_w, "│", C1)

        # title bar (row 0)
        rx_attr = curses.A_BOLD | C1 if rx_focus else curses.A_DIM
        tx_attr = curses.A_BOLD | C5 if tx_focus else curses.A_DIM
        safe_addstr(stdscr, 0, 0, " RX MONITOR ", rx_attr)
        rec = f" REC[{recorder.count}] " if recorder.active else ""
        rx_status = (f"src:{args.channel or 'stdin'} msgs:{len(rx_items)} "
                     f"frames:{rx_state.total} {fps:4.0f}/s srt:{SORT_KEYS[rx_sort_idx]}"
                     f"{' PAUSED' if rx_paused else ''}{(' flt:' + rx_filt) if rx_filt else ''}")
        safe_addstr(stdscr, 0, 12, rx_status[:max(0, rx_w - 13)], C3)
        if recorder.active:
            safe_addstr(stdscr, 0, rx_w - len(rec) - 1, rec, curses.A_BOLD | C4)
        safe_addstr(stdscr, 0, tx_x, " TX PANEL ", tx_attr)
        safe_addstr(stdscr, 0, tx_x + 10,
                    f"bus:{args.channel or '—'}{' (dry-run)' if args.dry_run else ''}"[:max(0, tx_w - 11)], C3)

        # ---- RX body: expanded full-message view OR list+preview ----
        if rx_expanded and rx_exp_key is not None:
            entry = rx_state.get(rx_exp_key)
            if entry is None:
                rx_expanded = False
            else:
                dlines = message_detail_lines(entry)
                hdr = (f" {entry.id_hex()} {entry.name or '(unknown)'} "
                       f"— {len(dlines)} signals  [Enter/Esc: back] ")
                safe_addstr(stdscr, 1, 0, hdr[:rx_w - 1], curses.A_BOLD | C1)
                avail = h - 3
                rx_exp_scroll = max(0, min(rx_exp_scroll, max(0, len(dlines) - avail)))
                for i in range(avail):
                    li = rx_exp_scroll + i
                    if li >= len(dlines):
                        break
                    safe_addstr(stdscr, 2 + i, 1, dlines[li][:rx_w - 2], C2)
                if len(dlines) > avail:
                    safe_addstr(stdscr, h - 2, 0,
                                f"  [{rx_exp_scroll + 1}-{min(rx_exp_scroll + avail, len(dlines))}"
                                f"/{len(dlines)}] ↑↓ PgUp/PgDn".ljust(rx_w - 1), curses.A_DIM | C1)
        else:
            # column header
            sig_width = max(4, rx_w - 1 - 50)
            safe_addstr(stdscr, 1, 0,
                        f"{'ID':<8} {'MESSAGE':<20} {'CNT':>6} {'ms':>7} {'age':>5} SIGNALS"[:rx_w - 1],
                        curses.A_BOLD | C1)

            detail_h = 0
            if sel_entry is not None:
                nsig = len(sel_entry.msg.signals) if sel_entry.msg else 1
                detail_h = min(max(nsig + 2, 3), max(3, h // 3))
            list_top = 2
            list_h = max(1, h - list_top - detail_h - 1)
            if rx_selected < rx_top:
                rx_top = rx_selected
            elif rx_selected >= rx_top + list_h:
                rx_top = rx_selected - list_h + 1

            for i, e in enumerate(rx_items[rx_top:rx_top + list_h]):
                idx = rx_top + i
                age = (now - e.last_mono) if e.last_mono else 1e9
                if idx == rx_selected:
                    attr = curses.A_REVERSE if rx_focus else curses.A_BOLD
                elif age < 0.25:
                    attr = C2
                elif e.error:
                    attr = C4
                else:
                    attr = 0
                safe_addstr(stdscr, list_top + i, 0,
                            rx_row_text(e, now, sig_width).ljust(rx_w - 1)[:rx_w - 1], attr)

            # small detail preview (with a hint if there's more to see)
            if sel_entry is not None and detail_h > 0:
                dy = list_top + list_h
                safe_addstr(stdscr, dy, 0, "─" * (rx_w - 1), C1)
                safe_addstr(stdscr, dy, 2, f" {sel_entry.id_hex()} {sel_entry.name or '(unknown)'} "[:rx_w - 5],
                            curses.A_BOLD | C3)
                dlines = message_detail_lines(sel_entry)
                shown = detail_h - 1
                for i, text in enumerate(dlines[:shown]):
                    if dy + 1 + i >= h - 1:
                        break
                    safe_addstr(stdscr, dy + 1 + i, 2, text[:rx_w - 3], C2)
                if len(dlines) > shown:
                    safe_addstr(stdscr, dy + shown, 2,
                                f"... +{len(dlines) - shown + 1} more — press Enter for full view"[:rx_w - 3],
                                curses.A_DIM | C3)

        # ---- TX body ----
        for i, (text, attr) in enumerate(tx_lines[tx_top:tx_top + tx_body_h]):
            safe_addstr(stdscr, 1 + i, tx_x, text[:tx_w - 1], attr)

        # ---- help bar ----
        if rx_focus and rx_expanded:
            help_txt = " ↑↓ PgUp/PgDn scroll   Enter/Esc/d back   Tab:TX   q quit "
        elif rx_focus:
            help_txt = " Tab:TX  ↑↓ sel  Enter expand  s sort  r rec  c clr  p pause  / filt  [ ] size  q quit "
        else:
            help_txt = " Tab:RX  ↑↓ sel  ←→ value  e bcast  a/A all  space send1  [ ] rate  q quit "
        safe_addstr(stdscr, h - 1, 0, help_txt.ljust(w - 1)[:w - 1], curses.A_REVERSE)
        stdscr.refresh()

        # ===== INPUT =====
        ch = stdscr.getch()
        if ch == -1:
            continue
        if ch in (ord("q"), ord("Q")):
            break
        if ch == ord("\t"):
            focus = "tx" if focus == "rx" else "rx"
            continue

        if focus == "rx":
            if rx_expanded:
                avail = h - 3
                if ch in (curses.KEY_DOWN, ord("j")):
                    rx_exp_scroll += 1
                elif ch in (curses.KEY_UP, ord("k")):
                    rx_exp_scroll = max(0, rx_exp_scroll - 1)
                elif ch == curses.KEY_NPAGE:
                    rx_exp_scroll += avail
                elif ch == curses.KEY_PPAGE:
                    rx_exp_scroll = max(0, rx_exp_scroll - avail)
                elif ch in (10, 13, 27, ord("d"), ord("D")):
                    rx_expanded = False
            else:
                if ch in (curses.KEY_DOWN, ord("j")):
                    rx_selected = min(rx_selected + 1, max(0, len(rx_items) - 1))
                elif ch in (curses.KEY_UP, ord("k")):
                    rx_selected = max(rx_selected - 1, 0)
                elif ch == curses.KEY_NPAGE:
                    rx_selected = min(rx_selected + 10, max(0, len(rx_items) - 1))
                elif ch == curses.KEY_PPAGE:
                    rx_selected = max(rx_selected - 10, 0)
                elif ch in (10, 13, ord("d"), ord("D")) and sel_entry is not None:
                    rx_expanded = True
                    rx_exp_key = (sel_entry.frame_id, sel_entry.is_extended)
                    rx_exp_scroll = 0
                elif ch in (ord("s"), ord("S")):
                    rx_sort_idx = (rx_sort_idx + 1) % len(SORT_KEYS)
                elif ch in (ord("p"), ord(" ")):
                    rx_paused = not rx_paused
                elif ch in (ord("c"), ord("C")):
                    rx_state.clear(); rx_selected = rx_top = 0; prev_items = []
                elif ch in (ord("r"), ord("R")):
                    recorder.toggle(args.record)
                elif ch == ord("/"):
                    res = text_prompt(stdscr, "filter: ")
                    if res is not None:
                        rx_filt = res.strip(); rx_selected = rx_top = 0
                elif ch == ord("["):
                    rx_div = max(0.25, rx_div - 0.05)
                elif ch == ord("]"):
                    rx_div = min(0.80, rx_div + 0.05)
        else:  # TX focus
            if ch in (curses.KEY_DOWN, ord("j")):
                tx_selected = min(tx_selected + 1, max(0, len(tx_controls) - 1))
            elif ch in (curses.KEY_UP, ord("k")):
                tx_selected = max(tx_selected - 1, 0)
            elif ch in (curses.KEY_LEFT, ord("h"), ord("-"), ord("_")) and tx_controls:
                with tx_lock:
                    tx_controls[tx_selected].left()
            elif ch in (curses.KEY_RIGHT, ord("l"), ord("+"), ord("=")) and tx_controls:
                with tx_lock:
                    tx_controls[tx_selected].right()
            elif ch == ord(" ") and tx_controls:
                with tx_lock:
                    tx_send_once(tx_ctrl_sim[tx_selected], bus)
            elif ch in (ord("e"), ord("E")) and tx_controls:
                with tx_lock:
                    s = tx_ctrl_sim[tx_selected]; s.enabled = not s.enabled; s.next_send = 0.0
            elif ch == ord("a"):
                with tx_lock:
                    for s in tx_sims:
                        s.enabled = True; s.next_send = 0.0
            elif ch == ord("A"):
                with tx_lock:
                    for s in tx_sims:
                        s.enabled = False
            elif ch == ord("[") and tx_controls:
                with tx_lock:
                    sim = tx_ctrl_sim[tx_selected]
                    sim.interval_ms = min(5000, sim.interval_ms * 2)
            elif ch == ord("]") and tx_controls:
                with tx_lock:
                    sim = tx_ctrl_sim[tx_selected]
                    sim.interval_ms = max(5, sim.interval_ms // 2)


# ===========================================================================
# CLI / main
# ===========================================================================

def build_parser():
    p = argparse.ArgumentParser(
        description="Combined RX monitor + TX preset panel TUI for SocketCAN + DBC.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    p.add_argument("dbc", nargs="+", help="DBC files, directories, or globs")
    p.add_argument("-c", "--channel", help="SocketCAN interface (e.g. can0)")
    p.add_argument("--record", metavar="FILE", help="record RX frames (log format); toggle with 'r'")
    p.add_argument("--sort", choices=SORT_KEYS, default="id", help="initial RX sort order")
    p.add_argument("--all-on", action="store_true", help="start all TX messages broadcasting")
    p.add_argument("--dry-run", action="store_true", help="do not open TX socket (preview only)")
    return p


def main():
    args = build_parser().parse_args()
    if not sys.stdout.isatty():
        sys.exit("The TUI needs a terminal on stdout (don't pipe the output away).")

    db = load_database(args.dbc)

    # TX
    tx_sims = build_simulation(db)
    tx_lock = threading.Lock()
    bus = None
    if not args.dry_run and args.channel:
        try:
            bus = CanSocket(args.channel)
        except SystemExit as exc:
            print(f"TX bus unavailable: {exc}", file=sys.stderr)
            bus = None
    if args.all_on:
        for s in tx_sims:
            s.enabled = True
            s.next_send = 0.0
    tx_stop = threading.Event()
    threading.Thread(target=tx_scheduler, args=(tx_sims, bus, tx_lock, tx_stop), daemon=True).start()

    # RX
    rx_state = RxState(db)
    recorder = Recorder()
    if args.record:
        recorder.start(args.record)
    rx_source, rx_proc = make_rx_source(args)
    rx_stop = threading.Event()
    if rx_source is not None:
        threading.Thread(target=rx_reader_loop,
                         args=(rx_source, rx_state, recorder, rx_stop), daemon=True).start()

    try:
        curses.wrapper(run_ui, rx_state, recorder, tx_sims, tx_lock, bus, args)
    finally:
        rx_stop.set()
        tx_stop.set()
        if rx_proc:
            rx_proc.terminate()
            rx_proc.wait()
        recorder.stop()
        if bus:
            bus.close()
        if recorder.path:
            print(f"Recorded {recorder.count} frames → {recorder.path}")


if __name__ == "__main__":
    main()