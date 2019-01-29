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

if __name__ == '__main__':
    import sys
    files = read_asts(sys.argv[1:])
    for k in files:
        file = files[k]
        print("function={} in {}".format(k, file['source']))
        ast = file['content']
        print(ast[1])
