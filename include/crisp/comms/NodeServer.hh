/** @file
 *
 * Declares a class `NodeServer` for TCP-based protocol-node variants.
 */
#ifndef crisp_comms_NodeServer_hh
#define crisp_comms_NodeServer_hh 1

#include <mutex>
#include <thread>
#include <unordered_set>

#include <boost/asio/spawn.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <crisp/comms/BasicNode.hh>
#include <crisp/comms/Configuration.hh>
#include <crisp/comms/MessageDispatcher.hh>

namespace crisp
{
  namespace comms
  {
    /** Listens for incoming connections on a specific endpoint, and manages connected nodes.
     *
     *  For configuration of the local end of each connection, NodeServer contains a
     *  Configuration object `configuration` and a MessageDispatcher object `dispatcher`, which
     *  are copied to each node created for a connection.
     */
    template < typename _Node >
    struct NodeServer
    {
      typedef _Node Node;
      typedef typename Node::Socket::protocol_type Protocol;
      typedef std::unordered_set<Node*> NodeSet;
      typedef typename Protocol::acceptor Acceptor;

      /** Reference to the I/O service used by Boost.Asio classes and methods.  */
      boost::asio::io_service& io_service;

      /** Server-socket used to accept incoming connections. */
      Acceptor acceptor;

      /** Connected nodes. */
      NodeSet nodes;

      std::mutex nodes_mutex;

      /** Configuration to be installed on created (nodes). */
      Configuration configuration;

      /** MessageDispatcher to be copied to created (nodes). */
      MessageDispatcher<_Node> dispatcher;

      /** Handle to the thread in which `run` is running when called indirectly via `launch`. */
      std::thread run_thread;

      /** Flag used to indicate that the server is currently halting. */
      std::atomic_flag halting;

      /** Flag used to communicate to the server loop that it should exit. */
      std::atomic<bool> stopped;


      /** Maximum number of simultaneous live connections.  Zero is
          unlimited. */
      size_t max_simultaneous_connections;

      /** Total (cumulative) number of connections after which the server will
          shut down.  Unlimited if 0.  */
      size_t max_cumulative_connections;

      /** Total number of connections seen by this server instance so far. */
      size_t num_cumulative_connections;

      NodeServer(boost::asio::io_service& _io_service,
                 const typename Protocol::endpoint& listen_endpoint,
                 size_t _max_simultaneous_connections = 0,
                 size_t _max_cumulative_connections = 0);

      ~NodeServer();

      void launch();

      void run();

      void halt();

      /** Main server routine.
       */
      void
      server_main(boost::asio::yield_context yield);

    };

    extern template class NodeServer< BasicNode<boost::asio::ip::tcp> >;
  }
}

#endif  /* crisp_comms_NodeServer_hh */
