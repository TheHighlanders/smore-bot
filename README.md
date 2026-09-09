# Smore Bot

Controls a smore making robot using a P1AM 200 Arduino based PLC.

This branch runs **without tray sensors**. Station progress is dead reckoned
from belt travel time instead of being confirmed by hardware.

## Commands

**Build:** `pio run -e p1am_200`  
**Flash:** `pio run -e p1am_200 -t upload`  
**Start 115200 Baud Serial Monitor:** `pio device monitor`  

## Setup

- Install preferred IDE and PlatformIO extension (VSCode or CLion should be easy)
- Clone this repo
- Open top level folder, and confirm PlatformIO starts
- Run `pio run -e p1am_200` to build code

## How it works

`Machine` owns an ordered line of stations plus the belt. It polls stations; a
station never calls back into the machine, so a finished station is advanced at
most once per tick.

Each `Station` runs one timing-driven phase machine:

```
Idle --activate--> Arriving --transitMs--> Working --workMs--> Done
Done --deactivate--> Clearing --clearMs--> Idle
```

Subclasses only supply hardware actions (`onActivate`, `onArrive`, `onComplete`,
`onRelease`, `onEStop`). All timing lives in the base class.

Times are measured against the **belt clock**, which advances only while the
machine is running. Holding the machine stops the belt and stops the clock
together, so a tray in transit stays where the machine thinks it is.

`clearMs` is the only interlock protecting a station from the tray behind it.
With no sensors there is nothing else, so it must be generous.

## Calibrating

All hardware addresses and times are in `include/Config.h`.

1. Flip the run switch and confirm the belt starts.
2. Time a tray from one station's release to the next station's stop. That is
   `transitMs` for the downstream station.
3. Time from release until the tray is fully past a station. That is `clearMs`.
4. Set `workMs` to the dispense or cook duration.

Type `status` into the serial monitor at any time for the phase of every station
and the current belt clock.

## Serial commands

| Command | Effect |
| --- | --- |
| `start` | Start a cycle, same as the start button |
| `estop` | Latch an emergency stop |
| `status` | Print machine and station state |
| `OVENtemp` | Toggle the oven's thermistor out of the loop |

## Running without a module

Set a `channelLabel` to `config::kAbsent` (slot 0) to mark hardware as not
fitted. Inputs read false and outputs are skipped. Remove the module from
`config::kModules` as well, or the boot check will refuse to start.

Setting `kOven.thermistor` to `kAbsent` runs the oven open-loop; it will report
ready immediately rather than waiting to reach setpoint.

## Safety

The base controller watchdog is configured in `HOLD` mode. If the sketch stops
petting it, every module output de-energizes and the CPU halts until a power
cycle.

## Useful Reference

[Documentation](https://facts-engineering.github.io/)  
[Library](https://github.com/facts-engineering/P1AM)  
