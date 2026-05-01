FROM ubuntu:24.04

LABEL org.opencontainers.image.source=https://github.com/OpenRTX/OpenRTX

COPY --from=ghcr.io/astral-sh/uv:0.11.7 /uv /uvx /bin/

# Install dependencies
RUN apt-get update -y && apt-get install -y --no-install-recommends \
  git \
  wget \
  sudo \
  build-essential \
  libncurses5-dev \
  byacc \
  flex \
  texinfo \
  patch \
  tar \
  unzip \
  lzip \
  libelf-dev \
  perl \
  libexpat1-dev \
  ca-certificates \
  pkg-config \
  meson \
  cmake \
  libstdc++6 \
  libusb-1.0-0 \
  libusb-1.0-0-dev \
  dfu-util \
  libsdl2-dev \
  libcodec2-dev \
  libreadline-dev \
  libcairo2 \
  gcc \
  g++ \
  && rm -rf /var/lib/apt/lists/*

# Install python
RUN uv python install

# compile miosix compiler
RUN git clone --depth 1 https://github.com/fedetft/miosix-kernel.git \
  && cd miosix-kernel/miosix/_tools/compiler/gcc-9.2.0-mp3.2 \
  && sh download.sh \
  && bash install-script.sh -j`nproc`

# Build radio_tool from source (since the deb package doesnt support all architectures)
RUN git clone https://github.com/v0l/radio_tool \
    && cd radio_tool \
    && git checkout tags/v0.3.0 \
    && mkdir build && cd build \
    && cmake .. \
    && make -j4 \
    && make install
ENV PATH="$PATH:/usr/local/bin"
