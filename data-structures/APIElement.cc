#include "APIElement.hh"
using namespace Robot;

#define API_STRINGS_LENGTH_TYPE uint8_t

enum ElementMasks
  {
    ELEMENT_SYMBOL	= 1 << 0,
    ELEMENT_USE		= 1 << 1,
    ELEMENT_DESCRIPTION	= 1 << 2,
  };

APIElement::APIElement()
  : symbol ( nullptr ), symbol_length ( 0 ),
    use ( nullptr ), use_length ( 0 ),
    description ( nullptr ), description_length ( 0 ),
    owns_api_strings ( false )
{}
APIElement::APIElement(APIElement&& api_element)
  : symbol ( api_element.symbol ), symbol_length ( api_element.symbol_length ),
    use ( api_element.use ), use_length ( api_element.use_length ),
    description ( api_element.description ), description_length ( api_element.description_length ),
    owns_api_strings ( api_element.owns_api_strings )
{ api_element.owns_api_strings = false; }


APIElement::APIElement(const char* _symbol, const char* _use, const char* _description, bool copy)
  : symbol ( _symbol ), symbol_length ( _symbol ? strlen(_symbol) : 0 ),
    use ( _use ), use_length ( _use ? strlen(_use) : 0 ),
    description ( _description ), description_length ( _description ? strlen(_description) : 0 ),
    owns_api_strings ( false )
{
  reset(_symbol, _use, _description, copy);
}

APIElement::~APIElement()
{
  reset();
}

void
APIElement::reset()
{
  if ( owns_api_strings )
    {
      if ( symbol ) free(const_cast<char*>(symbol));
      else if ( use ) free(const_cast<char*>(use));
      else if ( description ) free(const_cast<char*>(description));
    }
  owns_api_strings = false;
  symbol = use = description = nullptr;
  symbol_length = use_length = description_length = 0;
}
void
APIElement::reset(const char* _symbol, const char* _use, const char* _description, bool copy)
{
  reset();
  symbol_length = _symbol ? strlen(_symbol) : 0;
  use_length = _use ? strlen(_use) : 0;
  description_length = _description ? strlen(_description) : 0;

  if ( copy )
    {
      if ( symbol_length + use_length + description_length > 0 )
	{	
	  size_t alloc_size ( ( symbol_length ? symbol_length + 1 : 0 )+
			      ( use_length ? use_length + 1 : 0 ) +
			      ( description_length ? description_length + 1 : 0 ) );
	  char* sbuf ( static_cast<char*>(malloc(alloc_size)) );
	  memset(const_cast<char*>(sbuf), 0, alloc_size);

	  symbol = sbuf; sbuf += symbol_length + 1; memcpy(const_cast<char*>(symbol), _symbol, symbol_length);
	  use = sbuf; sbuf += use_length + 1; memcpy(const_cast<char*>(use), _use, use_length);
	  description = sbuf; memcpy(const_cast<char*>(description), _description, description_length);

	  owns_api_strings = true;
	}
    }
  else
    {
      symbol = _symbol;
      use = _use;
      description = _description;
      owns_api_strings = false;
    }
}


bool
APIElement::operator == (const APIElement& ae) const
{ return
    (symbol == nullptr || ae.symbol == nullptr ? symbol == ae.symbol : ! strcmp(symbol, ae.symbol)) &&
    (use == nullptr || ae.use == nullptr ? use == ae.use : ! strcmp(use, ae.use)) &&
    (description == nullptr || ae.description == nullptr ? description == ae.description : ! strcmp(description, ae.description));
}


/* Encoding/decoding:  An encoded APIElement consists of the following fields:
 *
 *    - a one-byte mask specifying which of `symbol`, `use`, and `description` are present
 *    - if `mask & ELEMENT_SYMBOL`...
 *      > one byte for the symbol length
 *      > and that many bytes of symbol string
 *    - if `mask & ELEMENT_USE`...
 *      > one byte for the use length
 *      > and that many bytes of use string
 *    - if `mask & ELEMENT_DESCRIPTION`...
 *      > one byte for the description length
 *      > and that many bytes of description string
 */

size_t
APIElement::get_encoded_size() const
{
  return 1 +
    ( symbol_length ? symbol_length + 1 : 0 ) +
    ( use_length ? use_length + 1 : 0 ) +
    ( description_length ? description_length + 1 : 0 );
}

EncodeResult
APIElement::encode(EncodeBuffer& buf) const
{
  if ( ! this->symbol && ! this->use && ! this->description )
    {
      buf.write<API_STRINGS_LENGTH_TYPE>(0);
      return EncodeResult::SUCCESS;
    }
  else
    {
      uint8_t mask =
	(symbol ? ELEMENT_SYMBOL : 0) |
	(use ? ELEMENT_USE : 0 ) |
	(description ? ELEMENT_DESCRIPTION : 0 );
      assert(mask != 0);
      buf.write<API_STRINGS_LENGTH_TYPE>(mask);

      if ( mask & ELEMENT_SYMBOL )
	buf.write<API_STRINGS_LENGTH_TYPE>(static_cast<API_STRINGS_LENGTH_TYPE>(symbol_length));
      if ( mask & ELEMENT_USE )
	buf.write<API_STRINGS_LENGTH_TYPE>(static_cast<API_STRINGS_LENGTH_TYPE>(use_length));
      if ( mask & ELEMENT_DESCRIPTION )
	buf.write<API_STRINGS_LENGTH_TYPE>(static_cast<API_STRINGS_LENGTH_TYPE>(description_length));

      if ( mask & ELEMENT_SYMBOL )
	buf.write(symbol, symbol_length);
      if ( mask & ELEMENT_USE )
	buf.write(use, use_length);
      if ( mask & ELEMENT_DESCRIPTION )
	buf.write(description, description_length);
    }
  return EncodeResult::SUCCESS;
}

DecodeResult
APIElement::decode(DecodeBuffer& buf)
{
  reset();

  uint8_t mask ( 0 );
  buf.read(&mask, sizeof(mask));

  if ( mask & ELEMENT_SYMBOL )
    symbol_length = buf.read<API_STRINGS_LENGTH_TYPE>();
  if ( mask & ELEMENT_USE )
    use_length = buf.read<API_STRINGS_LENGTH_TYPE>();
  if ( mask & ELEMENT_DESCRIPTION )
    description_length = buf.read<API_STRINGS_LENGTH_TYPE>();

  DecodeResult r ( DecodeResult::SUCCESS );
  size_t alloc_size ( ( symbol_length ? symbol_length + 1 : 0 ) +
		      ( use_length ? use_length + 1 : 0 ) +
		      ( description_length ? description_length + 1 : 0 ) );

  if ( alloc_size )
    {
      const char* sbuf ( static_cast<const char*>(malloc(alloc_size)) );
      owns_api_strings = true;
      memset(const_cast<char*>(sbuf), 0, alloc_size);

      if ( symbol_length )
	{ symbol = sbuf;
	  if ( (r = buf.read(sbuf, symbol_length)) != DecodeResult::SUCCESS )
	    return r;
	  sbuf += symbol_length + 1; }

      if ( use_length )
	{ use = sbuf;
	  if ( (r = buf.read(sbuf, use_length)) != DecodeResult::SUCCESS )
	    return r;
	  sbuf += use_length + 1; }

      if ( description_length )
	{ description = sbuf;
	  if ( (r = buf.read(sbuf, description_length)) != DecodeResult::SUCCESS )
	    return r;
	  sbuf += description_length + 1; }
    }

  return DecodeResult::SUCCESS;
}
