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
