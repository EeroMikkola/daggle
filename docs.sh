#!/bin/bash

cmake -B build -S . -DCMAKE_INSTALL_PREFIX=daggle-install
cmake --build build -j --target docs