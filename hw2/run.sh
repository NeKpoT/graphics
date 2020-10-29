#!/bin/bash

set -e
set -x

cd build

if [ $1 = "-d" ]
then
    gdb ./opengl-imgui-sample
else
    ./opengl-imgui-sample
fi