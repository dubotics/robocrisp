#ifndef input_MappedEventSource_hh
#define input_MappedEventSource_hh 1

#include <boost/signals2/signal.hpp>
#include <boost/move/move.hpp>

namespace input
{
  /** Base class for objects that can signal input events with associated
   * values.  MappedEventSource provides a thread-safe signal-based method of
   * notifying consumers of changes in the input state.
   *
   * @tparam _T		Type of the class derived from MappedEventSource.
   *
   * @tparam _Value	Mapped value-type obtained from the mapping function.
   *
   * @tparam _RawValue  "Raw", unmapped value type for events from the underlying
   * 			hardware.
   */
  template < typename _T, typename _RawValue, typename _Value >
  class MappedEventSource
  {
  public:
    typedef _T Type;	  	/**< Derived-class type. */
    typedef _Value Value;	/**< Type of values for events from this
				   source. */    
    typedef _RawValue RawValue;	/**< Type of _raw_ (unmapped) values for events
				   from this source. */

    /** Type for objects that represent a signal-handler's (callback's)
	connection to its signal source. */
    typedef boost::signals2::connection Connection;


    /** "Raw" signal type.  This signal type is used to notify that new raw
     *  values have been recieved, and is the kind of signal to which hook_raw
     *  connects callbacks.
     */
    typedef typename boost::signals2::signal<void(const Type&, RawValue)>
      RawSignal;


    /** "Full" signal type. This is the type of signal to which hook_full
     *	connects callbacks.
     */
    typedef boost::signals2::signal<void(const Type&, RawValue, Value)>
      FullSignal;


    MappedEventSource() = default;

    /** Move constructor. */
    MappedEventSource(MappedEventSource&& mes)
      : m_raw_signal ( ),
	m_full_signal ( )
    {
      m_raw_signal.swap(mes.m_raw_signal);
      m_full_signal.swap(mes.m_full_signal);
    }

    /** Defaulted virtual destructor provided for polymorphic use of derived
     * classes. */
    virtual ~MappedEventSource() = default;


    /** Linear raw-to-basic value-mapping function.  This method must be
     * provided by derived classes!
     *
     * @param raw_value The raw value delivered to the post() method.
     *
     * @return An implementation-dependent value mapped from the given raw
     *     value.
     */
    virtual Value map(RawValue raw_value) const = 0;


    /** Post a raw value to the event source for mapping and subsequent
     *  propagation to signal handlers.
     *
     * @param raw_value The raw value read from the underlying hardware.
     */
    void
    /* TODO: should this be implemented to handle new input values in a separate
       thread? */
    post(RawValue raw_value);


    /** Connect a callback triggered on "raw" input events.  This method is
     *  called before any mapping takes place.
     *
     * @param callback A that should be invoked whenever raw data is received.
     *
     * @return A connection object that can be used to disconnect the callback
     *     from the source signal.
     */
    Connection hook_raw(typename RawSignal::slot_type callback)
    { return m_raw_signal.connect(callback); }


    /** Connect a callback to be called on input events with both unmapped and
     *	mapped value data.
     *
     * @param callback A that should be invoked whenever raw data is received
     *     and has been processed.
     *
     * @return A connection object that can be used to disconnect the callback
     *     from the source signal.
     */
    Connection hook_full(typename FullSignal::slot_type callback)
    { return m_full_signal.connect(callback); }


  private:
    RawSignal m_raw_signal;	/**< Signal object used to manage raw-event */
    FullSignal m_full_signal;

  };
}

#endif	/* input_MappedEventSource_hh */
