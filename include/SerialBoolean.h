#ifndef SERIALBOOLEAN_H
#define SERIALBOOLEAN_H

#define EPHEMERAL true
#define PERSISTENT false

#include <map>
#include <string>

// A named boolean set by typing its key into the serial monitor.
// EPHEMERAL keys clear on read (one-shot); PERSISTENT keys toggle.
class SerialBoolean{
    public:
        SerialBoolean(std::string key, bool ephemeral = false, bool defaultValue = false);
        
        ~SerialBoolean();

        // Returns false if the key is not registered.
        static bool parseInput(const char* input, size_t length);

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

        // Construct-on-first-use: these objects are built in another
        // translation unit, before a namespace-scope map would exist.
        static std::map<std::string, SerialBoolean*>& registry();
};

#endif