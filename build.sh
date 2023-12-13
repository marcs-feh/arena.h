#!/usr/bin/env sh

CC=gcc
CFLAGS='-O2 -pipe'

set -xe

$CC $CFLAGS test.c -o test.bin
./test.bin
