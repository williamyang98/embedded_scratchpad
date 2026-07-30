#ifndef __WEBSOCKET_HANDLER_H__
#define __WEBSOCKET_HANDLER_H__

extern "C" {
#include "websocket.h"
}

void websocket_attach_handlers(struct Websocket* websocket);

#endif