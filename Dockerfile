from archlinux as builder

RUN pacman -Syyu --noconfirm \
	base-devel \
	meson \
	sdl2 \
	cmake \
	arm-none-eabi-binutils arm-none-eabi-gcc arm-none-eabi-newlib \
	git vim screen 

RUN git clone https://github.com/v0l/radio_tool.git /radio_tool && \
	cd /radio_tool && mkdir build && cd build && \
	cmake .. && make -j4
RUN cp /radio_tool/build/radio_tool /usr/bin/

RUN pacman -S --noconfirm wget
RUN wget https://miosix.org/toolchain/MiosixToolchainInstaller.run && sh MiosixToolchainInstaller.run

#allow for automated builds
ADD ./ /app

#allow for override for using as a dev environment
VOLUME /app

WORKDIR /app


#can even flash from within the container if you run it like this:
#sudo docker run --rm -it -v "${PWD}"/:/app --privileged -v /dev/bus/usb:/dev/bus/usb openrtx 
