#!/usr/bin/env python3

import sys
import pathlib
import tempfile
import subprocess
import cantools
import argparse

# ------------------------------------------------------------
# 1. Define messages per target
# ------------------------------------------------------------
TARGET_MESSAGES = {
    "vcu": [
        "BPS_Status",
        "VCU_Status",
        "Accel_Brake_Position",
        "Driver_Input_Status",
    ],
    "mppt": [
        "MPPT_A_Status", "MPPT_A_Power",
        "MPPT_B_Status", "MPPT_B_Power",
        "MPPT_C_Status", "MPPT_C_Power"
    ],
    "pdu": [
        "BBPDU_Status",
        "BBPDU_Set_Switches",
        "BBPDU_Set_Current_Limit"
    ],
    "pump": [
        "Pump_Status_FlowRate",
        "Coolant_Temperature",
        "Radiator_FanSpeed"
    ]
}

# Final filenames used for generation and C includes
GEN_H_NAME = "can_parsing_generated.h"
GEN_C_NAME = "can_parsing_generated.c"

# ------------------------------------------------------------
# 2. Locate Firmware directory
# ------------------------------------------------------------
def find_firmware_dir():
    current = pathlib.Path.cwd()
    while current != current.root:
        firmware = current / "Firmware"
        if firmware.exists() and firmware.is_dir():
            return firmware
        current = current.parent
    return pathlib.Path.cwd()

# ------------------------------------------------------------
# 3. Validate Classic CAN (<=8 bytes)
# ------------------------------------------------------------
def validate_classic_can(db):
    print("✔ Validating Classic CAN payload sizes...")
    for msg in db.messages:
        if msg.length > 8:
            print(f"❌ ERROR: Message '{msg.name}' is {msg.length} bytes (exceeds 8-byte limit)")
            sys.exit(1)
    print("✔ All messages are Classic CAN compliant\n")

# ------------------------------------------------------------
# 4. Filter and Generate
# ------------------------------------------------------------
def generate_code(db, src_dir, inc_dir, target=None):
    # If a target is provided, remove all other messages
    if target:
        if target not in TARGET_MESSAGES:
            print(f"❌ Error: Target '{target}' not defined in script.")
            sys.exit(1)
        
        keep_list = TARGET_MESSAGES[target]
        # We collect names first to avoid mutation errors during iteration
        all_names = [msg.name for msg in db.messages]
        
        for name in all_names:
            if name not in keep_list:
                db.messages.remove(db.get_message_by_name(name))
        
        print(f"✔ Filtered for target '{target}': Keeping {len(db.messages)} messages.")

    with tempfile.TemporaryDirectory() as tmp:
        tmp_dir = pathlib.Path(tmp)
        # We name the temp DBC "filtered" so cantools produces filtered.c/h
        temp_dbc = tmp_dir / "filtered.dbc"
        
        with open(temp_dbc, "w", encoding="utf-8") as f:
            f.write(db.as_dbc_string())

        print("✔ Running cantools generate_c_source...")
        result = subprocess.run(
            [
                "cantools",
                "generate_c_source",
                str(temp_dbc),
                "--output-directory",
                str(tmp_dir),
                "--no-floating-point-numbers",
            ],
            capture_output=True,
            text=True
        )

        if result.returncode != 0:
            print(f"❌ Cantools Generation Failed:\n{result.stderr}")
            sys.exit(1)

        # ------------------------------------------------------------
        # 5. Find and Replace + File Writing
        # ------------------------------------------------------------
        gen_c_dest = src_dir / GEN_C_NAME
        gen_h_dest = inc_dir / GEN_H_NAME

        header_comment = (
            "/* ======================================================\n"
            "   AUTO-GENERATED FILE - DO NOT EDIT\n"
            "   Classic CAN enforced (<=8 bytes)\n"
            "   ====================================================== */\n\n"
        )

        # Read generated files
        raw_c = (tmp_dir / "filtered.c").read_text()
        raw_h = (tmp_dir / "filtered.h").read_text()

        # Fix the include directive so it points to the renamed header
        # cantools generates #include "filtered.h" by default
        fixed_c = raw_c.replace('#include "filtered.h"', f'#include "{GEN_H_NAME}"')

        # Write final files
        gen_c_dest.write_text(header_comment + fixed_c)
        gen_h_dest.write_text(header_comment + raw_h)

        print(f"✅ Successfully generated:\n   - {gen_c_dest}\n   - {gen_h_dest}\n")

# ------------------------------------------------------------
# 6. Main Execution
# ------------------------------------------------------------
def main():
    parser = argparse.ArgumentParser(description="CAN Code Generator for Firmware")
    parser.add_argument("-vcu", action="store_true", help="Filter for VCU messages")
    parser.add_argument("-mppt", action="store_true", help="Filter for MPPT messages")
    parser.add_argument("-pdu", action="store_true", help="Filter for PDU messages")
    parser.add_argument("-pump", action="store_true", help="Filter for Pump messages")
    args = parser.parse_args()

    # Determine target string
    target = None
    if args.vcu: target = "vcu"
    elif args.mppt: target = "mppt"
    elif args.pdu: target = "pdu"
    elif args.pump: target = "pump"

    firmware_dir = find_firmware_dir()
    dbc_dir = firmware_dir / "dbc"
    src_dir = firmware_dir / "apps/src"
    inc_dir = firmware_dir / "apps/inc"

    # Ensure directories exist
    src_dir.mkdir(parents=True, exist_ok=True)
    inc_dir.mkdir(parents=True, exist_ok=True)

    # Load all DBCs into one database
    db = cantools.database.Database()
    dbc_files = list(dbc_dir.glob("*.dbc"))
    
    if not dbc_files:
        print(f"❌ No DBC files found in {dbc_dir}")
        sys.exit(1)

    print(f"✔ Found {len(dbc_files)} DBC file(s).")
    for dbc_path in dbc_files:
        db.add_dbc_file(dbc_path)
    
    # Run validation
    validate_classic_can(db)

    # Filter and Generate
    generate_code(db, src_dir, inc_dir, target=target)

if __name__ == "__main__":
    main()