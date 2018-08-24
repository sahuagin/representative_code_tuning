#!/usr/bin/env bash
mkdir -p release
cd release
cmake -DUSE_THREADS=1 -DCMAKE_BUILD_TYPE=Release -G Ninja ..
