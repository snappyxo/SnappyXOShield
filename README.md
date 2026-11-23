# SnappyXOShield

An Arduino library for driving the two motors on the SnappyXO Motor Shield. It knows which pins the shield's motor driver uses, so a sketch only says where to go. With [encoder motors](#encoder-motors), it also measures how far each wheel turns, drives a set distance and turns by an angle, with optional PID control tuned for each robot.

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

See [`examples/BasicMotorControl/`](examples/BasicMotorControl/BasicMotorControl.ino) for the full sketch, and [`examples/MicroBlueDrive/`](examples/MicroBlueDrive/MicroBlueDrive.ino) to drive from the MicroBlue app on an UNO R4 WiFi. For encoder motors, see [`EncoderTest`](examples/Encoders/EncoderTest/EncoderTest.ino), [`TraceShape`](examples/Encoders/TraceShape/TraceShape.ino), [`PIDSquare`](examples/Encoders/PIDSquare/PIDSquare.ino) and [`PIDSpeedControl`](examples/Encoders/PIDSpeedControl/PIDSpeedControl.ino).

## Shields

|                             | v3                          | v2                            |
|-----------------------------|-----------------------------|-------------------------------|
| Board                       | UNO R4 WiFi, or UNO R3      | UNO R3 with an HM-10 module   |
| In the sketch               | `SnappyXOShield shield(SnappyXOShield::V3);` | `SnappyXOShield shield(SnappyXOShield::V2);` |
| Left motor (M1) IN1 / IN2 / EN  | 4 / 7 / 5               | 4 / 5 / 6                     |
| Right motor (M2) IN1 / IN2 / EN | 8 / 9 / 6               | 12 / 13 / 11                  |
| Servo headers               | A0, A1, A2, A3              | 9, 10, A0, A1                 |
| [Encoder motors](#encoder-motors) (optional) | Left 2, A2 · right 3, A3 | Left 2, A2 · right 3, A3 |

The shield labels its motor outputs M1 and M2. The library calls them left and right: M1 is the left motor, M2 the right one.

The library sets up the motor pins, and the encoder pins once `beginEncoders()` is called. A sketch shouldn't use those pins for anything else. The servo headers are listed for reference: use the pin number printed on the board, for example `myServo.attach(A0);`. On the v3 shield, encoder motors take the A2 and A3 headers, which leaves A0 and A1 for servos.

On the UNO R3, the IRremote library takes over the timer that makes the PWM on pin 11, the v2 shield's right motor (M2) EN (speed) pin, so an IR receiver and the v2 shield's motors don't work together. The v3 shield's EN pins (5 and 6) aren't affected.

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

To make the car easier to drive, `drive()` squares both inputs first, keeping their signs. Small pushes give fine control and a full push still reaches full speed: half a push (128) becomes a quarter (64).

Then it mixes them into wheel speeds: left = throttle + steering, right = throttle − steering. When a wheel would go past full speed, both wheels slow down by the same factor, so the turn keeps its shape. With no throttle, steering spins the car in place.

| Sideways push, at full throttle | Left, right wheel |
|---------------------------------|-------------------|
| None                            | 255, 255          |
| Half                            | 255, 152 (gentle curve) |
| Full                            | 255, 0 (sharp turn)     |
| Full, with no throttle          | 255, −255 (spins in place) |

To use an input as it is, without squaring, pass `false` for it: `shield.drive(throttle, steering, squareThrottle, squareSteering)`. For example, `shield.drive(throttle, steering, true, false)` squares the throttle only. To set each wheel's speed exactly, use `setMotors()`.

The MicroBlue app's D-Pad, Joystick and Tilt send `x,y` with each part from 0 to 1023, so map them first:

```cpp
shield.drive(map(y, 0, 1023, -255, 255), map(x, 0, 1023, -255, 255));
```

### Motors that turn the wrong way

Call these in `setup()`, after `begin()`:

| Call                   | Use it when                                                  |
|------------------------|--------------------------------------------------------------|
| `shield.invertLeft()`  | The motor on M1, the shield's left output, turns the wrong way |
| `shield.invertRight()` | The motor on M2, the shield's right output, turns the wrong way |
| `shield.swapMotors()`  | The motors are plugged in the other way round: the left wheel's into M2, the right wheel's into M1 |

## Encoder motors

Encoder motors, like the DFRobot TT motor with encoder (6V, 160 RPM, 120:1), tell how far their wheels have turned: 8 pulses for each turn of the motor, so 960 for each turn of the wheel through the 120:1 gearbox. With them, the robot can drive a set distance and turn by an angle. Sketches without encoders don't need to change.

### Wiring

Wire each encoder by the labels on its port. The pins are the same on the v3 and v2 shield:

| Encoder     | Left wheel (M1) | Right wheel (M2) |
|-------------|-----------------|------------------|
| A           | 2          | 3           |
| B           | A2         | A3          |
| 5V (or VCC) | 5V         | 5V          |
| GND         | GND        | GND         |

A2 and A3 are the board's pins of that name, not the encoder's A. The A outputs must go to pins 2 and 3: they're the pins that can interrupt the sketch on every pulse, so none is missed. Upload [`examples/Encoders/EncoderTest/`](examples/Encoders/EncoderTest/EncoderTest.ino) to set up the robot. It turns each wheel and asks in the Serial Monitor which wheel turned and which way, then turns it backward to check its B wire. From your answers and the encoder counts, it works out what's wired wrong and says exactly what to change. At the end it prints the lines your sketches need in `setup()`, such as `shield.invertLeft();`, if any.

If you call `swapMotors()` because the motors are plugged in the other way round, it swaps the encoders too.

### Calls

| Call                                       | What it does                                                 |
|--------------------------------------------|--------------------------------------------------------------|
| `shield.beginEncoders()`                   | Starts counting the encoder pulses. Call once in `setup()`, after `begin()` |
| `shield.leftCount()`, `rightCount()`       | How far each wheel has turned, in pulses (960 per wheel turn with a 120:1 gearbox). Counts up rolling forward, down rolling backward |
| `shield.resetCounts()`                     | Sets both counts back to 0                                   |
| `shield.leftRPM()`, `rightRPM()`           | Each wheel's speed in turns per minute, negative rolling backward |
| `shield.driveInches(inches)`, `driveInches(inches, rpm)` | Drives straight that far, then stops. Negative inches drive backward. `rpm` is the wheels' speed in turns per minute, 60 if left out |
| `shield.driveCm(cm)`, `driveCm(cm, rpm)`   | The same in centimeters                                      |
| `shield.spinTurn(degrees)`, `spinTurn(degrees, rpm)` | Turns by that angle spinning in place, then stops: positive turns right, negative turns left |
| `shield.pivotTurn(degrees)`, `pivotTurn(degrees, rpm)` | Turns by that angle pivoting on one wheel, then stops: positive turns right, negative turns left |
| `shield.setWheelDiameter(mm)`              | The wheels' diameter in millimeters, 61 if not set           |
| `shield.setWheelSpacing(mm)`               | From the middle of one wheel to the middle of the other, in millimeters, 96 if not set. Only the turns use it |
| `shield.setGearRatio(ratio)`               | How many times the motor turns for one turn of the wheel, for example `48` for a 48:1 gearbox. 120 if not set |
| `shield.setRatedRPM(rpm)`                  | The wheels' speed at full power, from the motor's datasheet, 160 if not set. Without the speed controller, the moves guess the power for a speed from it |
| `shield.setSpeedPID(kp, ki, kd)`           | Turns on the speed controllers for the moves: see [Tuning your robot with PID](#tuning-your-robot-with-pid) |
| `shield.setHeadingPID(kp, ki, kd)`         | Turns on the heading controller                              |
| `shield.setPositionPID(kp, ki, kd)`        | Turns on the position controller                             |
| `shield.plotMoves(Serial)`                 | Prints what the controllers see during moves, for the Serial Plotter |

All four moves take the wheels' speed as an optional last number, `rpm`, in turns per minute, 60 if left out: `shield.driveInches(24, 90)`. Its sign is ignored: the distance or angle gives the direction. It's the same unit `leftRPM()` and `rightRPM()` measure. Without the speed controller, the library guesses the motor power for that speed from `setRatedRPM()`, so the wheels may turn slower. With it, they're held at that speed.

### Driving a distance and turning

```cpp
void setup() {
  shield.begin();
  shield.beginEncoders();

  shield.driveInches(24);  // 24 inches forward
  shield.spinTurn(90);     // spin right 90 degrees
  shield.driveCm(-30, 90); // 30 cm backward, faster: 90 RPM
  shield.pivotTurn(-45);   // pivot left 45 degrees
}
```

Each move finishes before the next line runs. On its own, a move guesses the power for the speed you give it, so the wheels may turn slower, and brakes when it reaches the distance. So it rolls a little past, more the faster it goes, and nothing keeps the wheels together, so the robot may curve. [PID control](#tuning-your-robot-with-pid), tuned for your robot, fixes both.

There are two ways to turn:

```
        spinTurn(90)                        pivotTurn(90)

  forward ↑         ↓ backward       forward ↑
         [L]   ●   [R]                      [L]       [●] stays still

   turns around the middle            turns around the right wheel

                    ● the point the robot turns around
```

`spinTurn()` stays on the spot, which helps in tight places. `pivotTurn()` moves only one wheel, so it takes twice as long and swings the robot around the wheel that stays still.

A move returns `false` when it couldn't finish: when `beginEncoders()` wasn't called, when a wheel stopped turning for a second without its power going up (an unplugged encoder, a stuck wheel or too little power), when a wheel turned the wrong way (its encoder's B unplugged, or its A and B swapped), or when the position controller hadn't settled on the target 3 seconds after getting there. The motors are stopped either way.

```cpp
if (!shield.driveInches(24)) {
  // didn't get there
}
```

Once the moves land on their target counts, what's left is the robot's measurements. Tires slip a little, more on some floors than others, so tune them in `setup()`:

- Turns too little: make the wheel spacing bigger, for example `shield.setWheelSpacing(100);`. Turns too far: smaller. Spin and pivot turns slip differently, so if one is right and the other isn't, call `setWheelSpacing()` with each one's value before it.
- Drives too short: make the wheel diameter smaller. Drives too far: bigger.

[`examples/Encoders/TraceShape/`](examples/Encoders/TraceShape/TraceShape.ino) drives a U shape.

### Tuning your robot with PID

Robots differ in weight, battery, motors and floor, so the library doesn't come tuned: each robot gets its own PID gains, set in `setup()`. There are three controllers, all off until a sketch sets them, listed in the order to tune them:

| Controller | Call | The error it measures | What it fixes |
|------------|------|-----------------------|---------------|
| Speed      | `setSpeedPID(kp, ki, kd)` | Target speed minus measured speed, for each wheel, in RPM | Holds the speed asked for instead of guessing the power, whatever the battery or the robot's weight |
| Heading    | `setHeadingPID(kp, ki, kd)` | How many pulses one wheel is ahead of the other | Keeps the wheels together, so the robot drives straight and spins on the spot |
| Position   | `setPositionPID(kp, ki, kd)` | Pulses still to go | Slows down near the end and settles on the target, backing up if it rolls past |

Leave out `ki` and `kd` for P only, for example `shield.setHeadingPID(0.3);`. Setting all three to 0 turns a controller off again, and a call with a negative gain is ignored.

The position and heading controllers ask for each wheel's speed, in RPM. Without the speed controller, the library guesses the power for that speed. With it, a speed controller on each wheel measures the speed and finds the power that holds it.

```
pulses still to go  ──► Position ──┐
                                   ├──► each wheel's target RPM ──► Speed ──► motor power
left wheel ahead by ──► Heading  ──┘
```

The heading controller works with or without the speed controller, but the position controller needs it. Near the end of a move, the position controller asks for speeds so slow that a guessed power can't turn the motors at all, so the robot stops short. The speed controller notices the wheels aren't turning and pushes harder until they do. Turning it on changes how the robot responds, so retune heading after turning it on: it usually needs a smaller gain.

The order matters because the speed controller sits next to the motors and the other two work through it. The controllers work out their values from the outside in, position and heading first, then speed, and you tune them from the inside out. Heading and position only ask for wheel speeds, so how they behave depends on how well the speed controller delivers them. Tune it first, and retune heading and position whenever you change it. As a rule, an outer controller should react more slowly than the one it works through: if heading or position pushes harder than the speed controller can follow, they fight and the robot wobbles. That's why both need smaller gains once the speed controller is on. The order of the `set…PID()` lines in `setup()` doesn't matter, since each one only stores its gains, but this README and the examples list them in tuning order.

Tune one controller at a time, in this order, with [`examples/Encoders/PIDSquare/`](examples/Encoders/PIDSquare/PIDSquare.ino):

1. Run `EncoderTest` first, so the motors and encoders are right.
2. Run `PIDSquare` with every gain at 0, to see what the controllers fix: the wheels turn slower than asked, each move rolls past its target, more the faster it goes, and the robot curves.
3. **Speed**: start with `speedKp` alone, add `speedKi` until each wheel reaches its target speed, then a little `speedKd` if the speed swings around it. Watch it on the Serial Plotter. It's optional for heading alone, but position needs it.
4. **Heading**: raise `headingKp` until the robot drives straight. Add `headingKd` if it wobbles, `headingKi` if a small difference stays.
5. **Position**: raise `positionKp` until each move slows down and stops on its target. Add `positionKd` if it rolls past and comes back. If it stops short, raise `speedKi`.
6. Then tune the measurements above, so the counts turn into the right distance and angle.

Copy the tuned lines into `setup()` of every sketch for that robot, after `beginEncoders()`.

| What you see | Try |
|--------------|-----|
| Rolls past the target and stays there | Turn on the position controller, with the speed controller: raise `positionKp` |
| Rolls past, backs up, rolls past again | Lower `positionKp`, or add `positionKd` |
| Stops short, and the move gives up | Turn on the speed controller, or if it's on, raise `speedKi` |
| The wheels turn slower than asked | Turn on the speed controller, or set `setRatedRPM()` to your robot's real top speed |
| Curves | Raise `headingKp` |
| Curves only when fast, with the speed controller | Ask for a lower `rpm`: above the robot's top speed, a wheel already at full power can't speed up to correct |
| Wobbles from side to side | Lower `headingKp`, or add `headingKd` |
| A wheel never reaches its target speed | Raise `speedKi`, or ask for less: the robot may be at its top speed |
| The speed swings above and below the target | Lower `speedKp` or `speedKi`, or add `speedKd` |

`shield.plotMoves(Serial)` prints what the controllers see, ten times a second during each move, for the Serial Plotter: the pulses still to go, how far the left wheel is ahead, and each wheel's target speed, measured speed (both in RPM) and power. Use `Serial.begin(115200)`, and keep the USB cable plugged in.

The library takes care of the usual PID pitfalls. The integral only grows while the output isn't at its limit, so it can't wind up. The derivative comes from the measured movement, so it doesn't jump when a move starts. And when a wheel would go past the limit, both slow down by the same amount, so keeping straight comes first.

[`examples/Encoders/PIDSpeedControl/`](examples/Encoders/PIDSpeedControl/PIDSpeedControl.ino) writes the speed controller out in the sketch, to show how a PID controller works inside.

## Upgrading from 0.2.x

Version 0.3.0 puts everything in the `SnappyXOShield` class, so sketches written for 0.2.x and earlier need these changes:

| 0.2.x                                 | 0.3.0                                                        |
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
- **Boards:** boards shaped like an UNO, which the shield plugs onto. The library needs pins A2 and A3, and interrupts on pins 2 and 3 for encoder motors. The examples are compiled for the UNO R3 and UNO R4 WiFi on every change.

## License

MIT License — Copyright (c) 2026 A+ Mobile Solutions Inc. See [LICENSE](LICENSE) for details.
