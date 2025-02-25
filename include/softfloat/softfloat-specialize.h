/*============================================================================
This C source fragment is part of the SoftFloat IEC/IEEE Floating-point
Arithmetic Package, Release 2b.

Written by John R. Hauser.  This work was made possible in part by the
International Computer Science Institute, located at Suite 600, 1947 Center
Street, Berkeley, California 94704.  Funding was partially provided by the
National Science Foundation under grant MIP-9311980.  The original version
of this code was written as part of a project to build a fixed-point vector
processor in collaboration with the University of California at Berkeley,
overseen by Profs. Nelson Morgan and John Wawrzynek.  More information
is available through the Web page `http://www.cs.berkeley.edu/~jhauser/
arithmetic/SoftFloat.html'.

THIS SOFTWARE IS DISTRIBUTED AS IS, FOR FREE.  Although reasonable effort has
been made to avoid it, THIS SOFTWARE MAY CONTAIN FAULTS THAT WILL AT TIMES
RESULT IN INCORRECT BEHAVIOR.  USE OF THIS SOFTWARE IS RESTRICTED TO PERSONS
AND ORGANIZATIONS WHO CAN AND WILL TAKE FULL RESPONSIBILITY FOR ALL LOSSES,
COSTS, OR OTHER PROBLEMS THEY INCUR DUE TO THE SOFTWARE, AND WHO FURTHERMORE
EFFECTIVELY INDEMNIFY JOHN HAUSER AND THE INTERNATIONAL COMPUTER SCIENCE
INSTITUTE (possibly via similar legal warning) AGAINST ALL LOSSES, COSTS, OR
OTHER PROBLEMS INCURRED BY THEIR CUSTOMERS AND CLIENTS DUE TO THE SOFTWARE.

Derivative works are acceptable, even for commercial purposes, so long as
(1) the source code for the derivative work includes prominent notice that
the work is derivative, and (2) the source code includes prominent notice with
these four paragraphs for those parts of this code that are retained.
=============================================================================*/

#ifndef _SOFTFLOAT_SPECIALIZE_H_
#define _SOFTFLOAT_SPECIALIZE_H_

#include "softfloat.h"

/*============================================================================
 * Adapted for Bochs (x86 achitecture simulator) by
 *            Stanislav Shwartsman [sshwarts at sourceforge net]
 * ==========================================================================*/

#define int16_indefinite ((int16_t)0x8000)
#define int32_indefinite ((int32_t)0x80000000)
#define int64_indefinite U64(0x8000000000000000)

#define uint16_indefinite (0xffff)
#define uint32_indefinite (0xffffffff)
#define uint64_indefinite U64(0xffffffffffffffff)

/*----------------------------------------------------------------------------
| Internal canonical NaN format.
*----------------------------------------------------------------------------*/

typedef struct {
    int sign;
    uint64_t hi, lo;
} commonNaNT;

#ifdef FLOAT16

/*----------------------------------------------------------------------------
| The pattern for a default generated half-precision NaN.
*----------------------------------------------------------------------------*/
#ifdef GET_CONSTANTS
const float16 float16_default_nan = 0xFE00;
#endif

#define float16_fraction extractFloat16Frac
#define float16_exp extractFloat16Exp
#define float16_sign extractFloat16Sign

