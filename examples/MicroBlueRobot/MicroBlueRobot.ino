// This example shows driving with the MicroBlue app with Arduino R4 Wifi.
// Requires the MicroBlue library -> install it via Sketch -> Include Library -> Manage Libraries.

#include <MicroBlue.h>
#include <SnappyXOShield.h>

MicroBlueManager manager; // handles BLE communication with the MicroBlue app

void setup() {
  // initMotors() sets all motor pins as outputs and configures inversion options.
  //
  // Signature: initMotors(invertL, invertR, invertLR)
  //   invertL   - set true to reverse the left motor direction
  //   invertR   - set true to reverse the right motor direction
  //   invertLR  - set true to swap the left and right motors entirely
  //
  // All parameters are optional and default to false.
  // Examples:
  //   initMotors()               - no inversion
  //   initMotors(true)           - left motor reversed
  //   initMotors(false, true)    - right motor reversed
  //   initMotors(false, false, true) - left and right motors swapped
  initMotors();

  manager.begin("My BLE Module"); // start BLE advertising with the given device name
}

void loop() {
  MicroBlueMessage msg = manager.read(); // read the latest message from the app (non-blocking)

  // The MicroBlue joystick sends messages on channel "d0" (analog) or "d1" (digital).
  // The value is two integers separated by a comma: "steering,throttle"
  // each in the range 0–1024, with 512 as the center (no movement).
  if (msg.id == "d0" || msg.id == "d1") {
    int throttle, steering;
    sscanf(msg.value.c_str(), "%d,%d", &steering, &throttle); // parse "steering,throttle" from the message

    throttle -= 512; // shift range from 0–1024 to -512–+512 (center = 0)
    steering -= 512; // same shift for steering

    drive(throttle, steering); // pass to differential drive - see library docs for drive() details
  }
}
