#!/usr/bin/env bash

for ((i=0; i<$1; i++)); do \
  $2 -k -f -p4 ./test.tar; \
  mv trace.bin trace.$i; \
done