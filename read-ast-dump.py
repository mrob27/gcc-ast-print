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


def create_tree(tree, nodenr):
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
        res.append(get_type(tree, node['type']))
        res.append(create_tree(tree, node['cond']))
        res.append(create_tree(tree, node['then']))
        if 'else' in node:
            res.append(create_tree(tree, node['else']))
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
        args = []
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

if __name__ == '__main__':
    import sys
    files = read_asts(sys.argv[1:])
    for k in files:
        file = files[k]
        ast = file['content']
        print("function={} in {}".format(k, file['source']))
        print(create_tree(ast, 1))
