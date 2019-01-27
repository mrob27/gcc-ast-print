int f1(int a)
{
#pragma GCCPLUGIN assume a < 5
  if (a < 10)
    a += 1;
  else
    a -= 1;
  return a;
}

double f2(int a, double g)
{
	if (a & 1)
#pragma GCCPLUGIN assume a > 0
    g += a < 0 ? -1 : 1;
  return g;
}

int fak(unsigned n)
{
	if (n <= 1)
    return 1;
  return fak(n-2) + fak(n-1);
}
