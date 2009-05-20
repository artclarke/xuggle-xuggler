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

// for isinf()
#include <math.h>

#include <com/xuggle/xuggler/IRational.h>
#include <com/xuggle/xuggler/Global.h>
#include "Helper.h"
#include "RationalTest.h"

using namespace VS_CPP_NAMESPACE;

void
RationalTest :: setUp()
{
  num = 0;
}

void
RationalTest :: testCreationAndDestruction()
{
  num = IRational::make();
  VS_TUT_ENSURE("", num);
  VS_TUT_ENSURE_EQUALS("", num->getNumerator(), 0);
  VS_TUT_ENSURE_EQUALS("", num->getDenominator(), 1);

  num = IRational::make(6.0);
  VS_TUT_ENSURE("", num);
  VS_TUT_ENSURE_EQUALS("", num->getNumerator(), 6);
  VS_TUT_ENSURE_EQUALS("", num->getDenominator(), 1);

  num = IRational::make(6.1);
  VS_TUT_ENSURE("", num);
  VS_TUT_ENSURE_EQUALS("", num->getNumerator(), 61);
  VS_TUT_ENSURE_EQUALS("", num->getDenominator(), 10);
}

void
RationalTest :: testReduction()
{
  int retval = -1;
  num = IRational::make(2.2);
  VS_TUT_ENSURE("", num);
  VS_TUT_ENSURE_EQUALS("", num->getNumerator(), 11);
  VS_TUT_ENSURE_EQUALS("", num->getDenominator(), 5);

  retval = num->reduce(num->getNumerator()*5,
      num->getDenominator()*10, 100);
  VS_TUT_ENSURE("not exact", retval == 1);
  VS_TUT_ENSURE_EQUALS("", num->getNumerator(), 11);
  VS_TUT_ENSURE_EQUALS("", num->getDenominator(), 10);

  retval = IRational::sReduce(num.value(), 33, 32, 10);
  VS_TUT_ENSURE("exact?", retval == 0);
  VS_TUT_ENSURE_EQUALS("", num->getNumerator(), 1);
  VS_TUT_ENSURE_EQUALS("", num->getDenominator(), 1);

  // should be infinity
  retval = num->reduce(33, 0, 10);
  VS_TUT_ENSURE("not exact", retval == 1);
  VS_TUT_ENSURE_EQUALS("", num->getNumerator(), 1);
  VS_TUT_ENSURE_EQUALS("", num->getDenominator(), 0);
  VS_TUT_ENSURE("", isinf(num->getDouble()));

}

void
RationalTest :: testGetDouble()
{
  double retval = -1;
  num = IRational::make();

  retval = num->getDouble();
  VS_TUT_ENSURE_EQUALS("", retval, 0);

  // now let make sure we can create infinity (and beyond...)
  num = IRational::make();
  num->reduce(1, 0, 10);
  retval = num->getDouble();
  VS_TUT_ENSURE("", isinf(retval));
}

void
RationalTest :: testMultiplication()
{
  RefPointer<IRational> a;
  RefPointer<IRational> b;

  a = IRational::make(12);
  b = IRational::make(3);
  num = IRational::sMultiply(a.value(), b.value());
  VS_TUT_ENSURE_EQUALS("", num->getDouble(), 36);      
}

void
RationalTest :: testAddition()
{
  RefPointer<IRational> a;
  RefPointer<IRational> b;

  a = IRational::make(12);
  b = IRational::make(3);
  num = IRational::sAdd(a.value(), b.value());
  VS_TUT_ENSURE_EQUALS("", num->getDouble(), 15);      
}

void
RationalTest :: testSubtraction()
{
  RefPointer<IRational> a;
  RefPointer<IRational> b;

  a = IRational::make(12);
  b = IRational::make(3);
  num = IRational::sSubtract(a.value(), b.value());
  VS_TUT_ENSURE_EQUALS("", num->getDouble(), 9);      
}

void
RationalTest :: testDivision()
{
  RefPointer<IRational> a;
  RefPointer<IRational> b;

  a = IRational::make(12);
  b = IRational::make(3);
  num = IRational::sDivide(a.value(), b.value());
  VS_TUT_ENSURE_EQUALS("", num->getDouble(), 4);      

  a = IRational::make(1);
  b = IRational::make(0.0);
  num = IRational::sDivide(a.value(), b.value());
  VS_TUT_ENSURE("", isinf(num->getDouble()));

  a = IRational::make(0.0);
  b = IRational::make(0.0);
  num = IRational::sDivide(a.value(), b.value());
  VS_TUT_ENSURE("", isnan(num->getDouble()));
}

void
RationalTest :: testConstructionFromNumeratorAndDenominatorPair()
{
  num = IRational::make(1, 10);
  VS_TUT_ENSURE_EQUALS("", num->getDouble(), 0.1);
  num = IRational::make(2, 10);
  VS_TUT_ENSURE_EQUALS("", num->getDouble(), 0.2);
  VS_TUT_ENSURE_EQUALS("", num->getNumerator(), 1);
  VS_TUT_ENSURE_EQUALS("", num->getDenominator(), 5);
}

void
RationalTest :: testRescaling()
{
  RefPointer<IRational> a;
  RefPointer<IRational> b;
  a = IRational::make(1, 100);
  VS_TUT_ENSURE_EQUALS("", a->getDouble(), 0.01);
  b = IRational::make(1,5);
  VS_TUT_ENSURE_EQUALS("", b->getDouble(), 0.2);

  VS_TUT_ENSURE_EQUALS("", a->rescale(1, b.value()), 20);
  VS_TUT_ENSURE_EQUALS("", b->rescale(1, a.value()), 0);
}
