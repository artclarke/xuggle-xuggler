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

#ifndef IRATIONAL_H_
#define IRATIONAL_H_

#include <com/xuggle/ferry/RefCounted.h>
#include <com/xuggle/xuggler/Xuggler.h>

namespace com { namespace xuggle { namespace xuggler
{

  /**
   * This class wraps represents a Rational number for the Xuggler.
   * <p>
   * Video formats often use rational numbers, and converting between
   * them willy nilly can lead to rounding errors, and eventually, out
   * of sync problems.  Therefore we use IRational objects to pass
   * around Rational Numbers and avoid conversion until the very last moment.
   * </p><p>
   * Note: There are some static convenience methods
   * in this class that start with s*.  They start with s
   * (as opposed to overloading methods (e.g. sAdd(...) vs. add(...))
   * because SWIG doesn't support static overloaded methods.
   * </p>
   */
  class VS_API_XUGGLER IRational : public com::xuggle::ferry::RefCounted
  {
  public:
    virtual int32_t getNumerator()=0;
    virtual int32_t getDenominator()=0;

    /**
     * Creates a new IRational object by copying (by value) this object.
     *
     * @return the new object
     */
    virtual IRational * copy()=0;

    /**
     * Compare a rational to this rational
     * @param other second rational
     * @return 0 if this==other, 1 if this>other and -1 if this<other.
     */
    virtual int32_t compareTo(IRational*other)=0;
    static int32_t sCompareTo(IRational *a, IRational *b);

    /**
     * Rational to double conversion.
     *
     * @return (double) a
     */
    virtual double getDouble()=0;

    /**
     * Reduce a fraction.
     * This is useful for framerate calculations.
     * @param num       the src numerator.
     * @param den       the src denominator.
     * @param max the maximum allowed for nom & den in the reduced fraction.
     * @return 1 if exact, 0 otherwise
     */
    virtual int32_t reduce(int64_t num, int64_t den, int64_t max)=0;
    static int32_t sReduce(IRational *dst, int64_t num,
        int64_t den, int64_t max);

    /**
     * Multiplies this number by arg
     * @param arg number to mulitply by.
     * @return this*arg.  Note caller must release() the return value.
     */
    virtual IRational* multiply(IRational *arg)=0;
    static IRational* sMultiply(IRational* a, IRational*b);

    /**
     * Divides this rational by arg.
     * @param arg The divisor to use.
     * @return this/arg.
     */
    virtual IRational* divide(IRational *arg)=0;
    static IRational* sDivide(IRational *a, IRational* b);

    /**
     * Subtracts arg from this rational
     * @param arg The amount to subtract from this.
     * @return this-arg.
     */
    virtual IRational* subtract(IRational *arg)=0;
    static IRational* sSubtract(IRational *a, IRational* b);

    /**
     * Adds arg to this rational
     * @param arg The amount to add to this.
     * @return this+arg.
     */
    virtual IRational* add(IRational *arg)=0;
    static IRational* sAdd(IRational *a, IRational* b);

    /**
     * Takes a value scaled in increments of origBase and gives the
     * equivalent value scaled in terms of this Rational.
     *
     * @param origValue The original int64_t value you care about.
     * @param origBase The original base Rational that origValue is scaled with.
     *
     * @return The new integer value, scaled in units of this IRational.
     */
    virtual int64_t rescale(int64_t origValue, IRational* origBase)=0;
    static int64_t sRescale(int64_t origValue, IRational* origBase, IRational* newBase);
    
    /**
     * Get a new rational that will be set to 0/0.
     * @return a rational number object
     */
    static IRational *make();
    
    /**
     * Converts a double precision floating point number to a rational.
     * @param d double to convert
     * @return A new Rational; caller must release() when done.
     */
    static IRational *make(double d);
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
    static IRational* make(IRational *src);

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
    static IRational *make(int32_t num, int32_t den);

  protected:
    IRational();
    virtual ~IRational();
  };

}}}

#endif /*IRATIONAL_H_*/
