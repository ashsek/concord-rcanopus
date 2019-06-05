#!/bin/sh

echo "--- Creating database named global_membership_service ---"

curl -X PUT http://127.0.0.1:5984/global_membership_service

echo "--- Populating Couchdb with predefined values ---"

echo "Byzantine Groups"

curl -X POST http://127.0.0.1:5984/global_membership_service -d @- -# -o output -H "Content-Type: application/json" < byzantine_groups.json

echo "Emulators"

curl -X POST http://127.0.0.1:5984/global_membership_service -d @- -# -o output -H "Content-Type: application/json" < emulators.json

echo "Public Keys"

curl -X POST http://127.0.0.1:5984/global_membership_service -d @- -# -o output -H "Content-Type: application/json" < public_keys.json

echo "Quorum_size"

curl -X POST http://127.0.0.1:5984/global_membership_service -d @- -# -o output -H "Content-Type: application/json" < quorum_size.json