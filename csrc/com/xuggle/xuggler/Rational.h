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

#ifndef RATIONAL_H_
#define RATIONAL_H_

#include <com/xuggle/xuggler/FfmpegIncludes.h>
#include <com/xuggle/xuggler/IRational.h>

namespace com { namespace xuggle { namespace xuggler
{

  class Rational : public IRational
  {
    VS_JNIUTILS_REFCOUNTED_OBJECT(Rational);
  public:

    // IRational Interface implementation
    virtual IRational* copy();
    virtual int32_t getNumerator() { return mRational.num; }
    virtual int32_t getDenominator() { return mRational.den; }
    virtual int32_t compareTo(IRational*other);
    virtual double getDouble();
    virtual int32_t reduce(int64_t num, int64_t den, int64_t max);
    virtual IRational* multiply(IRational *arg);
    virtual IRational* divide(IRational *arg);
    virtual IRational* subtract(IRational *arg);
    virtual IRational* add(IRational *arg);
    virtual int64_t rescale(int64_t origValue, IRational* origBase);

    /**
     * Converts a double precision floating point number to a rational.
     * @param d double to convert
     * @return A new Rational; caller must release() when done.
     */
    static Rational *make(double d);
    /**
     * Create a Rational from an AVRational struct.
     *
     * @param src       The source AVRational object.
     * @return A new Rational; caller must release() when done.  Null if
     *         src is null.
     */
    static Rational *make(AVRational *src);
    /**
     * Creates copy of a Rational from another Rational.
     *
     * Note: This is a NEW object.  To just keep tabs on the
     * original, use acquire() to keep a reference.
     *
     * @param src       The source Rational to copy.
     * @return A new Rational; caller must call release.  Returns null
     *         if src is null.
     */
    static Rational* make(Rational *src);

    /**
     * Create a rational from a numerator and denominator.
     *
     * We will always reduce this to the lowest num/den pair
     * we can, but never having den exceed what was passed in.
     *
     * @param num The numerator of the resulting Rational
     * @param den The denominator of the resulting Rational
     *
     * @return A new Rational; caller must call release.
     */
    static Rational *make(int32_t num, int32_t den);
    
    virtual int64_t rescale(int64_t origValue,
        IRational* origBase,
        Rounding rounding);

    static int64_t rescale(int64_t srcValue,
        int32_t dstNumerator,
        int32_t dstDenominator,
        int32_t srcNumerator,
        int32_t srcDenominator,
        Rounding rounding);
    
  protected:
    Rational();
    virtual ~Rational();
  private:
    // note not a pointer.
    AVRational mRational;
  };

}}}

#endif /*RATIONAL_H_*/
