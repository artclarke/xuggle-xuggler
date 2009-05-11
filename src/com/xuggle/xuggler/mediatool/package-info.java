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

/**
 * The media tools provde a simplified aproach to decoding, viewing and
 * encoding media.  Use a {@link
 * com.xuggle.xuggler.mediatool.MediaReader} to read and decode media.
 * Use {@link com.xuggle.xuggler.mediatool.MediaWriter} to encode media
 * and write it out.  A {@link
 * com.xuggle.xuggler.mediatool.mediatool.MediaWriter} can be added as a
 * listener to a {@link com.xuggle.xuggler.mediatool.MediaReader} to
 * "close the loop" and effectly transcode media.  To view the media a
 * {@link com.xuggle.xuggler.mediatool.MediaViewer} can be added as a
 * listener to either a {@link com.xuggle.xuggler.mediatool.MediaReader}
 * or writer.  Variouse hooks are provided via the {@link
 * com.xuggle.xuggler.mediatool.IMediaListener} interface.
 */

package com.xuggle.xuggler.mediatool;
