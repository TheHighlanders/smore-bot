#ifndef CHANNEL_H
#define CHANNEL_H

#include <P1AM.h>

// Slot 0 marks hardware that is not fitted: reads return false and writes are
// dropped, so one firmware runs on bases with different module sets.
const channelLabel kAbsent = {0, 0};

inline bool fitted(channelLabel label) { return label.slot != kAbsent.slot; }

inline bool readChannel(P1AM& p1, channelLabel label) {
    return fitted(label) && p1.readDiscrete(label);
}

inline void writeChannel(P1AM& p1, uint32_t value, channelLabel label) {
    if (fitted(label)) {
        p1.writeDiscrete(value, label);
    }
}

#endif
