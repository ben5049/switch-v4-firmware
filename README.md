# Switch v4 Firmware (WIP)

STM32CubeIDE source code for the 4th iteration of my single pair Ethernet (SPE) switch, written in C. The hardware can be found in [switch-v4-hardware](https://github.com/ben5049/switch-v4-hardware).

Also see my post on [r/embedded](https://www.reddit.com/r/embedded/comments/1mxale6/i_built_a_single_pair_ethernet_switch/)!

## Overview

This repository is split into firmwares for the primary and secondary MCUs on the switch board:
- The **primary** MCU does most of the configuration and management of the switch.
- The **seconday** MCU manages power to connected devices using Power over Datalines (PoDL).

Currently only the primary MCU firmware is working.

## Primary MCU Firmware

Most of the non-generated code can be found in the [Bootloader](primary/Secure/Bootloader) and [Application](primary/NonSecure/Application) directories. This project uses the ARM Trustzone architecture to have a secure bootloader and a non-secure application.

The secure bootloader is responsible for the following:
- Setting up peripheral attribution and initialising secure peripherals
- Calculating the SHA-256 hashes of the secure and non-secure firmwares to check for tampering/degradation
- Selecting a non-secure firmware image to boot from
- Storing encrypted metadata and event counters in the external FRAM
- Logging
- Error handling
- Background scrubbing for ECC errors (WIP)
- Writing new firmware images (WIP)
- Communicating with the secondary MCU (WIP)

The non-secure firmwre is responsible for the follwing:
- Initialising non-secure peripherals
- Initialising the switch and PHY chips
- Running a the networking stack
- Publishing diagnostic information with Zenoh Pico
- Switch and PHY maintainance
- Firmware updates (WIP)
- Running PTP (WIP)
- Running STP (WIP)

## Libraries

- [SJA1105 Driver](https://github.com/ben5049/stm32-sja1105) (by me)
- [PHY Drivers](https://github.com/ben5049/phy-drivers) (by me)
- [Zenoh Pico](https://github.com/eclipse-zenoh/zenoh-pico)
- [Nanopb](https://github.com/nanopb/nanopb)
- [mstp-lib](https://github.com/adigostin/mstp-lib) (provisionally)