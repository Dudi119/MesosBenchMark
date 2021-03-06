#!/bin/bash
workingDir=$PWD
if ! echo $LIBRARY_PATH | grep -q "$workingDir/Third_Party/lib"; then
    export "LIBRARY_PATH=$LIBRARY_PATH:$workingDir/Third_Party/lib"
fi

if ! echo $LD_LIBRARY_PATH | grep -q "$workingDir/Third_Party/lib"; then
    export "LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$workingDir/Third_Party/lib"
fi

if ! echo $PATH | grep -q "$workingDir/Third_Party/bin"; then
    export "PATH=$PATH:$workingDir/Third_Party/bin"
fi


if ! echo $MESOS_WORK_DIR | grep -q "$workingDir/Third_Party/tmp/mesos"; then
    export MESOS_WORK_DIR=$workingDir/Third_Party/tmp/mesos-"$USER"
fi

if ! echo $CPATH | grep -q "$workingDir/Third_Party/include"; then
export "CPATH=$CPATH:$workingDir/Third_Party/include/"
fi
cmake -D 3RD_PARTY_INSTALL_STEP=True . && make && cmake -D COMPILATION_STEP=True . && make
