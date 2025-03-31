# genFactoryData Tool

## Overview

The `genFactoryData` tool is a Python script designed to manage and provision firmware and data for STM32 microcontrollers. It supports reading and writing JSON and binary files, flashing firmware to specific memory addresses, and provisioning devices using STM32CubeProgrammer.

## Features

- Read and write JSON and binary files containing device configuration data.
- Flash application and coprocessor firmware to specified memory addresses.
- Provision devices using a specified provisioning script.
- List and check availability of serial ports.
- Handle multiple devices concurrently using threading.

## Requirements

- Python 3.x
- STM32CubeProgrammer installed (default path: `c:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin`)
- Required Python packages: `serial`, `concurrent.futures`, `argparse`, `json`, `struct`, `subprocess`, `tempfile`, `os`, `re`, `ssl`, `enum`, `threading`, `datetime`

## Usage

### Command Line Arguments

The script accepts several command line arguments to control its behavior:

- `--provisioning`: Mandatory. Set to `0` or `1` to indicate whether provisioning is required.
- `--json`: Path to a JSON file containing device configuration data (mutually exclusive with `--binary`).
- `--binary`: Path to a binary file containing device configuration data (mutually exclusive with `--json`).
- `--flashCoFW`: Path to the coprocessor firmware file (required if `--provisioning` is `1`).
- `--flashProvisioningFW`: Path to the application firmware provisioning file (required if `--provisioning` is `1`).
- `--provisioning_script`: Path to the provisioning script (required if `--provisioning` is `1`).
- `--flashAppFW`: Path to the application Matter firmware file (required if `--provisioning` is `1`).
- `--mode`: Mode for provisioning (`test` or `production`).
- `--AppFwAddr`, `-afwa`: Address where the application firmware is flashed. No default value.
- `--CoFwAddr`, `-cofwa`: Address where the coprocessor firmware is flashed. No default value.
- `--flashAddr`, `-fa`: Address where the binary output file is flashed. Mandatory.
- `--programmerPath`, `-pp`: Path to STM32CubeProgrammer. If not set, `c:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin` path will be used.
- `--externalLoader`, `-el`: External loader file to use. If not set, `S25FL128S_STM32WB5MM-DK` will be used.
- `--verbose`, `-v`: Display recognized items when reading JSON or binary file.

### Example Command

```sh
python genFactoryData.py --provisioning 1 --json config.json --flashCoFW coprocessor_firmware.bin --flashProvisioningFW provisioning_firmware.bin --provisioning_script provisioning_script.py --flashAppFW matter_firmware.bin --mode production --AppFwAddr 0x08000000 --CoFwAddr 0x08094000 --flashAddr 0x901C0000
