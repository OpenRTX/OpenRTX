# OpenRTX
## Modular Open Source Radio Firmware

OpenRTX is a free and open source firmware for digitam ham radios, top-down designed
with modularity, flexibility and performance in mind.

Currently OpenRTX is being actively developed for the TYT MD-380/390 and MD-UV380/390.

This firmware is *highly experimental* and is not in a usable state right now,
however contributions and testing are welcome and accepted.

## Installation

To build and install the firmware, first clone this repository:

```
git clone https://github.com/n1zzo/OpenRTX
```

To build the firmware you need to have a toolchain for the ARM ISA installed
on you system, you can install one using your package manager.

For example on Ubuntu you can install `arm-none-eabi-gcc`
```
sudo apt install gcc-arm-none-eabi
```

You can then proceed in building the firmware:

```
meson builddir
meson compile -C builddir linux
```

If everything compiled without errors you can connect your radio via USB,
put it in recovery mode (by powering it on with the PTT and the button
above it pressed), and flash the firmware:

```
make flash
```

Now you can power cycle your radio and enjoy the new breath of freedom!

## License

This software is released under the GNU GPL v3, the modified wrapping scripts
from Travis Goodspeed are licensed in exchange of two liters of India Pale Ale,
we still owe you the two liters, Travis!

## Credits

OpenRTX was created by:

- Niccolò Izzo IU2KIN <n@izzo.sh>
- Silvano Seva IU2KWO <silseva@fastwebnet.it>
- Federico Amedeo Izzo IU2NUO <federico@izzo.pro>

All this was made possible by the huge reverse engineering effort of
Travis Goodspeed and all the contributors of [md380tools](https://github.com/travisgoodspeed/md380tools).
A huge thank goes to Roger Clark, and his [OpenGD77](https://github.com/rogerclarkmelbourne/OpenGD77) which inspired this project.
