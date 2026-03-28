#!/usr/bin/env python3
import argparse
from pathlib import Path

OUT_DIR = Path(__file__).resolve().parent.parent / "Tasks/Inc"

BITS = {
    "NEUTRAL_BIT":               1 << 0,
    "FORWARD_BIT":               1 << 1,
    "REVERSE_BIT":               1 << 2,
    "CRUISE_CONTROL_BUTTON_BIT": 1 << 3,
    "REGEN_BUTTON_BIT":          1 << 4,
    "READY_TO_REGEN_BIT":        1 << 5,
    "REGEN_ENABLED_BIT":         1 << 6,
    "BRAKE_BIT":                 1 << 7,
    "PRECHARGE_BIT":             1 << 8,
}

STATES = [
    "STATE_INIT",
    "FORWARD_DRIVE",
    "NEUTRAL_DRIVE",
    "REVERSE_DRIVE",
    "REGEN",
    "CRUISE_CONTROL",
    "DISABLED",
    "CAR_NOT_READY",
]

STATE_IDX = {s: i for i, s in enumerate(STATES)}
NSL = 1 << len(BITS)

# preresolve some constants so its easier to write
NEU = BITS["NEUTRAL_BIT"]
FWD = BITS["FORWARD_BIT"]
REV = BITS["REVERSE_BIT"]
CC  = BITS["CRUISE_CONTROL_BUTTON_BIT"]
RB  = BITS["REGEN_BUTTON_BIT"]
RTR = BITS["READY_TO_REGEN_BIT"]
REN = BITS["REGEN_ENABLED_BIT"]
BRK = BITS["BRAKE_BIT"]
PC  = BITS["PRECHARGE_BIT"]

INIT      = STATE_IDX["STATE_INIT"]
FWD_DRIVE = STATE_IDX["FORWARD_DRIVE"]
NEUTRAL   = STATE_IDX["NEUTRAL_DRIVE"]
REV_DRIVE = STATE_IDX["REVERSE_DRIVE"]
REGEN     = STATE_IDX["REGEN"]
CRUISE    = STATE_IDX["CRUISE_CONTROL"]
DISABLED  = STATE_IDX["DISABLED"]
NOT_READY = STATE_IDX["CAR_NOT_READY"]


def transition_full(cur, bits):
    if cur == INIT:
        return NOT_READY

    if cur == NOT_READY:
        return NEUTRAL if (bits & PC) else NOT_READY

    if cur == DISABLED:
        return DISABLED

    if cur == NEUTRAL:
        if (bits & FWD) and not (bits & REV) and not (bits & BRK):
            return FWD_DRIVE
        if (bits & REV) and not (bits & FWD) and not (bits & BRK):
            return REV_DRIVE
        return NEUTRAL

    if cur == FWD_DRIVE:
        if (bits & REV) or (bits & NEU) or (bits & BRK):
            return NEUTRAL
        if (bits & REN) and (bits & RTR) and (bits & RB):
            return REGEN
        if (bits & CC) and (bits & REN):
            return CRUISE
        return FWD_DRIVE

    if cur == REV_DRIVE:
        if (bits & REV) and not (bits & FWD) and not (bits & BRK):
            return REV_DRIVE
        return NEUTRAL

    if cur == REGEN:
        if (bits & RB) and (bits & RTR) and (bits & REN) and (bits & FWD):
            return REGEN
        return FWD_DRIVE

    if cur == CRUISE:
        return CRUISE if (bits & CC) else FWD_DRIVE

    return NEUTRAL


def transition_dnr(cur, bits):
    if cur == INIT:
        return NOT_READY
 
    if cur == NOT_READY:
        return NEUTRAL if (bits & PC) else NOT_READY

    if cur == DISABLED:
        return DISABLED

    if cur == NEUTRAL:
        if (bits & FWD) and not (bits & REV) and not (bits & BRK):
            return FWD_DRIVE
        if (bits & REV) and not (bits & FWD) and not (bits & BRK):
            return REV_DRIVE
        return NEUTRAL

    if cur in (FWD_DRIVE, REGEN, CRUISE):
        if (bits & REV) or (bits & NEU) or (bits & BRK):
            return NEUTRAL
        return FWD_DRIVE

    if cur == REV_DRIVE:
        if (bits & REV) and not (bits & FWD) and not (bits & BRK):
            return REV_DRIVE
        return NEUTRAL

    return NEUTRAL


def write_table(path, fn, label):
    rows = []
    states = STATES
    nsl = NSL

    for i, name in enumerate(states):
        ns = [states[fn(i, j)] for j in range(nsl)]
        chunks = [
            ", ".join(ns[x:x+8])
            for x in range(0, nsl, 8)
        ]

        body = ",\n".join(f"            {c}" for c in chunks)

        rows.append(
            f"    [{name}] = {{ {name}, NULL, {{\n{body}\n    }}}},\n"
        )

    path.write_text(
        f"/**\n"
        f" * @file {path.name}\n"
        f" * @brief Auto-generated FSM table ({label}); do not edit\n"
        f" *        Regenerate: python3 generate_fsm.py --{label.lower()}\n"
        f" */\n\n"
        f"#pragma once\n"
        f'#include "FSM.h"\n\n'
        f"extern MocoState_t FSM[NUM_STATES];\n\n"
        f"#ifdef DEFINE_FSM_TABLE\n"
        f"MocoState_t FSM[NUM_STATES] = {{\n\n"
        + "".join(rows)
        + "};\n"
        f"#endif\n"
    )

    print(f"[{label:4}] {path.name} ({len(states)} states x {nsl} inputs)")



def main():
    parser = argparse.ArgumentParser()
    g = parser.add_mutually_exclusive_group(required=True)
    g.add_argument("--full", action="store_true")
    g.add_argument("--dnr",  action="store_true")
    g.add_argument("--all",  action="store_true")
    args = parser.parse_args()

    OUT_DIR.mkdir(parents=True, exist_ok=True)

    if args.full or args.all:
        write_table(OUT_DIR / "fsm_table.h",     transition_full, "FULL")
    if args.dnr  or args.all:
        write_table(OUT_DIR / "fsm_table_dnr.h", transition_dnr,  "DNR")


if __name__ == "__main__":
    main()