#!/usr/bin/perl

use warnings;
use strict;

use utf8;
use open qw(:std :utf8);

use POSIX;
use Encode qw(decode);

unless (@ARGV) {
    print "Usage $0 object1 object2\n";
}


for (@ARGV) {
    $_ = decode(utf8 => $_) unless utf8::is_utf8($_);
    print "Test $_ R_OK...";
    if (POSIX::access($_, &POSIX::R_OK)) {
        print " ok\n";
    } else {
        print " fail\n";
    }

    print "Test $_ W_OK...";
    if (POSIX::access($_, &POSIX::W_OK)) {
        print " ok\n";
    } else {
        print " fail\n";
    }

    print "Test $_ X_OK...";
    if (POSIX::access($_, &POSIX::X_OK)) {
        print " ok\n";
    } else {
        print " fail\n";
    }

    print "\n";
}
