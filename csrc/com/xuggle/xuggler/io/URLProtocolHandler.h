/*******************************************************************************
 * Copyright (c) 2012 Xuggle Inc.  All rights reserved.
 *  
 * This file is part of Xuggle-Xuggler-Main.
 *
 * Xuggle-Xuggler-Main is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Xuggle-Xuggler-Main is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Xuggle-Xuggler-Main.  If not, see <http://www.gnu.org/licenses/>.
 *******************************************************************************/

#ifndef URLPROTOCOLHANDLER_H_
#define URLPROTOCOLHANDLER_H_

#include <stdint.h>

#include <com/xuggle/xuggler/io/IO.h>

namespace com { namespace xuggle { namespace xuggler { namespace io
{
  class URLProtocolManager;
  class VS_API_XUGGLER_IO URLProtocolHandler
  {
  public:
    typedef enum SeekFlags {
      SK_SEEK_SET=0,
      SK_SEEK_CUR=1,
      SK_SEEK_END=2,
      SK_SEEK_SIZE=0x10000,
    } SeekFlags;
    typedef enum OpenFlags {
      URL_RDONLY_MODE=0,
      URL_WRONLY_MODE=1,
      URL_RDWR_MODE=2,
    } OpenFlags;

    typedef enum SeekableFlags {
      SK_NOT_SEEKABLE   =0x00000000,
      SK_SEEKABLE_NORMAL=0x00000001,
    } SeekableFlags;
    virtual ~URLProtocolHandler();
    virtual const char* getProtocolName();
    virtual URLProtocolManager *getProtocolManager() { return mManager; }

    // Now, let's have our forwarding functions
    virtual int url_open(const char *url, int flags)=0;
    virtual int url_close()=0;
    virtual int url_read(unsigned char* buf, int size)=0;
    virtual int url_write(const unsigned char* buf, int size)=0;
    virtual int64_t url_seek(int64_t position, int whence)=0;
    virtual SeekableFlags url_seekflags(const char* url, int flags)=0;

  protected:
    URLProtocolHandler(
        URLProtocolManager* mgr);

  private:
    URLProtocolManager* mManager;
  };
}}}}
#endif /*URLPROTOCOLHANDLER_H_*/
