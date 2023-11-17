#!/usr/bin/env perl
#

$unused_header = q`

                       p20-strip-unused.pl

This pass takes output from the plugin and removes any node that is
not referenced (depended on) by some other node. This is repeated
until there are no longer any such nodes. To prevent everything being
removed, it first increases the reference-count of the first node in
each function_decl block.
  This removes any library finctions that have a normal tree-like
structure, but anything containing a self-referential loop will
be retained.
  The result will contain gaps in the node IDs, so should be passed
through a renumbering step before being given to read-ast-dump.py


`;

sub accumulate_refcounts
{
  my($l) = @_;
  my($id, $p1, $n, $p2, $dbg);

  if ($l =~ m/^\@([0-9]+) (.*)$/) {
    $id = $1;
    $l = $2;
  } else {
    die "p20-strip-unused: no ID in line: $l\n";
  }
  $dbg = "arc $id:";
  while ($l =~ m/^([^@]+)\@([0-9]+) (.*)$/) {
    $p1 = $1; $n = $2; $p2 = $3;
    $dbg .= " $n";
    $refs[$n]++;
    $l = "$p1 $p2";
  }
  if ($id < 50) { print "# $dbg\n"; }
} # End of accumulate.refcounts

sub unlink_line
{
  my($ln) = @_;
  my($l, $id, $p1, $n, $p2, $ndel, $dbg);

  $l = $txt[$ln];

  if ($l =~ m/^\@([0-9]+) (.*)$/) {
    $id = $1;
    $l = $2;
  } else {
    # Nothing to do on this line
    return 0;
  }
  $ndel = 0;
  if ($refs[$id] == 0) {
    $dbg = "unlinking in $id:";
    while ($l =~ m/^([^@]+)\@([0-9]+) (.*)$/) {
      $p1 = $1; $n = $2; $p2 = $3;
      if ($refs[$n] <= 0) {
        die "unlink_line: Due to ID $id, refcount of \@$n went too low\n";
      }
      $dbg .= " $n";
      $refs[$n]--;
      $ndel++;
      $l = "$p1 $p2";
    }
    $txt[$ln] = '';
    print "# $dbg\n";
  }
  return $ndel;
} # End of unlink.line

sub unlink_and_dump
{
  $gg = 1;
  while ($gg) {
    $gg = 0;
    for ($i=0; $i<=$#txt; $i++) {
      if (&unlink_line($i)) {
        $gg = 1;
      }
    }
  }

  for ($i=0; $i<=$#txt; $i++) {
    if ($txt[$i] ne '') {
      print "$txt[$i]\n";
    }
  }
} # End of unlink.and.dump

@refs=(); $refs[1]=1; @txt=(); $ln=0; $last_id=0;
while ($l = <>) {
  chomp $l;
  if ($l =~ m/^\@([0-9]+)/) {
    $id = $1;
    if ($id < $last_id) {
      &unlink_and_dump();
      @refs=(); $refs[1]=1; @txt=(); $ln=0; $last_id=0;
    }
    # A node. Make a note of reference counts
    &accumulate_refcounts($l);
    $last_id = $id;
  }
  $txt[$ln++] = $l;
}
&unlink_and_dump();
