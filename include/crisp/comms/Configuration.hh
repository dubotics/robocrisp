#ifndef Configuration_hh
#define Configuration_hh 1

/* #include <cassert> */
#include "common.hh"
#include "Module.hh"
#include "Message.hh"

#include "SArray.hh"

#if defined(ENABLE_ASSERT) && ENABLE_ASSERT
#  include <cassert>
#endif

#ifdef SWIG
#  define Sensor Robot::Sensor
#  define Module Robot::Module
#endif

namespace Robot
{
  struct __attribute__ (( packed, align(1) ))
  Configuration
  {
    typedef Configuration TranscodeAsType;
    static constexpr MessageType Type = MessageType::CONFIGURATION_RESPONSE;
    static const size_t HeaderSize;


    Configuration(uint8_t modules_alloc = 0);
    Configuration(Configuration&& config);
    ~Configuration();
    void reset();

    bool
    operator ==(const Configuration& c) const;

    /** Get the encoded size of this configuration object. */
    size_t get_encoded_size() const;

    /** Write a serialzed form of this configuration to a memory buffer. */
    EncodeResult encode(Robot::MemoryEncodeBuffer& buf) const;

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
    spt::SArray<Module> modules;

#ifdef SWIG
%immutable;
#endif
    uint8_t
      next_module_id;
  };
}

#ifdef SWIG
#  undef Sensor
#  undef Module
#endif

#endif	/* Configuration_hh */
