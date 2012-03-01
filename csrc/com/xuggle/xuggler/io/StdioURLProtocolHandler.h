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

#ifndef STDIOURLPROTOCOLHANDLER_H_
#define STDIOURLPROTOCOLHANDLER_H_

#include <cstdio>
#include <com/xuggle/xuggler/io/URLProtocolHandler.h>

namespace com { namespace xuggle { namespace xuggler { namespace io
  {
  class StdioURLProtocolManager;

  /**
   * A C 32-Bit FILE IO handler.
   *
   * This class only works with file sizes < 2GB.  It is intended primarily
   * for test purposes.
   */
  class VS_API_XUGGLER_IO StdioURLProtocolHandler : public URLProtocolHandler
  {
  public:
    StdioURLProtocolHandler(StdioURLProtocolManager* mgr);
    virtual ~StdioURLProtocolHandler();

    // Now, let's have our forwarding functions
    virtual int url_open(const char *url, int flags);
    virtual int url_close();
    virtual int url_read(unsigned char* buf, int size);
    virtual int url_write(const unsigned char* buf, int size);
    virtual int64_t url_seek(int64_t position, int whence);
    virtual SeekableFlags url_seekflags(const char* url, int flags);
    virtual int64_t url_getsize();

  private:
    FILE* mFile;
    void reset();
  };
  }}}}
#endif /*STDIOURLPROTOCOLHANDLER_H_*/
