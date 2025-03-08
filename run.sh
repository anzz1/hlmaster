#!/bin/sh

if [ ! -f ./hlmaster ]; then
  echo "Compiling hlmaster ..."
  gcc -O3 -w -march=native -s -fvisibility=hidden -fno-strict-aliasing -fno-strict-overflow -fomit-frame-pointer -fno-stack-protector -fno-pie -no-pie -Wl,--no-export-dynamic hlmaster.c -o hlmaster
  chmod +x ./hlmaster
fi

echo "Starting hlmaster ..."
./hlmaster
