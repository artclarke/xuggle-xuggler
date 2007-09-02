/*****************************************************************************
 * common.h: h264 encoder
 *****************************************************************************
 * Copyright (C) 2007 x264 project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.
 *****************************************************************************/

#ifndef _OSDEP_H
#define _OSDEP_H

#define _LARGEFILE_SOURCE 1
#define _FILE_OFFSET_BITS 64
#include <stdio.h>

#ifdef HAVE_STDINT_H
#include <stdint.h>
#else
#include <inttypes.h>
#endif

#ifdef _MSC_VER
#include <io.h>    // _setmode()
#include <fcntl.h> // _O_BINARY
#define inline __inline
#define strncasecmp strnicmp
#define snprintf _snprintf
#define fseek _fseeki64
#define ftell _ftelli64
#define isfinite _finite
#define _CRT_SECURE_NO_DEPRECATE
#define X264_VERSION "" // no configure script for msvc
#endif

#ifdef SYS_OPENBSD
#define isfinite finite
#endif
#if defined(_MSC_VER) || defined(SYS_SunOS) || defined(SYS_MACOSX)
#define sqrtf sqrt
#endif
#ifdef __WIN32__
#define rename(src,dst) (unlink(dst), rename(src,dst)) // POSIX says that rename() removes the destination, but win32 doesn't.
#ifndef strtok_r
#define strtok_r(str,delim,save) strtok(str,delim)
#endif
#endif

#ifdef _MSC_VER
#define DECLARE_ALIGNED( type, var, n ) __declspec(align(n)) type var
#else
#define DECLARE_ALIGNED( type, var, n ) type var __attribute__((aligned(n)))
#endif

#if defined(__GNUC__) && (__GNUC__ > 3 || __GNUC__ == 3 && __GNUC_MINOR__ > 0)
#define UNUSED __attribute__((unused))
#else
#define UNUSED
#endif

/* threads */
#if defined(__WIN32__) && defined(HAVE_PTHREAD)
#include <pthread.h>
#define USE_CONDITION_VAR

#elif defined(SYS_BEOS)
#include <kernel/OS.h>
#define pthread_t               thread_id
#define pthread_create(t,u,f,d) { *(t)=spawn_thread(f,"",10,d); \
                                  resume_thread(*(t)); }
#define pthread_join(t,s)       { long tmp; \
                                  wait_for_thread(t,(s)?(long*)(s):&tmp); }
#ifndef usleep
#define usleep(t)               snooze(t)
#endif
#define HAVE_PTHREAD 1

#elif defined(HAVE_PTHREAD)
#include <pthread.h>
#define USE_CONDITION_VAR
#else
#define pthread_t               int
#define pthread_create(t,u,f,d)
#define pthread_join(t,s)
#endif //SYS_*

#ifndef USE_CONDITION_VAR
#define pthread_mutex_t         int
#define pthread_mutex_init(m,f)
#define pthread_mutex_destroy(m)
#define pthread_mutex_lock(m)
#define pthread_mutex_unlock(m)
#define pthread_cond_t          int
#define pthread_cond_init(c,f)
#define pthread_cond_destroy(c)
#define pthread_cond_broadcast(c)
#define pthread_cond_wait(c,m)  usleep(100)
#endif

#endif //_OSDEP_H
