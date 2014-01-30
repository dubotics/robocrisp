/** @file
 *
 * Explicit template instantiation of `NodeServer< BasicNode<boost::asio::tcp::ip> >`.
 */
#include <crisp/comms/NodeServer.hh>
#include <crisp/comms/bits/NodeServer.tcc>

namespace crisp
{
  namespace comms
  {
    template class NodeServer< BasicNode<boost::asio::ip::tcp> >;
  }
}
