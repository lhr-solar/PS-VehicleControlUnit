# VehicleControlUnit
Vehicle Control Unit (VCU) for the 2026 Longhorn Racing Solar car

![CubeMX Picture](https://github.com/lhr-solar/VehicleControlUnit/blob/RevA/VCU_LSOM_Pinout.png)


## Getting Started

### Cloning
Clone this repository using the following command
``` sh
git clone git@github.com:lhr-solar/PS-VehicleControlUnit.git --recursive
```

### Installation
Follow the installation instructions for your specific platform, found [here](https://lhrsolar.org/Embedded-Sharepoint/Installation/).  

Additionally, install the [Arduino IDE](https://docs.arduino.cc/software/ide/)

## Command Usage

### Compiling Firmware
All firmware is stored in the Firmware/ directory and must be run while in a nix shell. For the first time you will need to run
``` sh
chmod +x ./run_nix.sh
./run_nix.sh
```
This will place you in a nix shell with all needed dependencies. After the first time, you only need to run the below command to enter the nix shell.
``` sh
./run_nix.sh
```

To compile production firmware, simply run:
``` sh
make
```
### Compiling Tests
To compile a specific test file, run:
``` sh
make TEST=[name of test excluding .c and test_]
```
Test files are located in test/ and must have the prefix `test_` or else the test won't compile correctly. If you want to run the blinky test, which is located in `tests/blinky_test.c` you will run
``` sh
make TEST=blinky
```

