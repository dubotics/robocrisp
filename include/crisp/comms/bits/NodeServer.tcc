/** @file
 *
 * Contains implementation of the NodeServer template class.
 */
#ifndef crisp_comms_bits_NodeServer_tcc
#define crisp_comms_bits_NodeServer_tcc 1

#include <cstdint>

namespace crisp
{
  namespace comms
  {
    template < typename _Node >
    NodeServer<_Node>::NodeServer(boost::asio::io_service& _io_service,
                                  const typename NodeServer<_Node>::Protocol::endpoint& listen_endpoint,
                                  size_t _max_simultaneous_connections,
                                  size_t _max_cumulative_connections)
      : io_service ( _io_service ),
        acceptor ( io_service, listen_endpoint ),
        nodes ( ),
        nodes_mutex ( ),
        configuration ( ),
        dispatcher ( ),
        run_thread ( ),
        halting ( ),
        stopped ( false ),
        max_simultaneous_connections ( 0 ),
        max_cumulative_connections ( _max_cumulative_connections ),
        num_cumulative_connections ( 0 )
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

          io_service.post([](){}); /* nop */
          io_service.poll_one();

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

      /* HACK: max_cumulative_connections == 0 should mean "no limit", but the
         control logic is broken right now.  So we set
         max_cumulative_connections to a really big number instead. */
      if ( max_cumulative_connections == 0 )
        max_cumulative_connections = SIZE_MAX;

      while ( (num_cumulative_connections < max_cumulative_connections ||
               max_cumulative_connections == 0) &&
              ! stopped )
        {
          /* Block until we have an open connection slot. */
          if ( max_simultaneous_connections > 0 &&
               nodes.size() > max_simultaneous_connections )
            {
              fputs("Max connections reached; waiting for a connection slot.\n", stderr);
              fflush_unlocked(stderr);
              while ( nodes.size() >= max_simultaneous_connections )
                {
                  std::unique_lock<std::mutex> lock ( nodes_mutex );
                  current_connections_cv.wait(lock);
                }
              fputs("Connection slot open; continuing.", stderr);
              fflush_unlocked(stderr);
            }

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
              ++num_cumulative_connections;
              std::cerr << "Accepted connection from " << endpoint << std::endl;

              /* We've got a connection.  Create a new protocol-node on it. */
              Node* node ( new Node(std::move(socket), NodeRole::SLAVE) );

              /* Set up the node's callbacks and interface configuration. */
              node->configuration = configuration;
              node->dispatcher = dispatcher;
              node->dispatcher.set_target(*node);

              /* Add the node to the list of active connections.  */
              {
                std::unique_lock<std::mutex> lock ( nodes_mutex );
                nodes.insert(node);
                current_connections_cv.notify_all();
              }

              /* Remove the node from the set of active connections on
                 disconnect. */
              node->on_disconnect([&](const Node&) {
                    std::unique_lock<std::mutex> lock ( nodes_mutex );
                    nodes.erase(node);
                    delete node;
                    current_connections_cv.notify_all();
                });

              /* Launch the node's worker threads. */
              service.post(std::bind(&Node::launch, node));
            }
        }
      fprintf(stderr, "Exited `accept()` loop: %s.\n",
              num_cumulative_connections >= max_simultaneous_connections
              ? "max. cumulative connections reached"
              : (stopped
                 ? "halt requested"
                 : "reason unknown"));
      fflush_unlocked(stderr);

      /* Wait for all the connections to close. */
      while ( nodes.size() > 0 )
        {
          std::unique_lock<std::mutex> lock ( nodes_mutex );
          current_connections_cv.wait(lock);
        }

      halt();
    }
  }
}

#endif  /* crisp_comms_bits_NodeServer_tcc */
