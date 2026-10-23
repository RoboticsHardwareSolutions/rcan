#!/bin/bash

# Sets up two vcan interfaces bridged together via the Linux CAN_GW kernel
# gateway (`cangw`, from can-utils), so tests can exercise frames crossing
# from one CAN segment to another instead of only looping back on themselves.
IFACE1=vcan0
IFACE2=vcan1

exitcode=1

create_and_up() {
    local name=$1
    if ! ip link show dev "$name" > /dev/null 2>&1; then
        sudo ip link add dev "$name" type vcan
    fi
    sudo ip link set "$name" up
}

case $1 in
start)
    sudo modprobe vcan 2> /dev/null
    sudo modprobe can-gw 2> /dev/null
    create_and_up $IFACE1
    create_and_up $IFACE2

    # bridge the two buses bidirectionally: a frame sent on one interface is
    # echoed onto the other, simulating two CAN segments joined by a gateway.
    sudo cangw -A -s $IFACE1 -d $IFACE2 -e
    sudo cangw -A -s $IFACE2 -d $IFACE1 -e

    if ip link show $IFACE1 > /dev/null 2>&1 && ip link show $IFACE2 > /dev/null 2>&1; then
        echo "can_gateway up: $IFACE1 <-> $IFACE2"
        exitcode=0
    else
        echo "ERROR: failed to bring up $IFACE1/$IFACE2"
    fi
    ;;
stop)
    sudo cangw -F -s $IFACE1 -d $IFACE2 -e 2> /dev/null
    sudo cangw -F -s $IFACE2 -d $IFACE1 -e 2> /dev/null
    sudo ip link delete $IFACE1 2> /dev/null
    sudo ip link delete $IFACE2 2> /dev/null
    exitcode=0
    ;;
esac
exit $exitcode
