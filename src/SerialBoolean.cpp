#include "SerialBoolean.h"

std::map<std::string, SerialBoolean*>& SerialBoolean::registry(){
    static std::map<std::string, SerialBoolean*> commands;
    return commands;
}

SerialBoolean::SerialBoolean(std::string key, bool ephemeral, bool defaultValue) : key(key), ephemeral(ephemeral), defaultValue(defaultValue){
    // Add to map
    registry().emplace(key, this);
    currentValue = defaultValue;
}

SerialBoolean::~SerialBoolean(){
    auto it = registry().find(key);
    if(it != registry().end() && it->second == this){
        registry().erase(it);
    }
}

bool SerialBoolean::parseInput(const char* input, size_t length){
    std::string inputStr(input, length);

    // Trim whitespace off the end
    while(!inputStr.empty() && (inputStr.back() == '\r' || inputStr.back() == '\n' || inputStr.back() == ' ')){
        inputStr.pop_back();
    }

    if(inputStr.empty()){
        return true;
    }

    auto value = registry().find(inputStr);
    if(value == registry().end()){
        return false;
    }
    value->second->set();
    return true;
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