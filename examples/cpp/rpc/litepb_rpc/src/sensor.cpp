#include "sensor.h"

using namespace examples::sensor;
using namespace litepb;

void setup_sensor_service(RpcChannel& channel, SensorServiceServer& service)
{
    register_sensor_service(channel, service);
}
