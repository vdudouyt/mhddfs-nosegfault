#!/bin/bash

PATH=/sbin:$PATH

set -x
TMP_PATH="${1:-/tmp}"

image1="test1.xfs.img"
image2="test2.xfs.img"
mount="mnt.xfs"
mount1="test1.xfs"
mount2="test2.xfs"
logfile="logfile.xfs.log"

CWD=`pwd`
MHDDFS="$CWD/mhddfs"

if ! which attr 1>/dev/null 2>&1; then
  echo "Need attr command to test extended attributes"
  exit 1
fi

if ! sudo which mkfs.xfs 1>/dev/null 2>&1; then
  echo "Need mkfs.xfs command to test extended attributes"
  exit 1
fi

function run_test ()
{
  TESTNAME=$1
  { 
    $TESTNAME &&
    echo "Passed $TESTNAME ..." ||
    {
      echo "Failed $TESTNAME ..."
      return 1
    }
  }
  return 0
}

function setup() {
    test -r $image1 || dd if=/dev/zero of=$image1 bs=1M count=50
    test -r $image2 || dd if=/dev/zero of=$image2 bs=1M count=100
    
    # make xfs filesystems
    sudo mkfs.xfs -f $image1 1>/dev/null
    sudo mkfs.xfs -f $image2 1>/dev/null

    test -d $mount1 || mkdir $mount1
    test -d $mount2 || mkdir $mount2
    test -d $mount || mkdir $mount
  
    # mount loop back xfs filesystems
    sudo mount -o loop $image1 $mount1
    sudo mount -o loop $image2 $mount2

    sudo chown -R `whoami` $mount1 $mount2

    $MHDDFS -o allow_other,logfile=$logfile -o loglevel=0 $mount1 $mount2 $mount
}

function teardown () {
  fusermount -u $mount
  
  sudo umount $mount1
  sudo umount $mount2
  
  rm -rf $image1 $image2 $mount1 $mount2 $mount
}

function test_xattr () {
  touch $mount1/testfile
  ATTRNAME="X&E^REsd436"
  ATTRVAL="More random garbage UFI(fh99fHwhgp932wcP*(@*TGPew."
  
  attr -qs "$ATTRNAME" -V "$ATTRVAL" $mount1/testfile || return 1;
  
  _ATTRNAME=$(attr -ql $mount/testfile | { read A; echo -n "$A"; })
  [ "$_ATTRNAME" != "$ATTRNAME" ] &&
  {
    echo "Found attribute name $_ATTRNAME, but looking for $ATTRNAME"
    return 1
  }
  
  _ATTRVAL=$(attr -qg "$ATTRNAME" $mount/testfile)
  [ "$_ATTRVAL" != "$ATTRVAL" ] &&
  {
    echo "Found attribute value mismatch: $_ATTRVAL != $ATTRVAL"
    return 1
  }
  
  attr -qr "$ATTRNAME" $mount/testfile || return 1
  
  _ATTRNAME=$(attr -ql $mount/testfile | { read A; echo -n "$A"; })
  [ "$_ATTRNAME" != "" ] &&
  {
    echo "Found attribute name \"$_ATTRNAME\", but looking for \"\""
    return 1
  }
  
  return 0
}

function test_preserve_xattr_across_move () {
  mkdir $mount1/testdir
  touch $mount1/testdir/testfile
  attr -qs "testdirattr" -V "a test value for this dir attribute" $mount1/testdir &&
  sudo attr -qs "testfileattr" -V "a test value for this file attribute" $mount1/testdir/testfile
  
  # cause the file to be moved to another partition
  dd if=/dev/zero of=$mount/testdir/testfile bs=1M count=51
  
  # verify that attributes were copied to new location
  _ATTRVAL=$(attr -qg testdirattr $mount/testdir)
  if [ "$_ATTRVAL" != "a test value for this dir attribute" ]; then
    echo "Directory xattr test failed: got \"$_ATTRVAL\" but expected \"a test value for this dir attribute\""
    return 1
  fi
  _ATTRVAL=$(attr -qg testfileattr $mount/testdir/testfile)
  if [ "$_ATTRVAL" != "a test value for this file attribute" ]; then
    echo "File xattr test failed: got \"$_ATTRVAL\" but expected \"a test value for this file attribute\""
    return 1
  fi
  return 0
}

setup

# Run tests
{
  run_test "test_xattr" &&
  run_test "test_preserve_xattr_across_move"
} || RC=$?


read -p "Press Enter to teardown..."
teardown

exit $RC
