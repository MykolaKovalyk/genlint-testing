FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Etc/UTC

ARG ZEPHYR_WORKSPACE=/opt/zephyrproject
ARG ZEPHYR_REVISION=v4.1.0
ARG USERNAME=me
ARG USER_UID=1000
ARG USER_GID=$USER_UID

# Install essentials
RUN : \
  && apt-get update \
  && apt-get install -y \
    sudo \
  && :

# Zephyr dependencies
RUN : \
  && apt-get update \
  && apt-get install --no-install-recommends -y \
  git \
  cmake \
  ninja-build gperf \
  ccache \
  dfu-util \
  device-tree-compiler \
  wget \
  python3-dev \
  python3-pip \
  python3-venv \
  python3-setuptools \
  python3-tk \
  python3-wheel \
  xz-utils \
  file \
  make \
  gcc \
  gcc-multilib \
  g++-multilib \
  libsdl2-dev \
  libmagic1 \
  && :

# Create user
RUN : \
  && groupadd --gid $USER_GID $USERNAME \
  && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME \
  && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
  && chmod 0440 /etc/sudoers.d/$USERNAME \
  && :

USER $USERNAME

# Create Zephyr workspace
RUN : \
  && sudo mkdir -p $ZEPHYR_WORKSPACE \
  && sudo chown $USER_UID:$USER_GID $ZEPHYR_WORKSPACE \
  && python3 -m venv $ZEPHYR_WORKSPACE/.venv \
  && cd $ZEPHYR_WORKSPACE \
  && . .venv/bin/activate \
  && pip install --upgrade \
    pip \
    west \
  && west init $ZEPHYR_WORKSPACE --mr $ZEPHYR_REVISION \
  && west update \
  && west zephyr-export \
  && west packages pip --install \
  && :

# Install zephyr-sdk
RUN : \
  && cd $ZEPHYR_WORKSPACE \
  && . $ZEPHYR_WORKSPACE/.venv/bin/activate \
  && cd zephyr \
  && west sdk install \
  && :

# Install additional convenience tools
RUN : \
  && sudo apt-get update \
  && sudo apt-get install -y \
    build-essential \
    software-properties-common \
    ssh-client \
    gdb \
    wget \
    git \
    vim \
    fish \
  && :

# Clangd
ARG DEV_LLVM_TC_VERSION=19
RUN : \
  && wget --quiet -O - https://apt.llvm.org/llvm-snapshot.gpg.key | sudo apt-key add - \
  && sudo apt-add-repository "deb http://apt.llvm.org/jammy/ llvm-toolchain-jammy-${DEV_LLVM_TC_VERSION} main" --no-update \
  && sudo apt-get update \
  && sudo apt-get install -y clangd-${DEV_LLVM_TC_VERSION} \
  && :

# For a working menuconfig
ENV LC_ALL=C

# Set ZEPHYR_BASE environment variables
ENV ZEPHYR_BASE=$ZEPHYR_WORKSPACE/zephyr
