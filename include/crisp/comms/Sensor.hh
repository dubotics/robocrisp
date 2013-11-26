#ifndef Sensor_hh
#define Sensor_hh 1

#include <crisp/comms/DataDeclaration.hh>
#include <crisp/comms/common.hh>
#include <crisp/comms/decl-defn-macros.hh>
#include <crisp/util/arith.hh>

namespace crisp
{
  namespace comms
  {
    ENUM_CLASS(SensorReportingMode, uint8_t,
	       POLL, /**< Sensor responds only to SENSOR_POLL messages. */
	       ASYNC /**< Sensor responds when it gets data, which may or may not be at regular intervals. */
	       );


    /* These comments WERE on the entries of the SensorType enum that follows, but the SWIG parser
       got confused with 'em there. */
#if 0
    /**< "We're not exactly sure what the data means, but we're sending it anyway."  Probably won't be using this type. */
    /**< Some kind of switch. */											  
    /**< (Ultrasonic?) proximity sensor. */										  
    /**< e.g. for the arm position encoders. */									  
    /**< NMEA data stream?  Or if Owen wants, we could parse it robot-side and just send position and heading. */	  
    /**< Some sort of imaging device, presumably... */								  
#endif

    ENUM_CLASS(SensorType, uint8_t,
	       UNKNOWN,
	       SWITCH,
	       PROXIMITY,
	       ROTARY_ENCODER,
	       GPS,
	       CAMERA
	       );




    /** Sensor template. */
    template < typename _T = void, typename _Enable = _T >
    struct Sensor;

#define SENSOR_COMMON_FUNCS()						\
    bool operator ==(const Sensor<_T>& o) const				\
    {									\
      assert(this != nullptr && &o != nullptr);				\
									\
      typedef typename std::conditional<std::is_same<Sensor<_T>, TranscodeAsType>::value, \
					const Sensor<_T>&, TranscodeAsType>::type \
      common_type;							\
									\
      common_type ga ( *this );                                         \
      common_type gb ( o );                                             \
									\
      using crisp::util::p_abs;						\
      return ga.id == gb.id && ga.type == gb.type && ga.reporting_mode == gb.reporting_mode && \
	p_abs(ga.name_length) == p_abs(gb.name_length) && ga.data_type == gb.data_type && \
	(ga.name_length == 0 || ! memcmp(ga.name, gb.name, p_abs(ga.name_length))); \
  }

    /** Generic Sensor implementation, as stored in Configuration instances and sent over radio link. */
    template < typename _T >
    struct __attribute__ (( packed ))
    Sensor<_T, typename std::enable_if<std::is_void<_T>::value, _T>::type>
    {
      typedef Sensor<> TranscodeAsType;
      typedef Sensor<> Type;
      typedef DataDeclaration<_T> DataType;

      inline size_t
	get_name_length() const
      { return name ? crisp::util::p_abs(name_length) : 0; }

      SENSOR_COMMON_FUNCS()
	NAMED_TYPED_COMMON_FUNC_INLINES()
#ifdef SWIG
	;
#endif

      /** Header consists of the bytes always present at the start of an encoded Sensor object. */
      static const size_t HeaderSize;

      Sensor();
      Sensor(const Sensor&) = delete;
      Sensor(Sensor&& s);
      Sensor(uint16_t _id, SensorType _type, SensorReportingMode _mode, uint8_t _name_length, DataType&& _data_type, const char* _name);
      /* Sensor(const Sensor& s); */
      ~Sensor();

      Sensor&
	operator =(Sensor&& obj);

      inline bool
	operator != (const Sensor& o) const
      { return !(*this == o); }

      uint16_t id _FIELD_WIDTH(10);			/**< Sensor's unique ID within its configuration object. */
      SensorType type _FIELD_WIDTH(5);			/**< Kind of sensor this is. */
      SensorReportingMode reporting_mode _FIELD_WIDTH(1); /**< Reporting mode. */


      /** Number of characters in the ASCII name string that follows this structure. Note that the
       * encoded name string will _not_ be NUL-terminated!
       *
       * @note The name string follows any optional fields in the `data_type` field.
       */
      uint8_t name_length;

      /** Data type used by the sensor to report values.  */
      DataType data_type;

      char* name;		/**< Name string, or NULL if `name_length` is zero. */
      bool owns_name;
    };


    template < typename _T >
    struct __attribute__ (( packed ))
    Sensor<_T, typename std::enable_if<!std::is_void<_T>::value, _T>::type>
    {
      typedef Sensor<> TranscodeAsType;
      typedef Sensor<> Type;
      typedef DataDeclaration<_T> DataType;

      SENSOR_COMMON_FUNCS()
	NAMED_TYPED_COMMON_FUNC_INLINES()
#ifdef SWIG
	;
#endif

      inline size_t
	get_name_length() const
      { return name ? strlen(name) : 0; }

      inline
	Sensor(const char* _name, SensorType _type, DataType _data_type, SensorReportingMode _mode = SensorReportingMode::ASYNC)
	: id ( 0 ),
        type ( _type ),
        reporting_mode ( _mode ),
        data_type ( _data_type ),
        name ( _name ),
	owns_name ( false )
	{}


      /** Converts to generic Sensor type. */
      inline operator Sensor<>() const
      {
	return { id, type, reporting_mode, static_cast<uint8_t>(strlen(name)), data_type, name };
      }
    
      uint16_t id _FIELD_WIDTH(10);			/**< Sensor's unique ID */
      SensorType type _FIELD_WIDTH(5);			/**< Kind of sensor this is. */
      SensorReportingMode reporting_mode _FIELD_WIDTH(1); /**< Reporting mode. */

      /** Data type used by the sensor to report values.  */
      DataType data_type;

      const char* name;		/**< Name string, or NULL if `name_length` is zero. */
      bool owns_name;		/**< Dummy variable to satisfy functions in
				   NAMED_TYPED_COMMON_FUNC_INLINES() -- always false! */
    };

  }
}

#endif	/* Sensor_hh */
