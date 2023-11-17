/*
 REVISION HISTORY
  20201023 Add fpow for use by qd2; add qd2 - it is evident that we
get no paths from any function consisting entirely of
"return(some_expression)". Add qd3, which generates paths by using a
local variable (but is otherwise semantically equivalent to qd2).
 20201030 Correct fib(), fak() now uses less recursion
 20201103 fak() now calls itself instead of fib()
 20201111 Add fibnr()
 20201119 Simplify qdf() to help diagnosing the duplicate paths issue
 20201120 Add euler()

*/

#include <stdint.h>

unsigned long euler(unsigned long a, unsigned long b)
{
  while (a != b) {
    if (a > b) {
      a = a - b;
    } else {
      b = b - a;
    }
  }
  return a;
}

int f1(int a)
{
#pragma GCCPLUGIN assume a < 5
  if (a < 10) {
    a += 1;
  } else {
    a -= 1;
  }
  return a;
}

/* Corrected Fibonacci (originally called "fak") */
int fib(unsigned n)
{
  if (n <= 1) {
    return n;
  }
  return(fib(n-2) + fib(n-1));
}

/* Non-recursive Fibonacci (because recursive cannot be done in hardware) */
unsigned long fibnr(unsigned short n)
{
  unsigned short i;
  unsigned long p1, p2, rv;
  if (n <= 1) {
    return ((unsigned long) n);
  }
  p1 = 1; p2 = 0;
  for (i=2; i<=n; i++) {
    rv = p2+p1;
    p2 = p1;
    p1 = rv;
  }
  return (rv);
}

/* Fibonacci taking a short argument and returning a long result */
long int fbs(unsigned short n)
{
  if (n <= 1) {
    return ((long int) n);
  }
  return(fbs(n-2) + fbs(n-1));
}


/* less recursion */
unsigned fak(unsigned n)
{ /*                      0 1 2 3 4 */
  if (n <= 1u) {
    return n;          /* 0 1       */
  } else if (n <= 3u) {
    return n-1u;        /*     1 2 3 */
  } /*       (n > 3) */
  return(2u*fak(n-4u) + 3u*fak(n-3u));
}


double fpow(double b, unsigned n)
{
  unsigned i;
  double rv;
  rv = 1.0;
  for(i=0; i<n; i++) {
    rv *= b;
  }
  return rv;
}


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

__int128 llpow(__int128 b, unsigned n)
{
  unsigned i;
  __int128 rv;
  rv = 1;
  for(i=0; i<n; i++) {
    rv *= b;
  }
  return rv;
}

long double min(long double a, long double b)
{
  return (a<b) ? a : b;
}

double qd2(double a, double b, double c)
{
  return(fpow(b, 2) - 4.0*a*c);
}

double qd3(double a, double b, double c)
{
  double rv;

  rv = b*b;
  rv -= 4.0*a*c;
  return(rv);
}

float qdf(float a, float b, float c)
{
  return(b*b-4.0);
}

double qdisc(double a, double b, double c)
{
  return(b*b - 4.0*a*c);
}

/* rotate right by composite of shifts and bit_ior. I tried
   a few different versions found online and couldn't find
   anything that actually produces a rrotate_expr node */
uint32_t ror(uint32_t n, uint32_t c)
{
  /* const uint32_t mask = (8*sizeof(n)-1);
  c &= mask;
  return (n<<c) | (n>>((-c)&mask) */
/*  return ((n>>c) | (n<<((sizeof(n)<<3)-c))); */
  return ((n>>c) | (n<<(32-c)));

}

int slen(char * s)
{
  int rv = 0;
  while(*s) {
    rv++;
    s++;
  }
  return rv;
}

double f2(int a, double g)
{
  if (a & 1) {
#pragma GCCPLUGIN assume a > 0
    g += a < 0 ? -1 : 1;
  }
  return g;
}
