long ipow(long b, unsigned n)
{
  unsigned i;
  long rv;
  rv = 1;
  for(i=0; i<n; i++) {
    rv *= b;
  }
  return rv;
}
/*

g++  -fplugin=./astprint.so -c ipow.cc 2>&1 | ./read-ast-dump.py

function=ipow in ipow.cc:1

*/
