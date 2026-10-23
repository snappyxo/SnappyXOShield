// Drives a two-wheeled robot from the MicroBlue app's Drive screen, using the
// Arduino UNO R4 WiFi's built-in Bluetooth. The robot stops when the phone
// disconnects or the app's heartbeat stops (phone out of range, app closed or
// in the background).
//
// Requires the MicroBlue library 2.1 or later -> install it via Sketch -> Include Library -> Manage Libraries.

#include <MicroBlue.h>
#include <SnappyXOShield.h>

MicroBlueManager manager;  // talks to the MicroBlue app over the built-in Bluetooth
SnappyXOShield shield(SnappyXOShield::V3);  // v3 shield

void setup() {
  shield.begin();  // sets up the motor pins
  shield.setTurnSpeed(60);  // how hard the robot turns, 0-100 percent: lower makes steering gentler
  // shield.invertLeft();  // uncomment if a wheel turns the wrong way, see the BasicMotorControl example

  manager.begin("SnappyXO Robot");  // the name the app shows when you connect: change it to tell robots apart
}

void loop() {
  if (manager.isConnected()) {
    MicroBlueMessage msg = manager.read();  // the latest message from the app, if any

    // The D-Pad, Joystick and Tilt send "x,y" on IDs d0 and d1, each 0-1023:
    // 1023 is right or forward, 0 is left or back, and 512,512 is the center (stop)
    if (msg.id == "d0" || msg.id == "d1") {
      int x, y;
      // Read the numbers before and after the comma. sscanf() returns how many it found.
      if (sscanf(msg.value.c_str(), "%d,%d", &x, &y) == 2) {
        int throttle = map(y, 0, 1023, -255, 255);  // forward and back
        int steering = map(x, 0, 1023, -255, 255);  // left and right
        shield.drive(throttle, steering);
      }
    }
  }

  // Checked outside isConnected(), so the robot also stops after a disconnect
  if (!manager.isConnected() || manager.heartbeatLost()) {
    shield.stop();
  }
}
