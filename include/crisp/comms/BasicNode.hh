/** @file
 *
 * Declares template class BasicNode, which provides a Message-passing utility for IP protocols
 * based on Boost.Asio.
 *
 * See the note on BasicNode for instantiation information.
 */
#ifndef crisp_comms_BasicNode_hh
#define crisp_comms_BasicNode_hh 1


#include <crisp/comms/Configuration.hh>
#include <crisp/comms/Message.hh>
#include <crisp/comms/MessageDispatcher.hh>
#include <crisp/comms/common.hh>

#include <crisp/util/Scheduler.hh>
#include <crisp/util/WorkerObject.hh>
#include <crisp/util/SharedQueue.hh>
#include <crisp/util/Signal.hh>

#include <boost/asio/io_service.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <thread>

namespace crisp
{
  namespace comms
  {

    /** Basic Message node for use with Boost.Asio and IP-based protocols.
     *
     * @note By default, this template's implementation header is not included;
     * `extern template` declarations are used instead to reduce compile
     * overhead.  If you need a BasicNode instantiation that does *not* match
     * `BasicNode<boost::asio::ip::tcp>`, you must
     *
     *    #include <crisp/comms/bits/BasicNode.tcc>
     *
     * prior to the place of instantiation.
     */
    template < typename _Protocol >
    class BasicNode : protected crisp::util::WorkerObject
    {
    public:
      typedef _Protocol Protocol;
      typedef typename Protocol::socket Socket;
      typedef crisp::util::Signal<void(const BasicNode&)> DisconnectSignal;

    private:
      friend class MessageDispatcher<BasicNode>;
      /** Connected socket used for communication. */
      Socket m_socket;

      /** Handle to the scheduled period-action used for synchronization.  */
      std::weak_ptr<crisp::util::PeriodicAction> m_sync_action;

      /** Handle to the scheduled "halt" action, used to disconnect the node if
       *  a handshake isn't completed within five seconds of the node being
       *  launched.
       */
      std::weak_ptr<crisp::util::ScheduledAction> m_halt_action;

      crisp::util::SharedQueue<Message>
        m_outgoing_queue;         /**< Outgoing message queue. */

      /** Whether the node's `halt` method has been called. */
      std::atomic_flag m_halting;

      /** Referent for `stopped`.  This is used to provide a way for users of BasicNode to
          determine whether or not the node has been shut down. */
      std::atomic<bool> m_stopped;

      std::mutex m_halt_mutex;
      std::condition_variable m_halt_cv;

      /** Signal emitted when the connection is closed. */
      DisconnectSignal  m_disconnect_signal;

      /** Set to `true` when the disconnect signal has been emitted, to avoid
          emitting it twice (send AND receive loops can both trigger a halt).  */
      std::atomic_flag m_disconnect_emitted;

    public:
      /** Public "node-is-halting-or-stopped" flag. */
      const std::atomic<bool>& stopped;

      /** A scheduler for managing periodic communication (and any user-set actions) on the node.
          Within BasicNode, this is used for producing synchronization messages.  */
      crisp::util::Scheduler scheduler;

      /** Node role specified at object construction. */
      const NodeRole role;

      /** Configuration object.
       *
       *   * When `role` is `NodeRole::MASTER`, this will contain the interface configuration
       *     declared by the connected slave node, once it has been received.
       *
       *   * When `role` is `NodeRole::SLAVE`, this contains the local interface
       *     configuration.
       */
      Configuration configuration;

      /** Message dispatcher and user-set callbacks container.  Provides  */
      MessageDispatcher<BasicNode> dispatcher;


      /** Initialize a node using the given socket and role.
       *
       * @param _socket Rvalue reference to the socket to be used.  The socket *must* be
       *     connected to the remote node.
       *
       * @param _role Role to request when performing the initial handshake with the remote
       *     node.
       */
      BasicNode(Socket&& _socket, NodeRole _role);

      virtual ~BasicNode();

      /** Launch the node's worker threads, and wait for a call to halt() from
          any other thread. (i.e., blocks until another thread calls `halt`.) */
      void run();


      /** Launch worker threads to handle the node's IO.
       *
       * @return `false` if already running, and `true` otherwise.
       */
      bool
      launch();


      /** Halt the node by closing the socket and waiting for its worker threads to finish.
       *
       * @param force_try Force an attempt to halt even if it looks like the function was called
       *   from the wrong thread.
       *
       * @return `true` if the node was able to halt, and `false` if the function was called
       *     from the wrong thread or the node was already halted.
       */
      bool
      halt(bool force_try = false);


      /** Enqueue an outgoing message.  The message will be sent once all previously-queued outgoing
       * messages have been sent.
       *
       * @param m Message to send.
       */
      void send(const Message& m);


      /** Enqueue an outgoing message.  The message will be sent once all previously-queued outgoing
       * messages have been sent.
       *
       * @param m Message to send.
       */
      void send(Message&& m);

      /** Register a function to be called when the connection ends.
       *
       * @param func The function to be called when the connection ends.
       */
      typename DisconnectSignal::Connection
      on_disconnect(typename DisconnectSignal::Function func);

    protected:
      /** Send outgoing messages until the socket is closed.
       *
       * @param yield A Boost.Asio `yield_context`, used to enable resuming of this method from
       *     asynchronous IO-completion handlers.
       */
      void send_loop(boost::asio::yield_context yield);


      /** Read data from the socket until we find a valid sync message. */
      bool resync(boost::asio::yield_context& yield);


      /** Read incoming messages and insert them into the incoming-message queue.
       *
       * @param yield A Boost.Asio `yield_context`, used to enable resuming of this method from
       *     asynchronous IO-completion handlers.
       */
      void receive_loop(boost::asio::yield_context yield);
    };

    extern template class BasicNode<boost::asio::ip::tcp>;
  }
}

#endif  /* crisp_comms_BasicNode_hh */
