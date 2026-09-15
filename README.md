# Smore Bot

Controls a smore making robot using a P1AM 200 Arduino based PLC. Station
progress is timed: each station advances on elapsed time from configured
travel and actuator durations.

## Commands

**Build:** `pio run -e p1am_200`  
**Flash:** `pio run -e p1am_200 -t upload`  
**Start 115200 Baud Serial Monitor:** `pio device monitor`  
**Bring-up build:** `pio run -e p1am_200_bringup -t upload`  

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

Flash `p1am_200_bringup` to check wiring before running the machine:

```
pio run -e p1am_200_bringup -t upload
```

It verifies the module layout and starts **read only**. Every 5 seconds it
prints one line of input state:

```
   35s  start:ON  mmExit:off  run:off  oven:72F
```

`oven` reads `disabled` while `config::kOvenEnabled` is false. The onboard LED
lights while armed.

| Command | Effect |
| --- | --- |
| `arm` | Toggle the actuator arm. Starts disarmed |
| a station's name, e.g. `GC1` | Run that station's `selfTest()` while armed |

Station names are `GC1`, `CH`, `MM`, `OVEN`, `GC2` and `BELT`, in any case.
`GcPusher` steps through its configured move table, so the moves happen in the
order the rig expects.

Each station tests its hardware through the same config the machine build uses.
Reflash `p1am_200` to run the machine.

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

## Useful Reference

[Documentation](https://facts-engineering.github.io/)  
[Library](https://github.com/facts-engineering/P1AM)  
