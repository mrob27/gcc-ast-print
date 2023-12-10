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
 20231210 Add iter2() and iter4() from iterfl-caad.c

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

#define MAX_ITER 252

int iter2(float cr, float ci, int ref_period, float *Zref_r, float *Zref_i, int nmax)
{
  float zr, zi, zr2, zi2, Zr, Zi, abs_z, abs_Zz, zr3, zi3;
  int i, m;
  m = 0; zr = zi = 0;
  for (i=0; i<nmax; ++i) {
    Zr = Zref_r[m]; Zi = Zref_i[m];
    abs_Zz = (Zr+zr)*(Zr+zr) + (Zi+zi)*(Zi+zi);
    abs_z = zr*zr + zi*zi;
    if (abs_Zz < abs_z) {
      zr = Zr + zr;
      m = 0;
      Zr = Zref_r[m]; Zi = Zref_i[m];
    }
    abs_Zz = (Zr+zr)*(Zr+zr) + (Zi+zi)*(Zi+zi);
    if (abs_Zz > 8.0) {
      return i;
    }
    zr2 = Zr*2.0 + zr; // (2 * Z + z)
    zi2 = Zi*2.0 + zi; // (2 * Z + z)
    zr3 = zr2*zr - zi2*zi + cr; // (2 * Z + z) * z + c
    zi3 = zr2*zi + zi2*zr + ci; // (2 * Z + z) * z + c
    zr = zr3; zi = zi3;
    if (++m >= ref_period) {
      m = 0;
    }
  }
  return i;
} // End of iter2

#define I3STEP1(Zr, Zi, m, zr, zi, aZz, az) \
    (Zr) = Zref_r[(m)]; (Zi) = Zref_i[(m)]; \
    (aZz) = ((Zr)+(zr))*((Zr)+(zr)) + ((Zi)+(zi))*((Zi)+(zi)); \
    (az) = (zr)*(zr) + (zi)*(zi);
#define I3STEP2(Zr, Zi, m, zr, zi, aZz, az) \
    if ((aZz) < (az)) { \
      (zr) = (Zr) + (zr); (zi) = (Zi) + (zi); \
      (m) = 0; \
      (Zr) = Zref_r[(m)]; (Zi) = Zref_i[(m)]; \
    } \
    (aZz) = ((Zr)+(zr))*((Zr)+(zr)) + ((Zi)+(zi))*((Zi)+(zi));
#define I3STEP3(going, aZz, nogg, ans, i) \
    (going) = (going) & (-((aZz) < 8.0)); \
    (nogg) = ~(going); \
    (ans) = ((going) & ((i)+1)) | ((nogg) & (ans));
#define I3STEP4(zr2, zi2, zr3, zi3, Zr, Zi, zr, zi, cr, ci, m) \
    (zr2) = (Zr)*2.0 + (zr); \
    (zi2) = (Zi)*2.0 + (zi); \
    (zr3) = (zr2)*(zr) - (zi2)*(zi) + (cr); \
    (zi3) = (zr2)*(zi) + (zi2)*(zr) + (ci); \
    (zr) = (zr3); (zi) = (zi3); \
    if (++(m) >= ref_period) { (m) = 0; }

int iter4(float cra, float cia, float crb, float cib,
  float crc, float cic,  float crd, float cid,
  int ref_period, float *Zref_r, float *Zref_i)
{
  float zra, zia, zr2a, zi2a, Zra, Zia, abs_za, abs_Zza, zr3a, zi3a;
  int ma, goinga, nogga, ansa;
  float zrb, zib, zr2b, zi2b, Zrb, Zib, abs_zb, abs_Zzb, zr3b, zi3b;
  int mb, goingb, noggb, ansb;
  float zrc, zic, zr2c, zi2c, Zrc, Zic, abs_zc, abs_Zzc, zr3c, zi3c;
  int mc, goingc, noggc, ansc;
  float zrd, zid, zr2d, zi2d, Zrd, Zid, abs_zd, abs_Zzd, zr3d, zi3d;
  int md, goingd, noggd, ansd;
  int i;
  ma = 0; zra = zia = 0; goinga = -1; nogga = ansa = 0;
  mb = 0; zrb = zib = 0; goingb = -1; noggb = ansb = 0;
  mc = 0; zrc = zic = 0; goingc = -1; noggc = ansc = 0;
  md = 0; zrd = zid = 0; goingd = -1; noggd = ansd = 0;
  for (i=0; i<MAX_ITER; ++i) {
    I3STEP1(Zra, Zia, ma, zra, zia, abs_Zza, abs_za)
    I3STEP2(Zra, Zia, ma, zra, zia, abs_Zza, abs_za)
    I3STEP3(goinga, abs_Zza, nogga, ansa, i)
    I3STEP4(zr2a, zi2a, zr3a, zi3a, Zra, Zia, zra, zia, cra, cia, ma)
    I3STEP1(Zrb, Zib, mb, zrb, zib, abs_Zzb, abs_zb)
    I3STEP2(Zrb, Zib, mb, zrb, zib, abs_Zzb, abs_zb)
    I3STEP3(goingb, abs_Zzb, noggb, ansb, i)
    I3STEP4(zr2b, zi2b, zr3b, zi3b, Zrb, Zib, zrb, zib, crb, cib, mb)
    I3STEP1(Zrc, Zic, mc, zrc, zic, abs_Zzc, abs_zc)
    I3STEP2(Zrc, Zic, mc, zrc, zic, abs_Zzc, abs_zc)
    I3STEP3(goingc, abs_Zzc, noggc, ansc, i)
    I3STEP4(zr2c, zi2c, zr3c, zi3c, Zrc, Zic, zrc, zic, crc, cic, mc)
    I3STEP1(Zrd, Zid, md, zrd, zid, abs_Zzd, abs_zd)
    I3STEP2(Zrd, Zid, md, zrd, zid, abs_Zzd, abs_zd)
    I3STEP3(goingd, abs_Zzd, noggd, ansd, i)
    I3STEP4(zr2d, zi2d, zr3d, zi3d, Zrd, Zid, zrd, zid, crd, cid, md)
  }
  return (ansa<<21) ^ (ansb<<14) ^ (ansc<<7) ^ ansd;
} // End of iter4
