#!/usr/bin/env perl
#

$unused_header = q`

REVISION HISTORY

 20201015 Add a pass to simplify plugin's output by putting each node on a single line
 20201016 Remove 'chain' attributes
 20201018 Remove unreferenced nodes; add first renumber pass
 20201019 Remove nodes not needed by userfunctions; add a second
renumber pass. nodes per function is now reduced almost 100x (from
5400 to 62).
 20201020 read-ast-dump.py now generates JSON
 20201022 read-ast-dump.py is now broken into three separate passes
 20201105 Better error handling for q20 pass
 20201106 Renumber the earlier passes (making room for future additions)
 20201110 Add q25-genericise-types
 20201111 Run q25-genericise as a separate pass; better error reporting
 20201112 Add p40-print-paths and p41-tokenise. Use the tok files as
"expected" output.
 20201116 Move all generated output, errors, etc. to ./out
subdirectory. Removing 'chain' attribute is now a separate pass. Add
q15-parmlist.pl (needed later). Terser output (e.g. 'i' instead of
203).
 20201117 Put the object files in ./obj/
 20201118 Move "expect" files into ./exp/
 20201119 Error-handling for q30-get-paths; print summary table of
path counts (including unique and dupes) by function
 20201120 Sort and uniq the tok output. Add q45-reductions.pl
 20201128 Use dump-tree-all dump-rtl-all flags to dump all the tree
and RTL representations.

`;



$avc_esc = "\033";
$avc_normal = $avc_esc . "[0m";
$avc_bd_red = $avc_esc . "[0;1;31m";


sub sys1
{
  my ($cmd, $fatal) = @_;
  my ($rv);
  print "running: $cmd\n";
  system($cmd);
  $rv = ($? >> 8);

  if ($fatal && ($? >> 8)) {
    die "${avc_bd_red}error from '$cmd'$avc_normal}\n";
  }
  return $rv;
} # End of sys.1

