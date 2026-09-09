# Smore Bot

Controls a smore making robot using a P1AM 200 Arduino based PLC.

This branch runs **without tray sensors**. Station progress is dead reckoned
from belt travel time instead of being confirmed by hardware.

## Commands

**Build:** `pio run -e p1am_200`  
**Flash:** `pio run -e p1am_200 -t upload`  
**Start 115200 Baud Serial Monitor:** `pio device monitor`  
**Bring-up build:** `pio run -e p1am_200_bringup -t upload`  

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

Subclasses only supply hardware actions (`onActivate`, `onArrive`, `onWork`,
`onComplete`, `onRelease`, `onEStop`). All timing lives in the base class.

The line is `GC1 -> CHOC -> MM -> OVEN -> GC2`:

| Station | Type | Hardware |
| --- | --- | --- |
| GC1 | `LinearDispenser` | One linear actuator: extend, dwell, retract |
| CHOC | `LinearDispenser` | One linear actuator: extend, dwell, retract |
| MM | `MotorDispenser` | One motor on a discrete output: run, settle (hardware TBD) |
| OVEN | `Oven` | Heater relay, tray hold solenoid, optional thermistor |
| GC2 | `GcPusher` | Three pneumatic solenoids: grab, lift, translate, lower, release, return |
| BELT | `Belt` | One relay, runs continuously |

Each station carries its own `transitMs` and `clearMs` in its own `Config`, so
timings are tuned one station at a time. Work duration is never configured
directly: every station sums its own actuator sequence, so a station's phase
duration cannot drift out of step with the moves it actually performs.

Several trays can be in the line at once. A station only accepts a tray when it
is `Idle`, and the machine advances downstream-first, so trays cannot collide.
Pressing start while the entry station is occupied is ignored rather than
queued: holding the button down still produces one tray per cycle.

Times are measured against the **belt clock**, which advances only while the
machine is running. Holding the machine stops the belt and stops the clock
together, so a tray in transit stays where the machine thinks it is.

`clearMs` is the only interlock protecting a station from the tray behind it.
With no sensors there is nothing else, so it must be generous.

## Bring-up mode

Flash `p1am_200_bringup` to check wiring before running any machine logic:

```
pio run -e p1am_200_bringup -t upload
```

It verifies the module layout, then powers up **read only**: nothing moves.
Every 5 seconds it prints one line of input state:

```
   35s  estop:off  start:ON  run:off  oven:72F
```

The LED is blue while read only.

To move hardware, arm the actuators first:

| Command | Effect |
| --- | --- |
| `actuators` | Toggle the actuator arm. Off at power-up; the LED turns magenta when armed |
| `test` | Pulse every actuator once, in sequence. Refused while read only |

`test` drives each station in turn, one actuator at a time. `GcPusher` steps
through its real pick-and-place order rather than firing solenoids
individually, so the moves happen in an order the rig can survive.

A bring-up build never starts the machine and never starts the watchdog. Each
station tests its own hardware using the same config the real code uses, so
there is no second wiring table to drift out of sync. Reflash `p1am_200` to run
the machine.

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

## Safety

The base controller watchdog runs in `HOLD` mode: if it stops being petted,
every module output de-energizes and the CPU halts until a power cycle.

An e-stop deliberately stops all traffic to the base, so the watchdog expires
and the base de-energizes everything itself. Clearing an e-stop therefore means
a power cycle, and the heater does not depend on a single write landing.

The oven rejects readings outside 32-500 F. A burnt-out probe reads NaN and a
failed SPI read returns 0.0, both of which would otherwise look like a cold
oven and latch the heater on. A missing probe therefore stalls the line at MM
rather than cooking blind; the `OVENtemp` command forces the oven to report
ready if you need to run without one.

## Useful Reference

[Documentation](https://facts-engineering.github.io/)  
[Library](https://github.com/facts-engineering/P1AM)  
