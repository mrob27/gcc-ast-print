double qdisc(double a, double b, double c)
{
  return(b*b - 4.0*a*c);
}
/*

g++  -fplugin=./astprint.so -c qdisc.cc 2>&1 | ./read-ast-dump.py

function=qdisc in qdisc.cc:1
missing= ['mult_expr']
Traceback (most recent call last):
  File "./read-ast-dump.py", line 316, in <module>
    paths, _ = create_paths(tree)
  File "./read-ast-dump.py", line 248, in create_paths
    subres, subuppaths = create_paths(tree[i], newdownpaths)
  File "./read-ast-dump.py", line 248, in create_paths
    subres, subuppaths = create_paths(tree[i], newdownpaths)
  File "./read-ast-dump.py", line 238, in create_paths
    assert start != -1
AssertionError

*/
