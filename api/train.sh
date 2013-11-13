#!/usr/bin/env bash

for ((i=0; i<$1; i++)); do \
  $2; \
  mv trace.bin trace.$i; \
  mv whitelist.mem whitelist.mem.$i; \
done