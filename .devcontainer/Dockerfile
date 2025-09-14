FROM ubuntu:24.04

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
  python3-pip \
  && rm -rf /var/lib/apt/lists/*

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

# Install opengd77 sources and put the firmware loader and tools in PATH
RUN git clone https://github.com/open-ham/OpenGD77.git /tmp/OpenGD77
ENV PATH="$PATH:/tmp/OpenGD77/tools/Python/FirmwareLoader:/tmp/OpenGD77/firmware/tools"

# Install Python 3.11 using the deadsnakes PPA
RUN apt-get update -y \
    && apt-get install -y software-properties-common \
    && add-apt-repository -y ppa:deadsnakes/ppa \
    && apt-get update \
    && apt-get install -y python3.11 python3.11-venv python3.11-dev \
    && rm -rf /var/lib/apt/lists/*

# Create and activate virtual environment
ENV VIRTUAL_ENV=/opt/venv
RUN python3.11 -m venv $VIRTUAL_ENV
ENV PATH="$VIRTUAL_ENV/bin:$PATH"

# Copy requirements.txt and install dependencies
WORKDIR /app
COPY requirements.txt .
RUN pip install --no-cache-dir -U pip && \
    pip install --no-cache-dir -r requirements.txt
