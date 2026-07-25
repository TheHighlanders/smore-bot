#include <P1AMLogged.h>

void P1AMLogged::writeDiscrete(uint32_t data, uint8_t slot, uint8_t channel){
    P1AM::writeDiscrete(data, slot, channel);
    
    if(slot < 1 || slot > NUMBER_OF_MODULES){
        return;
    }

    // More guards can be implemented here to match P1AM.cpp, except for scoping issues with Module_List, and not wanting to modify the base library

    //channel = 0 is a sentinel for write to all channels
    if(channel == 0){
        discreteWritten[slot-1] = data & ((1 << MAX_DO) - 1); //Log only the MAX_DO lowest bits
    } else {
        // Targeted write to a specific channel (1-15)
        if(data & 0x1){
            // Set Bit
            discreteWritten[slot-1] |= (0x1 << (channel - 1));
        } else {
            // Clear Bit
            discreteWritten[slot-1] &= ~(0x1 << (channel - 1));
        }
    }
}
void P1AMLogged::writeDiscrete(uint32_t data, channelLabel label){
    writeDiscrete(data, label.slot, label.channel);
}

void P1AMLogged::writeAnalog(uint32_t data, uint8_t slot, uint8_t channel){
    P1AM::writeAnalog(data, slot, channel);
    analogWritten[slot - 1][channel - 1] = data & (0xFFFF); //Log only the lowest 2 bytes of data (max AO resolution is no more than 16bit)
}

void P1AMLogged::writeAnalog(uint32_t data, channelLabel label){
    writeAnalog(data, label.slot, label.channel);
}

// Reads the logged state value of a single channel
bool P1AMLogged::loggedDiscrete(uint8_t slot, uint8_t channel){
    return discreteWritten[slot-1] & (0x1 << (channel - 1));
}

// Reads the logged state of an entire module
uint16_t P1AMLogged::loggedDiscrete(uint8_t slot){
    return discreteWritten[slot-1];
}

uint16_t P1AMLogged::loggedAnalog(uint8_t slot, uint8_t channel){
    return analogWritten[slot-1][channel-1];
}