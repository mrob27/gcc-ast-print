#!/usr/bin/env perl
#

$unused_header = q`

                         q45-reductions.pl

This pass reduces consecutive identical operators when the operator is
of a reduction type.

REVISION HISTORY
 20201120 First version
 20201124 Add explicit list of reduction operators.

`;

%reduction_ops = (
  9 => 'add',
  22 => 'multiply',
  23 => 'bitwise_and',
  24 => 'bitwise_or',

  999 => 'end',
);

while($l = <>) {
  chomp $l;
  $gg = 1;
  while ($gg) {
    $gg = 0;
    foreach $tok (keys %reduction_ops) {
      if ($l =~ s/ $tok $tok / $tok /) {
        $gg = 1;
      }
    }
  }
  print "$l\n";
}
