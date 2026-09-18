#ifndef RIG_H
#define RIG_H

#include <Arduino.h>

#include "machine/Machine.h"
#include "stations/Oven.h"

// Hardware for the machine build: base controller start-up, module
// verification, the assembled station line, and the operator inputs.
namespace rig {

bool begin();  // True when the base matches Config.h
Machine& machine();
Oven& oven();  // Valid once begin() returns true

bool readLine(String& line);     // True once a full line is available, trimmed

bool runSwitchOn();
bool startEdge();   // True once per press
bool cancelEdge();  // True once per press

}  // namespace rig

#endif
