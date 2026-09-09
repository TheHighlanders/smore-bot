#ifndef RIG_H
#define RIG_H

#include "machine/Machine.h"

// Hardware shared by the machine and bring-up builds: base controller start-up,
// module verification, the assembled station line, and the operator inputs.
namespace rig {

bool begin();  // False if the base does not match Config.h
Machine& machine();

void pollSerial();

bool runSwitchOn();
bool eStopPressed();
bool startEdge();  // True once per press, so a held button repeats nothing

}  // namespace rig

#endif
