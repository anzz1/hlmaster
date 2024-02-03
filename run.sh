#!/bin/sh

if [ ! -f ./hlmaster ]; then
  echo "Compiling hlmaster ..."
  gcc -O2 -w -march=native -fvisibility=hidden hlmaster.c -o hlmaster
  chmod +x ./hlmaster
fi

echo "Starting hlmaster ..."
./hlmaster
