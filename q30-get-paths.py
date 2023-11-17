#! /usr/bin/env python3

"""

REVISION HISTORY
 20201021 Split off from read-ast-dump.py
 20201022 Remove code not needed for create.paths. Try to handle
decl_expr, for_stmt, real_const
 20201023 Expand a couple list comprehensions and add error-checking.
 20201030 integer_cst is now int_const
 20201105 Handle long64, 'bit_and_expr', 'bit_ior_expr',
'non_lvalue_expr; clearer error messages
 20201111 Remove remaining list comprehensions. encode.leaf now
handles decl_expr, real_const, and var_decl. Handle constants
properly. Disambiguate the leafcodes and typecodes from the nodecodes.
Printout and tokenise steps are now separate passes.
 20201112 Remove parm_decl and var_decl tokens from beginning/end of paths
 20201118 Add lots of debugging prints
 20201119 Restore missing path elements.

"""

import json


# Print a message to stdout and stderr (with red highlighting) and die
def fatal(msg):
  red = "\033[0;1;31m"
  norm = "\033[0;30m"
  print("# %s" % (msg))
  sys.stderr.write("%s%s%s\n" % (red, msg, norm))
  exit(-1)


# Print something to stderr without red highlighting
def dbg(msg):
  sys.stderr.write("%s" % (msg))


def create_paths(tree, downpaths=[], lvl=""):
  dbg("%screate_paths(%s,\n%s             %s)\n" % (lvl, str(tree), lvl, str(downpaths)))
  if tree[0] in ['int_const', 'real_const']:
    # This is a leafnode and therefore can be the end of a path.
    dbg("%s(1) leaf const\n" % (lvl))
    res = []
    for p in downpaths:
      # To this path, add the node tree[0] (which is a leafnode)
      # to create a path that goes into results.
      res.append(p + [ tree ])
    dbg("%sappend result:  %s\n" % (lvl, str(res[-1])))
    dbg("%s-> %s,\n%s   %s\n" % (lvl, res, lvl, [ [ tree ] ]))
    return res, [ [ tree ] ]

  if tree[0] in ['parm_decl', 'var_decl']:
    # We use tree[1:] to remove the initial 'xxx_decl' from the first/last
    # elements of the path
    dbg("%s(2) param or var\n" % (lvl))
    res = []
    for p in downpaths:
      res.append(p + [ tree[1:] ])
      dbg("%sappend result:  %s\n" % (lvl, str(res[-1])))
    dbg("%s-> %s,\n%s   %s\n" % (lvl, res, lvl, [ [ tree[1:] ] ]))
    return res, [ [ tree[1:] ] ]

  start = -1
  if tree[0] in ['bind_expr', 'bit_and_expr', 'bit_ior_expr', 'bit_not_expr',
           'component_ref', 'cond_expr',
           'eh_spec_block',
           'for_stmt',
           'ge_expr', 'gt_expr',
           'if_stmt',
           'le_expr', 'lshift_expr', 'lt_expr',
           'minus_expr', 'modify_expr', 'mult_expr',
           'ne_expr', 'negate_expr', 'nop_expr',
           'parameters', 'plus_expr',
           'return_expr', 'rshift_expr',
           'sizeof_expr',
           'var_decl',
           'while_stmt',
           'indirect_ref', 'postincrement_expr',
           'non_lvalue_expr',
           'statement_list']:
    start = 1
  elif tree[0] in ['convert_expr', 'float_expr',
                   'view_convert_expr']:
    start = 2
  elif tree[0] in ['call_expr', 'field_decl']:
    start = 3
  if start == -1:
    fatal("q30-get-paths: create_paths() needs a case for '{}'".format(tree[0]))
  assert start != -1

  dbg("%s(3) non-leaf start=%d\n" % (lvl, start))

  if not tree[0] in ['parameters']:
    newdownpaths = []
    for p in downpaths:
      newdownpaths.append(p + [ tree[0] ])
  else:
    newdownpaths = downpaths.copy()

  res = []
  uppaths = []
  for i in range(start, len(tree)):
    subres, subuppaths = create_paths(tree[i], newdownpaths, ("%s  " % (lvl)))
    ndp_append = []
    # Create more downpaths to send when we recurse on the next child.
    # These are also the uppaths we should return to our caller.
    # %%% We cannot simply pass back the uppaths we got from the
    #     recursive call, else the peak node would get omitted from
    #     the final constructed path!
    for u in subuppaths:
      ndp_append.append(u + [ tree[0] ])
    newdownpaths += ndp_append
    # Add the results returned by the sub-call to the global results
    #         ---original----   -restore peak--   --sort | uniq--
    #   fname paths uniq dupe   paths uniq dupe   paths uniq dupe
    #      f1    36   23    6      36   33    3      33   33    0
    #      f2    45   41    4      45   45    0      45   45    0
    #     fak    78   48   13      78   71    7      71   71    0
    #     fbs    21   15    4      21   18    3      18   18    0
    #     fib    21   15    4      21   18    3      18   18    0
    #   fibnr   300  172   90     300  257   41     257  257    0
    #    fpow    91   67   20      91   91    0      91   91    0
    #    ipow    91   66   21      91   91    0      91   91    0
    #   llpow    91   66   21      91   91    0      91   91    0
    #     qd2    10   10    0      10   10    0      10   10    0
    #     qd3    45   28   14      45   35   10      35   35    0
    #     qdf     3    2    1       3    2    1       2    2    0
    #   qdisc    10    7    3      10    7    3       7    7    0
    res += subres
    if (len(res) >= 1):
      dbg("%sappend result:  %s\n" % (lvl, str(res[-1])))
    # uppaths += subuppaths %%% wrong, omits peak node!
    uppaths += ndp_append

  dbg("%s-> %s,\n%s   %s\n" % (lvl, res, lvl, uppaths))
  return res, uppaths
  # End of create.paths


if __name__ == '__main__':
  import sys
  # Read tree (list of nested lists) from JSON file produced by the previous
  # pass
  tree = json.load(sys.stdin)

  # Find paths
  paths, _ = create_paths(tree)

  # Print the paths as json
  print(json.dumps(paths, indent=4, sort_keys=True))
