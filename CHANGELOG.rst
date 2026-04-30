..
  SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors

  SPDX-License-Identifier: GPL-3.0-or-later

=========
CHANGELOG
=========

v0.4.3 - 2025-12-15
===================
Added
-----
- Transmission of GNSS metadata in M17 voice streams

Changed
-------
- Major improvements of M17 demodulator code, fixing long-standing issues
- Updated voice prompts

Thanks to
---------
- Jim N1ADJ
- Ryan K0RET
- Rick KD0OSS
- Wojciech SP5WWP

v0.4.2 - 2025-10-05
===================
Added
-----
- GPS support on CS7000-M17 Plus
- FM settings menu

Changed
-------
- Cleanup and update of platform tests
- General refactoring of GPS drivers
- Started transitioning to new coding style
- Replaced bin2sgl binary tool with a python script
- Improved DC block filter implementation
- Made settings entry for RTC sync with GPS persistent across reboots
- Updated voice prompts data

Thanks to
---------
- Grzegorz SP6HFE
- Imostlylurk
- JKI757
- Marco DM4RCO
- Peter OE5BPA
- Ryan K0RET
- Tarandeep VA1FOX

v0.4.1 - 2025-07-12
===================
Added
-----
- 1750Hz tone to CS7000-M17 and CS7000-M17 Plus
- Dockerfile, devcontainer and vscode tasks for all targets

Changed
-------
- Improved computation of battery state of charge

Fixed
-----
- Bug in MD-UV3x0 audio volume management
- Broken 1750Hz tone in MD-3x0

Thanks to
---------
- Marco DM4RCO
- Morgan ON4MOD
- Peter OE5BPA

v0.4.0 - 2025-04-05
===================
Added
-----
- Support for Connect Systems CS7000-M17
- Support for Connect Systems CS7000-M17 Plus
- Support for Baofeng DM-1701

Changed
-------
- Refactored and reorganized MCU peripheral drivers
- Refactored and reorganized NVM drivers
- Using HR_C6000 as audio DAC on MD-UV3x0 devices

Thanks to
---------
- Alain
- Grzegorz SP6HFE
- Jim Ancona
- Lemielek
- Marco DM4RCO
- Marc HB9SSB
- Morgan ON4MOD
- Trriss
- Wojciech SP5WWP

v0.3.7 - 2024-07-14
===================
Added
-----
- M17: callsign-based squelch for incoming transmissions
- String table for spanish language
- Support for Module17 hardware revision 1.0

Changed
-------
- Made M17 destination callsign persistent across reboots
- Reorganized the nonvolatile memory interface
- Deeply restructured the M17 modulator, fixing the lock stability issues
- General code refactoring to reduce the size of the final binary images
- Removed usage of float from core sources

Fixed
-----
- Bug in AT1846S driver causing the output frequency to have an effective resolution of 1kHz instead of 62.5Hz.
- Missing transmission of the 1750Hz tone when keypad is locked

Thanks to
---------
- Derecho
- Edgetriggered
- Juan LW7EMN
- Marco DM4RCO
- Morgan ON4MOD
- Ryan Turner

v0.3.6 - 2023-10-20
===================
Added
-----
- M17: transmission of the EOT frame 
- M17: show stream info on screen during reception
- M17: implemented check for CAN match in RX
- Set up a dedicated UI for Module17
- Support for Module17 hardware revision 0.1e
- Transmission of 1750Hz squelch tone to MDx and GDx
- Icons to UI
- Screen lock to radio UI
- Support for a new target: Lilygo TTWR-plus

Changed
-------
- Reorganized the audio path and audio stream subsystem

Fixed
-----
- Corrected the conversion law for volume level in MDx devices

Thanks to
---------
- Antonio Vasquez Blanco
- Edgetriggered
- Jason K5JAE
- Joshua DC7IA
- Marco DM4RCO
- Mathis DB9MAT
- Mike "tarxvf" W2FBI
- Ryan Turner
- Wojciech SP5WWP

v0.3.5 - 2022-11-11
===================
Added
-----
- voice prompts for vision impaired hams

Thanks to
---------
- Joe Stephen VK7JS

v0.3.4 - 2022-09-02
===================
Added
-----
- Support for OpenRTX CPS data format
- Linux input stream driver

Changed
-------
- Reorganized and reduced the firmware threads
- Improved squelch mechanism for M17 mode to avoid random speaker activations in absence of an RF carrier
- UI font from Free Sans to Ubuntu Regular
- Reorganized linux emulator code
- Moved to statically allocated framebuffer

Fixed
-----
- Oscillation of RF carrier at the first transmission after boot 
- RX tone squelch bug on MD-UV380
- Various memory leaks

Thanks to
---------
- Alain
- Edgetriggered
- Jacob KI5VMF

v0.3.3 - 2022-05-31
===================
Added
-----
- Experimental support for M17 digital radio protocol:

    - Functional TX and RX of audio on MD-3x0 UHF devices
    - Functional RX of audio on MD-3x0 VHF devices
    - Functional TX and RX of audio on MD-UV3x0 devices

- Support for Module 17
- Persistence of user settings at power-off on MD-3x0 and MD-UV3x0
- Build file and code adaptations to allow compilation and running of the emulator under Mac OS X
- Automatic screen power-off with user-configurable timeout
- Menu entry allowing to reset user settings to their default state

Thanks to
---------
- Alessio Caiazza IU5BON
- Wojciech Kaczmarski SP5WWP
- Mike McGinty W2FBI
- Mathis Schmieder DB9MAT

v0.3.2 - 2021-06-07
===================
Added
-----
- Support for MD-380 VHF, use md3x0 target as MD-380 UHF
- Channel knob can be used to change frequency, channels and navigate menus
- Support for cross-band TX-RX operation in radio drivers
- CTCSS tone squelch on MD-UV380, GD-77 and DM-1801

Fixed
-----
- Radio configuration not updating when switching from MEM to VFO mode
- UI being showing all zero values for a moment after boot

v0.3.1 - 2021-04-09
===================
Added
-----
- Driver for channel selector knob on MD-UV380, thanks to Caleb Jamison.

Fixed
-----
- MD-3x0: error in ADC conversion sequence preventing squelch opening
- MD-3x0: bug in macro menu causing system crash on side button press
- MD-UV3x0: 5W power setting in radio driver
- MD-UV3x0: frequency shift caused by bad VCXO polarization preventing correct demodulation when configured for 12.5kHz bandwidth
- MD-UV3x0: bug in GPS task causing periodic glitches in menu system
- Swapped UP/DOWN keys in Display Settings


v0.3 - 2021-03-24
=================
Added
-----
- MD-UV3x0 analog Rx and Tx support
- GPS support to MD-3x0 and MD-UV3x0 radios

Changed
-------
- Switched to new Miosix RTOS and toolchain
- GD77 usability improvements (after testing by W9FUR)
- Improved codeplug readout
- Minor UI Improvements
- UV380/UV390(G) and RT3S radios can be flashed with mduv3x0 target

Fixed
-----
- Fix wrong screen orientation on MD-380G (VHF)

v0.2 - 2021-02-03
=================
Added
-----
- Support for two new targets: GD-77 and DM-1801, analog FM only

Changed
-------
- Single firmware image for MD-380 and MD-390, now merged into the common target "MD-3x0"

v0.1 - 2021-01-01
=================
- Alpha release of OpenRTX, with working FM on TYT MD380 and MD390
