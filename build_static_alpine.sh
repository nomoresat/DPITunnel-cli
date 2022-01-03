#!/bin/ash

apk update
apk add build-base cmake openssl openssl-dev openssl-libs-static linux-headers
cmake -B./build -DCMAKE_BUILD_TYPE=RELEASE -DSTATIC_BINARY=true .
make -C ./build -j $(nproc)
