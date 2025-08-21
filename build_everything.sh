#!/bin/bash

push ./demikernel
make init
make all-libs
sudo make install INSTALL_PREFIX=$INSTAL_PREFIX
./scripts/generate-config.sh
echo "copy config into $CONFIG_PATH"
pop

push ./demi_epoll
make build
sudo make install INSTALL_PREFIX=$INSTALL_PREFIX
pop

push ./node-24.0.0/
./configure --ninja --prefix=$INSTALL_PREFIX
make all
sudo make install
pop
