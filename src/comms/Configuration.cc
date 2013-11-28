#include <crisp/comms/Configuration.hh>
#include <crisp/comms/Message.hh>
#include <cstdio>

namespace crisp
{
  namespace comms
  {
    const size_t
    Configuration::HeaderSize =
      offsetof(Configuration, modules);

    Configuration::Configuration(uint8_t modules_alloc)
      : num_modules ( 0 ),
	modules ( modules_alloc ),
	next_module_id ( 0 )
    {}

    Configuration::Configuration(const Configuration& config)
      : num_modules ( config.num_modules ),
        modules ( config.modules ),
        next_module_id ( config.next_module_id )
    {}

    Configuration::Configuration(Configuration&& c)
      : num_modules ( c.num_modules ),
	modules ( std::move(c.modules) ),
	next_module_id ( c.next_module_id )
    {}

    Configuration::~Configuration()
    {}
    void
    Configuration::reset()
    {
      num_modules = 0;
      modules.clear();
      next_module_id = 0;
    }

    Configuration&
    Configuration::operator =(const Configuration& c)
    {
      num_modules = c.num_modules;
      modules = c.modules;
      next_module_id = c.next_module_id;
      return *this;
    }


    Configuration&
    Configuration::operator =(Configuration&& c)
    {
      num_modules = c.num_modules;
      modules = std::move(c.modules);
      next_module_id = c.next_module_id;
      return *this;
    }


    bool
    Configuration::operator ==(const Configuration& c) const
    {
      if ( num_modules != c.num_modules )
	fprintf(stderr, "Failing equality on `num_modules`: %d <=> %d\n", num_modules, c.num_modules);

      return
	num_modules == c.num_modules &&
	(__extension__
	 ({ bool o ( true );
	   for ( uint8_t i ( 0 ); i < num_modules; ++i )
	     if ( static_cast<const Module&>(modules[i]) != static_cast<const Module&>(c.modules[i]) )
	       { fprintf(stderr, "Failing equality on module %d\n", i);
		 o = false; break; }
	   o;
	 }));
    }

    size_t
    Configuration::get_encoded_size() const
    {
      size_t out ( HeaderSize );
      for ( uint8_t i ( 0 ); i < num_modules; ++i )
	out += modules[i].get_encoded_size();
      return out;
    }
    EncodeResult
    Configuration::encode(MemoryEncodeBuffer& buf) const
    {
      EncodeResult r;
      if ( (r = buf.write(&num_modules, sizeof(num_modules))) != EncodeResult::SUCCESS )
	return r;
      else
	for ( const Module& module : modules )
	  { if ( (r = module.encode(buf)) != EncodeResult::SUCCESS )
	      return r; }

      return EncodeResult::SUCCESS;
    }

    DecodeResult
    Configuration::decode(DecodeBuffer& buf)
    {
      reset();
      DecodeResult r;

      if ( (r = buf.read(&num_modules, sizeof(num_modules))) != DecodeResult::SUCCESS )
	return r;
      else
	{
	  modules.ensure_capacity(num_modules);
	  for ( uint8_t i ( 0 ); i < num_modules; ++i )
	    if ( (r = modules.emplace().decode(buf)) != DecodeResult::SUCCESS )
	      return r;
	}

      return DecodeResult::SUCCESS;
    }

  }
}
