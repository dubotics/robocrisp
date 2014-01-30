/** @file
 *
 * Test program for MessageDispatcher template.
 */
#define NODE_NO_DISPATCHER_CONTROL 1
#include <crisp/comms/MessageDispatcher.hh>
#include <deque>
#include <cstdio>

/** Simple node type for testing purposes. */
struct Node
{
  boost::asio::io_service& io_service;
  crisp::comms::MessageDispatcher<Node> dispatcher;
  crisp::comms::Configuration configuration;
  crisp::comms::NodeRole role;

  inline boost::asio::io_service&
  get_io_service() { return io_service; }

  Node(boost::asio::io_service& service)
    : io_service ( service ),
      dispatcher ( *this ),
      configuration ( ),
      role ( crisp::comms::NodeRole::MASTER )
  {}

  inline void
  receive(const crisp::comms::Message& message)
  { dispatcher.dispatch(crisp::comms::Message(message), crisp::comms::MessageDirection::INCOMING); }

  inline void
  send(const crisp::comms::Message& message)
  { dispatcher.dispatch(crisp::comms::Message(message), crisp::comms::MessageDirection::OUTGOING); }
};


int
main(int argc, char* argv[])
{
  using namespace crisp::comms;
  Configuration configuration;
  configuration.add_module( "drive", 2, 2)
    .add_input<int8_t>({ "speed", {} })
    .add_input<int8_t>({ "turn", {} })
    .add_sensor<uint16_t>({ "front proximity", SensorType::PROXIMITY, {} })
    .add_sensor<uint16_t>({ "rear proximity", SensorType::PROXIMITY, {} });

  configuration.add_module( "arm", 3 )
    .add_input<float>({ "rotation", {} })
    .add_input<float>({ "joint0", {} })
    .add_input<float>({ "joint1", {} });

  boost::asio::io_service service;
  Node node ( service );
  node.dispatcher.configuration_response.received
            .connect([&](Node& _node, const Configuration& config)
                     {
                       fprintf(stderr, "Got configuration:\n");
                       for ( const Module& module : config.modules )
                         {
                           fprintf(stderr, "    module %u: \"%s\"\n", module.id, module.name);
                           for ( const ModuleInput<>& input : module.inputs )
                             fprintf(stderr, "        input %u: \"%s\" (%s)\n", input.input_id, input.name, input.data_type.type_name());
                           for ( const Sensor<>& sensor : module.sensors )
                             fprintf(stderr, "       sensor %u: \"%s\" (%s)\n", sensor.id, sensor.name, sensor.data_type.type_name());
                         }
                     });


  node.receive(configuration);
  node.send(configuration);

  service.run();

  return 0;
}
