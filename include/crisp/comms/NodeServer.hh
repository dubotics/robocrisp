/** @file
 *
 * Declares a class `NodeServer` for TCP-based protocol-node variants.
 */
#ifndef crisp_comms_NodeServer_hh
#define crisp_comms_NodeServer_hh 1

#include <unordered_set>
#include <boost/asio/io_service.hpp>
#include <thread>

namespace crisp
{
  namespace comms
  {
    /** Listens for incoming connections on a specific endpoint, and manages
        connected nodes. */
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

      /** Configuration to be sent to remote nodes in response to CONFIGURATION_QUERY-type
          messages. */
      Configuration configuration;

      /** Handle to the thread in which `run` is running when call indirectly via `launch`. */
      std::thread run_thread;


      NodeServer(boost::asio::io_service& _io_service,
                 const typename Protocol::endpoint& listen_endpoint)
        : io_service ( _io_service ),
          acceptor ( io_service, listen_endpoint ),
          nodes ( ),
          configuration ( ),
          run_thread ( )
      {}

      ~NodeServer()
      {
        halt();
      }

      void launch()
      {
        run_thread = std::thread(std::bind(&NodeServer::run, this));
      }

      void
      run()
      {
        std::thread main_thread( std::bind(&NodeServer::server_main, this) );
        main_thread.join();
      }

      void
      halt()
      {
        acceptor.close();

        /* Shut down all of the client nodes. */
        for ( Node* node : const_cast<NodeSet&>(nodes) )
          {
            node->halt();
            delete node;
          }

        io_service.stop();
      }


      /** Main server routine.
       */
      void
      server_main()
      {
        acceptor.listen();		/* start listening for incoming connections. */

        /* Grab the I/O coordinator service used by the connection-acceptor object. */
        boost::asio::io_service& service ( acceptor.get_io_service() );


        fputs("Entering `accept()` loop.\n", stderr);
        while ( acceptor.is_open() )
          {
            boost::system::error_code ec;
            typename Protocol::endpoint endpoint;

            /* Create an unconnected socket for `accept` to initialize. */
            typename Protocol::socket socket ( service );

            /* `accept` blocks until we receive an incoming connection or an
               error occurs.  */
            acceptor.accept(socket, endpoint, ec);
            if ( ec )
              {
                fprintf(stderr, "accept: %s\n", strerror(ec.value()));
                break;
              }
            else
              {
                std::cerr << "Accepted connection from " << endpoint << std::endl;

                /* We've got a connection.  Create a new protocol-node on the
                 * connection and add it to our set of connected nodes. */
                Node* node ( new Node(std::move(socket), NodeRole::SLAVE) );

                /** @fixme: There's a bug somewhere in the copy-constructor of
                    `crisp::comms::Module` that prevents the following (commented-out) lines
                    from working.  */
                /* node->configuration = config; /\* copy the configuration *\/
                 * assert(node->configuration == config); */

                /* Set up callbacks on the node. */
                node->dispatcher.configuration_query.received =
                  [&](Node& _node) { _node.send(_node.configuration); };


                /* Launch the node's worker threads. */
                node->launch();

                nodes.insert(node);
              }
          };
      }
    };

  }
}

#endif  /* crisp_comms_NodeServer_hh */
