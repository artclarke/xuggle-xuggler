package com.xuggle.xuggler.io;

import com.xuggle.xuggler.io.IURLProtocolHandler;
import com.xuggle.xuggler.io.IURLProtocolHandlerFactory;
import com.xuggle.xuggler.io.NullProtocolHandler;

/**
 * Returns a new NullProtocolHandler factory.  By default Xuggler IO registers
 * the Null Protocol Handler under the protocol name "xugglernull".  Any
 * URL can be opened.
 * <p>
 * For example, "xugglernull:a_url"
 * </p>
 * @author aclarke
 *
 */
public class NullProtocolHandlerFactory implements IURLProtocolHandlerFactory
{

  public IURLProtocolHandler getHandler(String aProtocol, String aUrl,
      int aFlags)
  {
    return new NullProtocolHandler();
  }

}
