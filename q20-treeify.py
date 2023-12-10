#! /usr/bin/env python3

"""

This pass does a some useful simplification work. For example, the
parameter "n" of the ipow() function appears in the GCC-generated tree
like this:

  @40 parm_decl  name:@57 type:@7
    @57 identifieer_node  strg:'n'
    @7 integer_type  name:@18
      @18 type_decl  name:@35
        @35 identifier_node  strg:'unsigned int'

This is simplified to a 3-element tuple:

  [ "parm_decl", "unsigned_int32", "n" ]

REVISION HISTORY
 20201021 Split off from read-ast-dump.py
 20201022 Remove all code not needed for create.tree. Try to handle
decl_expr, for_stmt, non_lvalue_expr, real_const, and var_decl
 20201105 Handle bit_ior_expr
 20201111 Handle var_decl (now accessible because of the removal of
view_convert_expr)
 20231210 Recognise more node types

"""

import json


def fatal(msg):
  red = "\033[0;1;31m"
  norm = "\033[0;30m"
  print("# %s" % (msg))
  sys.stderr.write("%s%s%s\n" % (red, msg, norm))
  exit(-1)


def get_tree_list_len(tree, nodenr):
  node = tree[nodenr]
  if 'chan' in node:
    return 1 + get_tree_list_len(tree, node['chan'])
  return 1


def get_type(tree, nodenr):
  """ Returns a string for each type
  """
  node = tree[nodenr]
  nodestr = node['node']
  if nodestr in ['void_type', 'boolean_type']:
    type_decl = tree[node['name']]
    assert type_decl['node'] == 'type_decl'
    identifier_node = tree[type_decl['name']]
    assert identifier_node['node'] == 'identifier_node'
    return identifier_node['strg']
  if nodestr in ['integer_type', 'real_type']:
    type_decl = tree[node['name']]
    assert type_decl['node'] == 'type_decl'
    identifier_node = tree[type_decl['name']]
    assert identifier_node['node'] == 'identifier_node'
    size = tree[node['size']]
    return identifier_node['strg'] + size['int']
  if nodestr == 'function_type':
    nparams = get_tree_list_len(node['prms']) if 'prms' in node else 0
    return nodestr + ":" + get_type(tree, node['retn']) + ":" + nparams
  if nodestr == 'pointer_type':
    # XYZ do we need more detailed information, e.g., the
    #   type of the object pointed to?
    return nodestr
  if nodestr == 'record_type':
    return node['tag']
  print("type=",nodestr)
  sys.exit(1)
  return '???'
  # End of get.type


def get_string(tree, expect, nodenr):
  # print ("get_string(%d) -> '%s'" % (nodenr, tree[nodenr]['strg']), file=sys.stderr)
  assert tree[nodenr]['node'] == expect
  return tree[nodenr]['strg']


