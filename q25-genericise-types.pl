#!/usr/bin/env perl
#

$unused_header = q`

                          q25-genericise-types.pl

This pass remaps variants of data types when all variants would be
optimized the same way.

REVISION HISTORY
 20201108 First version
 20201111 Handle short unsigned and long_unsigned_int64

`;

while($l = <>) {
  chomp $l;
  $l =~ s/"__int128128"/"int64"/g;
  $l =~ s/"long_int64"/"int64"/g;
  $l =~ s/"long_long_int64"/"int64"/g;
  $l =~ s/"long_unsigned_int64"/"int64"/g;
  $l =~ s/"short_unsigned_int16"/"int32"/g;
  $l =~ s/"unsigned_int32"/"unsigned32"/g;
  print "$l\n";
}
