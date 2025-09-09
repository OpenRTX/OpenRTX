FROM ubuntu:24.04 AS build

# dependencies for miosix compiler
RUN apt-get update -y && apt-get install -y \
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
  && rm -rf /var/lib/apt/lists/*

# compile miosix compiler
RUN git clone --depth 1 https://github.com/fedetft/miosix-kernel.git \
  && cd miosix-kernel/miosix/_tools/compiler/gcc-9.2.0-mp3.2 \
  && sh download.sh \
  && bash install-script.sh -j`nproc`


# 2th stage build
FROM ubuntu:24.04

# copy arm-miosix-eabi compiler from build stage
COPY --from=build /opt /opt

# create symbolic links to compiler
RUN ln -s /opt/arm-miosix-eabi/bin/* /bin/

# dependencies for the project
RUN apt-get update -y && apt-get install -y \
  git \
  pkg-config \
  build-essential \
  meson \
  libsdl2-dev \
  libreadline-dev \
  dfu-util \
  cmake \
  libusb-1.0-0 \
  libusb-1.0-0-dev \
  codec2 \
  libcodec2-dev \
  wget \
  && rm -rf /var/lib/apt/lists/*

# build bin2sgl tool
RUN wget https://github.com/open-ham/OpenGD77/raw/refs/heads/main/tools/bin2sgl/bin2sgl/bin2sgl.c \
  && gcc bin2sgl.c -o bin2sgl \
  && mv bin2sgl /bin/bin2sgl \
  && chmod +x /bin/bin2sgl

# fix error from git with different users
RUN git config --system --add safe.directory '*'
