#!/usr/bin/perl

use warnings;
use strict;

use utf8;
use open qw(:std :utf8);

for (@ARGV) {
    my (
        $dev,   $ino,   $mode,  $nlink,
        $uid,   $gid,   $rdev,  $size,
        $atime, $mtime, $ctime, $blksize,
        $blocks
        ) = stat $_;

        print "\n$_\n";
        unless (defined $dev) {
            print "$!\n";
            last;
        }
        printf <<eof;
                        device number of filesystem : $dev
                                       inode number : $ino
                  file mode  (type and permissions) : $mode
                 number of (hard) links to the file : $nlink
                    numeric user ID of file's owner : $uid
                   numeric group ID of file's owner : $gid
         the device identifier (special files only) : $rdev
                       total size of file, in bytes : $size
        last access time in seconds since the epoch : $atime
        last modify time in seconds since the epoch : $mtime
       inode change time in seconds since the epoch : $ctime
           preferred block size for file system I/O : $blksize
                  actual number of blocks allocated : $blocks
eof
}
