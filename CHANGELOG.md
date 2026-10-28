# Changelog

## Unreleased

- README: the `drive()` section described an earlier version, with a dead zone, a 60% turn speed and `setTurnSpeed()`, none of which 0.3.0 has. It now says both inputs are squared and shows `drive(throttle, steering, squareThrottle, squareSteering)`.

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
