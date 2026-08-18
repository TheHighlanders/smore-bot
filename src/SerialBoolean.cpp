#include "SerialBoolean.h"

std::map<std::string, SerialBoolean*> SerialBoolean::registeredCommands;

SerialBoolean::SerialBoolean(std::string key, bool ephemeral, bool defaultValue) : key(key), ephemeral(ephemeral), defaultValue(defaultValue){
    // Add to map
    registeredCommands.emplace(key, this);
    currentValue = defaultValue;
}

SerialBoolean::~SerialBoolean(){
    auto it = registeredCommands.find(key);
    if(it != registeredCommands.end() && it->second == this){
        registeredCommands.erase(it);
    }
}

void SerialBoolean::parseInput(const char* input, size_t length){
    std::string inputStr(input, length);

    // Trim whitespace off the end
    while(!inputStr.empty() && (inputStr.back() == '\r' || inputStr.back() == '\n' || inputStr.back() == ' ')){
        inputStr.pop_back();
    }

    auto value = registeredCommands.find(inputStr);
    if(value != registeredCommands.end()){
        value->second->set();
    }
}

void SerialBoolean::set(){
    if(ephemeral){
        currentValue = !defaultValue;
    } else {
        // Persistent Booleans need to be toggled off
        currentValue = !currentValue;
    }
}

bool SerialBoolean::read(){
    bool out = currentValue;
    if(ephemeral){
        currentValue = defaultValue;
    }
    return out;
}