int fib(unsigned n)
{
  if (n <= 1) {
    return 1;
  }
  return(fib(n-2) + fib(n-1));
}
/*

g++  -fplugin=./astprint.so -c fib.cc 2>&1 | ./read-ast-dump.py

function=fib in fib.cc:1
[['integer_cst', '1'], 'if_stmt', 'return_expr', ['integer_cst', '1']]
[[2], 7, 13, [2]]
[['integer_cst', '1'], 'statement_list', 'return_expr', 'plus_expr', 'call_expr', 'minus_expr', ['integer_cst', '2']]
[[2], 12, 13, 9, 16, 10, [2]]
[['integer_cst', '1'], 'statement_list', 'return_expr', 'plus_expr', 'call_expr', 'minus_expr', ['integer_cst', '2']]
[[2], 12, 13, 9, 16, 10, [2]]
[['integer_cst', '1'], 'statement_list', 'return_expr', 'plus_expr', 'call_expr', 'minus_expr', ['integer_cst', '1']]
[[2], 12, 13, 9, 16, 10, [2]]
[['integer_cst', '1'], 'statement_list', 'return_expr', 'plus_expr', 'call_expr', 'minus_expr', ['integer_cst', '1']]
[[2], 12, 13, 9, 16, 10, [2]]
[['integer_cst', '2'], 'plus_expr', 'call_expr', 'minus_expr', ['integer_cst', '1']]
[[2], 9, 16, 10, [2]]

*/
