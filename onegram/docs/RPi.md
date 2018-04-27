# Rasperry Pi 3 (arm64v8) Environment

Using image available from [https://github.com/bamarni/pi64](https://github.com/bamarni/pi64)
which is using official Raspberry Pi patched kernel source tree from [https://github.com/raspberrypi/linux](https://github.com/raspberrypi/linux)

Boot, configure and update to the latest linux kernel (pi64-update).
<br/>
To check the running version:

    # uname -a
    Linux rpi64 4.11.12-pi64+ #1 SMP PREEMPT Sun Aug 27 14:50:58 CEST 2017 aarch64 GNU/Linux

# Rasperry Pi Development Environment

## Setup Arm64v8 Emulator on Build Host 

Install qemu, binfmt-support, and qemu-user-static:

    # apt-get install qemu binfmt-support qemu-user-static

Follow the readme from 
[https://github.com/computermouth/qemu-static-conf](https://github.com/computermouth/qemu-static-conf)
to activate qemu static configuration, which allows to execute arm64 binaries in your local environment.

## Prepare OneGram Build Host

Docker file consists of following steps:

- Use arm64v8/ubuntu:16.04 [https://hub.docker.com/r/arm64v8/ubuntu/](https://hub.docker.com/r/arm64v8/ubuntu/) as base
- Update system and install OneGram build prerequisites
- Clean temporary files


Build the docker image and store in repositary for later use (f.e. frankhorv/arm64v8-ubuntu-gcc).

## Build OneGram

Checkout OneGram source tree into OneGram project folder (f.e. /OneGramBuild/).
<br/>
Start arm64v8 docker, use volume switch to set the onegram project folder:

    # docker run -it -v /OneGramBuild/:/OneGramBuild/ frankhorv/arm64v8-ubuntu-gcc

Prepare ninja makefiles and build OneGram source code:

    # mkdir build64
    # cd build64
    # cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DGRAPHENE_EGENESIS_JSON="/OneGramBuild/OneGramDev/onegram/onegram-genesis.json" .
    # ninja

Deploy witness node binary, configuration and the genesis file to Raspberry Pi. 