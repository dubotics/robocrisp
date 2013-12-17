#ifndef input_MappedEventSource_hh
#define input_MappedEventSource_hh 1

#include <crisp/util/Signal.hh>

namespace crisp
{
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
      typedef _RawValue RawValue; /**< Type of _raw_ (unmapped) values for
					   events from this source. */

      /** State of the event source's value.  */
      struct State
      {
        RawValue raw_value;
        Value value;
      };


      typedef uint32_t ID;

      /** Signal type. This is the type of signal to which `hook` connects
       *  callbacks.
       */
      typedef crisp::util::Signal<void(const Type&, State)> Signal;

      /** Type for objects that represent a signal-handler's (callback's)
	  connection to its signal source. */
      typedef typename Signal::Connection Connection;


      /** Basic constructor.
       * @param _id ID code for this object.
       */
      MappedEventSource(ID _id)
        : id ( m_id ),
	  m_id ( _id ),
	  m_signal ( )
      {}


      /** Move constructor. */
      MappedEventSource(MappedEventSource&& mes)
        : id ( m_id ),
	  m_id ( std::move(mes.m_id) ),
	  m_signal ( std::move(mes.m_signal) )
      {
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
      post(RawValue raw_value)
      {
	Value mapped_value ( map(raw_value) );
	m_signal.emit(std::cref(static_cast<Type&>(*this)), { raw_value, mapped_value });
      }


      /** Connect a callback to be called on input events with both unmapped and
       *	mapped value data.
       *
       * @param callback A that should be invoked whenever raw data is received
       *     and has been processed.
       *
       * @return A connection object that can be used to disconnect the callback
       *     from the source signal.
       */
      Connection hook(typename Signal::Function callback)
      { return m_signal.connect(callback); }



      const ID& id;
    protected:
      Signal m_signal;
      ID m_id;
    };
  }
}
#endif	/* input_MappedEventSource_hh */
