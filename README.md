# Global Membership Service (RCanopus)
The following document first introduces the global membership service and then describes the steps required to set up the global membership service and later explains the features in the API.

## Introduction

Global Membership Service is a byzantine fault tolerent membership service for the [RCanopus](https://arxiv.org/abs/1810.09300) consensus protocol. It achieves byzantine fault tolerance by leveraging the features provided by [Concord-BFT](https://github.com/vmware/concord-bft), a generic state machine replication library that can handle malicious (byzantine) replicas.

Global Membership Service is provided by a set of Byzantine Group (BG) leaders, each elected from among the monitors of every live BG. It helps in achieving consensus on a global scale for:

  * obtaining the set of emulators for a vnode (virtual node) at the BG level or higher.
  * obtaining the public keys for all the monitors in a Byzantine Group.
  * obtaining the monitors for a Super Leaf (SL).
  * obtaining the byzantine group leaders for a Byzantine group.
  * learning the quorum size in each BG (the number of signatures in each Byzantine Group’s quorum
    certificate that is necessary to validate it) during each consensus cycle.

Note: In Global Membership Service, the Byzantine Group leaders must be hosted in different cloud providers, i.e., making them more diverse, which further makes them as resilient as possible. Thus if there is a security breach in one cloud, it is safe to assume that others won't be affected because of it.


## Build (Ubuntu Linux 18.04)
This method will also work for Ubuntu 16.04
### Setup
For setting up the system on Ubuntu 16.04 (xenial) you can directly run,
```bash
cd concord-rcanopus/bftengine/tests/membership_service/scripts/
./setup.sh
```
For Ubuntu 18.04 replace line 51 in setup.sh with,
```bash
echo "deb https://apache.bintray.com/couchdb-deb bionic main" | sudo tee -a /etc/apt/sources.list
```
In ideal conditions, this script should install all the dependencies for the global membership service and will build it at the end. If for some reason the script fails to install the requirements, follow the steps given below.

### Dependencies

CMake and clang:
```bash
sudo apt-get install cmake clang clang-format
```
Get GMP (dependency for [RELIC](https://github.com/relic-toolkit/relic)):
```bash
sudo apt-get install libgmp3-dev
```
Build and install [RELIC](https://github.com/relic-toolkit/relic)
```bash
cd
git clone https://github.com/relic-toolkit/relic
cd relic/
git checkout b984e901ba78c83ea4093ea96addd13628c8c2d0
mkdir -p build/
cd build/
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DALLOC=AUTO -DWSIZE=64 -DRAND=UDEV -DSHLIB=ON -DSTLIB=ON -DSTBIN=OFF -DTIMER=HREAL -DCHECK=on -DVERBS=on -DARITH=x64-asm-254 -DFP_PRIME=254 -DFP_METHD="INTEG;INTEG;INTEG;MONTY;LOWER;SLIDE" -DCOMP="-O3 -funroll-loops -fomit-frame-pointer -finline-small-functions -march=native -mtune=native" -DFP_PMERS=off -DFP_QNRES=on -DFPX_METHD="INTEG;INTEG;LAZYR" -DPP_METHD="LAZYR;OATEP" ..
make
sudo make install
```
Build and install [cryptopp](https://github.com/weidai11/cryptopp)
```bash
cd
git clone https://github.com/weidai11/cryptopp.git
cd cryptopp/
git checkout CRYPTOPP_5_6_5;
mkdir build/
cd build/
cmake ..
make
sudo make install
```
Get GNU Parallel
```bash
sudo apt-get install parallel
```
Get g++:
```bash
sudo apt-get install g++
```
#### log4cplus

We have simple console logger but if you wish to use log4cplus - we have an
infra that supports it.

Follow below steps for installing this library:
1. Install prerequisites:

```bash
    sudo apt-get install autoconf automake
```

2. Clone the repository:

```bash
    git clone https://github.com/log4cplus/log4cplus.git
```

3. Move to the extracted directory and checkout the appropriate branch:

```bash
	cd log4cplus
    git checkout REL_1_2_1
```

4. Edit `configure` to change "am__api_version" from 1.14 to 1.15, the
version that ubuntu 16.04 supports.

5. Configure/make/install
```bash
	./configure CXXFLAGS="--std=c++11"
	make
	sudo make install
```

Configuring with these flags is important. If log4cplus is build without `c++11` then athena will give linker errors while building.

At this point all library files and header files should be installed into `/usr/local`. (You may need to add `/usr/local/lib` to your `LD_LIBRARY_PATH`).
You may also need to export CPLUS_INCLUDE_PATH variable set to /usr/local/include for the header files.

After installation, set USE_LOG4CPP flag to TRUE in the main CmakeLists.txt . The library doesn't initialize the log4cpp subsystem, including formats and appenders, it expects that the upper level application will do it and the log4cpp subsystem is already initialized.

#### Couchdb
```bash
sudo apt-get install apt-transport-https
echo "deb https://apache.bintray.com/couchdb-deb {distribution} main"   | sudo tee -a /etc/apt/sources.list
```
replace `{distribution}` with the appropriate choice for your OS version:

Ubuntu 16.04: `xenial`
Ubuntu 18.04: `bionic`

	curl -L https://couchdb.apache.org/repo/bintray-pubkey.asc | sudo apt-key add -
	sudo apt-get update
	sudo apt-get install couchdb


#### gRPC (Go)
gRPC requires Go 1.6 or higher. You must have go(v1.6+) installed for gRPC to work. To install Go follow the [link](https://golang.org/doc/install#install)

```bash
go get -u google.golang.org/grpc
```

Install Protocol Buffers v3
Install the protoc compiler that is used to generate gRPC service code. The simplest way to do this is to download pre-compiled binaries for your platform(protoc-\<version>-\<platform>.zip) from here: https://github.com/google/protobuf/releases

Unzip this file.
Update the environment variable PATH to include the path to the protoc binary file.

Next, install the protoc plugin for Go,
```bash
go get -u github.com/golang/protobuf/protoc-gen-go
```
The compiler plugin, protoc-gen-go, will be installed in $GOBIN, defaulting to $GOPATH/bin. It must be in your $PATH for the protocol compiler, protoc, to find it.


	export PATH=$PATH:$GOPATH/bin


#### gRPC (C++)

```bash
sudo apt-get install build-essential autoconf libtool pkg-config
sudo apt-get install libgflags-dev libgtest-dev
sudo apt-get install clang libc++-dev
```

Cloning the gRPC repository,
```bash
git clone -b $(curl -L https://grpc.io/release) https://github.com/grpc/grpc
cd grpc
git submodule update --init
```

from the gRPC root directory run,
```bash
CXXFLAGS='-Wno-error' make
sudo make install
```
The last thing membership service depends upon is curl.h which can be installed via,

```bash
sudo apt-get install libcurl4-gnutls-dev
``` 

### Building the membership serivce

For making the binaries, first we need to create a build directory which will store the binaries.

```bash
cd concord-rcanopus
mkdir build
cd build
```
execute the following in the build directory to perform a default build,

```bash
cmake ..
make
```

## Run
The scripts are compiled into the build directory, and the binaries for the membership service can be found at,
It assumes that you are inside `concord-rcanopus` directory.
```bash
cd build/bftengine/tests/membership_service
```
In this directory, you'll find the client and server binaries which must be run from the `scripts` directory, else it wont be able to find the replica key files.

In the `scripts` directory, you'll find:
  
  * `couchdb_scripts`, contains a script `populate.sh` which makes use of `emulators.json`, `public_keys.json`,
    `byzantine_groups.json`,  `configuration.json`, `quorum_size.json` and,  `super_leafs.json`. This script   
    is used to bootstrap the couchdb and must be executed on all the replicas before starting them.

  * `go_client`, contains a directory `membership_client` which further contains go scripts, which can be used to test the network and contains sample API requests for reference.

  * `private_replica_{0,1,2,3}`, are the key files for each
    replica. The key files are generated from the script `gencert.sh`. The example is for four replicas, which supports an F=1 environment. The keyfile format used is the same as that output by the `GenerateConcordKeys` tool in the `concord-rcanopus/tools` folder; see `concord-rcanopus/tools/README.md` for more information on how
    these keyfiles are generated and used.

  * `setup.sh`, which installs all the dependencies of the membership service.

  * `sample_config.txt`, which contains the network configuration in the yaml format and must be changed when deploying it on a network.

### Deploying the network
The futher steps assume that you are at `~/concord-rcanopus/build/bftengine/tests/membership_service/scripts`

1. First we need to generate the replica key files,
	
		./gencert.sh

2. For deploying the service on a cluster or different machines there is a need to change the configuration file `sample_config.txt` under the `scripts` directory.

3. Modify the json files under `couchdb_scripts` to store the data relevant to the network. Then for each replica we need to bootstrap the couchdb which is done by,

		cd couchdb_scripts
		./populate.sh

4. Run the replicas,

		../server {i} (i = 0,1,2,3)
   
   If the following message appears then the deployment was successful,
		
		INFO - Node k received ReplicaStatusMsg from node i
   for all k in {0,1,2,3} and i in {{0,1,2,3} - k}

5. Run the client,

		../client
   
   The client will start a gRPC server and will wait for gRPC requests.

6. Use the go scripts to test the network,

		go run go_client/membership_client/read_operations.go
		go run go_client/membership_client/write_operations.go

## API Reference
The following section explains the vairous requests to which the global membership service responds. Membership service expects a json string which contains a "Mode", which tells it what operation to perform and other parameters as described below.

### Read Requests

#### Quorum Size
`"{"Mode": "quorum_size", "BGID": "0"}"`

It is used to return the quorum size for a specific Byzantine Group, It returns the cycle number, quorum size, and quorum_cert in the form of a JSON structured as,
```json
{
	"cycle": "3",
	"quorum_cert": "cert",
	"size": "4" 
} 
```
quorum_cert is a proof that the information is valid. This certificate is signed by the super majority of nodes in a byzantine group (BGID).

#### Public Keys
`"{"Mode": "public_keys", "BGID": "0"}"`

It is used to return public keys for all the monitors in a specific Byzantine Group. It returns the pairs <ip:port, public key> in the form of a JSON structured as,
```json
{
	"ip1:port1": "pk_1",
	"ip2:port2": "pk_2",
	"certificate_msp": "certificate" 
} 
```
Here the certificate is provided by the Certificate Authorithy (CA), a proof that these public keys are valid.


#### Byzantine Groups
`"{"Mode": "byzantine_groups", "BGID": "0"}"`

It is used to querty the leader's IP address for a byzantine group which is being refered by "BGID".

```json
{
	"cert": "cert",
	"leader_ip": "ip1:port1" 
} 
```
Here the certificate is signed by all the Byzantine Group leaders agreeing upon that the leader_ip is genuine and live.


#### Emulators
`"{"Mode": "emulators", "SLID": "0", "height":"h"}"`

It used to query the emulators for a virtual node which is at the height "h" and belongs to super leaf "SLID".

It returns all the parts of the "Leaf Only Tree" of which the node is not aware of, along with their respecitve quorum certificates.

```json
{
	"4": {
		"cert": "certificate",
		"ip": [
			"ip1:port1",
			".",
			"." 
		] 
	},
	"5": {
		"cert": "certificate",
		"ip": [
			"ip2:port2",
			 ".",
			 "."
		] 
	},
	"6": {
		"cert": "certificate",
		"ip": [
			"ip4:port4",
			".",
			"." 
		] 
	},
	"7": {
		"cert": "certificate",
		"ip": [
			"ip5:port5",
			".",
			"."
		] 
	} 
} 

```

#### Super Leafs
`"{"Mode": "super_leafs", "SLID": "0"}"`
It is used to query the IP address of monitor for the super leaf referenced by SLID.

```json
{
	"cert": "certificate",
	"monitor_ip": "ip1:port1" 
} 
```
### Write Requests
All the write requests return a global_quorum_certificate which is a proof that the request was successfuly carried out.

#### Quorum Size
`"{"Mode": "quorum_size", "BGID":"0", "size":"6", "quorum_cert": "cert" , "cycle":"3"}"`
After successful verification* of the quorum_cert, it updates the quorum size and cycle for a byzantine group referenced by "BGID" while also storing the certificate.

#### Public Keys
`{"Mode": "public_keys", "BGID":"0", "keys": {"ip1:port1": "pk_1", "ip2:port2": "pk_2", "certificate_msp":"certificate"}}`
After successful verification* of the certificate provided by msp, it updates the public keys for all the monitors emulating the byantine group referenced by BGID. 

#### Byzantine Groups
`{"Mode": "byzantine_groups", "BGID": "0", "IP": "ip1:port1"}`
It is used to update the byzantine group leader which further should result in changes in the membership service. Currently, Concord only supports static network configuration, hence this is of less importance.

#### Emulators
`{"Mode": "emulators", "SLID": "0", "ip": ["ip1:port1","ip2:port2", "ip3:port3"], "cert": "cert"}`
After successful verification* of the quorum_cert, It updates the list of emulators for a super leaf referenced by SLID.

#### Super leafs
`{"Mode": "super_leafs", "SLID": "0", "IP": "ip1:port1", "cert":"cert"}`
After successful verification* of the quorum_cert, it updates the monitors ip address for a super leaf referenced by SLID.

\* Currently the certificates are not being verified.

### Configuration file

In addition to these requests, all the byzantine group leaders store the network topology in a configuration file which is used to query the emulators. Since, the network topology will rarely change, all the changes in the configuration file are to be made by hand.