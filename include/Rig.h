#ifndef RIG_H
#define RIG_H

#include <Arduino.h>

#include "machine/Machine.h"

// Hardware shared by the machine and bring-up builds: base controller start-up,
// module verification, the assembled station line, and the operator inputs.
namespace rig {

bool begin();  // True when the base matches Config.h
Machine& machine();

void pollSerial();               // Dispatches a line to the registered SerialBooleans
bool readLine(String& line);     // True once a full line is available, trimmed

bool runSwitchOn();
bool startEdge();  // True once per press

}  // namespace rig

#endif
