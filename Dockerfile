FROM ubuntu:22.04

LABEL "RHS"="Robotics Hardware Solutions"
LABEL description="Image for rcan lib environment"
LABEL maintainer="doc.pillman@icloud.com"

RUN apt-get update -y

RUN apt-get install -y linux-headers-generic

RUN apt-get install -y --no-install-recommends\
    iproute2\
    sudo\
    cmake\
    git\
    libnng-dev\
    libnanomsg-dev\
    libusb-1.0-0\
    libusb-1.0-0-dev\
    make\
    ssh\
    nano\
    libsocketcan-dev\
    can-utils\
    build-essential\
    libpopt-dev \
    g++\
    gcc\
    clang-tidy\
    openssh-server\
    gdb\
    clang-format\
    openssh-server\
    wget\
    kmod\
    unzip\
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /rcan


COPY .  /tmp

