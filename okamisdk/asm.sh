#!/bin/bash

path=$(dirname $0)

luajit ${path}/asmfx/asmfx.lua "$@"
