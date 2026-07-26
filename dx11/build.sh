#!/bin/sh

zig c++ main.cpp -target x86_64-windows -O2 -ld3d11 -ldxgi -luser32 -lgdi32 -o triangle.exe
