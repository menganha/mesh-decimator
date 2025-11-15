#/bin/bash

if [ -z "$1" ] 
then 
    build/release/decimator models/teddy.obj
elif [ "$1" == "profile" ]
then 
    mkdir -p .callgrind
    valgrind --callgrind-out-file=.callgrind/callgrind.out.%p --tool=callgrind build/debug/decimator models/teddy.obj
else 
    echo "Unknow argument: $1"
    exit 1
fi
