#!/bin/bash

sudo sysctl -w kern.maxfiles=5000000
sudo sysctl -w kern.maxfilesperproc=5000000
sudo sysctl -w kern.ipc.somaxconn=100000
sudo sysctl -w net.inet.ip.portrange.first=1024
sudo sysctl -w net.inet.ip.portrange.hifirst=1024
sudo sysctl -w net.inet.tcp.msl=1000

sudo ulimit -n 5000000