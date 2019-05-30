##Starting up

Currently the code is really dirty and I have made some changes in the cmake files. So in order to run everything perfectly its better not to execute cmake again. 
You should have couchdb previously installed and some dependencies which concord mentions (see the README_original.md). Perform all the steps except building the concord.

You should have couchdb installed (http://docs.couchdb.org/en/stable/install/index.html) with the default settings as a local cluster and leave it in the admin party mode. 

```bash
cd ~/concord-bft/build/
make
```
It will throw some warnings because of libcurl library. Ignore them. (will try to fix later).


``` bash
cd ~/concord-bft/build/bftengine/tests/simpleTest/scripts
./testReplicasAndClient.sh
```

By default it will spin up the network start 4 replicas on the same machine (different ports) and a client which will generate some transactions (around 2800) and will invoke couchdb.