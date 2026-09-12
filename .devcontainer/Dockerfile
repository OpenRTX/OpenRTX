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
  gcc \
  g++ \
  python3 \
  && rm -rf /var/lib/apt/lists/*

# compile miosix compiler
# Pinned to Miosix v3.01: upstream layout changes have broken this build
# before (see issue #469), bump the SHA deliberately.
ARG MIOSIX_KERNEL_SHA=361e8c35b09ad73c15ded5332dff21ce8c9a711d
RUN git init miosix-kernel \
  && cd miosix-kernel \
  && git remote add origin https://github.com/fedetft/miosix-kernel.git \
  && git fetch --depth 1 origin ${MIOSIX_KERNEL_SHA} \
  && git checkout FETCH_HEAD \
  && cd tools/compiler/gcc-9.2.0-mp3.2 \
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

# Install opengd77 sources and put the firmware loader and tools in PATH
RUN git clone https://github.com/open-ham/OpenGD77.git /tmp/OpenGD77
ENV PATH="$PATH:/tmp/OpenGD77/tools/Python/FirmwareLoader:/tmp/OpenGD77/firmware/tools"

# Create and activate a virtual environment, using uv both to supply the
# interpreter (replacing the deadsnakes PPA) and to install into it.
ENV VIRTUAL_ENV=/opt/venv
ENV PATH="$VIRTUAL_ENV/bin:$PATH"

# Copy requirements.txt and install dependencies
WORKDIR /app
COPY requirements.txt .
RUN uv venv --python 3.11 $VIRTUAL_ENV \
    && uv pip install --no-cache -r requirements.txt
