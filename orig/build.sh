#!/usr/bin/env bash
mkdir -p debug
cd debug
cmake -USE_PTHREADS -DCMAKE_BUILD_TYPE=Debug -G Ninja ..
