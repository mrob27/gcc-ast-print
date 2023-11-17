#! /usr/bin/env python3

"""

REVISION HISTORY
 20201011 Ignore any lines starting with '#' to enable astprint.cc to
print more information for debugging.
 20201020 Add json output

"""

import json


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
        print (".   function %s() in %s done, storing in known['%s']."
                                                  % (fct_nm, fctsrc, fct_nm))
        # print (content)
      if l.startswith('function_decl'):
        # Here we glean the function name and source filename. The input
        # line looks like:
        #                function_decl f1 at test1.cc:1
        ls = l.split()
        #        ls[0]   function_decl
        fct_nm = ls[1] #               f1
        #        ls[2]                    at
        fctsrc = ls[3] #                     test1.cc:1
        print (". start function %s" % (fct_nm))
      content = [ {} ]
      node_id = 0
      print (". reset content to [ {} ]")
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
        print((".  %3d" % (node_id)), dic)
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
      # Remember the node's category, this also clears the
      # temporary dictionary 'dic'
      dic = { 'node': l[:idx], "'nid": node_id }
      # print ("# dic = { 'node': '%s' }" % (dic['node']))
      l = l[idx:]

    l = l.lstrip(' ')
    # print ("#  rest: %s" % (l))

    if dic['node'] == 'xx_ignore':   # was str_const
      pass
    else:
      # Pull the first "key : value" string off the line until there are
      # none left
      while l != '':
        # Find the colon
        idx = l.index(':')
        # The key is the part to the left of the colon, removing any trailing
        # spaces
        key = l[:idx].rstrip(' ')
        # remaining line 'l' is everything after the colon, also skipping
        # any spaces after the colon
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


def print_list(lbl, l):
  print("{}[\n  ".format(lbl), end='')
  for item in l:
    print("{},\n  ".format(item), end='')
  print("]")


if __name__ == '__main__':
  import sys
  # Read in one or more lists of nodes
  functions = handle_asts(sys.stdin.readlines())
  for k in functions:
    # k is the name of a function, e.g. 'fib'
    # Get the node-list for this function
    function = functions[k]
    ast = function['content']
    # 'ast' is actually a list of nodes, not a tree
    # Add the function name in the unused 0th element of the list
    ast[0] = { 'fn_name': k }
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
      fname = ("out/rad10-%s.json" % (k))
      with open(fname, 'w') as f:
        # pp = pprint.PrettyPrinter(indent=4, stream=f)
        # pp.pprint(ast)
        f.write(json.dumps(ast, indent=4, sort_keys=True))
        f.write("\n")
      print("wrote ast object to %s" % (fname))
