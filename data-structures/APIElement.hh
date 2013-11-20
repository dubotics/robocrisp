#ifndef APIElement_hh
#define APIElement_hh 1

#ifndef BOOST_PARAMETER_MAX_ARITY
#define BOOST_PARAMETER_MAX_ARITY 20
#endif	/* BOOST_PARAMETER_MAX_ARITY */
#include <boost/parameter.hpp>

#include "Buffer.hh"
#include "common.hh"

namespace Robot
{
  namespace keywords
  {
    BOOST_PARAMETER_NAME(symbol)
    BOOST_PARAMETER_NAME(use)
    BOOST_PARAMETER_NAME(description)
  }

  struct APIElement
  {
    const char *symbol; size_t symbol_length;
    const char *use; size_t use_length;
    const char *description; size_t description_length;

    bool owns_api_strings;

    APIElement();
    APIElement(const APIElement&) = delete;

    /** Move constructor. */
    APIElement(APIElement&&);

    /** Initializing constructor. Sets up the API element's symbol and documentation.
     * @param _symbol Symbol used to identify this APIElement.
     * @param _use Short description of the purpose of this element.
     * @param _description Longer description of how this element will be used.

     * @param copy If @c true, the strings pointed to by @p _symbol, @p _use, and @p
     * _description will be copied into newly-allocated memory and freed at object destruction.
     */
    APIElement(const char* _symbol, const char* _use, const char* _description, bool copy = false);

    /** Boost.Parameter-enabled constructor using `keywords::_symbol`, `keywords::_use`, and
     *	`keywords::_description`.
     */
    template < typename _ArgumentPack >
    APIElement(_ArgumentPack&& args)
    : symbol ( args[keywords::_symbol | nullptr] ), symbol_length ( 0 ),
      use ( args[keywords::_use | nullptr] ), use_length ( 0 ),
      description ( args[keywords::_description | nullptr] ), description_length ( 0 ),
      owns_api_strings ( false )
    {}

    ~APIElement();

    /**< Free all referenced memory. */
    void reset();

    /** Free all referenced memory, and re-initialize from the given constants.
     * @param _symbol Symbol used to identify this APIElement.
     * @param _use Short description of the purpose of this element.
     * @param _description Longer description of how this element will be used.

     * @param copy If @c true, the strings pointed to by @p _symbol, @p _use, and @p
     * _description will be copied into newly-allocated memory and freed at object destruction.
     */
    void reset(const char* _symbol, const char* _use, const char* _description, bool copy = false);

    bool operator == (const APIElement&) const;

    size_t get_encoded_size() const;

    EncodeResult
    encode(EncodeBuffer& buf) const;

    DecodeResult
    decode(DecodeBuffer& buf);

  };
}

#endif	/* APIElement_hh */
