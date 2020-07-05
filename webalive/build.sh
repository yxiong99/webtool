#!/bin/bash
#
ARG=$1
BASE=`pwd`
TARGET=webalive
SERVER=192.168.112.30

dns=$(cat /etc/resolv.conf | grep $SERVER)
if [ -z "$dns" ]; then
    sudo sed -i '1inameserver '$SERVER'' /etc/resolv.conf
fi
# Start build new target
if [ ! -e Makefile ]; then
    cmake .
fi
if [ "$ARG" = "clean" ]; then
    make clean
fi
make
# Replace with new target
pid=$(ps -e | grep $TARGET | awk '{print $1}')
if [ -n $pid ]; then
    kill -9 $pid > /dev/null 2>&1
fi
sudo mv $TARGET /usr/local/sbin/.

exit 0