# Quote a string in the manner needed to pass it to a shell command without
# quotes. Example:
# $pq = &quote1($path); system("ls -l $pq");
sub quote1
{
  my($s) = @_;

  $s =~ s/([,?'"&\\(){}\[\]|>< `~!#\$\%^*+:;])/\\$1/g; # "

  return $s;
}

# Get the CRC of a file using #bcrc#.
sub path_crc
{
  my ($pn) = @_;
  my ($cmd, $rv);

  if (!(-f $pn)) {
    # Not a file
    return -1;
  }

  $cmd = 'bcrc' . " " . &quote1($pn);
  $rv = `$cmd`; chomp $rv;
  if ($rv =~ m/^([0-9]+) /) {
    return $1;
  }
  # Bad result string
  return -2;
} # End of path.crc

sub mustdir
{
  my($d) = @_;
  if (-d $d) {
    return;
  } elsif (-e $d) {
    die "We want to mkdir '$d' but something else is there\n";
  }
  system('mkdir', $d);
} # End of must.dir

# We'll use colordiff only if it is available.
if ($df = "/usr/bin/colordiff", (-x $df)) {
  # ok
} else {
  $df = "/usr/bin/diff";
}

&mustdir("obj");
&mustdir("out");

# Clean everything in the obj and output directories)
system("rm out/*txt out/*json out/*tok out/*paths 2>/dev/null");
system("rm obj/*.o obj/*.so obj/*.cc.[0-9][0-9][0-9][rt].* 2>/dev/null");

$p1a = "z-gcc-source/gcc/tree.def";
$p1b = "z-gcc-source/gcc/cp/cp-tree.def";
$p2 = "./t-def.h";
if ((-f $p1a) && (!(-e $p2))) {
  &sys1("cat $p1a $p1b > $p2");
}

$testcc = "test1.cc";
$fn_pat = ".";

$arg = shift;
if (-f $arg) {
  $testcc = $arg;
} elsif ($arg =~ m/^[a-zA-Z][0-9_a-zA-Z]+$/) {
  $fn_pat = $arg;
} elsif ($arg eq '') {
} else {
  die "Unrecognised argument '$arg'\n";
}
print "processing ASTs of $testcc matching /$fn_pat/...\n";

$tfn = $testcc;
$tfn =~ s/\.cc$//;

$o = "obj/$tfn.o";
$so = "obj/$tfn.so";

# Find the directory for devtoolset plugin include
$dtil = "gcc/x86_64-redhat-linux/9/plugin/include";
foreach $d (split(/:/, $ENV{"LD_LIBRARY_PATH"})) {
  if (-d "$d/$dtil") {
    $dti = "$d/$dtil";
    break;
  }
}

# Try Ubuntu paths
if ($dti eq '') {
  $dti = "/usr/lib/gcc/x86_64-linux-gnu/9/plugin/include";
  if (!(-d $dti)) { $dti = ''; }
}

if ($dti eq '') {
  die "Could not find $dtil in LD_LIBRARY_PATH\n";
} else {
  print "dti: $dti\n";
}

# Compile the astprint code into a shared library that can be used as
# a GCC plugin
&sys1("g++ -std=gnu++2a -Og -g -fdiagnostics-color=auto -DVERSION=\\\"0.0\\\""
          . " -Wall -I$dti -c -fPIC -fno-rtti -o $o astprint.cc");
&sys1("g++ -shared -o $so $o ");

$out0 = "out/00.txt";

# Invoke GCC on a test C program using the plugin. A .o will be
# generated but we only care about the side-effect of messages printed
# by the plugin.
$err0 = "out/err00.txt";
$out_o = $testcc; $out_o =~ s/\.c+$/.o/;
&sys1("g++ -fplugin=$so -c $testcc -fdump-tree-all -fdump-rtl-all"
                                . " -o obj/$out_o > $out0 2> $err0");
$errs = (`cat $err0 | egrep '$testcc:[0-9]+:[0-9]+' | wc -l`) + 0;
if ($errs) {
  &sys1("cat $err0");
  die "${avc_bd_red}error from compiling $testcc$avc_normal\n";
}

# Consolidate all the dumps into a single file
opendir($DIR, "obj");
while ($fn = readdir($DIR)) {
  if ($fn =~ m/$tfn\.cc\.[0-9][0-9][0-9]/) {
    $ck_files{$fn} = 1;
  }
}
closedir($DIR);
$alldumps = "out/alldumps-$tfn.txt";
$prev_cs = 0; $prev_file = "";
$cdf = "/usr/bin/cdiff"; $cdf = (-x $cdf) ? "| $cdf" : "";
foreach $fn (sort (keys %ck_files)) {
  $sz = (-s "obj/$fn");
  $cs = &path_crc("obj/$fn");
  if (($sz > 0) && ($cs != $prev_cs)) {
    print sprintf("%10d %s\n", $cs, $fn);

    if ($prev_file ne "") {
      open ($OUT, ">> $alldumps");
      print $OUT (("-=" x 38) . "\n");
      print $OUT "|| $prev_file -> $fn\n";
      close $OUT;
      system("diff obj/$prev_file obj/$fn >> $alldumps");
    }

    open ($OUT, ">> $alldumps");
    print $OUT (("-=" x 38) . "\n");
    print $OUT "|| $fn\n";
    close $OUT;
    system("cat obj/$fn >> $alldumps");

    $prev_cs = $cs;
    $prev_file = $fn;
  }
}

# Simplify output by compressing whitespace
$out10 = "out/10.txt";
&sys1("cat $out0 | ./p10-unwrap.pl > $out10");

# Remove chain: fields, except those inside a parm_decl
$out15 = "out/15.txt";
$err15 = "out/err15.txt";
&sys1("cat $out10 | ./p15-dechain.pl > $out15 2> $err15");
$errs = (-s $err15);
if ($errs) {
  &sys1("cat $err15");
  die "${avc_bd_red}p15-dechain gave error(s)$avc_normal\n";
}

# Simplify output by removing unreferenced nodes (retains cycles)
$out20 = "out/20.txt";
&sys1("cat $out15 | ./p20-strip-unused.pl > $out20");

# Renumber nodes
$out22 = "out/22.txt";
&sys1("cat $out20 | ./p22-renumber.pl > $out22");

# Remove nodes not referenced directly or indirectly by user functions
$out30 = "out/30.txt";
&sys1("cat $out22 | ./p30-keep-used.pl > $out30");

# Renumber it again
$out32 = "out/32.txt";
&sys1("cat $out30 | ./p22-renumber.pl > $out32");

# Remove nodes like view_convert_expr that contribute nothing useful to
# the paths that get generated later.
$out40 = "out/40.txt";
&sys1("cat $out32 | ./p40-remove-invariants.pl > $out40");

# Another renumber is required
$out50 = "out/50.txt";
&sys1("cat $out40 | ./p22-renumber.pl > $out50");

# Convert the functions' graphs into JSON file(s).
# We cannot use stdout this time because there might be multiple functions,
# each producing a separate JSON file.
$out60 = "out/60-0-prelude.txt";
$cmd = "cat $out50 | ./q10-astpr-to-json.py";
print "$cmd\n";
open($IN, "$cmd |");
open($OUT, "> $out60");
while ($l = <$IN>) {
  chomp $l;
  if ($l =~ m|wrote ast object to out/rad10-([^ ]+)\.json|) {
    $fn = $1;
    if ($fn =~ m/$fn_pat/) {
      $json_fns{$fn} = 1;
    }
  } elsif ($l =~ m/start function ([0-9a-zA-Z]+)/) {
    $fn = $1;
    $out60 = "out/60-$fn.txt";
    close $OUT; open($OUT, "> $out60");
  }
  print $OUT "$l\n";
  # print "$l\n";
}
close $OUT;

# Handle the later passes function-by-function
foreach $fn (sort (keys %json_fns)) {
  # Function-specific output from previous pass is the input for the next
  $out60 = "out/60-$fn.txt";

  # Get the parameter list of this function (this is used by q50-canonvars.pl
  # several steps later)
  $out65 = "out/65-$fn.txt";
  $err65 = "out/err65-$fn.txt";
  &sys1("cat $out60 | ./q15-parmlist.pl > $out65 2> $err65");

  # Python pass 2: Turn the node list into an actual tree; this also
  # removes a lot of un-needed nodes and attributes
  $rad20 = "out/rad20-$fn.json";
  $err70 = "out/err70-$fn.txt";
  &sys1("./q20-treeify.py out/rad10-$fn.json >$rad20 2> $err70");
  $errs = (`cat $err70 | egrep '(\033|Traceback)' | wc -l`) + 0;
  if ($errs) {
    &sys1("cat $err70");
    die "${avc_bd_red}q20-treeify error on function $fn$avc_normal\n";
  }

  # Pass 2 unifies the type declarations into single strings; we can now
  # remap equivalent types (unsigned short, signed int, etc.) onto a single
  # type.
  $rad25 = "out/rad25-$fn.json";
  &sys1("cat $rad20 | grep -v '^#' | ./q25-genericise-types.pl > $rad25");

  # Python pass 3: Generate paths
  $rad30 = "out/rad30-$fn.json";
  $err80 = "out/err80-$fn.txt";
  $srv = &sys1("cat $rad25 | ./q30-get-paths.py >$rad30 2> $err80");
  $errs = (`cat $err80 | egrep \033 | wc -l`) + 0;
  if ($srv || $errs) {
    &sys1("cat $err80");
    die "${avc_bd_red}q30-get-paths error on function $fn$avc_normal\n";
  }

  # Print paths in simple text format (i.e. not JSON)
  $testpaths = "out/$fn.paths";
  $err90 = "out/err90-$fn.txt";
  &sys1("cat $rad30 | ./q40-print-paths.py > $testpaths 2> $err90");
  $errs = (`cat $err90 | grep \033 | wc -l`) + 0;
  if ($errs + (-s $err90)) {
    &sys1("cat $err90");
    die "${avc_bd_red}q40-print-paths error on function $fn$avc_normal\n";
  }

  # Convert strings like 'while_stmt' into numeric tokens
  $out91 = "out/91-$fn.tok";
  $err91 = "out/err91-$fn.txt";
  &sys1("cat $rad30 | ./q41-tokenise.py > $out91 2> $err91");
  $errs = (`cat $err91 | grep \033 | wc -l`) + 0;
  if ($errs + (-s $err91)) {
    &sys1("cat $err91");
    die "${avc_bd_red}q41-tokenise error on function $fn$avc_normal\n";
  }

  # Remove consecutive identical operators when the operator is of
  # a reduction type (add, multiply)
  $out95 = "out/95-$fn.tok";
  $err95 = "out/err95-$fn.txt";
  &sys1("cat $out91 | ./q45-reductions.pl > $out95 2> $err95");
  $errs = (`cat $err95 | grep \033 | wc -l`) + 0;
  if ($errs + (-s $err95)) {
    &sys1("cat $err95");
    die "${avc_bd_red}q45-reductions error on function $fn$avc_normal\n";
  }

  # Eliminate variations that are due only to the choice of
  # names for parameters and variables
  $testtok = "out/$fn.tok";
  $err100 = "out/err100-$fn.txt";
  &sys1("./q50-canonvars.pl $out65 $out95 > $testtok 2> $err100");
  $errs = (`cat $err100 | grep \033 | wc -l`) + 0;
  if ($errs) {
    &sys1("cat $err100");
    die "${avc_bd_red}q50-canonvars error on function $fn$avc_normal\n";
  }

  $testex = "exp/$fn.expect";
  if(!(-f $testex)) {
    die "${avc_bd_red}No file '$testex' exists (use ./accept-all to fix)$avc_normal\n";
  }

  &sys1("$df -u $testex $testtok");
}

print sprintf(" %6s %5s %4s %4s\n", 'fname', 'paths', 'uniq', 'dupe');
foreach $fn (sort (keys %json_fns)) {
  $out95 = "out/95-$fn.tok";
  $testtok = "out/$fn.tok";
  $paths = `wc -l $out95` + 0;
  $duped = `cat $out95 | sort | uniq -d | wc -l` + 0;
  $unique = `cat $testtok | wc -l` + 0;
  print sprintf(" %6s %5d %4d %4d\n", $fn, $paths, $unique, $duped);
}
