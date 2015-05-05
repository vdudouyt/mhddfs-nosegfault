#!/usr/bin/perl

use warnings;
use strict;

use utf8;
use open qw(:std :utf8);

use Getopt::Std qw(getopts);
use Filesys::Statvfs;


sub usage()
{
    print <<eof;
    usage: $0 [ OPTIONS ] [ path ]

        OPTIONS:

            -h      - this helpscreen
eof
    exit 0;
}

usage unless getopts('h', \my %opts);
usage if $opts{h};

my @paths = @ARGV;

unless (@paths) {
    open my $fh, '-|', 'mount'
        or die "Cannot start 'mount'\n";

    while(<$fh>) {
        my @f = split /\s+/, $_;
        next unless @f > 3;
        next unless $f[1] eq 'on';
        push @paths, $f[2];
    }
}

for (@paths) {
    my @stat = statvfs $_;

    my @hi = (
        'File system block size',
        'Fragment size',
        'Size of fs in "Fragment size" units',
        'Free blocks',
        'Free blocks for unprivileged users',
        'Inodes',
        'Free inodes',
        'Free inodes for unprivileged users',
        'Mount flags',
        'Maximum filename length'
    );

    printf "%s\n\t%s\n\n", $_,
        join "\n\t", map { sprintf "%40s: %s", $hi[$_], $stat[$_] } 0 .. $#stat;
}

