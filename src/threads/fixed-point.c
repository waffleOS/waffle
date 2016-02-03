#include "fixed-point.h"

#define FRAC_DIGITS 14 /* Number of fractional digits in binary. */
#define F (1 << FRAC_DIGITS)

fixed_F fixed_point(int n) {
    return (fixed_F) n * F;
}

fixed_F fixed_frac(int p, int q) { 
    return p * F / q;
}

fixed_F fixed_mult(fixed_F a, fixed_F b) { 
    return (fixed_F) (((int64_t) a) * b / F);
}

fixed_F fixed_div(fixed_F p, fixed_F q) { 
    return (fixed_F) (((int64_t) p) * F / q);
}

int fixed_to_int(fixed_F x) {
    int ans;
    if (x >= 0) { 
        ans = (x + F / 2) / F;
    } 
    else { 
        ans = (x - F / 2) / F;
    }
    return ans;
}
