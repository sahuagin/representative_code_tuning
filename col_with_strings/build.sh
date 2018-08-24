#!/usr/bin/env bash
mkdir -p debug
cd debug
cmake -DUSE_THREADS=1 -DCMAKE_BUILD_TYPE=Debug -G Ninja ..
