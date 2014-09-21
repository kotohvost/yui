/*
	$Id: hash.cc,v 3.2.2.1 2007/07/24 09:58:07 shelton Exp $
*/


unsigned long hashstr(const char* x)
{
  unsigned long h = 0;
  unsigned long g;

  while (*x)
  {
    h = (h << 4) + *x++;

    if ((g = h & 0xf0000000) != 0)
      h = (h ^ (g >> 24)) ^ g;

    /*
    if ((g = h & 0x0f000000) != 0)
      h = (h ^ (g >> 20)) ^ g;
    */
  }
  h &= 0x0fffffff;
  h |= 0x40000000;
  return h;
}

unsigned long hashdouble(double x)
{
  union { unsigned long i[2]; double d; } u;
  u.d = x;
  unsigned long u0 = u.i[0];
  unsigned long u1 = u.i[1];
  return u0 ^ u1;
}



unsigned long multiplicativehash(int x)
{
  // uses a const close to golden ratio * pow(2,32)
  return ((unsigned long)x) * 2654435767L;
}












