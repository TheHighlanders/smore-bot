#ifndef P1AMLOGGED_h
#define P1AMLOGGED_h

#define MAX_DO 16 //Maximum number of DO per module. Accurate as of 2026-07-24
#define MAX_AO 8  //Maximum number of AO per module. Accurate as of 2026-07-24  

#include <P1AM.h>

// Subclass of P1AM to log written states
class P1AMLogged : public P1AM{
    public:
        //Note, these functions are redefined, and not overriden, so all pointers/references should be to the P1AMLogged type

        //Data IO Functions	- For more info see function headers in P1AM.cpp
	    void writeDiscrete(uint32_t data,uint8_t slot, uint8_t channel = 0);		//Write Discrete Module. Passing 0 instead of a channel will write data for all of the channels at once.
	    void writeAnalog(uint32_t data,uint8_t slot, uint8_t channel);				//Write Analog Module. Send up to 32 bits of data. 16/14/12/etc bit modules are masked on Base Controller.

        //Labelled equivalents
	    void writeDiscrete(uint32_t data, channelLabel label);
	    void writeAnalog(uint32_t data, channelLabel label);

        //Getters for logged state
        bool loggedDiscrete(uint8_t slot, uint8_t channel);
        uint16_t loggedDiscrete(uint8_t slot);
        
        uint16_t loggedAnalog(uint8_t slot, uint8_t channel);
    protected:
        //Internal fields that hold the written states
        // While valid slot(module) & channel ids are all 1 indexed, these are 0 indexed
        uint16_t discreteWritten[NUMBER_OF_MODULES]; //16 Outputs maximum per module. Bit n corresponds to channel n + 1, ie bit 0 -> channel 1, and bit 15 -> channel 16 (meaningless)
        uint16_t analogWritten[NUMBER_OF_MODULES][MAX_AO];
};

#endif