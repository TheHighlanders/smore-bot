# Smore Bot

Controls a smore making robot using a P1AM 200 Arduino based PLC. Station
progress is timed: each station advances on elapsed time from configured
travel and actuator durations.

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

`Machine` owns an ordered line of stations plus the belt. Each tick it updates
every station, then moves finished trays down the line, downstream first.

Each `Station` runs one timed phase machine:

```
Idle --activate--> Arriving --transitMs--> Working --workMs--> Done
Done --deactivate--> Clearing --clearMs--> Idle
```

Subclasses supply hardware actions (`onActivate`, `onArrive`, `onWork`,
`onComplete`, `onRelease`, `onReset`); timing lives in the base class. `onWork`
returns true to finish work early (MM's exit sensor), or false to let `workMs`
time out.

The line is `GC1 -> CH -> MM -> OVEN -> GC2`:

| Station | Type | Hardware |
| --- | --- | --- |
| GC1 | `LinearDispenser` | One linear actuator: extend, retract |
| CH | `LinearDispenser` | One linear actuator: extend, retract |
| MM | `MotorDispenser` | Motor relay + light sensor: runs until the sensor reads blocked then clear (debounced 0.1s) |
| OVEN | `Oven` | Heater relay + tray hold solenoid, on for `cookMs`. Enabled by `config::kOvenEnabled` |
| GC2 | `GcPusher` | Lifter + claw + pusher, stepped through `config::kGrahamCracker2Moves` |
| BELT | `Belt` | Conveyor motor output, on while the machine runs |

Each station's `Config` carries its own `transitMs` and `clearMs`, so timings
are tuned one station at a time. A station's work duration is the sum of its
own actuator times.

GC2's sequence is a table in `include/Config.h`. Each row sets the lifter, claw
and pusher together and holds them for that row's time.

Several trays can be in the line at once. A station accepts a tray when it is
`Idle`, and trays move downstream first, so each station holds one tray. A
start press begins a cycle when the entry station is free, so holding the
button yields one tray per cycle.

`clearMs` is the interlock that protects a station from the tray behind it, so
set it generously.

`src/Rig.cpp` holds what both builds share: base start-up, module verification,
the station line, and the operator inputs. `src/main.cpp` and `src/BringUp.cpp`
are the two entry points; `build_src_filter` picks one per environment.

## Run switch

The faceplate switch runs the machine. Switching it off resets every station
to `Idle` and de-energizes every output, which returns the stations to their
unpowered rest positions. Clear the belt, then switch back on to start fresh.

## Bring-up mode

The bring-up build checks wiring one station at a time before the machine runs.
It reports inputs on its own and moves an actuator only on command. The machine
and the watchdog stay stopped the whole time.

### 1. Flash and connect

```
pio run -e p1am_200_bringup -t upload
pio device monitor
```

Type each command into the monitor and press Enter.

### 2. Watch it start

On boot it waits for the base controller, then checks the module layout
against `config::kModules`:

```
Smore Bot starting, waiting for base controller
No response? Check the external 24V supply is on.
...
Bring-up, read only. Type 'arm', then a station name.
```

A layout mismatch prints each wrong slot. Fix the base to match
`include/Config.h` and power cycle.

### 3. Check inputs (read only)

Every 5 seconds it prints one line:

```
   35s  start:ON  mmExit:off  run:off  oven:72F
```

| Field | Input |
| --- | --- |
| `start` | Start button, `config::kStartButton` |
| `mmExit` | MM exit light sensor |
| `run` | Faceplate run switch |
| `oven` | Thermistor in degF, or `disabled` while `config::kOvenEnabled` is false |

Press the button, break the MM beam, and flip the run switch, and watch each
field change in the next report. Inputs are reported only; they start nothing.

### 4. Arm and test a station

```
arm
GC1
```

`arm` toggles the actuators on and prints `Actuators ARMED`; the onboard LED
lights while armed. Type `arm` again to return to read only. A station name
typed while read only prints a reminder.

Type a station's name to run that station's self test. Names match in any case:

| Name | What moves |
| --- | --- |
| `GC1`, `CH` | Tray stop for 0.75s, then the linear actuator extends for `extendMs` and retracts for `retractMs` |
| `MM` | Tray stop for 0.75s, then the motor runs until the exit sensor reads blocked then clear, or `timeoutMs` |
| `OVEN` | Tray hold solenoid for 0.75s, heater relay for 0.75s, then prints the temperature. Skipped while disabled |
| `GC2` | Tray stop for 0.75s, then every row of `config::kGrahamCracker2Moves` in order |
| `BELT` | Conveyor output for 0.75s |

Each step prints as it runs. A self test holds the serial loop until it
finishes, so the 5-second report pauses for its duration (about 23s for GC2).

Test stations one at a time, starting with the simplest: `BELT`, then `GC1`,
`CH`, `MM`, `OVEN`, `GC2`. Stay clear of the rig while armed.

### 5. Return to the machine build

Self tests use the same `include/Config.h` as the machine build, so a channel
that works here works there. When every station checks out:

```
pio run -e p1am_200 -t upload
```

## Calibrating

Hardware addresses and times are in `include/Config.h`; station order is in
`src/Rig.cpp`.

1. Flip the run switch and confirm the belt starts.
2. Time a tray from one station's release to the next station's stop. That is
   `transitMs` for the downstream station.
3. Time from release until the tray is fully past a station. That is `clearMs`.
4. Set each station's actuator times; its work duration is their sum.

Type `status` into the serial monitor for the phase of every station.

## Operator inputs

| Input | Where |
| --- | --- |
| Start | Discrete input `config::kStartButton`, edge triggered |
| Run | Faceplate switch; off resets the line |
| E-stop | Hardware power cut |

## Serial commands

| Command | Effect |
| --- | --- |
| `status` | Print machine and station state |
| `start` | Start a cycle, same as the button |

## Safety

The base controller watchdog runs in `HOLD` mode: when it goes unpetted, every
module output de-energizes and the CPU halts until a power cycle. Losing the
base controller skips the pet, so the watchdog takes over.

## Tests

`pio test -e native` runs the machine and station logic on the host, with no
board. `test/stubs/Arduino.h` supplies `millis()` and the `Serial` calls the
logger makes, and `FakeStation` stands in for hardware, flagging any moment two
trays share a station.

The tests cover:

- One tray visits every station exactly once, in order
- Five trays with the start button held pass through without collisions, and
  the line drains
- A blocked station reports completion once and keeps its tray
- A station that is not ready (a cold oven) blocks the line, which resumes once
  it is ready
- A start press begins a cycle only when the entry station is free; activation
  waits through busy, clearing, and not-ready states
- Lifecycle hooks fire in order, and `onWork` runs for the whole work phase
- Early completion from a sensor ends work on the next tick
- Stopping resets every station, and the line starts fresh when switched back on
- Clear time gates the next tray into a station
- The belt restarts across repeated stops and runs until reset
- Bring-up's `selfTestNamed` runs exactly the named station, in any case
- The line keeps running across the `millis()` rollover at ~49.7 days

## Useful Reference

[Documentation](https://facts-engineering.github.io/)  
[Library](https://github.com/facts-engineering/P1AM)  
