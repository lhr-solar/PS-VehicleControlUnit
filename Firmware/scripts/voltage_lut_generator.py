#!/usr/bin/env python3
"""
voltage_lut_generator.py
Generates ADC_Voltage_LUT.h - the ADC count -> millivolts lookup table.
Motor and battery voltage sense channels share the same divider/op-amp
front end (see ADC_Sense.c), so both look up into this one table.
"""

from pathlib import Path

OUT = Path(__file__).resolve().parent.parent / "Drivers/Inc/ADC_Voltage_LUT.h"

BITS, VREF, R_TOP, R_BOTTOM, AMP_GAIN = 12, 3.337, 100_000.0, 2490.0, 0.4

MAX_COUNTS = (2 ** BITS) - 1
DIVIDER_RATIO = R_BOTTOM / (R_TOP + R_BOTTOM)

def counts_to_mv(counts):
    v_adc_pin = (counts / MAX_COUNTS) * VREF
    v_divider_out = v_adc_pin / AMP_GAIN
    v_original_volts = v_divider_out / DIVIDER_RATIO
    return round(v_original_volts * 1000)

table = [counts_to_mv(c) for c in range(2 ** BITS)]

rows = [f"    " + ", ".join(f"{v:7d}" for v in table[i:i+8])
        + f",  // {i}-{min(i + 7, len(table) - 1)}"
        for i in range(0, len(table), 8)]

OUT.write_text(
    "/**\n"
    " * @file ADC_Voltage_LUT.h\n"
    " * @brief Auto-generated ADC count -> voltage lookup table — do not edit\n"
    " *        Regenerate: python3 scripts/voltage_lut_generator.py\n"
    " *\n"
    " * Shared by the motor and battery voltage sense channels, which use the\n"
    " * same resistor divider / op-amp gain stage ahead of the ADC.\n"
    " * Index : raw 12-bit ADC count (0-4095)\n"
    " * Value : sensed voltage in millivolts (mV)\n"
    " */\n\n"
    "#pragma once\n\n"
    "#include <stdint.h>\n\n"
    "// Index: raw 12-bit ADC count (0-4095)\n"
    "// Value: sensed voltage in millivolts (mV)\n"
    f"static const uint32_t Voltage_LUT[{2 ** BITS}] = {{\n"
    + "\n".join(rows) + "\n"
    "};\n"
)
print(f"Generated {OUT}  ({len(table)} entries, 0 mV to {table[-1]} mV)")
