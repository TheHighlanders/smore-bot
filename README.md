# Smore Bot

Controls a smore making robot using a P1AM 200 Arduino based PLC.

This branch runs **without tray sensors**. Station progress is dead reckoned
from belt travel time instead of being confirmed by hardware.

## Commands

**Build:** `pio run -e p1am_200`  
**Flash:** `pio run -e p1am_200 -t upload`  
**Start 115200 Baud Serial Monitor:** `pio device monitor`  
**Bring-up build:** `pio run -e p1am_200_bringup -t upload`  
**Unit tests (host, no hardware):** `pio test -e native`  

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
`onWork` returns a bool: true finishes the work immediately (MM's exit sensor
does this), false lets `workMs` run out as a timer, which is the plain
dead-reckoned case every other station uses.

The line is `GC1 -> CHOC -> MM -> OVEN -> GC2`:

| Station | Type | Hardware |
| --- | --- | --- |
| GC1 | `LinearDispenser` | One linear actuator: extend, dwell, retract |
| CHOC | `LinearDispenser` | One linear actuator: extend, dwell, retract |
| MM | `MotorDispenser` | Motor relay + a light sensor: runs until the sensor confirms the marshmallow has exited |
| OVEN | `Oven` | Heater relay, tray hold solenoid, optional thermistor. Disabled via `config::kOvenEnabled` |
| GC2 | `GcPusher` | Linear actuator + gripper + lift: push, lower, grab, raise, release |
| BELT | `Belt` | One conveyor motor output, runs continuously |

Each station carries its own `transitMs` and `clearMs` in its own `Config`, so
timings are tuned one station at a time. Work duration is never configured
directly: every station sums its own actuator sequence, so a station's phase
duration cannot drift out of step with the moves it actually performs.

`src/Rig.cpp` holds everything both builds share: base start-up, module
verification, the station line, and the operator inputs. `src/main.cpp` and
`src/BringUp.cpp` are the two entry points, and `build_src_filter` picks one per
environment, so neither build carries the other's code.

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
   35s  start:ON  mmExit:off  run:off  oven:72F
```

`oven` reads `disabled` while `config::kOvenEnabled` is false; the temperature
is not read at all in that case. Buttons and every other input are only ever
reported here, never acted on - typing a station name is the only thing that
moves hardware.

The onboard LED is off while read only.

| Command | Effect |
| --- | --- |
| `actuators` | Toggle the actuator arm. Off at power-up; the LED lights when armed |
| a station's name, e.g. `GC1` | Run that station's own `selfTest()`, armed only |

Type a station's exact name (`GC1`, `CHOC`, `MM`, `OVEN`, `GC2`, `BELT`) to run
just that one station's sequence - nothing else moves. Typing one while read
only prints a reminder instead. `GcPusher` steps through its real
push/lower/grab/raise/release order rather than firing solenoids individually,
so the moves happen in an order the rig can survive.

A bring-up build never starts the machine and never starts the watchdog. Each
station tests its own hardware using the same config the real code uses, so
there is no second wiring table to drift out of sync. Reflash `p1am_200` to run
the machine.

## Calibrating

All hardware addresses and times are in `include/Config.h`; station order is in
`src/Rig.cpp`, where the stations are built.

1. Flip the run switch and confirm the belt starts.
2. Time a tray from one station's release to the next station's stop. That is
   `transitMs` for the downstream station.
3. Time from release until the tray is fully past a station. That is `clearMs`.
4. Set each station's own move times; its work duration is their sum.

Type `status` into the serial monitor at any time for the phase of every station
and the current belt clock.

## Operator inputs

Start is a discrete input, wired to the `P1-16ND3` at `config::kStartButton`,
edge triggered so holding it repeats nothing. Run/hold is the faceplate
switch.

The e-stop is hardware: it cuts power directly, with no PLC channel and no
software in the loop. The only e-stop-shaped thing software still does is
`Machine::eStop()` on a base controller fault (lost communication), which
safes every station and latches until a power cycle - see Safety below.

There is no way to abort a cycle in progress otherwise: anything short of a
fault or the physical e-stop lets the sequence finish.

## Serial commands

| Command | Effect |
| --- | --- |
| `status` | Print machine and station state |
| `OVENtemp` | Toggle the oven's thermistor out of the loop |

## Safety

The base controller watchdog runs in `HOLD` mode: if it stops being petted,
every module output de-energizes and the CPU halts until a power cycle.

A base controller fault deliberately stops all further traffic to the base, so
the watchdog expires and the base de-energizes everything itself. Clearing it
therefore means a power cycle, and the heater does not depend on a single
write landing.

Set `config::kOvenEnabled` to `false` in `include/Config.h` to take the oven
out of testing entirely. It is `false` as shipped. Disabled, the oven never
writes the heater or hold solenoid and never reads the thermistor, and always
reports ready so the rest of the line runs without stalling behind it. The
`OVENtemp` command has no effect while disabled.

The oven rejects readings outside 32-500 F. A burnt-out probe reads NaN and a
failed SPI read returns 0.0, both of which would otherwise look like a cold
oven and latch the heater on. A missing probe therefore stalls the line at MM
rather than cooking blind; the `OVENtemp` command forces the oven to report
ready if you need to run without one.

## Tests

`pio test -e native` runs the machine and station logic on the host. No board
needed: `test/stubs/Arduino.h` supplies `millis()` and the two `Serial` calls
the logger makes, and `FakeStation` stands in for real hardware, failing the
test if the machine ever puts two trays in one station.

The tests cover the properties that are hard to check by eye on a machine with
no sensors:

- One tray visits every station exactly once, in order
- Five trays with the start button held never collide, and the line drains
- A blocked station reports completion exactly once, however long it waits.
  This is the regression guard for the deadlock the rewrite exists to prevent
- A station that is not ready (a cold oven) blocks the line, and the line
  resumes when it comes ready
- Start is ignored rather than queued while the entry station is busy, and
  activation is refused while busy, clearing, or not ready
- Lifecycle hooks fire in order, and `onWork` runs through the work phase
  without overrunning it
- The belt clock freezes while the machine is held
- Clear time gates the next tray into a station
- E-stop safes every station and cannot be released
- The belt restarts across repeated holds, and never completes on its own
- The belt clock survives the `millis()` rollover at ~49.7 days

## Useful Reference

[Documentation](https://facts-engineering.github.io/)  
[Library](https://github.com/facts-engineering/P1AM)  
