#!/usr/bin/env python3
"""
FSM header generator for Tritium FSM
Matches DriveMotor.h BitfieldInputs enum order.
Generates FSM.h directly in the correct Firmware/apps/inc folder.
"""

import os

# === Config ===
NUM_STATES = 8
NEXT_STATES_LENGTH = 1 << 7  # 2^BITFIELD_INPUT_COUNT = 128

# Bitfield flags (must match DriveMotor.h order)
NEUTRAL_BIT = 0x1
FORWARD_BIT = 0x2
REVERSE_BIT = 0x4
CRUISE_CONTROL_BUTTON_BIT = 0x8
REGEN_BUTTON_BIT = 0x10
READY_TO_REGEN_BIT = 0x20
REGEN_ENABLED_BIT = 0x40

# FSM states
STATE_INIT = 0
FORWARD_DRIVE = 1
NEUTRAL = 2
REVERSE_DRIVE = 3
REGEN = 4
CRUISE_CONTROL = 5
DISABLED = 6
CAR_NOT_READY = 7

state_names = [
    "STATE_INIT",
    "FORWARD_DRIVE",
    "NEUTRAL",
    "REVERSE_DRIVE",
    "REGEN",
    "CRUISE_CONTROL",
    "DISABLED",
    "CAR_NOT_READY"
]

# === FSM logic ===
def compute_next_state(i, j):
    """Compute the next state given current state i and input bitfield j."""
    if i == CAR_NOT_READY or i == STATE_INIT:
        return CAR_NOT_READY
    if i == DISABLED:
        return DISABLED

    if i == NEUTRAL:
        if (j & FORWARD_BIT) and not (j & REVERSE_BIT):
            return FORWARD_DRIVE
        elif (j & REVERSE_BIT):
            return REVERSE_DRIVE

    elif i == FORWARD_DRIVE:
        if (j & REVERSE_BIT) or (j & NEUTRAL_BIT):
            return NEUTRAL
        elif (j & REGEN_ENABLED_BIT) and (j & READY_TO_REGEN_BIT) and (j & REGEN_BUTTON_BIT):
            return REGEN
        elif (j & CRUISE_CONTROL_BUTTON_BIT) and (j & REGEN_ENABLED_BIT):
            return CRUISE_CONTROL
        else:
            return FORWARD_DRIVE

    elif i == REVERSE_DRIVE:
        if (j & REVERSE_BIT) and not (j & FORWARD_BIT):
            return REVERSE_DRIVE
        else:
            return NEUTRAL

    elif i == REGEN:
        if ((j & REGEN_BUTTON_BIT) and (j & READY_TO_REGEN_BIT) and
            (j & REGEN_ENABLED_BIT) and (j & FORWARD_BIT)):
            return REGEN
        else:
            return FORWARD_DRIVE

    elif i == CRUISE_CONTROL:
        if not (j & CRUISE_CONTROL_BUTTON_BIT):
            return FORWARD_DRIVE
        else:
            return CRUISE_CONTROL

    return NEUTRAL  # fallback safety

# === Main generation ===
def main():
    # Determine the project root based on this script's location
    script_dir = os.path.dirname(os.path.realpath(__file__))
    project_root = os.path.abspath(os.path.join(script_dir, ".."))  # assume script is in Firmware/scripts

    # Correct target path inside repo
    output_path = os.path.join(project_root, "apps", "inc", "FSM.h")

    # Ensure folder exists
    os.makedirs(os.path.dirname(output_path), exist_ok=True)

    # Write FSM header (overwrite if it exists)
    with open(output_path, "w") as f:
        f.write("#pragma once\n\n")
        f.write('#include "DriveMotor.h"\n\n')
        f.write("TritiumState_t FSM[NUM_STATES] = {\n")

        for i in range(NUM_STATES):
            next_states = [compute_next_state(i, j) for j in range(NEXT_STATES_LENGTH)]
            c_array = ", ".join(state_names[state] for state in next_states)
            f.write(f"    {{{state_names[i]}, NULL, {{{c_array}}}}},\n")

        f.write("};\n")

    print(f"Generated {output_path} successfully!")

if __name__ == "__main__":
    main()
