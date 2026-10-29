#!/usr/bin/bash

ARG=$1

set -e

(
    set -x
    cmake -B build/linux_86_64 -DCMAKE_BUILD_TYPE=Debug -Wno-deprecated
    cmake --build build/linux_86_64
)

if [ -z $ARG ]; then
    :
elif [ $ARG == "run" ]; then
    (
        set -x
        ./build/linux_86_64/cgf
    )
else
    echo "[ERROR] unknown command \"$ARG\""
fi
