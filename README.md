## rcan

simple C CAN lib for : `unix` | `macos` | `windows` | `stm32`

supported hardware : `stm32 can`  `PCAN-USB` `PCAN-PCI` `SOCET CAN UNIX`

for install:

mac os - [https://gitlab.com/RoboticsHardwareSolutions/maccan](https://gitlab.com/RoboticsHardwareSolutions/maccan)
  
linux - [https://www.peak-system.com/fileadmin/media/linux/files/peak-linux-driver-8.10.2.tar.gz](https://www.peak-system.com/fileadmin/media/linux/files/peak-linux-driver-8.10.2.tar.gz)
linux socket can (use README) :

```
$ sudo apt-get install  libsocketcan-dev   
$ sudo su    
$ echo peak_pci >> /etc/modules-load.d/peak_pci.conf
$ echo peak_usb >> /etc/modules-load.d/peak_usb.conf
$ echo vcan >> /etc/modules-load.d/vcan.conf

```


linux peak can proprietary (recomended for using pcanview) - [https://www.peak-system.com/fileadmin/media/linux/files/peak-linux-driver-8.10.2.tar.gz](https://www.peak-system.com/fileadmin/media/linux/files/peak-linux-driver-8.10.2.tar.gz)
install using manual [https://www.peak-system.com/fileadmin/media/linux/files/PCAN-Driver-Linux_UserMan_eng.pdf](https://www.peak-system.com/fileadmin/media/linux/files/PCAN-Driver-Linux_UserMan_eng.pdf)

```
$ sudo su
$ echo pcan >> /etc/modules-load.d/pcan.conf
```

Download and install the following file peak-system.list from the PEAK-System website:

```
$ wget -q http://www.peak-system.com/debian/dists/`lsb_release -cs`/peak-system.list -O- | sudo tee /etc/apt/sources.list.d/peak-system.list
```

Note: If the lsb_release tool is not installed on your Linux system then replace `lsb_release -cs` by the name of your Linux distribution. For example:

```
$ wget -q http://www.peak-system.com/debian/dists/wheezy/peak-system.list -O- | sudo tee /etc/apt/sources.list.d/peak-system.list
```

Then, download and install the PEAK-System public key for apt-secure, so that the repository is trusted:

```
$ wget -q http://www.peak-system.com/debian/peak-system-public-key.asc -O- | sudo apt-key add -

```
To install pcanview-ncurses next, do:

```
$ sudo apt-get update
$ sudo apt-get install pcanview-ncurses
```

after install PCAN-Basic API for linux [https://www.peak-system.com/quick/BasicLinux](https://www.peak-system.com/quick/BasicLinux)

```
$ cd libpcanbasic/pcanbasic
$ make 
$ sudo make install 
```

windows - [https://www.peak-system.com/fileadmin/media/files/PEAK-System_Driver-Setup.zip](https://www.peak-system.com/fileadmin/media/files/PEAK-System_Driver-Setup.zip)   
STM32 - just use last HAL   
 
for start using:
  
`$ cd ~/your_project_root_directory`    
`$ git submodule add git@gitlab.com:RoboticsHardwareSolutions/rlibs/rcan.git`   
 
OR   
`$ cd ~/your_project_root_directory`  
`$ mkdir rcan`  
after copy content of repository to folder rcan   


For using in CMake project add in CMakeLists.txt next string 

```
include(rcan/librcan.cmake)

include_directories( ... ${RCAN_DIRECTORIES})

add_executable(... ${RCAN_EXECUTABLE_FILES})
```

next string need only for UNIX WINDOWS MACOS platforms:

```
target_link_libraries(....  ${LIBRARIES}) 
```


## example :
 
```
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include "stdbool.h"
#include "rcan.h"


volatile bool quit = false;

rcan can;

rcan_frame frame = {.id = 123, .type = std_id, .len = 5,
        .payload = {0x01, 0x02, 0x03, 0x04, 0x05}};


void sigterm(int signo) {
    quit = true;
}

int main(int argc, char *argv[]) {

    if ((signal(SIGTERM, sigterm) == SIG_ERR) ||
        (signal(SIGINT, sigterm) == SIG_ERR)) {
        perror("Error");
        return errno;
    }

    rcan_start(&can, PCAN_USBBUS1, 1000000);
    rcan_send(&can, &frame);

    while (!quit) {

        if (rcan_receive(&can, &frame))
            rcan_view_frame(&frame);

        if (!rcan_is_ok(&can))
            quit = true;
    }
    return 0;
}
```