/*----------------------------------------------------------------------------
| Returns the fraction bits of the half-precision floating-point value `a'.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE uint16_t extractFloat16Frac(float16 a)
{
    return a & 0x3FF;
}

/*----------------------------------------------------------------------------
| Returns the exponent bits of the half-precision floating-point value `a'.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE int16_t extractFloat16Exp(float16 a)
{
    return (a>>10) & 0x1F;
}

/*----------------------------------------------------------------------------
| Returns the sign bit of the half-precision floating-point value `a'.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE int extractFloat16Sign(float16 a)
{
    return a>>15;
}

/*----------------------------------------------------------------------------
| Packs the sign `zSign', exponent `zExp', and significand `zSig' into a
| single-precision floating-point value, returning the result.  After being
| shifted into the proper positions, the three fields are simply added
| together to form the result.  This means that any integer portion of `zSig'
| will be added into the exponent.  Since a properly normalized significand
| will have an integer portion equal to 1, the `zExp' input should be 1 less
| than the desired result exponent whenever `zSig' is a complete, normalized
| significand.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE float16 packFloat16(int zSign, int zExp, uint16_t zSig)
{
    return (((uint16_t) zSign)<<15) + (((uint16_t) zExp)<<10) + zSig;
}

/*----------------------------------------------------------------------------
| Returns 1 if the half-precision floating-point value `a' is a NaN;
| otherwise returns 0.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE int float16_is_nan(float16 a)
{
    return (0xF800 < (uint16_t) (a<<1));
}

/*----------------------------------------------------------------------------
| Returns 1 if the half-precision floating-point value `a' is a signaling
| NaN; otherwise returns 0.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE int float16_is_signaling_nan(float16 a)
{
    return (((a>>9) & 0x3F) == 0x3E) && (a & 0x1FF);
}

/*----------------------------------------------------------------------------
| Returns 1 if the half-precision floating-point value `a' is denormal;
| otherwise returns 0.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE int float16_is_denormal(float16 a)
{
   return (extractFloat16Exp(a) == 0) && (extractFloat16Frac(a) != 0);
}

/*----------------------------------------------------------------------------
| Convert float16 denormals to zero.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE float16 float16_denormal_to_zero(float16 a)
{
  if (float16_is_denormal(a)) a &= 0x8000;
  return a;
}

/*----------------------------------------------------------------------------
| Returns the result of converting the half-precision floating-point NaN
| `a' to the canonical NaN format. If `a' is a signaling NaN, the invalid
| exception is raised.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE commonNaNT float16ToCommonNaN(float16 a, float_status_t *status)
{
    commonNaNT z;
    if (float16_is_signaling_nan(a)) float_raise(status, float_flag_invalid);
    z.sign = a>>15;
    z.lo = 0;
    z.hi = ((uint64_t) a)<<54;
    return z;
}

/*----------------------------------------------------------------------------
| Returns the result of converting the canonical NaN `a' to the half-
| precision floating-point format.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE float16 commonNaNToFloat16(commonNaNT a)
{
    return (((uint16_t) a.sign)<<15) | 0x7E00 | (uint16_t)(a.hi>>54);
}

#endif

#ifdef GET_CONSTANTS
/*----------------------------------------------------------------------------
| Commonly used single-precision floating point constants
*----------------------------------------------------------------------------*/
const float32 float32_negative_inf  = 0xff800000;
const float32 float32_positive_inf  = 0x7f800000;
const float32 float32_negative_zero = 0x80000000;
const float32 float32_positive_zero = 0x00000000;
const float32 float32_negative_one  = 0xbf800000;
const float32 float32_positive_one  = 0x3f800000;
const float32 float32_max_float     = 0x7f7fffff;
const float32 float32_min_float     = 0xff7fffff;

/*----------------------------------------------------------------------------
| The pattern for a default generated single-precision NaN.
*----------------------------------------------------------------------------*/
const float32 float32_default_nan   = 0xffc00000;
#endif

#define float32_fraction extractFloat32Frac
#define float32_exp extractFloat32Exp
#define float32_sign extractFloat32Sign

#define FLOAT32_EXP_BIAS 0x7F

