#!/usr/bin/env perl
#

$unused_header = q`

                          p15-dechain.pl

This pass takes tree-dump output that has been un-wrapped to a single
line per node. It removes 'chain:' attributes except those in a
parm_decl node.

Sample input (as generated by p10-unwrap.pl) :

  function_decl fpow at test1.cc:72
  ...
  @24 function_decl name: @40 type: @41 scpe: @23 srcp: test1.cc;61 chain: @42 args: @43 link: extern body: @44
  @25 parm_decl name: @45 type: @27 scpe: @8 srcp: test1.cc;72 chain: @46 argt: @27 size: @28 algn: 64 used: 1
  @26 identifier_node strg: rv lngt: 2
  ...

Output of this pass:

  function_decl fpow at test1.cc:72
  ...
  @24 function_decl name: @40 type: @41 scpe: @23 srcp: test1.cc;61 args: @43 link: extern body: @44
  @25 parm_decl name: @45 type: @27 scpe: @8 srcp: test1.cc;72 chain: @46 argt: @27 size: @28 algn: 64 used: 1
  @26 identifier_node strg: rv lngt: 2
  ...

`;

# Given a node in 1-line tree-dump format, remove the 'chain'
# attribute unless it is a parm_decl node.
sub process
{
  my($node) = @_;

  if ($node =~ m/^\@[0-9]+ parm_decl /) {
    # Leave this node alone
  } elsif ($node =~ m/^\@[0-9]+ /) {
    # Every other type of node, remove chain attribute (if any)
    $node =~ s/ chain: \@[0-9]+//;
  } else {
    # Not sure how this would happen
    print STDERR "p15-dechain process() parse error:\n  $l\n";
  }

  print "$node\n";
}

$node = $cmts = $spcl = '';
while($l = <>) {
  chomp $l;
  $l =~ s/[ \t]+/ /g;
  if ($l =~ m/^\@[0-9]/) {
    # A node. Process it and output.
    &process($l);
  } elsif ($l =~ m/^function_decl ([_0-9a-zA-Z]+) at /) {
    # One of these at the beginning of each function
    print "$l\n";
  } elsif ($l =~ m/^finish compilation/) {
    # We see this once at the end
    print "$l\n";
  } elsif ($l =~ m/^#/) {
    # Comments - do not print.
  } else {
    # Unknown line type
    print STDERR "p15-dechain parse error:\n  $l\n";
  }
}

