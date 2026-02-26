#!/usr/bin/env python3
"""
generate_fsm.py
Parses fsm.h for BitfieldBitIndex and FSMStates enums,
computes the full next-state table, and writes fsm_table.h.

Usage: python3 generate_fsm.py
Assumed layout: script lives in Firmware/scripts/
Output:         Firmware/apps/inc/fsm_table.h
"""

import re
from pathlib import Path

# === Paths ===
SCRIPT_DIR   = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parent
FSM_H        = PROJECT_ROOT / "core" / "Inc" / "fsm.h"
FSM_TABLE_H  = PROJECT_ROOT / "core" / "Inc" / "fsm_table.h"


# === Enum parsers ===

def _strip_line(line: str) -> str:
    line = re.sub(r"//.*", "", line)
    line = re.sub(r"/\*.*?\*/", "", line)
    return line.strip().rstrip(",").strip()


def parse_sequential_enum(text: str, typedef_name: str, sentinel: str):
    """
    Parse a typedef enum <typedef_name> { ... } ... ;
    Returns (entries, sentinel_value) where entries = [(name, index), ...]
    in declaration order (implicit sequential values only).
    """
    m = re.search(
        rf"typedef\s+enum\s+{typedef_name}\s*\{{(?P<body>.*?)\}}\s*\w+\s*;",
        text, re.DOTALL
    )
    if not m:
        raise RuntimeError(f"Could not find 'typedef enum {typedef_name}' in fsm.h")

    entries = []
    value = 0
    sentinel_value = None

    for raw in m.group("body").splitlines():
        line = _strip_line(raw)
        if not line:
            continue

        parts = [p.strip() for p in line.split("=", 1)]
        name = parts[0]

        if len(parts) > 1:
            try:
                value = int(parts[1], 0)
            except ValueError:
                pass  # expression-based value; keep incrementing

        if name == sentinel:
            sentinel_value = value
            break

        entries.append((name, value))
        value += 1

    if sentinel_value is None:
        raise RuntimeError(f"Sentinel '{sentinel}' not found in enum {typedef_name}")

    return entries, sentinel_value


# === FSM transition logic ===

def compute_next_state(cur, bits, masks, state_idx):
    """
    Returns the next FSMStates index given:
      cur   - current state index
      bits  - input bitfield integer (0..NEXT_STATES_LENGTH-1)
      masks - dict: logical_name -> mask value
      state_idx - dict: state_name -> state index
    """
    s = state_idx

    INIT    = s["STATE_INIT"]
    FWD_ST  = s["FORWARD_DRIVE"]
    NEU_ST  = s["NEUTRAL"]
    REV_ST  = s["REVERSE_DRIVE"]
    REGEN_ST= s["REGEN"]
    CC_ST   = s["CRUISE_CONTROL"]
    DIS_ST  = s["DISABLED"]
    NR_ST   = s["CAR_NOT_READY"]

    FWD  = masks["FORWARD_BIT"]
    REV  = masks["REVERSE_BIT"]
    NEU  = masks["NEUTRAL_BIT"]
    CC   = masks["CRUISE_CONTROL_BUTTON_BIT"]
    RB   = masks["REGEN_BUTTON_BIT"]
    RTR  = masks["READY_TO_REGEN_BIT"]
    REN  = masks["REGEN_ENABLED_BIT"]

    if cur in (INIT, NR_ST):
        return NR_ST
    if cur == DIS_ST:
        return DIS_ST

    if cur == NEU_ST:
        if  (bits & FWD) and not (bits & REV):  return FWD_ST
        if   bits & REV:                         return REV_ST

    elif cur == FWD_ST:
        if   (bits & REV) or (bits & NEU):       return NEU_ST
        if   (bits & REN) and (bits & RTR) and (bits & RB): return REGEN_ST
        if   (bits & CC)  and (bits & REN):      return CC_ST
        return FWD_ST

    elif cur == REV_ST:
        if   (bits & REV) and not (bits & FWD):  return REV_ST
        return NEU_ST

    elif cur == REGEN_ST:
        if   (bits & RB) and (bits & RTR) and (bits & REN) and (bits & FWD): return REGEN_ST
        return FWD_ST

    elif cur == CC_ST:
        if   bits & CC:  return CC_ST
        return FWD_ST

    return NEU_ST  # fallback


# === Pretty-print helper ===

def format_next_states(state_names, indices, indent="            ", per_line=8):
    rows, chunk = [], []
    for idx, st in enumerate(indices):
        chunk.append(state_names[st])
        if len(chunk) == per_line or idx == len(indices) - 1:
            rows.append(indent + ", ".join(chunk))
            chunk = []
    return (",\n").join(rows)


# === Main ===

def main():
    text = FSM_H.read_text()

    # Parse BitfieldBitIndex → bit masks
    bit_entries, bitfield_input_count = parse_sequential_enum(
        text, "BitfieldBitIndex", "BITFIELD_INPUT_COUNT"
    )

    # Build logical-name → mask mapping
    masks = {}
    name_map = {
        "NEUTRAL":               "NEUTRAL_BIT",
        "FORWARD":               "FORWARD_BIT",
        "REVERSE":               "REVERSE_BIT",
        "CRUISE_CONTROL_BUTTON": "CRUISE_CONTROL_BUTTON_BIT",
        "REGEN_BUTTON":          "REGEN_BUTTON_BIT",
        "READY_TO_REGEN":        "READY_TO_REGEN_BIT",
        "REGEN_ENABLED":         "REGEN_ENABLED_BIT",
    }
    for enum_name, bit_idx in bit_entries:
        # enum_name is like BIT_IDX_NEUTRAL → strip prefix
        key = enum_name.removeprefix("BIT_IDX_")
        if key in name_map:
            masks[name_map[key]] = 1 << bit_idx

    # Parse FSMStates
    state_entries, num_states = parse_sequential_enum(
        text, "FSMStates", "NUM_STATES"
    )
    state_names = [name for name, _ in state_entries]
    state_idx   = {name: idx for name, idx in state_entries}

    next_states_length = 1 << bitfield_input_count

    # Write fsm_table.h
    FSM_TABLE_H.parent.mkdir(parents=True, exist_ok=True)

    with FSM_TABLE_H.open("w") as f:
        f.write("/**\n")
        f.write(" * @file fsm_table.h\n")
        f.write(" * @brief Auto-generated FSM next-state table\n")
        f.write(" *        DO NOT EDIT — regenerate with generate_fsm.py\n")
        f.write(" */\n\n")
        f.write("#pragma once\n")
        f.write('#include "fsm.h"\n\n')
        f.write("// Instantiated in exactly one .c file via:\n")
        f.write("//   #define DEFINE_FSM_TABLE\n")
        f.write("//   #include \"fsm_table.h\"\n\n")
        f.write("extern TritiumState_t FSM[NUM_STATES];\n\n")
        f.write("#ifdef DEFINE_FSM_TABLE\n")
        f.write("TritiumState_t FSM[NUM_STATES] = {\n\n")

        for i, state_name in enumerate(state_names):
            next_states = [
                compute_next_state(i, j, masks, state_idx)
                for j in range(next_states_length)
            ]
            ns_str = format_next_states(state_names, next_states)
            f.write(f"    [{state_name}] = {{ {state_name}, NULL, {{\n")
            f.write(ns_str + "\n")
            f.write("    }},\n\n")

        f.write("};\n")
        f.write("#endif // DEFINE_FSM_TABLE\n")

    print(f"Generated {FSM_TABLE_H}  ({num_states} states × {next_states_length} inputs)")


if __name__ == "__main__":
    main()
