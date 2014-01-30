/** @file
 *
 * Contains implementation of the NodeServer template class.
 */
#ifndef crisp_comms_bits_NodeServer_tcc
#define crisp_comms_bits_NodeServer_tcc 1

namespace crisp
{
  namespace comms
  {
    template < typename _Node >
    NodeServer<_Node>::NodeServer(boost::asio::io_service& _io_service,
                           const typename NodeServer<_Node>::Protocol::endpoint& listen_endpoint)
      : io_service ( _io_service ),
        acceptor ( io_service, listen_endpoint ),
        nodes ( ),
        nodes_mutex ( ),
        configuration ( ),
        dispatcher ( ),
        run_thread ( ),
        halting ( ),
        stopped ( false )
    {
      halting.clear();
    }

    template < typename _Node >
    NodeServer<_Node>::~NodeServer()
    {
      halt();
    }

    template < typename _Node >
    void
    NodeServer<_Node>::launch()
    {
      boost::asio::spawn(io_service, std::bind(&NodeServer<_Node>::server_main,
                                               this, std::placeholders::_1));
    }


    template < typename _Node >
    void
    NodeServer<_Node>::run()
    {
      launch();
      while ( ! stopped )
        io_service.run_one();
    }


    template < typename _Node >
    void
    NodeServer<_Node>::halt()
    {
      if ( ! halting.test_and_set() )
        {
          stopped = true;
          acceptor.close();
          io_service.run_one();

          /* Shut down all of the client nodes. */
          for ( Node* node : const_cast<NodeSet&>(nodes) )
            {
              node->halt();
              delete node;
            }
        }
    }

    template < typename _Node >
    void
    NodeServer<_Node>::server_main(boost::asio::yield_context yield)
    {
      acceptor.listen();		/* start listening for incoming connections. */

      /* Grab the I/O coordinator service used by the connection-acceptor object. */
      boost::asio::io_service& service ( acceptor.get_io_service() );


      fputs("Entering `accept()` loop.\n", stderr);
      while ( acceptor.is_open() && ! stopped )
        {
          boost::system::error_code ec;
          typename Protocol::endpoint endpoint;

          /* Create an unconnected socket for `accept` to initialize. */
          typename Protocol::socket socket ( service );

          /* `accept` blocks until we receive an incoming connection or an
             error occurs.  */
          acceptor.async_accept(socket, endpoint, yield[ec]);
          if ( ec )
            {
              if ( ec.value() != boost::asio::error::operation_aborted )
                fprintf(stderr, "accept: %s\n", strerror(ec.value()));
              break;
            }
          else
            {
              std::cerr << "Accepted connection from " << endpoint << std::endl;

              /* We've got a connection.  Create a new protocol-node on the
               * connection and add it to our set of connected nodes. */
              Node* node ( new Node(std::move(socket), NodeRole::SLAVE) );

              node->configuration = configuration;
              node->dispatcher = dispatcher;
              node->dispatcher.set_target(*node);

              /* Launch the node's worker threads. */
              {
                std::unique_lock<std::mutex> lock ( nodes_mutex );
                nodes.insert(node);
              }

              service.post(std::bind(&Node::launch, node));
            }
        }
      fputs("Exiting `accept()` loop.\n", stderr);
    }
  }
}

#endif  /* crisp_comms_bits_NodeServer_tcc */
