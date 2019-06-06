#!/bin/sh
set -e

toolsdir=/home/a2sekhar/concord-rcanopus/build/tools
scriptdir=$(cd $(dirname $0); pwd -P)

echo "Generating new keys..."

rm -f private_replica_*

$toolsdir/GenerateConcordKeys -n 4 -f 1 -o private_replica_