/*----------------------------------------------------------------------------
| Returns the fraction bits of the single-precision floating-point value `a'.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE uint32_t extractFloat32Frac(float32 a)
{
    return a & 0x007FFFFF;
}

/*----------------------------------------------------------------------------
| Returns the exponent bits of the single-precision floating-point value `a'.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE int16_t extractFloat32Exp(float32 a)
{
    return (a>>23) & 0xFF;
}

/*----------------------------------------------------------------------------
| Returns the sign bit of the single-precision floating-point value `a'.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE int extractFloat32Sign(float32 a)
{
    return a>>31;
}

/*----------------------------------------------------------------------------
| Packs the sign `zSign', exponent `zExp', and significand `zSig' into a
| single-precision floating-point value, returning the result.  After being
| shifted into the proper positions, the three fields are simply added
| together to form the result.  This means that any integer portion of `zSig'
| will be added into the exponent.  Since a properly normalized significand
| will have an integer portion equal to 1, the `zExp' input should be 1 less
| than the desired result exponent whenever `zSig' is a complete, normalized
| significand.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE float32 packFloat32(int zSign, int16_t zExp, uint32_t zSig)
{
    return (((uint32_t) zSign)<<31) + (((uint32_t) zExp)<<23) + zSig;
}

/*----------------------------------------------------------------------------
| Returns 1 if the single-precision floating-point value `a' is a NaN;
| otherwise returns 0.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE int float32_is_nan(float32 a)
{
    return (0xFF000000 < (uint32_t) (a<<1));
}

/*----------------------------------------------------------------------------
| Returns 1 if the single-precision floating-point value `a' is a signaling
| NaN; otherwise returns 0.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE int float32_is_signaling_nan(float32 a)
{
    return (((a>>22) & 0x1FF) == 0x1FE) && (a & 0x003FFFFF);
}

/*----------------------------------------------------------------------------
| Returns 1 if the single-precision floating-point value `a' is denormal;
| otherwise returns 0.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE int float32_is_denormal(float32 a)
{
   return (extractFloat32Exp(a) == 0) && (extractFloat32Frac(a) != 0);
}

/*----------------------------------------------------------------------------
| Convert float32 denormals to zero.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE float32 float32_denormal_to_zero(float32 a)
{
  if (float32_is_denormal(a)) a &= 0x80000000;
  return a;
}

/*----------------------------------------------------------------------------
| Returns the result of converting the single-precision floating-point NaN
| `a' to the canonical NaN format.  If `a' is a signaling NaN, the invalid
| exception is raised.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE commonNaNT float32ToCommonNaN(float32 a, float_status_t *status)
{
    commonNaNT z;
    if (float32_is_signaling_nan(a)) float_raise(status, float_flag_invalid);
    z.sign = a>>31;
    z.lo = 0;
    z.hi = ((uint64_t) a)<<41;
    return z;
}

/*----------------------------------------------------------------------------
| Returns the result of converting the canonical NaN `a' to the single-
| precision floating-point format.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE float32 commonNaNToFloat32(commonNaNT a)
{
    return (((uint32_t) a.sign)<<31) | 0x7FC00000 | (uint32_t)(a.hi>>41);
}

/*----------------------------------------------------------------------------
| Takes two single-precision floating-point values `a' and `b', one of which
| is a NaN, and returns the appropriate NaN result.  If either `a' or `b' is a
| signaling NaN, the invalid exception is raised.
*----------------------------------------------------------------------------*/

float32 propagateFloat32NaN_two_args(float32 a, float32 b, float_status_t *status);

/*----------------------------------------------------------------------------
| Takes single-precision floating-point NaN `a' and returns the appropriate
| NaN result.  If `a' is a signaling NaN, the invalid exception is raised.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE float32 propagateFloat32NaN(float32 a, float_status_t *status)
{
    if (float32_is_signaling_nan(a))
        float_raise(status, float_flag_invalid);

    return a | 0x00400000;
}

#ifdef GET_CONSTANTS
/*----------------------------------------------------------------------------
| Commonly used single-precision floating point constants
*----------------------------------------------------------------------------*/
const float64 float64_negative_inf  = U64(0xfff0000000000000);
const float64 float64_positive_inf  = U64(0x7ff0000000000000);
const float64 float64_negative_zero = U64(0x8000000000000000);
const float64 float64_positive_zero = U64(0x0000000000000000);
const float64 float64_negative_one  = U64(0xbff0000000000000);
const float64 float64_positive_one  = U64(0x3ff0000000000000);
const float64 float64_max_float     = U64(0x7fefffffffffffff);
const float64 float64_min_float     = U64(0xffefffffffffffff);

/*----------------------------------------------------------------------------
| The pattern for a default generated double-precision NaN.
*----------------------------------------------------------------------------*/
const float64 float64_default_nan = U64(0xFFF8000000000000);
#endif

#define float64_fraction extractFloat64Frac
#define float64_exp extractFloat64Exp
#define float64_sign extractFloat64Sign

#define FLOAT64_EXP_BIAS 0x3FF

