# OpenRTX
## Modular Open Source Radio Firmware

OpenRTX is a free and open source firmware for digital ham radios, top-down designed
with modularity, flexibility and performance in mind.

Currently OpenRTX is being actively developed for the following radios:

- TYT MD-380/390
- TYT MD-UV380/390
- TYT MD-9600
- Radioddity GD77

This firmware is *highly experimental* and is not in a usable state right now,
however contributions and testing are welcome and accepted.

For information on currently supported radios and features, see the [Development status](https://github.com/n1zzo/OpenRTX/wiki/Development-status) wiki page.

For hardware and software documentation visit [openrtx.org](https://openrtx.org/)

## Flash on MD-3x0 and MD-UV3x0

On TYT MD-3x0 and MD-UV3x0 you can flash our firmware using tarxvf's web based flashing tool.

- Download the latest OpenRTX release for your radio from [the releases page](https://github.com/OpenRTX/OpenRTX/releases).
- Connect your radio to the PC, put it in DFU mode (turn off, turn on pressing PTT and the button just above).
- On any operating system, open Chrome/Chromium and navigate to [dmr.tools](https://dmr.tools),
- Click on "Upgrade", then "Connect Radio(s)", under "Select firmware file" choose "Upload from my computer",
- Click on "Choose File", select the latest OpenRTX release you downloaded before, and click on "Upgrade Radio".
- Reboot your radio

## Compile and flash

For instructions on how to compile and flash OpenRTX to your radio,
or just run OpenRTX on Linux see the [How to compile](https://github.com/n1zzo/OpenRTX/wiki/How-to-compile) wiki page.

## Screenshots
<img src="assets/splash.gif" width="200" height="auto">

## Contact

To reach out, come to our [M17 Reflector](https://m17.openrtx.org) or into our [Discord Server](https://discord.gg/TbR2FVtMya).

## License

This software is released under the GNU GPL v3.

minmea is released under the DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE v2.

## Disclaimer

This project was created for research and amateur radio use only,
we are not responsible for improper uses of this code which might lead to
unauthorized transmission, reception or patent infringments.

## Credits

OpenRTX was created by:

- Niccolò Izzo IU2KIN <n@izzo.sh>
- Silvano Seva IU2KWO <silseva@fastwebnet.it>
- Federico Amedeo Izzo IU2NUO <federico@izzo.pro>
- Frederik Saraci IU2NRO <frederik.saraci@gmail.com>

All this was made possible by the huge reverse engineering effort of
Travis Goodspeed and all the contributors of [md380tools](https://github.com/travisgoodspeed/md380tools).
A huge thank goes to Roger Clark, and his [OpenGD77](https://github.com/rogerclarkmelbourne/OpenGD77) which not only inspired this project, but as a precursor, provided a working code example for the GD77 radio family.
Finally, a warm thank you goes to SP5WWP and the [M17](https://m17project.org) community for bringing their libre protocol into our obscure undocumented hardware. 
