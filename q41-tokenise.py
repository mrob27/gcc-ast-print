#! /usr/bin/env python3

"""

REVISION HISTORY
 20201111 Split off from q30-get-paths.py
 20201116 encode.leaf now no longer has to handle parm_decl etc.
Simplify the types and const types to 1 or 2 letters. Remove unneeded
punctuation.
 20231210 More node types

"""

import json

import re

r1 = re.compile('[\[\]\',]')

def fatal(msg):
  red = "\033[0;1;31m"
  norm = "\033[0;30m"
  print("# %s" % (msg))
  sys.stderr.write("%s%s%s\n" % (red, msg, norm))
  exit(-1)


nodecodes = {
  'ge_expr': 1,
  'gt_expr': 2,
  'le_expr': 3,
  'lt_expr': 4,
  'eq_expr': 5,
  'ne_expr': 6,
  'if_stmt': 7,
  'modify_expr': 8,
  'plus_expr': 9,
  'minus_expr': 10,
  'expr_stmt': 11,
  'statement_list': 12,
  'return_expr': 13,
  'float_expr': 14,
  'cond_expr': 15,
  'call_expr': 16,
  'postincrement_expr': 17,
  'convert_expr': 18,
  'indirect_ref': 19,
  'component_ref': 20,
  'for_stmt': 21,
  'mult_expr': 22,
  'bit_and_expr': 23,
  'bit_ior_expr': 24,
  'non_lvalue_expr': 25,
  'parameters': 26,
  'while_stmt': 27,
  'lshift_expr': 28,
  'rshift_expr': 29,
  'negate_expr': 30,
  'sizeof_expr': 31,
  'pointer_plus_expr': 32,
  'bit_xor_expr': 33,
  'bit_not_expr': 34,
  'preincrement_expr': 35,
}
leafcodes = {
  'int_const' : 'Ki',
  'real_const' : 'Kr',
}
typecodes = {
  'float32': 'r',
  'double64': 'R', 'long_double128': 'R',
  'int32': 'i', 'unsigned32': 'i', 'uint32_t32': 'i',
  'int64': 'I', 'long64': 'I',
  'pointer_type': 'p',
}


def encode_leaf(leaf):
  first = leaf[0]
  if first in [ 'int_const', 'real_const' ]:
    if not first in leafcodes:
      fatal("q41-tokenise: leafcodes['%s'] not defined" % (first))
    res = [ leafcodes[first], leaf[1] ]
  elif first in [ 'double64', 'float32', 'int32', 'int64',
                  'long64', 'unsigned32', 'uint32_t32', 'long_double128',
                  'pointer_type' ]:
    if not first in typecodes:
      fatal("q41-tokenise: typecodes['%s'] not defined" % (first))
    res = [ typecodes[first], leaf[1] ]
  else:
    fatal("q41-tokenise: unhandled first=='%s'" % (first))
    # res = leaf
  return [ res ]


def encode_nodes(path):
  rv = [ ]
  for n in path:
    try:
      nc = nodecodes[n]
    except:
      fatal("q41-tokenise: nodecodes['%s'] not defined" % (n))
    rv.append(nc)
  return rv


def encode(path):
  return encode_leaf(path[0])+encode_nodes(path[1:-1])+encode_leaf(path[-1])


if __name__ == '__main__':
  import sys
  # Read paths from JSON file produced by the previous pass
  paths = json.load(sys.stdin)

  for p in paths:
    # Tokenise the path into numbers and symbols
    coded = encode(p)
    # Make the nested lists into a single string
    cstr = str(coded)
    # Remove un-needed punctuation: "[[Kr a], 22, 10, [i 7]]" becomes
    # "Kr a 22 10 i 7"
    abr = r1.sub('', cstr)
    print(" %s " % (abr))
