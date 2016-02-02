#ifndef THREADS_FIXED_POINT_H
#define THREADS_FIXED_POINT_H

#include <stdint.h>
typedef int fixed_F; /* F decimal points. */

fixed_F fixed_point(int n);
fixed_F fixed_frac(int p, int q);
fixed_F fixed_mult(fixed_F a, fixed_F b); 
fixed_F fixed_div(fixed_F p, fixed_F q);
int fixed_to_int(fixed_F x);


#endif /* threads/fixed-point.h */
