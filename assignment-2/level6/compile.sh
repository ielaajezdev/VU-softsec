#!/bin/bash

set -e

LEVEL=6
CFLAGS="-fno-stack-protector -z execstack -mpreferred-stack-boundary=4"
: ${DIR=/var/challenge/level$LEVEL}
c++ $CFLAGS $DIR/$LEVEL.cpp $DIR/cooking.cpp -o $LEVEL
