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
`onComplete`, `onRelease`). All timing lives in the base class. `onWork`
returns a bool: true finishes the work immediately (MM's exit sensor does
this), false lets `workMs` run out as a timer, which is the plain
dead-reckoned case every other station uses.

The line is `GC1 -> CHOC -> MM -> OVEN -> GC2`:

| Station | Type | Hardware |
| --- | --- | --- |
| GC1 | `LinearDispenser` | One linear actuator: extend, retract |
| CHOC | `LinearDispenser` | One linear actuator: extend, retract |
| MM | `MotorDispenser` | Motor relay + a light sensor: runs until the sensor reads blocked then clears (debounced 0.1s) |
| OVEN | `Oven` | Heater relay + tray hold solenoid, purely timed. Disabled via `config::kOvenEnabled` |
| GC2 | `GcPusher` | Lifter + claw + pusher: a fixed 7-move sequence, see below |
| BELT | `Belt` | One conveyor motor output, runs continuously |

Each station carries its own `transitMs` and `clearMs` in its own `Config`, so
timings are tuned one station at a time. Work duration is never configured
directly: every station sums its own actuator sequence, so a station's phase
duration cannot drift out of step with the moves it actually performs.

`GcPusher` (GC2) is the one exception to "configure the sequence in Config.h":
its moves are a fixed table in `GcPusher.cpp`, each row naming the state of
the lifter, claw, and pusher together with how long to hold it -
```
lifter up,   claw open,   pusher out,  5s
lifter down, claw open,   pusher out,  2s
lifter down, claw closed, pusher out,  2s
lifter up,   claw closed, pusher in,   5s
lifter down, claw closed, pusher in,   2s
lifter down, claw open,   pusher in,   1s
lifter up,   claw open,   pusher in,   2s
```
so the sequence reads as one list end to end rather than being reconstructed
from separate config fields. The tray stop is Config.h's `capture` channel,
unrelated to this table.

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

The linear actuators (GC1, CHOC) drive a relay that is always actively
extending or retracting - there is no third, idle state to hold at full
extension. A dwell at full extension is therefore just more extend time, so
`LinearDispenser::Config` has no separate `dwellMs`; fold any hold time
straight into `extendMs`.

The oven has no setpoint or control loop: its heater relay and tray hold
solenoid are on together for exactly `cookMs`, the same dead-reckoned window
every other station uses for its work duration - no preheating between trays.
Its thermistor is read every tick purely for the status line - nothing ever
gates on it, so a dead or unplugged probe cannot stall the line.

## Bring-up mode

Flash `p1am_200_bringup` to check wiring before running any machine logic:

```
pio run -e p1am_200_bringup -t upload
```

It verifies the module layout, then powers up **read only**: nothing moves.
Every 5 seconds it prints one line of input state. Reading a typed command
never blocks - it only consumes bytes already in the input buffer - so an
in-progress or partial command cannot delay that report. A station's own
sequence still runs one blocking step at a time once triggered, so the report
pauses for the few seconds that takes, same as it would on real hardware.

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

Type a station's name (`GC1`, `CHOC`, `MM`, `OVEN`, `GC2`, `BELT`; case does
not matter) to run just that one station's sequence - nothing else moves.
Typing one while read only prints a reminder instead. `GcPusher` steps through
its real move sequence rather than firing outputs individually, so the moves
happen in an order the rig can survive.

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

The e-stop is entirely hardware: it cuts power directly, with no PLC channel
and nothing for software to read, act on, or safe on its behalf.

There is no way to abort a cycle in progress otherwise: anything short of the
physical e-stop lets the sequence finish.

## Serial commands

| Command | Effect |
| --- | --- |
| `status` | Print machine and station state |

## Safety

The base controller watchdog runs in `HOLD` mode: if it stops being petted,
every module output de-energizes and the CPU halts until a power cycle.

Losing the base controller (no comms, or a module missing) is the one fault
software still watches for: it skips petting the watchdog and reports the
fault every loop, rather than trying to safe anything itself. If the base
recovers before the watchdog's window elapses, the machine simply resumes; if
it does not, HOLD mode de-energizes everything and halts the CPU, same as it
would for a genuinely hung sketch.

Set `config::kOvenEnabled` to `false` in `include/Config.h` to take the oven
out of testing entirely; it is `true` as shipped. Disabled, the oven never
writes the heater or hold solenoid and never reads the thermistor. Enabled or
not, the oven never gates on temperature - see "How it works" above.

## Useful Reference

[Documentation](https://facts-engineering.github.io/)  
[Library](https://github.com/facts-engineering/P1AM)  
