/*
 * This file is part of Xuggler.
 * 
 * Xuggler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 * 
 * Xuggler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public
 * License along with Xuggler.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "IRational.h"
#include "Global.h"
#include "Rational.h"

namespace com { namespace xuggle { namespace xuggler
  {

    IRational::IRational()
    {
    }

    IRational::~IRational()
    {
    }

    int32_t
    IRational :: sCompareTo(IRational *a, IRational *b)
    {
      return a->compareTo(b);
    }

    int32_t
    IRational :: sReduce(IRational *dst, int64_t num,
        int64_t den, int64_t max)
    {
      return (dst ? dst->reduce(num, den, max) : 0);
    }

    IRational*
    IRational :: sDivide(IRational *a, IRational* b)
    {
      return a->divide(b);
    }

    IRational*
    IRational :: sSubtract(IRational *a, IRational* b)
    {
      return a->subtract(b);
    }

    IRational*
    IRational :: sAdd(IRational *a, IRational* b)
    {
      return a->add(b);
    }

    IRational*
    IRational :: sMultiply(IRational *a, IRational* b)
    {
      return a->multiply(b);
    }

    int64_t
    IRational :: sRescale(int64_t origValue, IRational* origBase, IRational* newBase)
    {
      return newBase->rescale(origValue, origBase);
    }

    IRational*
    IRational :: make()
    {
      Global::init();
      return Rational::make();
    }
    
    IRational*
    IRational :: make(double d)
    {
      Global::init();
      return Rational::make(d);
    }
    
    IRational*
    IRational :: make(IRational *aSrc)
    {
      Global::init();
      Rational* src = dynamic_cast<Rational*>(aSrc);
      IRational* retval = 0;
      if (src)
      {
        retval = Rational::make(src);
      }
      return retval;
    }
    
    IRational*
    IRational :: make(int32_t num, int32_t den)
    {
      Global::init();
      return Rational::make(num, den);
    }
  }}}
