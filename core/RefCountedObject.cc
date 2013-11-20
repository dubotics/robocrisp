#include "RefCountedObject.hh"

namespace spt
{
  RefCountedObject::RefCountedObject()
    : refCount ( 0 )
  {
  }

  RefCountedObject::~RefCountedObject() throw ( std::runtime_error )
  {
    if ( refCount > 0 )
      throw std::runtime_error("In RefCountedObject::~RefCountedObject(): destructor called with refCount > 0");
    else if ( refCount < 0 )
      throw std::runtime_error("In RefCountedObject::~RefCountedObject(): destructor called with refCount < 0");
  }
}
