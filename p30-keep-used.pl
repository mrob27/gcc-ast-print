#!/usr/bin/env perl
#

$unused_header = q`

                         p40-keep-used.pl

This pass takes output from the plugin and removes any nodes that are
not depended on (either directly or indirectly) by the first node in a
function_decl block.
  The result will contain gaps in the node IDs, so should be passed
through a renumbering step before being given to read-ast-dump.py

`;

# Follow references in a line
sub follow_line
{
  my($ln) = @_;
  my($l, $id, $p1, $n, $p2, $nnew, $dbg);

  $l = $txt[$ln];

  if ($l =~ m/^\@([0-9]+) (.*)$/) {
    # A node - we can look at this
    $id = $1;
    $l = $2;
  } else {
    # Nothing to do on this line
    return 0;
  }

  if ($refs[$id] == 0) {
    # This line isn't referenced yet, do nothing
    return 0;
  }

  $nnew = 0;
  $dbg = "following in $id:";
  while ($l =~ m/^([^@]+)\@([0-9]+) (.*)$/) {
    $p1 = $1; $n = $2; $p2 = $3;
    if ($refs[$n] == 0) {
      $refs[$n]++;
      $dbg .= " $n";
      $nnew++;
    }
    $l = "$p1 $p2";
  }
  print "# $dbg\n";
  return $nnew;
} # End of follow.line

sub follow_and_dump
{
  my($gg, $i, $l, $id);
  # Repeat until we don't find any new dependencies
  $gg = 1;
  while ($gg) {
    $gg = 0;
    # Look at all the lines
    for ($i=0; $i<=$#txt; $i++) {
      if (&follow_line($i)) {
        # We found a node that references previously-unreferenced nodes
        $gg = 1;
      }
    }
  }

  # Now print it all out, except lines that are not needed by ID @1
  for ($i=0; $i<=$#txt; $i++) {
    $l = $txt[$i];
    if ($l =~ m/^\@([0-9]+) (.*)$/) {
      # A node - we need to see if it is referenced
      $id = $1;
      if ($refs[$id] > 0) {
        print "$txt[$i]\n";
      }
    } else {
      # Any other type of line, always print
      print "$txt[$i]\n";
    }
  }
} # End of follow.and.dump

@refs=(); $refs[1]=1; @txt=(); $ln=0; $last_id=0;
while ($l = <>) {
  chomp $l;
  if ($l =~ m/^\@([0-9]+)/) {
    $id = $1;
    if ($id < $last_id) {
      &follow_and_dump();
      @refs=(); $refs[1]=1; @txt=(); $ln=0;
    }
    $last_id = $id;
  }
  if ($l =~ m/^#/) {
    # Comment from previous pass - remove
  } else {
    # Anything else, keep
    $txt[$ln++] = $l;
  }
}
&follow_and_dump();