/*----------------------------------------------------------------------------
| Returns the fraction bits of the double-precision floating-point value `a'.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE uint64_t extractFloat64Frac(float64 a)
{
    return a & U64(0x000FFFFFFFFFFFFF);
}

/*----------------------------------------------------------------------------
| Returns the exponent bits of the double-precision floating-point value `a'.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE int16_t extractFloat64Exp(float64 a)
{
    return (int16_t)(a>>52) & 0x7FF;
}

/*----------------------------------------------------------------------------
| Returns the sign bit of the double-precision floating-point value `a'.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE int extractFloat64Sign(float64 a)
{
    return (int)(a>>63);
}

/*----------------------------------------------------------------------------
| Packs the sign `zSign', exponent `zExp', and significand `zSig' into a
| double-precision floating-point value, returning the result.  After being
| shifted into the proper positions, the three fields are simply added
| together to form the result.  This means that any integer portion of `zSig'
| will be added into the exponent.  Since a properly normalized significand
| will have an integer portion equal to 1, the `zExp' input should be 1 less
| than the desired result exponent whenever `zSig' is a complete, normalized
| significand.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE float64 packFloat64(int zSign, int16_t zExp, uint64_t zSig)
{
    return (((uint64_t) zSign)<<63) + (((uint64_t) zExp)<<52) + zSig;
}

/*----------------------------------------------------------------------------
| Returns 1 if the double-precision floating-point value `a' is a NaN;
| otherwise returns 0.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE int float64_is_nan(float64 a)
{
    return (U64(0xFFE0000000000000) < (uint64_t) (a<<1));
}

/*----------------------------------------------------------------------------
| Returns 1 if the double-precision floating-point value `a' is a signaling
| NaN; otherwise returns 0.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE int float64_is_signaling_nan(float64 a)
{
    return (((a>>51) & 0xFFF) == 0xFFE) && (a & U64(0x0007FFFFFFFFFFFF));
}

/*----------------------------------------------------------------------------
| Returns 1 if the double-precision floating-point value `a' is denormal;
| otherwise returns 0.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE int float64_is_denormal(float64 a)
{
   return (extractFloat64Exp(a) == 0) && (extractFloat64Frac(a) != 0);
}

/*----------------------------------------------------------------------------
| Convert float64 denormals to zero.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE float64 float64_denormal_to_zero(float64 a)
{
  if (float64_is_denormal(a)) a &= ((uint64_t)(1) << 63);
  return a;
}

/*----------------------------------------------------------------------------
| Returns the result of converting the double-precision floating-point NaN
| `a' to the canonical NaN format.  If `a' is a signaling NaN, the invalid
| exception is raised.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE commonNaNT float64ToCommonNaN(float64 a, float_status_t *status)
{
    commonNaNT z;
    if (float64_is_signaling_nan(a)) float_raise(status, float_flag_invalid);
    z.sign = (int)(a>>63);
    z.lo = 0;
    z.hi = a<<12;
    return z;
}

/*----------------------------------------------------------------------------
| Returns the result of converting the canonical NaN `a' to the double-
| precision floating-point format.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE float64 commonNaNToFloat64(commonNaNT a)
{
    return (((uint64_t) a.sign)<<63) | U64(0x7FF8000000000000) | (a.hi>>12);
}

/*----------------------------------------------------------------------------
| Takes two double-precision floating-point values `a' and `b', one of which
| is a NaN, and returns the appropriate NaN result.  If either `a' or `b' is a
| signaling NaN, the invalid exception is raised.
*----------------------------------------------------------------------------*/

float64 propagateFloat64NaN_two_args(float64 a, float64 b, float_status_t *status);

/*----------------------------------------------------------------------------
| Takes double-precision floating-point NaN `a' and returns the appropriate
| NaN result.  If `a' is a signaling NaN, the invalid exception is raised.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE float64 propagateFloat64NaN(float64 a, float_status_t *status)
{
    if (float64_is_signaling_nan(a))
        float_raise(status, float_flag_invalid);

    return a | U64(0x0008000000000000);
}

#ifdef FLOATX80

/*----------------------------------------------------------------------------
| The pattern for a default generated extended double-precision NaN.  The
| `high' and `low' values hold the most- and least-significant bits,
| respectively.
*----------------------------------------------------------------------------*/
#define floatx80_default_nan_exp 0xFFFF
#define floatx80_default_nan_fraction U64(0xC000000000000000)

#define floatx80_fraction extractFloatx80Frac
#define floatx80_exp extractFloatx80Exp
#define floatx80_sign extractFloatx80Sign

#define FLOATX80_EXP_BIAS 0x3FFF

