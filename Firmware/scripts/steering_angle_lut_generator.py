#!/usr/bin/env python3
"""
generate_rollover_table.py
Generates rollover_speed_table.h — the max allowable speed lookup table.
LWS_Angle is int16_t in 0.1 degree units; table indexed by abs(LWS_Angle / 10).
"""

import math
from pathlib import Path

OUT = Path(__file__).resolve().parent.parent / "core/Inc/rollover_speed_table.h"

G, HALF_TRACK, CG_LATERAL = 9.81, 1.35, 1.612
CG_HEIGHT, WHEELBASE, STEERING_RATIO, MIN_DEG = 0.481, 2.25, 15.0, 2.0
NO_LIMIT = 0xFFFF

def v_max_cms(deg):
    wheel_deg = deg / STEERING_RATIO
    if abs(wheel_deg) < MIN_DEG:
        return NO_LIMIT
    R = WHEELBASE / math.tan(math.radians(abs(wheel_deg)))
    v_ms = math.sqrt(G * R * HALF_TRACK * CG_LATERAL / (2.0 * CG_HEIGHT * WHEELBASE))
    return min(int(v_ms * 100), NO_LIMIT - 1)

table = [v_max_cms(i) for i in range(721)]

rows = [f"    " + ", ".join(f"0x{v:04X}" for v in table[i:i+8]) + f",  // {i}-{min(i+7, len(table)-1)} deg"
        for i in range(0, len(table), 8)]

OUT.write_text(
    "/**\n"
    " * @file rollover_speed_table.h\n"
    " * @brief Auto-generated rollover speed limit lookup table — do not edit\n"
    " *        Regenerate: python3 scripts/generate_rollover_table.py\n"
    " *\n"
    " * Index : abs(LWS_Angle / 10)  [integer degrees, 0-720]\n"
    " * Value : max allowable speed in cm/s  |  0xFFFF = no limit (straight)\n"
    " */\n\n"
    "#pragma once\n\n"
    "#include <stdint.h>\n\n"
    "#define ROLLOVER_TABLE_NO_LIMIT  0xFFFFU\n"
    "#define ROLLOVER_TABLE_MAX_DEG   720U\n\n"
    "// Index: abs(LWS_Angle / 10) in integer degrees (0-720)\n"
    "// Value: max allowable speed in cm/s  |  0xFFFF = no limit\n"
    "static const uint16_t rollover_speed_table[721] = {\n"
    + "\n".join(rows) + "\n"
    "};\n"
)
print(f"Generated {OUT}  ({len(table)} entries, {len(table)*2} bytes)")
