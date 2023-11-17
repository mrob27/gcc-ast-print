#! /usr/bin/env python3

"""

REVISION HISTORY
 20201011 Ignore any lines starting with '#' to enable astprint.cc to
print more information for debugging.
 20201020 Add json output

"""

import json


def my_split(s, seps):
  res = [s]
  for sep in seps:
    s, res = res, []
    for seq in s:
      res += [ sp for sp in seq.split(sep) if sp != '' ]
  return res


def handle_asts(lines):
  fct_nm = ''
  fctsrc = ''
  node_id = 0
  dic = {}
  content = [ {} ]
  known = {}
  for l in lines:
    l = l.rstrip()

    if l.startswith('#'):
      # comment from an earlier pass
      continue

    if l.startswith('function_decl') or l.startswith('finish compilation'):
      if node_id > 0:
        # Add the last node from the previous function
        content.append(dic)
        # assert len(content) == node_id + 1
        # Add the whole function to the list of functions we have parsed
        known[fct_nm] = { 'source': fctsrc, 'content': content }
      if l.startswith('function_decl'):
        # Here we glean the function name and source filename. The input
        # line looks like:
        #                function_decl f1 at test1.cc:1
        ls = l.split()
        #        ls[0]   function_decl
        fct_nm = ls[1] #               f1
        #        ls[2]                    at
        fctsrc = ls[3] #                     test1.cc:1
      content = [ {} ]
      node_id = 0
      print (". reset")
      continue

    # If we reach this point we have a line that is not a comment or
    # function_decl. We assert fct_nm != '' to make sure we were able to
    # parse out the function's name
    assert fct_nm != ''

    if l.startswith('@'):
      if node_id > 0:
        # we have been parsing a node from earlier lines, it must be complete
        # now so we can add it to the list
        content.append(dic)
        # astprint's node IDs should be consecutive starting with 1, and this
        # number resets to 1 for each new function_decl
        # print ("len(content)=%d, node_id=%d" % (len(content), node_id))
        # assert len(content) == node_id + 1
      # Get the ID of this new node
      idx = l.index(' ')
      l2 = l[idx+1:].lstrip(' ')
      # print ("# '%s' %s" % (l[1:idx], l2));
      node_id = int(l[1:idx])
      # Remove the @num, this will give us the first key:value
      l = l[idx+1:].lstrip(' ')
      try:
        idx = l.index(' ')
      except ValueError:
        # There are no additional values on the line, such as in
        # @712  error_mark
        # we just ignore this type of node
        continue
      # Remember the first node attribute name, this also clears the
      # temporary dictionary 'dic'
      dic = { 'node': l[:idx] }
      # print ("# dic = { 'node': '%s' }" % (dic['node']))
      l = l[idx:]

    l = l.lstrip(' ')
    # print ("#  rest: %s" % (l))

    if dic['node'] == 'string_cst':
      pass
    else:
      while l != '':
        idx = l.index(':')
        key = l[:idx].rstrip(' ')
        l = l[idx+1:].lstrip(' ')
        try:
          idx = l.index(' ')
        except ValueError:
          idx = len(l)
        dic[key] = l[:idx]
        if dic[key].startswith('@'):
          # value is a pointer to another node. We remove the '@' and make it
          # an actual integer. There is no ambiguity problem for our purposes
          # because we have the key
          assert l != ''
          dic[key] = int(dic[key][1:])
        l = l[idx:].lstrip(' ')

  # This assert is to make sure we got a "finish compilation" at the end
  assert node_id == 0

  return known
  # End of handle.asts


def read_asts(fname):
  if len(fname):
    with open(fname[0], 'r') as f:
      return handle_asts(f.readlines())
  else:
    return handle_asts(sys.stdin.readlines())


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


def get_tree_list_len(tree, nodenr):
  node = tree[nodenr]
  if 'chan' in node:
    return 1 + get_tree_list_len(tree, node['chan'])
  return 1


def get_field_types(tree, nodenr):
  # XYZ should emit types in list but how to avoid recursion?
  return [1]


def get_string(tree, expect, nodenr):
  assert tree[nodenr]['node'] == expect
  return tree[nodenr]['strg']