/*----------------------------------------------------------------------------
| Returns the fraction bits of the extended double-precision floating-point
| value `a'.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE uint64_t extractFloatx80Frac(floatx80 a)
{
    return a.fraction;
}

/*----------------------------------------------------------------------------
| Returns the exponent bits of the extended double-precision floating-point
| value `a'.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE int32_t extractFloatx80Exp(floatx80 a)
{
    return a.exp & 0x7FFF;
}

/*----------------------------------------------------------------------------
| Returns the sign bit of the extended double-precision floating-point value
| `a'.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE int extractFloatx80Sign(floatx80 a)
{
    return a.exp>>15;
}

/*----------------------------------------------------------------------------
| Packs the sign `zSign', exponent `zExp', and significand `zSig' into an
| extended double-precision floating-point value, returning the result.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE floatx80 packFloatx80(int zSign, int32_t zExp, uint64_t zSig)
{
    floatx80 z;
    z.fraction = zSig;
    z.exp = (zSign << 15) + zExp;
    return z;
}

/*----------------------------------------------------------------------------
| Returns 1 if the extended double-precision floating-point value `a' is a
| NaN; otherwise returns 0.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE int floatx80_is_nan(floatx80 a)
{
    // return ((a.exp & 0x7FFF) == 0x7FFF) && (int64_t) (a.fraction<<1);
    return ((a.exp & 0x7FFF) == 0x7FFF) && (((int64_t) (a.fraction<<1)) != 0);
}

/*----------------------------------------------------------------------------
| Returns 1 if the extended double-precision floating-point value `a' is a
| signaling NaN; otherwise returns 0.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE int floatx80_is_signaling_nan(floatx80 a)
{
    uint64_t aLow = a.fraction & ~U64(0x4000000000000000);
    return ((a.exp & 0x7FFF) == 0x7FFF) &&
            ((uint64_t) (aLow<<1)) && (a.fraction == aLow);
}

/*----------------------------------------------------------------------------
| Returns 1 if the extended double-precision floating-point value `a' is an
| unsupported; otherwise returns 0.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE int floatx80_is_unsupported(floatx80 a)
{
    return ((a.exp & 0x7FFF) && !(a.fraction & U64(0x8000000000000000)));
}

/*----------------------------------------------------------------------------
| Returns the result of converting the extended double-precision floating-
| point NaN `a' to the canonical NaN format. If `a' is a signaling NaN, the
| invalid exception is raised.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE commonNaNT floatx80ToCommonNaN(floatx80 a, float_status_t *status)
{
    commonNaNT z;
    if (floatx80_is_signaling_nan(a)) float_raise(status, float_flag_invalid);
    z.sign = a.exp >> 15;
    z.lo = 0;
    z.hi = a.fraction << 1;
    return z;
}

/*----------------------------------------------------------------------------
| Returns the result of converting the canonical NaN `a' to the extended
| double-precision floating-point format.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE floatx80 commonNaNToFloatx80(commonNaNT a)
{
    floatx80 z;
    z.fraction = U64(0xC000000000000000) | (a.hi>>1);
    z.exp = (((uint16_t) a.sign)<<15) | 0x7FFF;
    return z;
}

/*----------------------------------------------------------------------------
| Takes two extended double-precision floating-point values `a' and `b', one
| of which is a NaN, and returns the appropriate NaN result.  If either `a' or
| `b' is a signaling NaN, the invalid exception is raised.
*----------------------------------------------------------------------------*/

floatx80 propagateFloatx80NaN_two_args(floatx80 a, floatx80 b, float_status_t *status);

/*----------------------------------------------------------------------------
| Takes extended double-precision floating-point  NaN  `a' and returns the
| appropriate NaN result. If `a' is a signaling NaN, the invalid exception
| is raised.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE floatx80 propagateFloatx80NaN(floatx80 a, float_status_t *status)
{
    if (floatx80_is_signaling_nan(a))
        float_raise(status, float_flag_invalid);

    a.fraction |= U64(0xC000000000000000);

    return a;
}

/*----------------------------------------------------------------------------
| The pattern for a default generated extended double-precision NaN.
*----------------------------------------------------------------------------*/
extern const floatx80 floatx80_default_nan;

#endif /* FLOATX80 */

#ifdef FLOAT128

#include "softfloat-macros.h"

/*----------------------------------------------------------------------------
| The pattern for a default generated quadruple-precision NaN. The `high' and
| `low' values hold the most- and least-significant bits, respectively.
*----------------------------------------------------------------------------*/
#define float128_default_nan_hi U64(0xFFFF800000000000)
#define float128_default_nan_lo U64(0x0000000000000000)

#define float128_exp extractFloat128Exp

