#!/usr/bin/env perl
#

$unused_header = q`

                         p30-renumber.pl

This pass renumbers the nodes within each function_decl block and renumbers
them, preserving the order but eliminating 'gaps' in the ID sequence.

`;


# Renumber the contents of a single line. Non-node lines are accepted, and
# any "@NNN" incide a comment will also be renumbered
sub ren_line
{
  my($l) = @_;
  my($p1, $id, $p2, $rv);

  $rv = '';
  while($l ne '') {
    if ($l =~ m/^\@([0-9]+)(.*)$/) {
      $id = $1;
      $p2 = $2;
      $rv = '@' . $id_seq[$id];
      $l = $p2;
    } elsif ($l =~ m/^([^@]+)\@([0-9]+)(.*)$/) {
      $p1 = $1; $id = $2; $p2 = $3;
      $rv = $rv . $p1 . '@' . $id_seq[$id];
      $l = $p2;
    } else {
      $rv = $rv . $l;
      $l = '';
    }
  }
  return $rv;
} # End of ren.line

sub renumber_and_dump
{
  my($i, $l);

  for ($i=0; $i<=$#txt; $i++) {
    $l = &ren_line($txt[$i]);
    print "$l\n";
  }
}

@id_seq=(); @txt=(); $seq=0; $last_id=0; $ln=0;
while($l = <>) {
  chomp $l;
  if ($l =~ m/^#/) {
    # Comment from previous pass - remove
  } else {
    # Anything else, keep
    $txt[$ln++] = $l;
  }
  if ($l =~ m/^\@([0-9]+)/) {
    $id = $1;
    if ($id < $last_id) {
      # We're at the beginning of a new function
      &renumber_and_dump();
      @id_seq=(); @txt=(); $seq=0; $last_id=0; $ln=0;
    }
    # A node. Its sequence number will be its new node ID.
    $seq++;
    $id_seq[$id] = $seq;
    if ($id != $seq) {
      print "# Node $id will become \@$seq\n";
    }
    $last_id = $id;
  }
}
# Process the final function
&renumber_and_dump();
