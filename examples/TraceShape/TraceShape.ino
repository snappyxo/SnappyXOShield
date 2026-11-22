// Drives a U shape with encoder motors: straight ahead, a right turn, across,
// another right turn, and back. Put the robot on the floor, then press reset
// on the board: it starts two seconds later.
//
// Wiring, the same on the v3 and v2 shield, by the labels on each encoder's port:
//   left wheel (M1):  A to pin 2, B to pin A2
//   right wheel (M2): A to pin 3, B to pin A3
//   5V (or VCC) to 5V, GND to GND
// Run the EncoderTest example first: it checks the wiring and says which of
// the lines in setup() below to uncomment.
//
// Developed by A+ Mobile Solutions Inc
// Licensed under the MIT License. See LICENSE for details.

#include <SnappyXOShield.h>

SnappyXOShield shield(SnappyXOShield::V3);  // the shield version printed on the board: V3 or V2

void setup() {
  shield.begin();          // sets up the motor pins
  shield.beginEncoders();  // starts counting the encoders' pulses

  // Uncomment the lines EncoderTest printed for this robot:
  // shield.invertLeft();   // the left motor (M1) turns backward when it should go forward
  // shield.invertRight();  // the right motor (M2) turns backward when it should go forward
  // shield.swapMotors();   // the left wheel's motor is plugged into M2, the right wheel's into M1

  // Without PID control, the wheels turn slower than asked, each move rolls
  // a little past its end, and nothing keeps the wheels together. Paste your
  // robot's tuned values here (the PIDSquare example shows how to find them):
  // shield.setSpeedPID(0);
  // shield.setHeadingPID(0);
  // shield.setPositionPID(0);

  // The distance between the middles of the wheels, in millimeters. If the
  // robot turns too little, make this bigger; too far, smaller.
  // shield.setWheelSpacing(96);

  delay(2000);  // time to step back

  shield.driveInches(24);  // straight ahead 24 inches. In centimeters: shield.driveCm(60);
  delay(500);
  shield.spinTurn(90);  // spin right 90 degrees on the spot. Left is negative: shield.spinTurn(-90);
  delay(500);
  shield.driveInches(12);
  delay(500);
  shield.pivotTurn(90);  // the other way to turn: the left wheel rolls around the right one
  delay(500);
  shield.driveInches(24, 90);  // faster: the wheels' speed in RPM as the last number, 60 if left out
}

void loop() {
  // Nothing to repeat: press reset to drive the shape again
}
