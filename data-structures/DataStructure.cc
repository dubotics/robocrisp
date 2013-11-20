#include "DataStructure.hh"
using namespace Robot;

#define ELEMENT_FIELDS (1 << 3)
#define FIELD_COUNT_TYPE uint8_t

DataStructure::DataStructure(const char* _symbol, const char* _use, const char* _description,
			     const std::initializer_list<DataField<void> >& _fields)
  : APIElement(_symbol, _use, _description),
    fields ( _fields )
{}

DataStructure::~DataStructure()
{ reset(); }

void
DataStructure::reset()
{
  APIElement::reset();
  fields.clear();
}
size_t
DataStructure::get_encoded_size() const
{
  size_t u ( APIElement::get_encoded_size() + ( fields.size > 0 ? sizeof(FIELD_COUNT_TYPE) : 0 ) );
  for ( const DataField<>& f : fields )
    u += f.get_encoded_size();
  return u;
}



EncodeResult
DataStructure::encode(EncodeBuffer& buf) const
{
  EncodeResult subencode_result;
  {
    /* I'm making the code uglier here for the sake of using alloca() instead of malloc()... */
    size_t alloc_size ( APIElement::get_encoded_size() );
    char* api_mem ( static_cast<char*>(alloca(alloc_size)) );
    MemoryEncodeBuffer membuf ( api_mem, alloc_size );

    if ( (subencode_result = APIElement::encode(membuf)) != EncodeResult::SUCCESS )
      return subencode_result;
    if ( fields.size > 0 )
      membuf[0] |= ELEMENT_FIELDS;

    if ( (subencode_result = buf.write(membuf.data, membuf.offset)) != EncodeResult::SUCCESS )
      return subencode_result;
  }

  if ( fields.size > 0 )
    {
      if ( (subencode_result = buf.write<FIELD_COUNT_TYPE>(fields.size)) != EncodeResult::SUCCESS )
	return subencode_result;

      for ( const DataField<>& f : fields )
	{ if ( (subencode_result = f.encode(buf)) != EncodeResult::SUCCESS )
	    return subencode_result;
	}
    }
  return EncodeResult::SUCCESS;
}

DecodeResult
DataStructure::decode(DecodeBuffer& buf)
{
  uint8_t mask  ( buf.data[buf.offset] );
  DecodeResult subdecode_result ( APIElement::decode(buf) );

  if ( subdecode_result != DecodeResult::SUCCESS )
    return subdecode_result;

  if ( ! (mask & ELEMENT_FIELDS) )
    return DecodeResult::SUCCESS;

  uint8_t field_count ( buf.read<FIELD_COUNT_TYPE>() );

  fields.ensure_capacity(field_count);
  for ( size_t i ( 0 );  i < field_count; ++i )
    { DataField<> f;
      if ( (subdecode_result = f.decode(buf)) != DecodeResult::SUCCESS )
	return subdecode_result;
      else
	fields.push(std::move(f));
    }

  return DecodeResult::SUCCESS;
}


bool
DataStructure::operator == (const DataStructure& ds) const
{
  return
    APIElement::operator ==(ds) && fields.size == ds.fields.size &&
    (__extension__
     ({ bool u ( true );
       for ( size_t i ( 0 ); i < fields.size; ++i )
	 if ( fields[i] != ds.fields[i] )
	   { u = false; break; }
       u; }));
}
