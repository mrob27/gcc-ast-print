#!/usr/bin/env perl
#

$unused_header = q`

                          q15-parmlist.pl

This pass extracts the parameter list of the function and prints the
parameter names in order.

REVISION HISTORY
 20201116 First version

`;

# Read everything in and remember the function names, first parameter
# of each function, parameter chain, and strings
while($l = <>) {
  chomp $l;
  if ($l =~ m/^\. start function ([_a-zA-Z0-9]+)/) {
    $fname = $1;
    print "# fname = $fname\n";
  } elsif ($l =~ m/^\. +([0-9]+) (.+)$/) {
    $nid = $1; $node = $2;
    if ($node =~ m/ 'strg': '([^']+)'/) {
      $strings[$nid] = $1;
      print "# strings[$nid] = '$strings[$nid]'\n";
    }
    if ($node =~ m/'node': 'function_decl',.+, 'args': ([0-9]+), .+$/) {
      $fn_arg1[$nid] = $1;
      print "# fn_arg1[$nid] = '$fn_arg1[$nid]'\n";
    }
    if ($node =~ m/'node': 'function_decl',.+, 'name': ([0-9]+), .+$/) {
      $fn_ids{$nid} = 1;
      $fn_name[$nid] = $1;
      print "# fn_name[$nid] = '$fn_name[$nid]'\n";
    }
    if ($node =~ m/'node': 'parm_decl',.+, 'name': ([0-9]+), .+$/) {
      $name[$nid] = $1;
      print "# name[$nid] = '$name[$nid]'\n";
    }
    if ($node =~ m/'node': 'parm_decl',.+, 'chain': ([0-9]+), .+$/) {
      $chain[$nid] = $1;
      print "# chain[$nid] = '$chain[$nid]'\n";
    }
  }
}

# Find the node ID of the function_decl for the function of interest
foreach $id (keys %fn_ids) {
  print "# id $id string_id $fn_name[$id] name $strings[$fn_name[$id]]\n";
  if ($strings[$fn_name[$id]] eq $fname) {
    print "# -> This is the function\n";
    $fdid = $id;
  }
}

# Get the node ID of the first argument
$arg1 = $fn_arg1[$fdid];
print "# First argument is at ID $arg1\n";

print "$fname:";
while ($arg1 > 0) {
  print " $strings[$name[$arg1]]";
  $arg1 = $chain[$arg1];
}
print "\n";
