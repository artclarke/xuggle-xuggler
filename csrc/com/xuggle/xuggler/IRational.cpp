/*
 * Copyright (c) 2008-2009 by Xuggle Inc. All rights reserved.
 *
 * It is REQUESTED BUT NOT REQUIRED if you use this library, that you let 
 * us know by sending e-mail to info@xuggle.com telling us briefly how you're
 * using the library and what you like or don't like about it.
 *
 * This library is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any later
 * version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along
 * with this library; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
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
