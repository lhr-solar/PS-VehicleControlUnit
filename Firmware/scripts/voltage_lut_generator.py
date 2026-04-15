import numpy as np
import os

def generate_voltage_lut(csv_path, output_path):
    # Get the directory where the script itself is located
    script_dir = os.path.dirname(os.path.abspath(__file__))
    
    adc_max = 4095  # 12-bit ADC

    # Constants
    VREF_MV = 3300
    GAIN_NUM = 5
    GAIN_DEN = 2
    R_NUM = 102490
    R_DEN = 2490

    adc_range = np.arange(4096, dtype=np.uint64)

    # Integer math for voltage calculation
    numerator = adc_range * VREF_MV * GAIN_NUM * R_NUM
    denominator = adc_max * GAIN_DEN * R_DEN
    voltage_milli_v = (numerator + denominator // 2) // denominator  # rounded division

    # Prepare C header file content
    header_content = [
        "#pragma once",
        "",
        "#include <stdint.h>",
        "",
        "/**",
        " * @brief Look-up table for voltage ADC counts to voltage.",
        " * Index: 12-bit ADC Count (0 - 4095)",
        " * Value: Voltage in millivolts (mV)",
        " */",
        "static const uint32_t voltage_lut[4096] = {"
    ]

    # Add array entries formatted for readability (10 per line)
    for i in range(0, len(voltage_milli_v), 10):
        row = voltage_milli_v[i:i+10]
        row_str = "    " + ", ".join(f"{val:7d}" for val in row)
        if i + 10 < len(voltage_milli_v):
            row_str += ","
        header_content.append(row_str)

    header_content.append("};")
    
    # Get the directory where the script itself is located
    script_dir = os.path.dirname(os.path.abspath(__file__))
    # Join that directory with your filename
    output = os.path.join(script_dir, output_path)

    # Ensure directory exists and write file
    os.makedirs(os.path.dirname(output), exist_ok=True)
    with open(output, 'w') as f:
        f.write("\n".join(header_content))

    print(f"Successfully generated {output}")

if __name__ == "__main__":
    csv_filename = 'ERTJ1VR.csv'
    output_filename = '../voltage_lut.h'
    generate_voltage_lut(csv_filename, output_filename)