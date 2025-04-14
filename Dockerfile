FROM --platform=linux/amd64 ubuntu
WORKDIR /work

RUN apt-get update && apt-get install -y \
  git \
  wget \
  sudo \
  pkg-config \
  build-essential \
  meson \
  ninja-build \
  cmake \
  libusb-1.0-0 \
  libusb-1.0-0-dev \
  libsdl2-dev \
  libcodec2-dev \
  libreadline-dev

RUN wget https://miosix.org/toolchain/MiosixToolchainInstaller.run && sh MiosixToolchainInstaller.run && rm MiosixToolchainInstaller.run