def create_tree(tree, nodenr = 1):
  node = tree[nodenr]
  res = [ ]
  nodestr = node['node']
  res = [ nodestr ]
  if nodestr == 'statement_list':
    cnt = 0
    while str(cnt) in node:
      assert type(node[str(cnt)]) == int
      res.append(create_tree(tree, node[str(cnt)]))
      cnt += 1
  elif nodestr == 'if_stmt':
    res.append(create_tree(tree, node['cond']))
    res.append(create_tree(tree, node['then']))
    if 'else' in node:
      elsetree = create_tree(tree, node['else'])
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
    res = create_tree(tree, node['op 0'])
  elif nodestr in ['expr_stmt']:
    t = get_type(tree, node['type'])
    if t == 'void':
      res = create_tree(tree, node['expr'])
    else:
      res.append(t)
      res.append(create_tree(tree, node['expr']))
  elif nodestr == 'convert_expr':
    t = get_type(tree, node['type'])
    if t == 'void':
      res = create_tree(tree, node['op 0'])
    else:
      res.append(t)
      res.append(create_tree(tree, node['op 0']))
  elif nodestr == 'float_expr':
    res.append(get_type(tree, node['type']))
    res.append(create_tree(tree, node['op 0']))
  elif nodestr in ['lt_expr', 'le_expr', 'modify_expr', 'plus_expr', 'minus_expr', 'cond_expr']:
    # res.append(get_type(tree, node['type']))
    cnt = 0
    while ('op ' + str(cnt) in node):
      res.append(create_tree(tree, node['op ' + str(cnt)]))
      cnt += 1
  elif nodestr == 'parm_decl':
    res.append(get_type(tree, node['type']))
    res.append(get_string(tree, 'identifier_node', node['name']))
  elif nodestr == 'return_expr':
    res.append(create_tree(tree, node['expr']))
  elif nodestr == 'init_expr':
    res = create_tree(tree, node['op 1'])
  elif nodestr == 'call_expr':
    res.append(get_type(tree, node['type']))
    res.append(create_tree(tree, node['fn']))
    args = ['parameters']
    cnt = 0
    while str(cnt) in node:
      args.append(create_tree(tree, node[str(cnt)]))
      cnt += 1
    res.append(args)
  elif nodestr == 'addr_expr':
    res = create_tree(tree, node['op 0'])
  elif nodestr == 'nop_expr':
    res = create_tree(tree, node['op 0'])
  elif nodestr == 'function_decl':
    if 'mngl' in node:
      res = get_string(tree, 'identifier_node', node['mngl'])
    else:
      res = get_string(tree, 'identifier_node', node['name'])
  elif nodestr == 'integer_cst':
    res.append(node['int'])
  elif nodestr == 'view_convert_expr':
    res.append(get_type(tree, node['type']))
  elif nodestr == 'indirect_ref':
    res.append(create_tree(tree, node['op 0']))
  elif nodestr in ['postincrement_expr', 'component_ref']:
    res.append(create_tree(tree, node['op 0']))
    res.append(create_tree(tree, node['op 1']))
  elif nodestr == 'field_decl':
    res.append(get_type(tree, node['type']))
    res.append(get_string(tree, 'identifier_node', node['name']))

  return res


def create_paths(tree, downpaths=[]):
  if tree[0] in ['parm_decl', 'integer_cst']:
    res = [ p + [ tree ] for p in downpaths ]
    return res, [ [ tree ] ]

  start = -1
  if tree[0] in ['statement_list', 'lt_expr', 'ne_expr', 'ge_expr',
           'gt_expr', 'modify_expr', 'return_expr', 'plus_expr',
           'minus_expr', 'cond_expr', 'le_expr', 'parameters',
           'if_stmt', 'nop_expr', 'indirect_ref', 'postincrement_expr',
           'component_ref', 'var_decl', 'non_lvalue_expr',
           'eh_spec_block', 'bind_expr', 'negate_expr']:
    start = 1
  elif tree[0] in ['float_expr', 'view_convert_expr', 'convert_expr']:
    start = 2
  elif tree[0] in ['call_expr', 'field_decl']:
    start = 3
  if start == -1:
    print('missing=',tree)
  assert start != -1

  if not tree[0] in ['parameters']:
    newdownpaths = [ p + [ tree[0] ] for p in downpaths ]
  else:
    newdownpaths = downpaths.copy()

  res = []
  uppaths = []
  for i in range(start, len(tree)):
    subres, subuppaths = create_paths(tree[i], newdownpaths)
    newdownpaths += [ u + [ tree[0] ] for u in subuppaths ]
    res += subres
    uppaths += subuppaths

  return res, uppaths


leafcodes = { 'parm_decl': 1,
        'integer_cst' : 2
      }
typecodes = { 'int32': 1,
        'double64': 2,
        'unsigned32': 3,
      }
nodecodes = { 'ge_expr': 1,
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
      }


def encode_leaf(leaf):
  first = leaf[0]
  if first in [ 'parm_decl' ]:
    if not leaf[1] in typecodes:
      typecodes[leaf[1]] = len(typecodes) + 1
    res = [ leafcodes[first], typecodes[leaf[1]] ]
  elif first in [ 'integer_cst' ]:
    res = [ leafcodes[first] ]
  else:
    res = leaf
  return [ res ]


def encode_nodes(path):
  return [ nodecodes[n] for n in path ]


def encode(path):
  return encode_leaf(path[0])+encode_nodes(path[1:-1])+encode_leaf(path[-1])


def print_codes(name, codes):
  print('{}:'.format(name))
  for c in codes:
    print('  {}={}'.format(c, codes[c]))


def print_list(lbl, l):
  print("{}[\n  ".format(lbl), end='')
  for item in l:
    print("{},\n  ".format(item), end='')
  print("]")


if __name__ == '__main__':
  import sys
  functions = handle_asts(sys.stdin.readlines())  # read_asts(sys.argv[1:])
  for k in functions:
    function = functions[k]
    ast = function['content']
    print("function=%s in %s" % (k, function['source']))
    if False:
      # Use Python's own object formatter
      print("  ast : ", end='')
      print("{}".format(ast))
    if False:
      # Use bespoke formatter
      print_list("  ast: ", ast)
    if True:
      # Use pprint to json
      fname = ("rad10-%s.json" % (k))
      with open(fname, 'w') as f:
        # pp = pprint.PrettyPrinter(indent=4, stream=f)
        # pp.pprint(ast)
        f.write(json.dumps(ast, indent=4, sort_keys=True))
      print("wrote ast object to %s" % (fname))
    tree = create_tree(ast)
    paths, _ = create_paths(tree)
    for p in paths:
      print(p)
      coded = encode(p)
      print(coded)
  # print_codes('leafcodes', leafcodes)
  # print_codes('typecodes', typecodes)
  # print_codes('nodecodes', nodecodes)
