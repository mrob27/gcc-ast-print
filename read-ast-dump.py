#! /usr/bin/env python3


def my_split(s, seps):
    res = [s]
    for sep in seps:
        s, res = res, []
        for seq in s:
            res += [ sp for sp in seq.split(sep) if sp != '' ]
    return res

def add_fct(dic, fct, fctsrc, content):
    dic[fct] = { 'source': fctsrc, 'content': content }

def handle_asts(lines):
    fct = ''
    fctsrc = ''
    count = 0
    dic = {}
    content = [ {} ]
    known = {}
    for l in lines:
        l = l.rstrip()
        if l.startswith('function_decl') or l.startswith('finish compilation'):
            if count > 0:
                content.append(dic)
                assert len(content) == count + 1
                add_fct(known, fct, fctsrc, content)
            if l.startswith('function_decl'):
                ls = l.split()
                fct = ls[1]
                fctsrc = ls[3]
            content = [ {} ]
            count = 0
            continue
        else:
            assert fct != ''

        if l.startswith('@'):
            if count > 0:
                content.append(dic)
                assert len(content) == count + 1
            count = int(l[1:8])
            l = l[8:]
            idx = l.index(' ')
            dic = { 'node': l[:idx] }
            l = l[idx:]

        l = l.lstrip(' ')
        while l != '':
            idx = l.index(':')
            key = l[:idx].rstrip(' ')
            l = l[idx+1:].lstrip(' ')
            assert l != ''
            try:
                idx = l.index(' ')
            except ValueError:
                idx = len(l)
            dic[key] = l[:idx]
            if dic[key].startswith('@'):
                dic[key] = int(dic[key][1:])
            l = l[idx:].lstrip(' ')

    assert count == 0
    return known

def read_asts(fname):
    if len(fname):
        with open(fname[0], 'r') as f:
            return handle_asts(f.readlines())
    else:
        return handle_asts(sys.stdin.readlines())


def get_type(tree, nodenr):
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
    return '???'


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
    elif nodestr == 'function_decl':
        if 'mngl' in node:
            res = get_string(tree, 'identifier_node', node['mngl'])
        else:
            res = get_string(tree, 'identifier_node', node['name'])
    elif nodestr == 'integer_cst':
        res.append(node['int'])

    return res


def create_paths(tree, downpaths=[]):
    if tree[0] in ['parm_decl', 'integer_cst']:
        res = [ p + [ tree ] for p in downpaths ]
        return res, [ [ tree ] ]

    start = -1
    if tree[0] in ['statement_list', 'lt_expr', 'ne_expr', 'ge_expr', 'gt_expr', 'modify_expr', 'return_expr', 'plus_expr', 'minus_expr', 'cond_expr', 'le_expr', 'parameters', 'if_stmt']:
        start = 1
    elif tree[0] in ['float_expr']:
        start = 2
    elif tree[0] in ['call_expr']:
        start = 3
    # if start == -1:
    #     print('missing=',tree[0])
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
            }

def encode_leaf(leaf):
    if leaf[0] in [ 'parm_decl' ]:
        res = [ leafcodes[leaf[0]], typecodes[leaf[1]] ]
    elif leaf[0] in [ 'integer_cst' ]:
        res = [ leafcodes[leaf[0]] ]
    else:
        res = leaf
    return [ res ]

def encode_nodes(path):
    return [ nodecodes[n] for n in path ]

def encode(path):
    return encode_leaf(path[0]) + encode_nodes(path[1:-1]) + encode_leaf(path[-1])

def print_codes(name, codes):
    print('{}:'.format(name))
    for c in codes:
        print('  {}={}'.format(c, codes[c]))

if __name__ == '__main__':
    import sys
    files = read_asts(sys.argv[1:])
    for k in files:
        file = files[k]
        ast = file['content']
        print("function={} in {}".format(k, file['source']))
        tree = create_tree(ast)
        paths, _ = create_paths(tree)
        for p in paths:
            print(p)
            coded = encode(p)
            print(coded)
    print_codes('leafcodes', leafcodes)
    print_codes('typecodes', typecodes)
    print_codes('nodecodes', nodecodes)
