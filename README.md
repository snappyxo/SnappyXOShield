# SnappyXOShield

An Arduino library for driving the two motors on the SnappyXO Motor Shield. It knows which pins the shield's motor driver uses, so a sketch only says where to go.

## Installation

In the Arduino IDE, open **Sketch → Include Library → Manage Libraries…**, search for **SnappyXOShield**, and click Install.

## Example

```cpp
#include <SnappyXOShield.h>

SnappyXOShield shield(SnappyXOShield::V3);  // the shield version printed on the board: V3 or V2

void setup() {
  shield.begin();
}

void loop() {
  shield.forward();
  delay(1000);

  shield.spinLeft(150);
  delay(500);

  shield.stop();
  delay(1000);
}
```

See [`examples/BasicMotorControl/`](examples/BasicMotorControl/BasicMotorControl.ino) for the full sketch, and [`examples/MicroBlueDrive/`](examples/MicroBlueDrive/MicroBlueDrive.ino) to drive from the MicroBlue app on an UNO R4 WiFi.

## Shields

|                             | v3                          | v2                            |
|-----------------------------|-----------------------------|-------------------------------|
| Board                       | UNO R4 WiFi, or UNO R3      | UNO R3 with an HM-10 module   |
| In the sketch               | `SnappyXOShield shield(SnappyXOShield::V3);` | `SnappyXOShield shield(SnappyXOShield::V2);` |
| Left motor IN1 / IN2 / PWM  | 4 / 7 / 5                   | 4 / 5 / 6                     |
| Right motor IN1 / IN2 / PWM | 8 / 9 / 6                   | 12 / 13 / 11                  |
| Servo headers               | A0, A1, A2, A3              | 9, 10, A0, A1                 |

The library sets up the motor pins only, and a sketch shouldn't use them for anything else. The servo headers are listed for reference: use the pin number printed on the board, for example `myServo.attach(A0);`.

On the UNO R3, the IRremote library takes over the timer that makes the PWM on pin 11, the v2 shield's right motor speed pin, so an IR receiver and the v2 shield's motors don't work together. The v3 shield's speed pins (5 and 6) aren't affected.

## API

Speeds run from **-255** (full backward) to **255** (full forward), and 0 stops. Values outside that range are limited to it.

| Call                                | What it does                                              |
|-------------------------------------|-----------------------------------------------------------|
| `SnappyXOShield shield(SnappyXOShield::V3);` | Creates the shield, above `setup()`. The version printed on the board, `SnappyXOShield::V3` or `SnappyXOShield::V2`, is required |
| `shield.begin()`                    | Sets up the motor pins. Call once in `setup()`            |
| `shield.forward()`                  | Both wheels forward at full speed                         |
| `shield.forward(speed)`             | Both wheels forward at `speed` (0–255)                    |
| `shield.reverse()`, `reverse(speed)`| Both wheels backward                                      |
| `shield.spinLeft()`, `spinLeft(speed)`  | Spins left in place: left wheel backward, right wheel forward |
| `shield.spinRight()`, `spinRight(speed)`| Spins right in place                                  |
| `shield.setMotors(left, right)`     | Sets each wheel's speed (-255 to 255)                     |
| `shield.drive(throttle, steering)`  | Joystick-style driving, see below                         |
| `shield.stop()`                     | Coasting stop: cuts the power, the robot rolls to a halt  |
| `shield.brake()`                    | Hard stop: locks both motors                              |

### `drive(throttle, steering)`

Drives a two-wheeled car like a joystick. Both inputs run from -255 to 255:

- `throttle`: forward (positive) or backward (negative)
- `steering`: right (positive) or left (negative)

`drive()` mixes the two into wheel speeds: left = throttle + steering, right = throttle − steering. When a wheel would go past full speed, both wheels slow down by the same factor, so the turn keeps its shape. With no throttle, steering spins the car in place.

The MicroBlue app's D-Pad, Joystick and Tilt send `x,y` with each part from 0 to 1023, so map them first:

```cpp
shield.drive(map(y, 0, 1023, -255, 255), map(x, 0, 1023, -255, 255));
```

### Motors that turn the wrong way

Call these in `setup()`, after `begin()`:

| Call                   | Use it when                                                  |
|------------------------|--------------------------------------------------------------|
| `shield.invertLeft()`  | The motor on the shield's left output turns the wrong way    |
| `shield.invertRight()` | The motor on the shield's right output turns the wrong way   |
| `shield.swapMotors()`  | The left and right motors are plugged in the other way round |

## Upgrading from 0.x

Version 1.0.0 puts everything in the `SnappyXOShield` class, so sketches written for 0.x need these changes:

| 0.x                                   | 1.0                                                          |
|---------------------------------------|--------------------------------------------------------------|
| `initMotors();`                       | `SnappyXOShield shield(SnappyXOShield::V3);` above `setup()`, then `shield.begin();` |
| `initMotors(true, false, false);`     | `shield.begin();` then `shield.invertLeft();`                |
| `initMotors(false, true, false);`     | `shield.begin();` then `shield.invertRight();`               |
| `initMotors(false, false, true);`     | `shield.begin();` then `shield.swapMotors();`                |
| `#define SNAPPYXO_SHIELDV2`           | `SnappyXOShield shield(SnappyXOShield::V2);`                 |
| `forward()`, `reverse()`, `brake()`   | `shield.forward()`, `shield.reverse()`, `shield.brake()`     |
| `turnLeft()`, `turnRight()`           | `shield.spinLeft()`, `shield.spinRight()`                    |
| `pause()`                             | `shield.stop()`                                              |
| `_motorWrite(left, right)`            | `shield.setMotors(left, right)`                              |
| `drive(throttle, steering)`, each -512 to 512 | `shield.drive(throttle, steering)`, each -255 to 255. From the MicroBlue app, `map(value, 0, 1023, -255, 255)` |

## Compatibility

- **SnappyXO Motor Shield:** v3 and v2
- **Boards:** any Arduino, since the library only uses `pinMode()`, `digitalWrite()` and `analogWrite()`. The examples are compiled for the UNO R3 and UNO R4 WiFi on every change.
