# Smore Bot

Controls a smore making robot using a P1AM 200 Arduino based PLC.

This branch runs **without tray sensors**. Station progress is dead reckoned
from belt travel time instead of being confirmed by hardware.

## Commands

**Build:** `pio run -e p1am_200`  
**Flash:** `pio run -e p1am_200 -t upload`  
**Start 115200 Baud Serial Monitor:** `pio device monitor`  

## Setup

Open the top level folder in any IDE with the PlatformIO extension (VSCode or
CLion), then build with the command above.

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

All hardware addresses and times are in `include/Config.h`; station order is in
`src/main.cpp`, where the stations are built.

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
| `skip` | Force the furthest-along station to finish, for testing without waiting |
| `OVENtemp` | Toggle the oven's thermistor out of the loop |

## Running without a module

Set a `channelLabel` to `kAbsent` (slot 0, from `include/Channel.h`) to mark
hardware as not fitted. Inputs read false and outputs are dropped. Remove the
module from `config::kModules` as well, or the boot check will refuse to start.

Setting `kOven.thermistor` to `kAbsent` runs the oven open-loop; it will report
ready immediately rather than waiting to reach setpoint.

## Safety

The base controller watchdog runs in `HOLD` mode: if it stops being petted,
every module output de-energizes and the CPU halts until a power cycle.

An e-stop deliberately stops all traffic to the base, so the watchdog expires
and the base de-energizes everything itself. Clearing an e-stop therefore means
a power cycle, and the heater does not depend on a single write landing.

The oven rejects readings outside 32-500 F. A burnt-out probe reads NaN and a
failed SPI read returns 0.0, both of which would otherwise look like a cold
oven and latch the heater on.

## Useful Reference

[Documentation](https://facts-engineering.github.io/)  
[Library](https://github.com/facts-engineering/P1AM)  
