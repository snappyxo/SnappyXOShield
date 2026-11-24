# Changelog

## Unreleased

- New: optional encoder motors, on either shield: the left wheel's encoder A and B outputs on pins 2 and A2, the right wheel's on 3 and A3. `beginEncoders()` starts counting. `leftCount()`, `rightCount()` and `resetCounts()` tell how far each wheel has turned (960 pulses per wheel turn with an 8-pulse encoder and a 120:1 gearbox), `leftRPM()` and `rightRPM()` how fast. `driveInches()` and `driveCm()` drive straight for a distance. `spinTurn()` and `pivotTurn()` turn by an angle, spinning in place or pivoting on one wheel. The moves take the wheels' speed in RPM, 60 if left out, and brake when they reach the distance. `setSpeedPID()`, `setHeadingPID()` and `setPositionPID()` turn on PID control with each robot's own gains, off until set: the speed controllers hold each wheel at the speed asked for, which is otherwise guessed from the motor's rated speed, the heading controller keeps the wheels together, and the position controller settles on the target. `showMoves()` helps tune them: it hands what the controllers see during each move to the Serial Plotter, or to a function of the sketch's that takes a `MoveStatus`. Each sketch sets its robot's measurements with `setGearRatio()`, `setEncoderPulses()`, `setRatedRPM()`, `setWheelDiameter()` and `setWheelSpacing()`, so any encoder motor works. There are no defaults: a move returns false until the measurements it needs are set. `swapMotors()` swaps the encoders too. Sketches without encoders need no changes.
- Examples, in `examples/Encoders`: `EncoderTest` sets up a robot: it turns each wheel, asks what you saw, says what to rewire and prints the lines its sketches need, `TraceShape` drives a U shape, `PIDSquare` drives a 40 cm square to tune a robot's PID controllers one at a time, `MicroBluePIDSquare` does the same from the MicroBlue app on an UNO R4 WiFi, with the gains, speed and wheel spacing typed on the phone and the wheels' speed on a chart (its layout is in `extras/` as a QR code, and in the README), and `PIDSpeedControl` holds both wheels at the same speed with a PID controller to tune.
- `library.properties` sets `dot_a_linkage=true`, so the encoder code and its settings are only linked into sketches that use them. Sketches without encoders barely grow: on the UNO R3, `BasicMotorControl` takes 1750 bytes of flash and 19 bytes of RAM, against 1668 and 18 in 0.3.2. Without `dot_a_linkage`, it would take 1934 and 59.
- Each shield version's pins, for the motors and the encoders, are now one row of a table kept in flash, so a future shield version is one more row.
- Fixed: `drive()` inputs beyond -255 to 255 wrapped around on the UNO R3 once squared, so `drive(3000, 0)` ran full reverse. They're now limited to -255 to 255 first, as the README says.

## 0.3.2 (2026-11-07)

- `library.properties` now has the release's version number. It still said 0.3.0.

## 0.3.1 (2026-10-28)

- Fixed: `drive()` overflowed on the UNO R3 when throttle and steering together passed full speed, giving wrong wheel speeds. The wheel speeds are now worked out with 32-bit numbers.
- README: the `drive()` section described an earlier version, with a dead zone, a 60% turn speed and `setTurnSpeed()`, none of which 0.3.0 has. It now says both inputs are squared and shows `drive(throttle, steering, squareThrottle, squareSteering)`.
- `keywords.txt` no longer lists `setTurnSpeed`, which was removed in 0.3.0.

## 0.3.0 (2026-10-26)

**Upgrading from 0.2.1.** Sketches written for 0.2.x and earlier need changes; the README's "Upgrading from 0.2.x" section lists each one.

- Everything is now in the `SnappyXOShield` class: declare `SnappyXOShield shield(SnappyXOShield::V3);` above `setup()` and call `shield.begin()`, `shield.forward()` and so on. The library no longer defines global functions such as `forward()` and `drive()`, which clashed with functions of the same name in sketches: a sketch's own `drive()` (like the one in MicroBlue's drive example) failed to compile, and a sketch's `turnRight(float)` called as `turnRight(90)` ran the library's `turnRight(int speed)` instead.
- Every sketch names its shield version, `SnappyXOShield shield(SnappyXOShield::V3);` or `SnappyXOShield shield(SnappyXOShield::V2);`. There's no default. This replaces `#define SNAPPYXO_SHIELDV2` before the `#include`.
- `drive(throttle, steering)` takes -255 to 255 for each input (was -512 to 512), the same scale as every other speed.
- `drive()` steers more gently, so the robot is easier to drive: throttle and steering are both squared (sign kept), so small pushes give fine corrections and bigger pushes still reach full speed. New `drive(throttle, steering, squareThrottle, squareSteering)` overload turns that off per axis.
- New `setMotors(left, right)` sets each wheel's speed. It replaces calling the internal `_motorWrite()`.
- `pause()` is now `stop()`.
- `turnLeft()` and `turnRight()` are now `spinLeft()` and `spinRight()`, since they spin the robot in place. Turning while moving is `drive()` with steering.
- `initMotors(invertL, invertR, invertLR)` is now `begin()` followed by `invertLeft()`, `invertRight()` or `swapMotors()` as needed.
- Fixed: `drive()` overflowed on the UNO R3, where an `int` is 16 bits, so most joystick positions gave wrong speeds; full forward stopped the motors.
- Fixed: speeds above 255 wrapped around (300 ran at 44 on the UNO R3). Every speed is now limited to -255 to 255.
- Fixed: including the library from more than one file of a sketch failed with "multiple definition" errors.
- Examples: `MicroBlueRobot` is now `MicroBlueDrive`, for MicroBlue 2.1: it reads the app's `x,y` (0-1023) and stops the motors when the phone disconnects or the app's heartbeat stops.
- README: removed the link to the deleted v2 example, added the servo headers of each shield.
- License: now MIT (was GPL-3.0), with A+ Mobile Solutions Inc as the copyright holder, like MicroBlue. The source files and examples say so at the top.
- CI compiles the examples for the UNO R3 and UNO R4 WiFi.