# Given a node list in node_list, create a "tree" (list of items that
# are either strings or lists of items that are either...)
def create_tree(node_list, nodenr=1, indt=""):
  node = node_list[nodenr]
  res = [ ]
  nodestr = node['node']
  print(".%s ct(%d:%s)" % (indt, nodenr, nodestr), file=sys.stderr)
  res = [ nodestr ]
  if nodestr == 'statement_list':
    cnt = 0
    while str(cnt) in node:
      assert type(node[str(cnt)]) == int
      res.append(create_tree(node_list, node[str(cnt)], indt=("  %s"%indt)))
      cnt += 1
  elif nodestr == 'if_stmt':
    res.append(create_tree(node_list, node['cond'], indt=("  %s"%indt)))
    res.append(create_tree(node_list, node['then'], indt=("  %s"%indt)))
    if 'else' in node:
      # It's an if..then..else
      elsetree = create_tree(node_list, node['else'], indt=("  %s"%indt))
      # If the top level of the condition is a comparison relation, we
      # can canonicalise it a bit by substuting the inverse relation and
      # inserting the 'else' action where the 'then' action was.
      if res[1][0] == 'lt_expr':
        res[1][0] = 'ge_expr'
        res.insert(2, elsetree)
      elif res[1][0] == 'le_expr':
        res[1][0] = 'gt_expr'
        res.insert(2, elsetree)
      elif res[1][0] == 'ne_expr':
        res[1][0] = 'eq_expr'
        res.insert(2, elsetree)
      else:
        res.append(elsetree)
  elif nodestr == 'cleanup_point_expr':
    res = create_tree(node_list, node['op 0'], indt=("  %s"%indt))
  elif nodestr in ['expr_stmt']:
    t = get_type(node_list, node['type'])
    if t == 'void':
      res = create_tree(node_list, node['expr'], indt=("  %s"%indt))
    else:
      res.append(t)
      res.append(create_tree(node_list, node['expr'], indt=("  %s"%indt)))
  elif nodestr == 'convert_expr':
    t = get_type(node_list, node['type'])
    if t == 'void':
      res = create_tree(node_list, node['op 0'], indt=("  %s"%indt))
    else:
      res.append(t)
      res.append(create_tree(node_list, node['op 0'], indt=("  %s"%indt)))
  elif nodestr == 'float_expr':
    res.append(get_type(node_list, node['type']))
    res.append(create_tree(node_list, node['op 0'], indt=("  %s"%indt)))
  elif nodestr in ['bit_and_expr', 'bit_ior_expr', 'bit_not_expr',
                   'bit_xor_expr', 'cond_expr',
                   'ge_expr', 'gt_expr',
                   'le_expr', 'lt_expr', 'lshift_expr',
                   'minus_expr','modify_expr', 'mult_expr',
                   'ne_expr', 'negate_expr',
                   'plus_expr', 'pointer_plus_expr',
                   'rshift_expr', 'sizeof_expr']:
    # res.append(get_type(node_list, node['type']))
    cnt = 0
    while ('op ' + str(cnt) in node):
      res.append(create_tree(node_list, node['op ' + str(cnt)], indt=("  %s"%indt)))
      cnt += 1
  elif nodestr == 'parm_decl':
    res.append(get_type(node_list, node['type']))
    res.append(get_string(node_list, 'identifier_node', node['name']))
  elif nodestr == 'var_decl':
    res.append(get_type(node_list, node['type']))
    res.append(get_string(node_list, 'identifier_node', node['name']))
  elif nodestr == 'return_expr':
    res.append(create_tree(node_list, node['expr'], indt=("  %s"%indt)))
  elif nodestr == 'init_expr':
    res = create_tree(node_list, node['op 1'], indt=("  %s"%indt))
  elif nodestr == 'call_expr':
    res.append(get_type(node_list, node['type']))
    res.append(create_tree(node_list, node['fn'], indt=("  %s"%indt)))
    args = ['parameters']
    cnt = 0
    while str(cnt) in node:
      args.append(create_tree(node_list, node[str(cnt)], indt=("  %s"%indt)))
      cnt += 1
    res.append(args)
  elif nodestr == 'addr_expr':
    res = create_tree(node_list, node['op 0'], indt=("  %s"%indt))
  elif nodestr == 'nop_expr':
    res = create_tree(node_list, node['op 0'], indt=("  %s"%indt))
  elif nodestr == 'function_decl':
    if 'mngl' in node:
      res = get_string(node_list, 'identifier_node', node['mngl'])
    else:
      res = get_string(node_list, 'identifier_node', node['name'])
  elif nodestr == 'int_const':
    res.append(node['int'])
  elif nodestr == 'view_convert_expr':
    res = create_tree(node_list, node['vcop'], indt=("  %s"%indt))
  elif nodestr == 'indirect_ref':
    res.append(create_tree(node_list, node['op 0'], indt=("  %s"%indt)))
  elif nodestr in ['preincrement_expr', 'postincrement_expr', 'component_ref']:
    res.append(create_tree(node_list, node['op 0'], indt=("  %s"%indt)))
    res.append(create_tree(node_list, node['op 1'], indt=("  %s"%indt)))
  elif nodestr == 'field_decl':
    res.append(get_type(node_list, node['type']))
    res.append(get_string(node_list, 'identifier_node', node['name']))
  elif nodestr == 'bind_expr':
    res.append(create_tree(node_list, node['body'], indt=("  %s"%indt)))
  elif nodestr == 'for_stmt':
    res.append(create_tree(node_list, node['cond'], indt=("  %s"%indt)))
    res.append(create_tree(node_list, node['body'], indt=("  %s"%indt)))
    res.append(create_tree(node_list, node['expr'], indt=("  %s"%indt)))
  elif nodestr == 'while_stmt':
    res.append(create_tree(node_list, node['cond'], indt=("  %s"%indt)))
    res.append(create_tree(node_list, node['body'], indt=("  %s"%indt)))
  elif nodestr == 'real_const':
    res.append(node['valu'])
  elif nodestr == 'non_lvalue_expr':
    # res.append(create_tree(node_list, node['op 0'], indt=("  %s"%indt)))
    pass
  elif nodestr in ['decl_expr']:
    pass

  else:
    fatal("q20-treeify: Need to handle '%s'" % (nodestr))

  return res
  # End of create.tree


if __name__ == '__main__':
  import sys
  # Read graph node list from JSON file produced by the previous pass
  print ("# reading JSON from %s" % (sys.argv[1]))
  with open(sys.argv[1], 'r') as f:
    node_list = json.load(f)
    k = node_list[0]['fn_name']
    node_list[0] = {}
    print("# fn_name: '{}'".format(k))

    # Make the node into an actual tree: a list in which individual
    # elements can either be string (leaf) or a list (subtree)
    tree = create_tree(node_list)

    # Print the tree as json
    print(json.dumps(tree, indent=4, sort_keys=True))
