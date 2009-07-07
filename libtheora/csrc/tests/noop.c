/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggTheora SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE Theora SOURCE CODE IS COPYRIGHT (C) 2002-2007                *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

  function: routines for validating codec initialization
  last mod: $Id: noop.c 14378 2008-01-08 01:08:12Z giles $

 ********************************************************************/

#include <theora/theoraenc.h>

#include "tests.h"

static int
noop_test_encode ()
{
  th_info ti;
  th_enc_ctx *te;

  INFO ("+ Initializing theora_info struct");
  th_info_init (&ti);

  INFO ("+ Allocating encoder context");
  te = th_encode_alloc(&ti);
  if (te == NULL)
    FAIL("td_encode_alloc returned a null pointer");

  INFO ("+ Clearing theora_info struct");
  th_info_clear (&ti);

  INFO ("+ Freeing encoder context");
  th_encode_free(te);

  return 0;
}

static int
noop_test_comments ()
{
  th_comment tc;

  th_comment_init (&tc);
  th_comment_clear (&tc);

  return 0;
}

int main(int argc, char *argv[])
{
  noop_test_encode ();

  noop_test_comments ();

  exit (0);
}
