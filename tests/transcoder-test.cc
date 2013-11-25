#include <cstdint>
#include <cstring>
#include <cassert>
#include <cmath>		/* for M_PI_2 */

#include <crisp/util/timeutil.h>
#include <crisp/comms/common.hh>
#include <crisp/comms/Sensor.hh>
#include <crisp/comms/Module.hh>
#include <crisp/comms/ModuleControl.hh>
#include <crisp/comms/Configuration.hh>
#include <crisp/comms/Handshake.hh>

using namespace crisp::comms;

#include <typeinfo>
#include <cxxabi.h>
inline const char*
demangle(const char* name)
{
  return abi::__cxa_demangle(name, 0, 0, NULL);
}


static void
dump_buffers(const char* b1, size_t l1,
	     const char* b2, size_t l2)
{
  fprintf(stderr, "  original: ");
  for ( size_t i = 0; i < l1; ++i )
    fprintf(stderr, " %02hhX", b1[i]);
  fprintf(stderr, "\n");

  fprintf(stderr, "   decoded: ");
  for ( size_t i = 0; i < l2; ++i )
    {
      bool match ( i < l1 && b1[i] == b2[i] );
      fprintf(stderr, " %s%02hhX%s", match ? "" : "[1;31m", b2[i], match ? "" : "[0m");
    }
  fprintf(stderr, "\n");

}

template < typename _T, typename... _Args >
static void
test_encode(const _T& obj, _Args&&... args)
{
  timeutil_init_mark_variables();
  typedef typename _T::TranscodeAsType TranscodeAsType;
  const char* demangled_names[] { demangle(typeid(_T).name()), demangle(typeid(typename _T::TranscodeAsType).name()) };

  fprintf(stderr,
	  "[1m%s[0m ⇒ [1m%s[0m\n",
	  demangled_names[0], demangled_names[1]);

  /* It's probably unlikely that even a mildly-buggy `encode` method will write past *twice* the
     object's reported encoded size...  And if it's severely buggy?  Well, this *is* a test
     program, and in the worst-case scenario we crash and burn.  */
  size_t alloc_size ( 2 * obj.get_encoded_size() );
  char* alloc_buf ( static_cast<char*>(alloca(alloc_size)) );
  memset(alloc_buf, 0, alloc_size);

  MemoryEncodeBuffer encode_buffer ( alloc_buf, alloc_size );

  /* If `obj` is of it's own transcode-type, we simply reference it; otherwise we copy-construct
     an instance of that type from it. */
  typename std::conditional<std::is_same<_T, TranscodeAsType>::value, const _T&, TranscodeAsType>::type obj_generic ( obj );

  if ( std::is_same<_T, TranscodeAsType>::value )
    {
      fprintf(stderr, "is_standard_layout<%s> => %hhu\n", demangled_names[0], static_cast<uint8_t>(std::is_standard_layout<_T>::value));
      /* fprintf(stderr, "        is_trivial<%s> => %hhu\n", demangled_names[0], static_cast<uint8_t>(std::is_trivial<_T>::value));
       * fprintf(stderr, "            is_pod<%s> => %hhu\n", demangled_names[0], static_cast<uint8_t>(std::is_pod<_T>::value)); */
    }
  else
    {
      fprintf(stderr, "is_standard_layout<%s> => %hhu\n", demangled_names[0], static_cast<uint8_t>(std::is_standard_layout<_T>::value));
      fprintf(stderr, "is_standard_layout<%s> => %hhu\n", demangled_names[1], static_cast<uint8_t>(std::is_standard_layout<TranscodeAsType>::value));
      /* fprintf(stderr, "        is_trivial<%s> => %hhu\n", demangled_names[0], static_cast<uint8_t>(std::is_trivial<_T>::value));
       * fprintf(stderr, "        is_trivial<%s> => %hhu\n", demangled_names[1], static_cast<uint8_t>(std::is_trivial<TranscodeAsType>::value));
       * fprintf(stderr, "            is_pod<%s> => %hhu\n", demangled_names[0], static_cast<uint8_t>(std::is_pod<_T>::value));
       * fprintf(stderr, "            is_pod<%s> => %hhu\n", demangled_names[1], static_cast<uint8_t>(std::is_pod<TranscodeAsType>::value)); */
    }

  timeutil_begin("encoding");
  EncodeResult r ( obj_generic.encode(encode_buffer) );
  timeutil_end();
  if ( r != EncodeResult::SUCCESS )
    fprintf(stderr, "[1;31mencode failed[0m\n");
  assert(r == EncodeResult::SUCCESS);


  DecodeBuffer decode_buffer ( alloc_buf, alloc_size );
  TranscodeAsType decoded_obj;
  timeutil_begin("decoding");
  DecodeResult dr ( decoded_obj.decode(decode_buffer, args...) );
  timeutil_end();
  if ( dr != DecodeResult::SUCCESS )
    fprintf(stderr, "[1;31mdecode failed[0m\n");
  else
    {

      fprintf(stderr, "    [1;97msizes:[0m encoded %zu, decoded %zu\n", obj_generic.get_encoded_size(), decoded_obj.get_encoded_size());

      /* Encode and display the decoded object for comparison  */
      size_t alloc2_size ( 2 * decoded_obj.get_encoded_size() );
      char* alloc2_buf ( static_cast<char*>(alloca(alloc2_size)) );
      memset(alloc2_buf, 0, alloc2_size);
      assert(alloc2_buf != NULL);
      MemoryEncodeBuffer encode2_buffer ( alloc2_buf, alloc2_size );
      assert(encode2_buffer.data != NULL);

      assert( (r = decoded_obj.encode(encode2_buffer)) == EncodeResult::SUCCESS);

      dump_buffers(alloc_buf, encode_buffer.offset,
		   alloc2_buf, encode2_buffer.offset);

      fprintf(stderr, "  [1;97moffsets:[0m encoded %zu, decoded %zu\n", encode_buffer.offset, decode_buffer.offset);
      fprintf(stderr, "  decoded == original? %s\n", decoded_obj == obj ? "[1;32myes[0m" : "[1;31mno[0m");

      assert(decoded_obj == obj);
      /* if ( std::is_same<_T, TranscodeAsType>::value )
       * 	assert(decoded_obj.get_encoded_size() == encode_buffer.offset);
       * else
       * 	assert(obj_generic.get_encoded_size() == encode_buffer.offset); */


    }
  
  for ( const char*& name : demangled_names )
    free(const_cast<char*>(name));

}

