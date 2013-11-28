#ifndef Configuration_hh
#define Configuration_hh 1

/* #include <cassert> */
#include <crisp/comms/common.hh>
#include <crisp/comms/Module.hh>
#include <crisp/comms/Message.hh>

#include <crisp/util/SArray.hh>

#if defined(ENABLE_ASSERT) && ENABLE_ASSERT
#  include <cassert>
#endif

#ifdef SWIG
#  define Sensor crisp::comms::Sensor
#  define Module crisp::comms::Module
#endif

namespace crisp
{
  namespace comms
  {
    struct __attribute__ (( packed, align(1) ))
    Configuration
    {
      typedef Configuration TranscodeAsType;
      static constexpr MessageType Type = MessageType::CONFIGURATION_RESPONSE;
      static const size_t HeaderSize;


      Configuration(uint8_t modules_alloc = 0);
      Configuration(const Configuration& config); /**< Copy constructor. */
      Configuration(Configuration&& config);      /**< Move constructor. */
      ~Configuration();                           /**< Destructor. */
      void reset();

      Configuration&
      operator =(Configuration&& c);

      Configuration&
      operator =(const Configuration&);

      bool
      operator ==(const Configuration& c) const;

      /** Get the encoded size of this configuration object. */
      size_t get_encoded_size() const;

      /** Write a serialzed form of this configuration to a memory buffer. */
      EncodeResult encode(crisp::comms::MemoryEncodeBuffer& buf) const;

      /** Decode a serialzed Configuration into the current object. */
      DecodeResult decode(DecodeBuffer& buf);

      static inline TranscodeAsType
	decode_copy(DecodeBuffer& buf)
      { TranscodeAsType out; out.decode(buf);
	return out; }

      /** Add a module to the Configuration.
       *
       * @return @p module to enable buildup of the module instance.
       */
      template < typename... _Args >
	inline Module&
	add_module(_Args... args)
      { 
	if ( modules.owns_data )
	  {
	    ++num_modules;
	    Module& out ( modules.emplace(args...) );
	    out.id = next_module_id++;
	    return out;
	  }
	else
	  return modules[num_modules - 1];
      }

#ifdef SWIG
      %immutable;
#endif
      uint8_t
	num_modules;		/**< Number of initialized modules at @c modules. */
    
#ifdef SWIG
      %immutable;
#endif
      crisp::util::SArray<Module> modules;

#ifdef SWIG
      %immutable;
#endif
      uint8_t
	next_module_id;
    };
  }
}

#ifdef SWIG
#  undef Sensor
#  undef Module
#endif

#endif	/* Configuration_hh */
