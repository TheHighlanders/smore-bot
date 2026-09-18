#ifndef HMILINK_H
#define HMILINK_H

#include <Arduino.h>

// Modbus RTU link to the EA3-T4CL touchscreen on Serial1 (see Config.h for
// the register map and wiring notes). The display is the Modbus master and
// polls this board as the slave, so begin() only sets up the register map;
// poll() answers those polls.
namespace hmi {

bool begin();  // True once the port and register map are up.
void poll();   // Call every loop() iteration.

// Mirrors a formatted line to the display's status text tag, truncated to
// its width. Separate from logLine() so debug-console-only chatter (wiring
// checks, per-station status) doesn't clutter the operator-facing screen.
__attribute__((format(printf, 1, 2)))
void showLine(const char* fmt, ...);

// True once per press, mirroring a momentary coil write from the screen the
// same way SerialBoolean's EPHEMERAL commands debounce a held key.
bool startEdge();
bool cancelCookEdge();

}  // namespace hmi

#endif
