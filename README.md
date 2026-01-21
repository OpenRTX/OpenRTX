# OpenRTX
## Modular Open Source Radio Firmware

OpenRTX is a free and open source firmware for digital amateur radio devices, top-down designed
with modularity, flexibility and performance in mind.

Currently OpenRTX supports the following devices:

- TYT MD-380/390
- TYT MD-UV380/390
- TYT MD-9600
- Radioddity GD77
- Baofeng DM-1801
- Baofeng DM-1701
- Connect Systems CS7000-M17
- Connect Systems CS7000-M17 Plus
- Module17

This firmware is *highly experimental* and currently under development, this means
that it may not have all the expected functionalities. Anyway, contributions and testing will be warmly welcomed and accepted!

For information on the radios that are currently supported and their features, see the [Development Status](https://openrtx.org/#/dev_status?id=current-support) page on our website.

For hardware and software documentation visit [openrtx.org](https://openrtx.org/)

## Obtaining the firmware

**Warning: Read the disclaimer section first!**

Pre-built binary images of the OpenRTX firmware for each one of the supported devices are available on [the releases page](https://github.com/OpenRTX/OpenRTX/releases): to flash them to your radio, you can use the OEM firmware upgrade tool or — alternatively and for TYT MD-3x0 and MD-UV3x0 radios only — you can also use the [radio_tool](https://github.com/v0l/radio_tool) program or tarxvf's web based flashing tool [dmr.tools](https://dmr.tools).

Between releases, pre-built binary images containing the latest firmware updates are available through nightly builds from the master branch.
The nightly builds are available here:
- on [Phil DF5PMF's page](https://openrtx.schinken-radio.de/nightly/)
- on the [OpenRTX nightly builds page](https://files.openrtx.org/nightly/)

Finally, the instructions on how to compile the OpenRTX firmware for hardware as well as emulation on Linux, are available on the [compilation instructions](https://openrtx.org/#/compiling) page on our website.

Have a look at the the [dedicated page](https://openrtx.org/#/user_guide) for detailed instructions on flashing the firmware to your radio, look at the OpenRTX website or reach out to us on our channels!

## M17 support

From the release version 0.3.3 onwards the OpenRTX firmware provides experimental support for the M17 digital voice mode.

The following radios are currently supported for use with this digital mode:
- MD-380, MD-390 and RT3 **UHF version** are supported for both modulation and demodulation.
- MD-380, MD-390 and RT3 **VHF version** are supported for demodulation **only**, modulation is a work in progress.
- MD-UV380 and MD-UV390 are supported for both modulation and demodulation.
- GD77 and DM-1801 currently **are not supported** for the new digital voice mode.

To make the digital mode work, some modding is required: Refer to the [dedicated page](https://openrtx.org/#/M17/m17?id=hardware-modifications) on our website for the details on that.

## Disclaimer

This project was created for research and amateur radio use only, we are not
responsible for improper use of this code which might lead to unauthorized
transmission, reception or any patent infringments.

The OpenRTX firmware is released WITHOUT ANY WARRANTY: Anyone flashing a binary
image obtained from the sources made available through this repository does so at
their own risk. We always test all the code on our devices before publishing it
on the repository, however we cannot guarantee the absolute absence of bugs nor of
potential side effects.

## Contact

To reach out, visit our [M17 Reflector](https://m17.openrtx.org) or join our Matrix room for general discussion [#openrtx_general:matrix.org
](https://matrix.to/#/#openrtx_general:matrix.org) or the Matrix space [#openrtx:matrix.org](https://matrix.to/#/#openrtx:matrix.org) which contains many additional rooms or our [Discord Server](https://discord.gg/TbR2FVtMya).

## Donate

To support the development of OpenRTX you can donate using [Liberapay](https://liberapay.com/OpenRTX/donate). \
If you want to donate hardware to facilitate porting and development of OpenRTX, please [contact us](https://github.com/OpenRTX/OpenRTX#contact).

## License

This software is released under the GNU GPL v3.

minmea is released under the DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE v2.

Code for STM32F405 USB driver is released under the MCD-ST Liberty SW License Agreement V2.

uf2conf.py and related files are released by Microsoft Corporation under MIT license.

## Credits

OpenRTX is being made by:

- Niccolò Izzo IU2KIN <n@izzo.sh>
- Silvano Seva IU2KWO <silseva@fastwebnet.it>
- Federico Amedeo Izzo IU2NUO <federico@izzo.pro>
- Frederik Saraci IU2NRO <frederik.saraci@gmail.com>

Our wholehearted thanks go to the following contributors from the community:

- Joseph Stephen VK7JS, who implemented voice prompts

All this is possible by the huge reverse engineering effort of Travis Goodspeed and all the contributors of the [md380tools](https://github.com/travisgoodspeed/md380tools).

A huge thanks goes to Roger Clark, and his [OpenGD77](https://github.com/rogerclarkmelbourne/OpenGD77) (repo no longer available) which not only inspired this project, but as a precursor, provided a working code example for the GD77 radio family.

A warm thank you goes to Wojciech Kaczmarski SP5WWP and the [M17](https://m17project.org) community for bringing their libre protocol into our obscure undocumented hardware.

Also, a thank you for donating hardware to this project goes to:
* M17 Project
* laurivosandi
* smeegle5000

And thanks to everyone who donated via [LiberaPay](https://liberapay.com/OpenRTX/donate).

## Translation

Contribute to the crowdsourced effort to make OpenRTX more accessible by helping [translate it](https://hosted.weblate.org/projects/openrtx/). Weblate proudly supports libre projects like OpenRTX with their tools to help this.

[![Translation status](https://hosted.weblate.org/widget/openrtx/287x66-grey.png)](https://hosted.weblate.org/engage/openrtx/)
