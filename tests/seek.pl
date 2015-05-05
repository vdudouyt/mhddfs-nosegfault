#!/usr/bin/perl

use warnings;
use strict;

sub usage()
{
  print <<endusage;
  usage $0 test_seek_file_name
endusage
  exit;
}

sub test_seek($)
{
  my $file_name=shift;
  if (-e $file_name)
  {
    print "$file_name exists! test skipped\n";
    return;
  }

  unless (open my $file, '>', $file_name)
  {
    print "Can not create file `$file_name': $!\n";
    return;
  }
  else
  {
    print "Save data...\n";
    unless (syswrite $file, "test\n")
    {
      print "error syswrite: $!\n";
    }

    print "Seek...\n";
    unless (sysseek $file, 300, 0)
    {
      print "error sysseek: $!\n";
    }

    print "Save data...\n";
    unless (syswrite $file, "test\n")
    {
      print "error syswrite: $!\n";
    }
  }

  system md5sum => $file_name;
  unlink $file_name;
}

use Getopt::Std qw(getopts);
getopts('h', \my %opts) or usage;
$opts{h} and usage;
@ARGV or usage;

test_seek $_ for (@ARGV);

