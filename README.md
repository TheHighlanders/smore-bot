# Smore Bot

Controls a smore making robot using a P1AM 200 Arduino based PLC.

## Commands

**Build:** `pio run -e p1am_200`  
**Flash:** `pio run -e p1am_200 -t upload`  
**Start 115200 Baud Serial Monitor:** `pio device monitor` 

## Setup

- Install preferred IDE and PlatformIO extension (VSCode or CLion should be easy)
- Clone this repo
- Open top level folder, and confirm PlatformIO starts
- Run `pio run -e p1am_200` to build code

## Useful Reference

[Documentation](https://facts-engineering.github.io/)  
[Library](https://github.com/facts-engineering/P1AM)  
