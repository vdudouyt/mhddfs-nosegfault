#!/bin/bash

dir1=`mktemp -d`
dir2=`mktemp -d`
mnt=`mktemp -d`
log=`mktemp`

ret=0

cleantemp() {
    rm -fr $dir1 $dir2 $mnt $log
}

mkdir -p $dir1/1/{2,3,4}/{5,6,7}
cwd=`pwd`
cd $dir1/1 
find -type d  -exec ln -s '{}' '{}link' ';'
cd $cwd

sleep 1

./mhddfs $dir1 $dir2 $mnt

rsync -a $mnt/1 $mnt/2 2> $log

logrsync=`cat $log`

fusermount -u $mnt


if test -z "$logrsync"; then
    echo "**************** PASSED ******************"
    cleantemp
    exit 0
else
    echo "FAILED:"
    cat $log
    cleantemp
    exit -1
fi
