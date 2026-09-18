# LED strip probe

Finds the speed and color order of a 3-wire LED strip. Use the answers to
control the strip.

You need a USB cable, a power supply for the strip, a 330 ohm resistor and
jumper wires.

## 1. Look at the strip

A 3-wire strip has power, ground and data. Look for these:

| On the strip | Meaning |
| --- | --- |
| `5V`, `12V` or `24V` | The voltage to use |
| Cut marks between every LED | Usually a 5V strip |
| Cut marks every 3 LEDs | Usually a 12V strip. Each group lights as one LED. |
| Cut marks every 6 LEDs | Usually a 24V strip. Each group lights as one LED. |
| `5V` or `12V`, `DI` or `DIN`, `GND` | Power, data in, ground |
| Arrows | Data flows the way they point. Use the end where the arrows start. |

Too much voltage breaks a strip. If nothing tells you the voltage, start with 5V.

The strip needs at least 4 LEDs.

## 2. Wire it

Power the strip from its own supply. Put the resistor near the strip.

```
supply (+) ────────────────────────► strip 5V (or 12V)
supply (-) ──────┬─────────────────► strip GND
controller GND ──┘
controller pin 4 ──[ 330 ohm ]─────► strip DIN
```

The controller sends 3.3V, so a 5V strip works best with a 3.3V to 5V level
shifter (74AHCT125) between pin 4 and the resistor.

To use another pin, change `kDataPin` in `tools/led_probe/main.cpp`.

## 3. Run the probe

1. Turn off the robot's 24V power so the robot's outputs stay off.
2. Plug the controller into your computer with a USB cable.
3. Send the probe:

   ```
   pio run -e led_probe -t upload
   ```

4. Open the serial monitor (part 3 of the main README) and follow the steps.

The probe lights the strip and asks what you see. Type the letters and press
Enter. If you close the monitor, press the controller's reset button and open it
again. Run the probe twice to check that the answers match.

When you finish, send the robot code with `pio run -e p1am_200 -t upload`,
then turn the 24V power on.

## 4. Read the results

```
Results
  Speed:        800 kHz
  Color order:  GRB
  Setting:      NEO_GRB + NEO_KHZ800
```

| Speed | Color order | Most likely |
| --- | --- | --- |
| 800 kHz | GRB | WS2812B, WS2812, SK6812 |
| 800 kHz | Any other 3 letters | WS2811 |
| 800 kHz | 4 letters | SK6812 RGBW |
| 400 kHz | Any | Older WS2811, UCS1903. A weak data signal can also cause this. Add a level shifter and run again. |

Count the LEDs and put the number in `kLedCount` in `include/Config.h`.
