#!/bin/bash

set -e

LEVEL=9
CFLAGS="-fno-stack-protector -z execstack -mpreferred-stack-boundary=4"
: ${DIR=/var/challenge/level$LEVEL}
cc $CFLAGS $DIR/$LEVEL.c $DIR/mymalloc.c -o $LEVEL
