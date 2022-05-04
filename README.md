# OpenRTX
## Modular Open Source Radio Firmware

OpenRTX is a free and open source firmware for digital ham radios, top-down designed
with modularity, flexibility and performance in mind.

Currently OpenRTX is being actively developed for the following radios:

- TYT MD-380/390
- TYT MD-UV380/390
- TYT MD-9600
- Radioddity GD77 and Baofeng DM-1801

This firmware is *highly experimental* and currently under development, this means
that it may not have all the expected functionalities. Contributions and testing
are anyway warmly welcome and accepted!

For information on currently supported radios and features, see the [Supported Platforms](https://openrtx.org/#/platforms) page on our website.

For hardware and software documentation visit [openrtx.org](https://openrtx.org/)

## Obtaining the firmware

**Warning: read the Disclaimer section first!**

Pre-built binary images of the OpenRTX firmware for each one of the supported devices are available on [the releases page](https://github.com/OpenRTX/OpenRTX/releases): to flash them on your radio you can use the OEM firmware upgrade tool or, alternatively and for TYT MD-3x0 and MD-UV3x0 radios only, you can also use the [radio_tool](https://github.com/v0l/radio_tool) program or tarxvf's web based flashing tool [dmr.tools](https://dmr.tools).

Between releases, pre-built binary images containing the latest firmware updates are available through nightly builds from the master branch.
The nightly builds are available here:
- on [Phil DF5PMF's page](https://openrtx.schinken-radio.de/nightly/)
- on the [OpenRTX nightly builds page](https://files.openrtx.org/nightly/)

Finally, the instructions on how to compile the OpenRTX firmware, also in its emulator version on Linux, are available on the [compilation instructions](https://openrtx.org/#/compiling) page on our website.

If you need detailed instructions on how to flash the firmware to your radio look at the [dedicated page](https://openrtx.org/#/user_guide) on OpenRTX's website or reach us on our channels!

## M17 support

From the release version 0.3.3 the OpenRTX firmware provides experimental support for the M17 digital voice mode.

Currently the radios supporting this digital mode are the following:
- MD-380, MD-390 and RT3 **UHF version** support both modulation and demodulation.
- MD-380, MD-390 and RT3 **VHF version** support **only** demodulation, modulation is a work in progress.
- MD-UV380 and MD-UV390 support support both modulation and demodulation.
- GD77 and DM-1801 currently **do not support** the new digital voice mode.

To make the digital mode work, some modding is required: refer to the [dedicated page](https://openrtx.org/#/M17/m17?id=hardware-modifications) on our website for the details.

## Disclaimer

This project was created for research and amateur radio use only, we are not
responsible for improper uses of this code which might lead to unauthorized
transmission, reception or patent infringments.

The OpenRTX firmware is released WITHOUT ANY WARRANTY: anyone flashing a binary
image obtained from the sources made available through this repository does it
at its own risk. We always test all the code on our devices before publishing it
on the repository, however we cannot guarantee the absolute absence of bugs and
potential side effects.

## Contact

To reach out, come to our [M17 Reflector](https://m17.openrtx.org) or into our [Discord Server](https://discord.gg/TbR2FVtMya).

## Donate

To support the development of OpenRTX you can make us a donation using [Liberapay](https://liberapay.com/OpenRTX/donate). \
If you want to donate some hardware to facilitate OpenRTX porting and development, [contact us](https://github.com/OpenRTX/OpenRTX#contact).

## License

This software is released under the GNU GPL v3.

minmea is released under the DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE v2.

Code for STM32F405 USB driver is released under the MCD-ST Liberty SW License Agreement V2.

## Credits

OpenRTX was created by:

- Niccolò Izzo IU2KIN <n@izzo.sh>
- Silvano Seva IU2KWO <silseva@fastwebnet.it>
- Federico Amedeo Izzo IU2NUO <federico@izzo.pro>
- Frederik Saraci IU2NRO <frederik.saraci@gmail.com>

Our wholehearted thanks to the contributions from the community:

- Joseph Stephen VK7JS, which implemented voice prompts

All this was made possible by the huge reverse engineering effort of Travis Goodspeed and all the contributors of [md380tools](https://github.com/travisgoodspeed/md380tools).

A huge thank goes to Roger Clark, and his [OpenGD77](https://github.com/rogerclarkmelbourne/OpenGD77) which not only inspired this project, but as a precursor, provided a working code example for the GD77 radio family.

A warm thank you goes to SP5WWP and the [M17](https://m17project.org) community for bringing their libre protocol into our obscure undocumented hardware.

Also thank you for donating hardware to the project:
* M17 Project
* laurivosandi

And thanks to everyone who donated via [LiberaPay](https://liberapay.com/OpenRTX/donate).
