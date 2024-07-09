#!/bin/bash

echo "Running make properclean..."
make properclean

echo "Running make -j 4..."
make -j 4

echo "Running aarch64-linux-gnu-objcopy..."
aarch64-linux-gnu-objcopy -O binary build/app-helloworld_kvm-arm64 build/app-helloworld_kvm-arm64.bin

if [ $? -eq 0 ]; then
    echo "Build and conversion successful!"
else
    echo "An error occurred during the build or conversion process."
    exit 1
fi
