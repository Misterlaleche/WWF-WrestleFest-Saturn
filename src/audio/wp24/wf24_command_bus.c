#include "wf24_command_bus.h"

#include <string.h>

bool
wf24_command_bus_init(wf24_command_bus_t *bus,
                      const wf24_command_bus_ops_t *ops)
{
    if (!bus || !ops || !ops->stop_music || !ops->play_music ||
        !ops->special_fade_1f || !ops->play_sfx)
        return false;

    memset(bus, 0, sizeof(*bus));
    bus->ops = *ops;
    bus->last_route = WF24_ROUTE_INVALID;
    bus->last_result = WF24_DISPATCH_INVALID;
    return true;
}

wf24_dispatch_result_t
wf24_command_dispatch(wf24_command_bus_t *bus, uint8_t command)
{
    bool ok = false;
    wf24_route_t route;
    wf24_dispatch_result_t result;

    if (!bus)
        return WF24_DISPATCH_BACKEND_ERROR;

    route = wf24_route_command(command);
    switch (route) {
    case WF24_ROUTE_STOP:
        ok = bus->ops.stop_music(bus->ops.user);
        result = ok ? WF24_DISPATCH_OK : WF24_DISPATCH_BACKEND_ERROR;
        break;
    case WF24_ROUTE_MUSIC:
        ok = bus->ops.play_music(command, bus->ops.user);
        result = ok ? WF24_DISPATCH_OK : WF24_DISPATCH_BACKEND_ERROR;
        break;
    case WF24_ROUTE_SPECIAL_FADE:
        ok = bus->ops.special_fade_1f(bus->ops.user);
        result = ok ? WF24_DISPATCH_OK : WF24_DISPATCH_BACKEND_ERROR;
        break;
    case WF24_ROUTE_SFX:
        ok = bus->ops.play_sfx(command, bus->ops.user);
        result = ok ? WF24_DISPATCH_OK : WF24_DISPATCH_BACKEND_ERROR;
        break;
    case WF24_ROUTE_REGISTERED_NO_PRODUCER:
        result = WF24_DISPATCH_NO_PRODUCER;
        ++bus->rejected_count;
        break;
    default:
        result = WF24_DISPATCH_INVALID;
        ++bus->rejected_count;
        break;
    }

    bus->last_command = command;
    bus->last_route = route;
    bus->last_result = result;
    ++bus->dispatch_count;
    return result;
}
