/*
 * Copyright (c) 2008, 2009 by Xuggle Incorporated.  All rights reserved.
 * 
 * This file is part of Xuggler.
 * 
 * You can redistribute Xuggler and/or modify it under the terms of the GNU
 * Affero General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any
 * later version.
 * 
 * Xuggler is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public
 * License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with Xuggler.  If not, see <http://www.gnu.org/licenses/>.
 */

// for std::numeric_limits
#include <limits>

#include <com/xuggle/xuggler/Rational.h>

namespace com { namespace xuggle { namespace xuggler
{

  Rational :: Rational()
  {
    // default to 0 for the value.
    mRational.den = 1;
    mRational.num = 0;
  }

  Rational :: ~Rational()
  {
  }

  Rational *
  Rational :: make(double d)
  {
    Rational *result=0;
    result = Rational::make();
    if (result) {
      result->mRational = av_d2q(d, 0x7fffffff);
    }
    return result;
  }

  Rational *
  Rational :: make(AVRational *src)
  {
    Rational *result=0;
    if (src)
    {
      result = Rational::make();
      if (result) {
        result->mRational = *src;
      }
    }
    return result;
  }

  IRational *
  Rational :: copy()
  {
    return Rational::make(this);
  }

  Rational *
  Rational :: make(Rational *src)
  {
    Rational *result=0;
    if (src)
    {
      result = Rational::make();
      if (result) {
        result->mRational = src->mRational;
      }
    }
    return result;
  }
  Rational *
  Rational :: make(int32_t num, int32_t den)
  {
    Rational *result=0;
    result = Rational::make();
    if (result) {
      result->mRational.num = num;
      result->mRational.den = den;
      (void) result->reduce(num, den, FFMAX(den, num));
    }
    return result;
  }
  int32_t
  Rational :: compareTo(IRational *other)
  {
    int32_t result = 0;
    Rational *arg=dynamic_cast<Rational*>(other);
    if (arg)
      result = av_cmp_q(mRational, arg->mRational);
    return result;
  }

  double
  Rational :: getDouble()
  {
    double result = 0;
    // On some runs in Linux calling av_q2d will raise
    // a FPE instead of returning back NaN or infinity,
    // so we try to short-circuit that here.
    if (mRational.den == 0)
      if (mRational.num == 0)
        result = std::numeric_limits<double>::quiet_NaN();
      else
        result = std::numeric_limits<double>::infinity();
    else
      result =  av_q2d(mRational);
    return result;
  }

  int32_t
  Rational :: reduce(int64_t num, int64_t den, int64_t max)
  {
    int32_t result = 0;
    result =  av_reduce(&mRational.num, &mRational.den,
        num, den, max);
    return result;
  }

  IRational *
  Rational :: multiply(IRational *other)
  {
    Rational *result = 0;
    Rational *arg=dynamic_cast<Rational*>(other);
    if (arg)
    {
      result = Rational::make();
      if (result)
      {
        result->mRational = av_mul_q(this->mRational,
            arg->mRational);
      }
    }
    return result;
  }

  IRational *
  Rational :: divide(IRational *other)
  {
    Rational *result = 0;
    Rational *arg=dynamic_cast<Rational*>(other);
    if (arg)
    {
      result = Rational::make();
      if (result)
      {
        result->mRational = av_div_q(this->mRational,
            arg->mRational);
      }
    }
    return result;
  }

  IRational *
  Rational :: subtract(IRational *other)
  {
    Rational *result = 0;
    Rational *arg=dynamic_cast<Rational*>(other);
    if (arg)
    {
      result = Rational::make();
      if (result)
      {
        result->mRational = av_sub_q(this->mRational,
            arg->mRational);
      }
    }
    return result;
  }
  IRational *
  Rational :: add(IRational *other)
  {
    Rational *result = 0;
    Rational *arg=dynamic_cast<Rational*>(other);
    if (arg)
    {
      result = Rational::make();
      if (result)
      {
        result->mRational = av_add_q(this->mRational,
            arg->mRational);
      }
    }
    return result;
  }

  int64_t
  Rational :: rescale(int64_t origValue, IRational *origBase)
  {
    int64_t retval=origValue;
    Rational *arg=dynamic_cast<Rational*>(origBase);

    if (arg)
    {
      retval = av_rescale_q(origValue, arg->mRational, this->mRational);
    }
    return retval;
  }
  
  int64_t
  Rational :: rescale(int64_t origValue, IRational *origBase,
      Rounding rounding)
  {
    int64_t retval=origValue;
    Rational *arg=dynamic_cast<Rational*>(origBase);

    if (arg)
    {
      int64_t b = arg->mRational.num  * (int64_t)this->mRational.den;
      int64_t c = this->mRational.num * (int64_t)arg->mRational.den;

      retval = av_rescale_rnd(origValue, b,
          c, (enum AVRounding)rounding);
    }
    return retval;
  }
  
  int64_t
  Rational :: rescale(int64_t srcValue,
      int32_t dstNumerator,
      int32_t dstDenominator,
      int32_t srcNumerator,
      int32_t srcDenominator,
      Rounding rounding)
  {
    int64_t retval = srcValue;
    if (!dstNumerator || !dstDenominator ||
        !srcNumerator || !srcDenominator)
      return 0;

    int64_t b = srcNumerator * (int64_t)dstDenominator;
    int64_t c = dstNumerator * (int64_t)srcDenominator;

    retval = av_rescale_rnd(srcValue, b,
        c, (enum AVRounding)rounding);

    return retval;
  }
 
}}}
