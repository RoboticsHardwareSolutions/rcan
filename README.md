## rcan

simple C CAN lib for : `unix` | `macos` | `windows` | `stm32`

supported hardware : `stm32 can`  `PCAN-USB` `PCAN-PCI` `SOCET CAN UNIX`

for install:

mac os - [https://gitlab.com/RoboticsHardwareSolutions/maccan](https://gitlab.com/RoboticsHardwareSolutions/maccan)  
linux - [https://www.peak-system.com/fileadmin/media/linux/files/peak-linux-driver-8.10.2.tar.gz](https://www.peak-system.com/fileadmin/media/linux/files/peak-linux-driver-8.10.2.tar.gz)
linux socket can -  `$sudo apt-get install  libsocketcan-dev`   
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

rcan_frame frame = {.id = 123, .type = std, .len = 5,
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

