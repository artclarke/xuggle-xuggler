package com.xuggle.mediatool;

public interface IMediaDebugListener
{
  /**
   * How much detail on each event you want to log.
   */
  public enum Mode {

    /** log no events */

    NOTHING, 

    /** log events without details */

    EVENT,

    /** log events with source or destination URL */

    URL, 

    /** log parameters passed to the event */

    PARAMETERS};

  /** The different type of events you'd like to print
   * data for.  */
  
  public enum Event
  {
    /** Video events */

    VIDEO         (0x001, "onVideoPicture"),

    /** Audio events */

    AUDIO       (0x002, "onAudioSamples"),

    /** Open events */

    OPEN        (0x004, "onOpen"),

    /** Close events */

    CLOSE       (0x008, "onClose"),

    /** Add stream events */

    ADD_STREAM  (0x010, "onAddStream"),

    /** Open stream events */

    OPEN_STREAM (0x020, "onOpenStream"),

    /** Close stream events */

    CLOSE_STREAM(0x040, "onCloseStream"),

    /** Read packet events */

    READ_PACKET (0x080, "onReadPacket"),

    /** Write packet events */

    WRITE_PACKET(0x100, "onWritePacket"),

    /** Write header events */

    HEADER      (0x200, "onWriteHeader"),

    /** Write trailer events */

    TRAILER     (0x400, "onWriteTrailer"),

    /** Flush events */

    FLUSH       (0x800, "onFlush"),

    /** All events */

    ALL         (0xfff, "<ALL-EVENT>"),

    /** No events */

    NONE        (0x000, "<NO-EVENTS>"),

    /**
     * {@link #VIDEO}, {@link #AUDIO}, {@link #READ_PACKET} and
     * {@link #WRITE_PACKET} events
     */

    DATA        (0x183, "<DATA-EVENTS>"),

    /**
     * All events except {@link #VIDEO}, {@link #AUDIO}, {@link
     * #READ_PACKET} and {@link #WRITE_PACKET} events
     */

    META_DATA   (0xfff & ~0x183, "<META-DATA-EVENTS>");

    // event flag

    private int mFlag;

    // name of called method

    private String mMethod;

    /**
     * Create an event.
     */
    Event(int flag, String method)
    {
      mFlag = flag;
      mMethod = method;
    }

    /**
     * Get the event flag.
     * @return the flag.
     */

    public int getFlag()
    {
      return mFlag;
    }

    /**
     * Get the {@link IMediaPipeListener} event this event
     * will fire for.
     * @return The method.
     */
    public String getMethod()
    {
      return mMethod;
    }

  };


  /** 
   * Get the current count of events of a particular type.
   *
   * @param event the specified event type
   * 
   * @return the number of events of the specified type which have been
   *         transpired
   */

  public abstract long getCount(Event event);

  /** 
   * Reset all the event counts.
   */

  public abstract void resetCounts();

  /** 
   * Set the event types which will be logged.
   *
   * @param events the events which will be logged
   */

  public abstract void setLogEvents(Event... events);

  /** 
   * Get the flags which specify which events will be logged.
   * 
   * @return the flags.
   */

  public abstract int getFlags();

}
