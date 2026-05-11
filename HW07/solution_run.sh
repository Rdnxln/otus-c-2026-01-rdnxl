#!/bin/bash

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./lerr
[ -x ./solution ] && ./solution
#cat ./errors.log