#include "switch.h"

using namespace switch_service;
using namespace litepb;

void setup_switch_service(RpcChannel& channel, SwitchServiceServer& service)
{
    register_switch_service(channel, service);
}
