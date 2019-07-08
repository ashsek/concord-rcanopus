#!/bin/sh

echo "____________________________________________________________________"
echo "============= Warning: This script is for ubnuntu 16 ==============="
echo "____________________________________________________________________"

sudo apt-get update
sudo apt-get install cmake clang clang-format
sudo apt-get install libgmp3-dev
sudo apt-get install parallel
sudo apt-get install g++
sudo apt-get install autoconf automake
sudo apt-get install libcurl4-gnutls-dev
sudo apt-get install apt-transport-https
sudo apt-get install build-essential autoconf libtool pkg-config
sudo apt-get install libgflags-dev libgtest-dev
sudo apt-get install clang libc++-dev

echo "______________________________________________"
echo "============= Installing relic ==============="
echo "______________________________________________"

cd
git clone https://github.com/relic-toolkit/relic
cd relic/
git checkout b984e901ba78c83ea4093ea96addd13628c8c2d0
mkdir -p build/
cd build/
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DALLOC=AUTO -DWSIZE=64 -DRAND=UDEV -DSHLIB=ON -DSTLIB=ON -DSTBIN=OFF -DTIMER=HREAL -DCHECK=on -DVERBS=on -DARITH=x64-asm-254 -DFP_PRIME=254 -DFP_METHD="INTEG;INTEG;INTEG;MONTY;LOWER;SLIDE" -DCOMP="-O3 -funroll-loops -fomit-frame-pointer -finline-small-functions -march=native -mtune=native" -DFP_PMERS=off -DFP_QNRES=on -DFPX_METHD="INTEG;INTEG;LAZYR" -DPP_METHD="LAZYR;OATEP" ..
make
sudo make install

echo "______________________________________________"
echo "=============Installing cryptopp============="
echo "______________________________________________"

cd
git clone https://github.com/weidai11/cryptopp.git
cd cryptopp/
git checkout CRYPTOPP_5_6_5;
mkdir build/
cd build/
cmake ..
make
sudo make install

echo "______________________________________________"
echo "=============Installing couchdb============="
echo "______________________________________________"

echo "deb https://apache.bintray.com/couchdb-deb xenial main" | sudo tee -a /etc/apt/sources.list
curl -L https://couchdb.apache.org/repo/bintray-pubkey.asc    | sudo apt-key add -
sudo apt-get update
sudo apt-get install couchdb

echo "______________________________________________"
echo "=============Installing Go ==================="
echo "______________________________________________"

cd
wget https://dl.google.com/go/go1.12.6.linux-amd64.tar.gz
tar -C /usr/local -xzf go1.12.6.linux-amd64.tar.gz
export PATH=$PATH:/usr/local/go/bin

echo "______________________________________________"
echo "=============Installing gRPC (go)============="
echo "______________________________________________"

go get -u google.golang.org/grpc
go get -u github.com/golang/protobuf/protoc-gen-go
export PATH=$PATH:$GOPATH/bin

echo "______________________________________________"
echo "=============Installing gRPC (c++)============"
echo "______________________________________________"
cd
git clone -b $(curl -L https://grpc.io/release) https://github.com/grpc/grpc
cd grpc
git submodule update --init
CXXFLAGS='-Wno-error' make
make install

echo "______________________________________________"
echo "============ Concord-RCanopus     ============"
echo "______________________________________________"
cd
git clone https://github.com/ashsek/concord-rcanopus
mkdir -p concord-rcanopus/build
cd concord-rcanopus/build
cmake ..
make