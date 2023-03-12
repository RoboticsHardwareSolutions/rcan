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
    uuid-dev\
    clang-tidy\
    libzmq3-dev\
    openssh-server\
    gdb\
    clang-format\
    openssh-server\
    catch2\
    wget\
    kmod\
    unzip\
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*



##RUN modprobe vcan
#
#RUN apt-get install -y linux-modules-extra

WORKDIR /rcan

RUN wget  https://www.peak-system.com/fileadmin/media/linux/files/peak-linux-driver-8.15.2.tar.gz --no-check-certificate

RUN tar -xzf peak-linux-driver-8.15.2.tar.gz \
    && cd peak-linux-driver-8.15.2 \
    && make KERNEL_LOCATION=/lib/modules/5.15.0-67-generic/build/ \
    && make KERNEL_LOCATION=/lib/modules/5.15.0-67-generic/build/ install -C libpcanbasic


#    && make KERNEL_LOCATION=/where/are/the/kernel/headers \
#    && make clean \
#    && make -C libpcanbasic \
#    && make install





#RUN wget https://gitlab.com/RoboticsHardwareSolutions/rlibs/rcan/-/archive/add_zeromq_canbus/rcan-add_zeromq_canbus.zip --no-check-certificate \
#    && unzip rcan-add_zeromq_canbus.zip \
#    && rm rcan-add_zeromq_canbus.zip
#
#WORKDIR /rcan-add_zeromq_canbus
#
#RUN cd test\
#    && mkdir build\
#    && cd build\
#    && cmake ..\
#    && make test_public_x64_c11 \
#    && sudo ./test_public_x64_c11

COPY .  /tmp
#ENTRYPOINT [ "python2.7", "/co_objdictgen-master/objdictedit.py" ]
