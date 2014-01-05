/** @file
 *
 * Contains explicit template instantiations of the BasicNode template.
 */
#include <crisp/comms/BasicNode.hh>
#include <crisp/comms/bits/BasicNode.tcc>

namespace crisp
{
  namespace comms
  {
    template class BasicNode<boost::asio::ip::tcp>;
  }
}
