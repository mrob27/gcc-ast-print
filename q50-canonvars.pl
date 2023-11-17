#!/usr/bin/env perl
#

$unused_header = q`

                         q50-canonvars.pl

This pass eliminates variations that are due only to the choice of
names for parameters and variables.

REVISION HISTORY
 20201124 First version
 20201125 Register variables with original orders and types

`;

$norm = "\033[0m";
$red = "\033[30;41m";

$p_ident = "[A-Za-z][_0-9a-zA-Z]*";
$p_vartype = "[iIrR]";

sub reg_var
{
  my($typ, $nam) = @_;
  if ($vtype{$nam} ne '') {
    # we've already seen this one
  } elsif ($isparm{$nam}) {
    $vtype{$nam} = $typ;
    print STDERR "'$nam' is a parameter of type $typ\n";
  } else {
    # non-parameter, first time seen
    $vtype{$nam} = $typ;
    print STDERR "'$nam' is a local of type $typ\n";
    $vnum += 1;
    $orig_order{$nam} = $vnum;
    print STDERR "ID of '$nam' is $vnum\n";
  }
} # End of reg.var

$| = 1;

while ($arg = shift) {
  if (!(-f $arg)) {
    die "${red}No file '$arg' exists, or argument not recognised.$norm\n";
  } elsif ($parmlist eq '') {
    $parmlist = $arg;
  } elsif ($paths eq '') {
    $paths = $arg;
  } else {
    die "${red}Give only two file/pathname arguments.$norm\n";
  }
}

print STDERR "parmlist $parmlist\npaths $paths\n";

$parms = $fname = '';
open($IN, $parmlist);
while ($l = <$IN>) {
  chomp $l;
  if ($l =~ m/^# fname = ($p_ident)/) {
    $fname = $1;
  } elsif ($l =~ m/^#/) {
    # Another comment
  } else {
    if ($fname eq '') {
      die "${red}No fname found, and unrecognised line$norm:\n  $l\n";
    }
    if ($l =~ m/^$fname: (.+)$/) {
      $parms = $1;
      print STDERR "got parms $parms\n";
    } elsif ($parms eq '') {
      die "${red}Can't parse parameter list$norm:\n  $l\n";
    } else {
      die "${red}Unrecognised line after parameter list$norm:\n  $l\n";
    }
  }
}
close $IN;

# Scan the parameter list file (output from e.g. q15-parmlist.pl) to
# learn the names of the function parameters; we also note their order
# as a tie-breaker for the canon order decision.
undef %isparm;
undef %orig_order;
$pnum = 0;
while ($parms ne '') {
  if ("$parms " =~ m/^ *($p_ident)( .*)$/) {
    $p = $1; $parms = $2;
    $isparm{$p} = 1;
    $pnum += 1;
    $orig_order{$p} = $pnum;
    print STDERR "ID of '$p' is $pnum\n";
  } elsif ($parms =~ m/^ +$/) {
    $parms = '';
  } else {
    die "${red}parse error: '$parms'$norm\n";
  }
}

# Read the paths, remember each unique path
open($IN, $paths);
while($l = <$IN>) {
  chomp $l;
  $ll{$l} = 1;
}
close $IN;

# Sort the paths, note variables at beginning and/or end
foreach $l (sort (keys %ll)) {
  print "$l\n";
  if ($l =~ m/^ ($p_vartype) ($p_ident) /) {
    &reg_var($1, $2);
  }
  if ($l =~ m/ ($p_vartype) ($p_ident) $/) {
    &reg_var($1, $2);
  }
}