/*----------------------------------------------------------------------------
| Returns the least-significant 64 fraction bits of the quadruple-precision
| floating-point value `a'.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE uint64_t extractFloat128Frac1(float128 a)
{
    return a.lo;
}

/*----------------------------------------------------------------------------
| Returns the most-significant 48 fraction bits of the quadruple-precision
| floating-point value `a'.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE uint64_t extractFloat128Frac0(float128 a)
{
    return a.hi & U64(0x0000FFFFFFFFFFFF);
}

/*----------------------------------------------------------------------------
| Returns the exponent bits of the quadruple-precision floating-point value
| `a'.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE int32_t extractFloat128Exp(float128 a)
{
    return ((int32_t)(a.hi>>48)) & 0x7FFF;
}

/*----------------------------------------------------------------------------
| Returns the sign bit of the quadruple-precision floating-point value `a'.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE int extractFloat128Sign(float128 a)
{
    return (int)(a.hi >> 63);
}

/*----------------------------------------------------------------------------
| Packs the sign `zSign', the exponent `zExp', and the significand formed
| by the concatenation of `zSig0' and `zSig1' into a quadruple-precision
| floating-point value, returning the result.  After being shifted into the
| proper positions, the three fields `zSign', `zExp', and `zSig0' are simply
| added together to form the most significant 32 bits of the result.  This
| means that any integer portion of `zSig0' will be added into the exponent.
| Since a properly normalized significand will have an integer portion equal
| to 1, the `zExp' input should be 1 less than the desired result exponent
| whenever `zSig0' and `zSig1' concatenated form a complete, normalized
| significand.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE float128 packFloat128(int zSign, int32_t zExp, uint64_t zSig0, uint64_t zSig1)
{
    float128 z;
    z.lo = zSig1;
    z.hi = (((uint64_t) zSign)<<63) + (((uint64_t) zExp)<<48) + zSig0;
    return z;
}

/*----------------------------------------------------------------------------
| Packs two 64-bit precision integers into into the quadruple-precision
| floating-point value, returning the result.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE float128 packFloat128_simple(uint64_t zHi, uint64_t zLo)
{
    float128 z;
    z.lo = zLo;
    z.hi = zHi;
    return z;
}

#ifdef _MSC_VER
#define PACK_FLOAT_128(hi,lo) { lo, hi }
#else
//#define PACK_FLOAT_128(hi,lo) packFloat128(U64(hi),U64(lo))
#define PACK_FLOAT_128(high, low) { .hi = high, .lo = low}
#endif

/*----------------------------------------------------------------------------
| Returns 1 if the quadruple-precision floating-point value `a' is a NaN;
| otherwise returns 0.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE int float128_is_nan(float128 a)
{
    return (U64(0xFFFE000000000000) <= (uint64_t) (a.hi<<1))
        && (a.lo || (a.hi & U64(0x0000FFFFFFFFFFFF)));
}

/*----------------------------------------------------------------------------
| Returns 1 if the quadruple-precision floating-point value `a' is a
| signaling NaN; otherwise returns 0.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE int float128_is_signaling_nan(float128 a)
{
    return (((a.hi>>47) & 0xFFFF) == 0xFFFE)
        && (a.lo || (a.hi & U64(0x00007FFFFFFFFFFF)));
}

/*----------------------------------------------------------------------------
| Returns the result of converting the quadruple-precision floating-point NaN
| `a' to the canonical NaN format.  If `a' is a signaling NaN, the invalid
| exception is raised.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE commonNaNT float128ToCommonNaN(float128 a, float_status_t *status)
{
    commonNaNT z;
    if (float128_is_signaling_nan(a)) float_raise(status, float_flag_invalid);
    z.sign = (int)(a.hi>>63);
    shortShift128Left(a.hi, a.lo, 16, &z.hi, &z.lo);
    return z;
}

/*----------------------------------------------------------------------------
| Returns the result of converting the canonical NaN `a' to the quadruple-
| precision floating-point format.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE float128 commonNaNToFloat128(commonNaNT a)
{
    float128 z;
    shift128Right(a.hi, a.lo, 16, &z.hi, &z.lo);
    z.hi |= (((uint64_t) a.sign)<<63) | U64(0x7FFF800000000000);
    return z;
}

/*----------------------------------------------------------------------------
| Takes two quadruple-precision floating-point values `a' and `b', one of
| which is a NaN, and returns the appropriate NaN result.  If either `a' or
| `b' is a signaling NaN, the invalid exception is raised.
*----------------------------------------------------------------------------*/

float128 propagateFloat128NaN(float128 a, float128 b, float_status_t *status);

/*----------------------------------------------------------------------------
| The pattern for a default generated quadruple-precision NaN.
*----------------------------------------------------------------------------*/
extern const float128 float128_default_nan;

#endif /* FLOAT128 */

#endif