template < typename _T, typename... _Args >
static void
test_message_encode(const _T& x, _Args&&... args)
{
  typedef typename _T::TranscodeAsType TranscodeAsType;
  assert(x == x);

  const char* demangled_names[] { demangle(typeid(_T).name()), demangle(typeid(typename _T::TranscodeAsType).name()) };

  MemoryEncodeBuffer eb ( x.get_encoded_size() + Message::HeaderSize + MESSAGE_CHECKSUM_SIZE );
  Message m(x);
  assert(m.type == _T::Type);

  const detail::MessageTypeInfo& type_info ( detail::get_type_info(m.type) );

  m.encode(eb);
  fprintf(stderr, "%s\n  encoded message: ", __PRETTY_FUNCTION__);
  for ( size_t i = 0; i < eb.offset; ++i )
    fprintf(stderr, " %02hhX", eb[i]);
  fprintf(stderr, "\n");
  
  DecodeBuffer db ( eb.buffer );
  Message dm ( Message::decode(db) );
  TranscodeAsType dx ( std::move(m.as<_T>(args...)) );

  assert(dx == x);

  if ( type_info.has_checksum )
    {
      assert(dm.checksum == m.compute_checksum());
      fprintf(stderr, "  stored checksum: %08X\n", m.checksum);
      fprintf(stderr, "computed checksum: %08X\n", m.compute_checksum());
    }
  /* assert(m.checksum_ok());
   * assert(dm.checksum_ok()); */

  for ( const char*& name : demangled_names )
    free(const_cast<char*>(name));
}

int
main(int, char*[])
{
  using namespace crisp::comms::keywords;


  Configuration config;

  config.add_module( "drive", 2 )
    .add_input<int8_t>({ "speed", {_neutral = 0, _minimum = -127, _maximum = 127} })
    .add_input<int8_t>({ "turn", { _neutral = 0, _minimum = -127, _maximum = 127 } });

  Module& drive ( config.modules.back() );
  assert(drive.inputs[0].input_id != drive.inputs[1].input_id);

  config.add_module( "arm", 2 )
    .add_input<float>({ "joint0", { _minimum = -M_PI_2, _maximum = M_PI_2 }})
    .add_input<float>({ "joint1", { _minimum = -M_PI_2, _maximum = M_PI_2 }});
  /* DataDeclaration */
  DataDeclaration<float> float_type { _neutral = -2.0, _minimum = 1.0,  _maximum = 4.0 }; /* ...heh.  Maybe we should add bounds/neutral order checks? */
  test_encode(float_type);
  test_encode(DataDeclaration<int> { });

  /* Sensors */
  config.modules[0]
    .add_sensor<uint16_t>({ "front proximity", SensorType::PROXIMITY, { _minimum = 50, _maximum = 500 } });
  
  fprintf(stderr, "\n[1mTranscoding Tests[0m\n");

  /* Modules and inputs */
  for ( const Module& module : config.modules )
    {
      for ( const ModuleInput<>& input : module.inputs )
	test_encode(input);
      test_encode(module);
    }

  /* ModuleControl */
  ModuleControl
    cta ( &(config.modules[1]) ),
    ctd ( &(config.modules[0]) ),
    ctt ( &(config.modules[0]) );

  ctd.set<int8_t>("turn", 3);
  ctd.set<int8_t>("speed", 127);

  fprintf(stderr, "iids %d, speed %d, turn %d\n", ctd.input_ids, ctd.get<int8_t>("speed"), ctd.get<int8_t>("turn"));
  assert(ctd.get<int8_t>("turn") == 3);
  assert(ctd.get<int8_t>("speed") == 127);

  test_encode(ctd, config);
  
  cta.set<float>("joint0", M_PI_4);
  cta.set<float>("joint1", M_PI_2);
  test_encode(cta, config);

  ctt.clear("speed", "turn");
  
  test_encode(ctt, config);

  /* Config */
  test_encode(config);

  test_encode(Handshake(0, NodeRole::MASTER));

  MemoryEncodeBuffer eb ( 7 );
  Message::encode(eb, MessageType::SYNC);
  fprintf(stderr, "\n"
	  "[1mSpecial: SYNC[0m\n"
	  "  encodes as: ");
  for ( size_t i = 0; i < eb.offset; ++i )
    fprintf(stderr, " %02hhX", eb[i]);
  fprintf(stderr, "\n");
  
  

  fprintf(stderr, "\n[1mMessage Encapsulation Tests[0m\n");
  test_message_encode(ctt, config);
  test_message_encode(Handshake(0, NodeRole::SLAVE));
  test_message_encode(HandshakeResponse { HandshakeAcknowledge::ACK, NodeRole::SLAVE });
  test_message_encode(config);
  return 0;
}
