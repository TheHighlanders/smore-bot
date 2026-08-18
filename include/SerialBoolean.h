#ifndef SERIALBOOLEAN_H
#define SERIALBOOLEAN_H

#define EPHEMERAL true
#define PERSISTENT false

#include <map>

// Class for managing received serial commands.
// This will update an internal array of booleans, in two forms, either persistent, or ephemeral, with the difference being clear on read.
// Default of both states are supported
class SerialBoolean{
    public:
        SerialBoolean(std::string key, bool ephemeral = false, bool defaultValue = false);
        
        ~SerialBoolean();

        static void parseInput(const char* input, size_t length);

        bool read();

        SerialBoolean(const SerialBoolean&) = delete;
        SerialBoolean& operator=(const SerialBoolean&) = delete;
        SerialBoolean(SerialBoolean&&) = delete;
        SerialBoolean& operator=(SerialBoolean&&) = delete;

    private:
        const std::string key;
        const bool ephemeral;
        const bool defaultValue;

        bool currentValue;

        // Sets the command to be distinct from its default
        void set();

        static std::map<std::string, SerialBoolean*> registeredCommands;
};

#endif