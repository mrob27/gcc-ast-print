#!/usr/bin/env perl
#

$unused_header = q`

                         p40-remove-invariants.pl

This pass removes nodes that have no effect on optimization policy. It
also simplifies real literals by removing 'e+0'.
  A later pass remaps variants of data types when all variants would
be optimized the same way.

REVISION HISTORY
 20201106 First version
 20201111 Remove useless non_lvalue_expr and decl_expr nodes
 20201124 Add simplify.literals

`;

# Remap links in a single node. Non-node lines are accepted, and
# any "@NNN" inside a comment will also be renumbered
sub ren_line
{
  my($l) = @_;
  my($p1, $id, $p2, $rv);

  $rv = '';
  while($l ne '') {
    if ($l =~ m/^\@([0-9]+)(.*)$/) {
      # This line is a node. Its ID does not change, but we'll try to
      # redirect any references inside the line.
      $id = $1; $p2 = $2;
      $rv = '@' . $id;
      $l = $p2;
    } elsif ($l =~ m/^([^@]+)\@([0-9]+)(.*)$/) {
      # Whole line, or remaining portion of a line, has a node reference.
      # This also captures references inside comments.
      $p1 = $1; $id = $2; $p2 = $3;
      $rv = $rv . $p1 . '@' . $remap[$id];
      $l = $p2;
    } else {
      # The rest has no node IDs
      $rv = $rv . $l;
      $l = '';
    }
  }
  return $rv;
} # End of ren.line

sub simplify_literals
{
  my($l) = @_;
  my($p1, $p2, $p3);
  if ($l =~ m/^(\@.+) real_const (.+valu.+)e\+0(.*)$/) { 
    $p1 = $1; $p2 = $2; $p3 = $3;
    $l = "$p1 real_const $p2$p3";
  }
  return $l;
} # End of simplify.literals

sub remove_and_dump
{
  my($i, $l, $parent, $child);

  for ($i=0; $i<=$#txt; $i++) {
    $l = $txt[$i];
    #             @11 view_convert_expr type: @15 vcop: @17 
    if (0) {
    } elsif ($l =~ m/^\@([0-9]+) decl_expr .+ decl: \@([0-9]+)/) {
      $parent = $1; $child = $2;
      $remap[$parent] = $child;
      print "# remove \@$parent decl_expr -> \@$child\n";
      $purge[$parent] = 1;
    } elsif ($l =~ m/^\@([0-9]+) non_lvalue_expr .+ op 0: \@([0-9]+)/) {
      $parent = $1; $child = $2;
      $remap[$parent] = $child;
      print "# remove \@$parent n_l_expr -> \@$child\n";
      $purge[$parent] = 1;
    } elsif ($l =~ m/^\@([0-9]+) view_convert_expr .+ vcop: \@([0-9]+)/) {
      $parent = $1; $child = $2;
      $remap[$parent] = $child;
      print "# remove \@$parent view_c_expr -> \@$child\n";
      $purge[$parent] = 1;
    } elsif ($l =~ m/^\@([0-9]+) /) {
      $id = $1;
      $remap[$id] = $id;
    }
    $txt[$i] = $l;
  }

  for ($i=0; $i<=$#txt; $i++) {
    $l = &ren_line($txt[$i]);
    $l = &simplify_literals($l);
    print "$l\n";
  }
} # End of remove.and.dump

@txt=(); $last_id=0; $ln=0;
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
      &remove_and_dump();
      @txt=(); $last_id=0; $ln=0;
    }
    # A node. Its sequence number will be its new node ID.
    $last_id = $id;
  }
}
# Process the final function
&remove_and_dump();
