#Starting up

You should have some dependencies installed which concord needs (see the README_original.md). Perform all the steps except building the concord.

You should have couchdb installed (http://docs.couchdb.org/en/stable/install/index.html) with the default settings as a single node and leave it in the admin party mode. 

``` bash
mkdir build
cd build/
cmake ..
```
In the same build directory,
```bash
make
```
It will throw some warnings because of libcurl library. Ignore them. (will try to fix later).

The test script can be found in,
``` bash
cd ~/concord-rcanopus/build/bftengine/tests/simpleTest/scripts
./testReplicasAndClient.sh
```

By default it will spin up the network start 4 replicas on the same machine (different ports) and a client which will generate some transactions (around 2800) and will invoke couchdb.

To check if everything is working goto:
```web
http://127.0.0.1:5984/_utils/
```
There should be a database named "test3" which should contain some documents.