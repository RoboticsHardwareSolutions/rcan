FROM ubuntu:22.04

LABEL "RHS"="Robotics Hardware Solutions"
LABEL description="Image for rcan lib tests"
LABEL maintainer="doc.pillman@icloud.com"

RUN apt-get update

RUN apt-get install -y --no-install-recommends\
    cmake\
    make\
    libsocketcan-dev\
    build-essential\
    clang-tidy\
    clang-format\
    catch2\
    wget\
    unzip\
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*


RUN wget https://gitlab.com/RoboticsHardwareSolutions/rlibs/rcan/-/archive/master/rcan-master.zip --no-check-certificate \
    && unzip rcan-master.zip \
    && rm rcan-master.zip

WORKDIR /rcan-master

RUN cd test \
    && mkdir build \
    && cd build \
    && cmake .. \
    && make test_public_x64_c11 \
    && sudo ./test_public_x64_c11


#ENTRYPOINT [ "python2.7", "/co_objdictgen-master/objdictedit.py" ]
