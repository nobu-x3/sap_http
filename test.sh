#!/bin/bash

cmake -B build -G "Ninja"
(cd build && ninja)
ctest --test-dir build --output-on-failure