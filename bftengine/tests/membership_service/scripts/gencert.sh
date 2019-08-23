#!/bin/sh
set -e

toolsdir=$HOME/Projects/concord-rcanopus/tools
scriptdir=$(cd $(dirname $0); pwd -P)

echo "Generating new keys..."

rm -f private_replica_*

$toolsdir/GenerateConcordKeys -n 4 -f 1 -o private_replica_
