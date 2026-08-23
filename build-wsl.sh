#!/bin/bash
# Build scsidayna.device with vbcc under WSL (local setup - not part of upstream).
# The ndk-fix dir shims bebbo-NDK devices/timer.h (anonymous unions) for vbcc C90.
export VBCC=$HOME/vbcc
export PATH=$VBCC/bin:$PATH
make IPATH="-Indk-fix -I/opt/amiga/m68k-amigaos/ndk-include" "$@"
