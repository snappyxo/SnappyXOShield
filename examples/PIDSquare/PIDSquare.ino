// Drives a 40 cm square to tune your robot's PID controllers. The library
// runs the controllers; this sketch sets their gains and shows how each move
// went. Tune one controller at a time, in this order:
//
//   0. Run it with every gain at 0. The wheels turn slower than asked, each
//      move rolls past its target, more the faster it goes, and nothing
//      keeps the wheels together.
//   1. Speed: set speedKp, speedKi and speedKd so each wheel turns at the
//      RPM you ask for, whatever the battery and the robot's weight. Without
//      it, the robot only guesses the power and goes slower. Watch it with
//      PLOT on. Heading works without it, but position needs it.
//   2. Heading: raise headingKp until each side is straight and "left ahead
//      by" stays near 0. Add headingKd if the robot wobbles, headingKi if a
//      small difference stays.
//   3. Position: raise positionKp until each move stops near its target.
//      Add positionKd if it rolls past and comes back. If it stops short,
//      raise speedKi.
//
// Then copy your values into setup() in your other sketches.
//
// Starting points to try, not the answer for every robot: speedKp 4 and
// speedKi 13 (speedKd 0 to start), then headingKp 0.3 and positionKp 0.2.
// Without the speed controller, heading takes a bigger gain: about 1.2.
//
// After each move, the Serial Monitor (9600 baud) shows the pulses each wheel
// made against the target. Keep a table of them. With PLOT set to true, the
// Serial Plotter (115200 baud) instead shows what the controllers see during
// each move, with the USB cable plugged in.
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

// PID gains: tune these, one controller at a time. 0 is off.
float positionKp = 0, positionKi = 0, positionKd = 0;
float headingKp = 0, headingKi = 0, headingKd = 0;
float speedKp = 0, speedKi = 0, speedKd = 0;
int rpm = 60;  // the wheels' speed, in turns per minute

const bool PLOT = false;  // true: the Serial Plotter instead of the table

// The robot's measurements
const float WHEEL_DIAMETER_MM = 61;
const float WHEEL_SPACING_MM = 96;      // from the middle of one wheel to the middle of the other
const float GEAR_RATIO = 120;           // the motor turns 120 times for each turn of the wheel
const float PULSES_PER_MOTOR_TURN = 8;  // what the encoder gives
const float PULSES_PER_WHEEL_TURN = PULSES_PER_MOTOR_TURN * GEAR_RATIO;  // 960
const float RATED_RPM = 160;            // the wheel's speed at full power, from the motor's datasheet

const float SIDE_CM = 40;  // the square's sides

void setup() {
  Serial.begin(PLOT ? 115200 : 9600);
  shield.begin();          // sets up the motor pins
  shield.beginEncoders();  // starts counting the encoders' pulses

  // Uncomment the lines EncoderTest printed for this robot:
  // shield.invertLeft();
  // shield.invertRight();
  // shield.swapMotors();

  shield.setWheelDiameter(WHEEL_DIAMETER_MM);
  shield.setWheelSpacing(WHEEL_SPACING_MM);
  shield.setGearRatio(GEAR_RATIO);
  shield.setRatedRPM(RATED_RPM);

  shield.setPositionPID(positionKp, positionKi, positionKd);
  shield.setHeadingPID(headingKp, headingKi, headingKd);
  shield.setSpeedPID(speedKp, speedKi, speedKd);
  if (PLOT) {
    shield.plotMoves(Serial);
  }

  delay(2000);  // time to put the robot down

  long sidePulses = mmToPulses(SIDE_CM * 10);
  // Spinning 90 degrees, each wheel rolls a quarter of a circle as wide as the
  // wheel spacing
  long turnPulses = mmToPulses(PI * WHEEL_SPACING_MM / 4);

  for (int side = 1; side <= 4; side++) {
    shield.resetCounts();
    bool finished = shield.driveCm(SIDE_CM, rpm);
    printResults("Side", side, sidePulses, finished);
    delay(500);

    shield.resetCounts();
    finished = shield.spinTurn(-90, rpm);  // spin left
    printResults("Turn", side, turnPulses, finished);
    delay(500);
  }
}

void loop() {
  // Nothing to repeat: press reset to drive the square again
}

// How many pulses a wheel makes rolling this far
long mmToPulses(float mm) {
  float wheelTurns = mm / (PI * WHEEL_DIAMETER_MM);
  return wheelTurns * PULSES_PER_WHEEL_TURN + 0.5;
}

// Prints how a move went, for the table. The move has just finished.
void printResults(const char *move, int number, long target, bool finished) {
  if (PLOT) {
    return;  // text would get in the way on the Serial Plotter
  }
  delay(300);  // until the robot stands still, counting how far it rolls on
  long left = shield.leftCount();
  long right = shield.rightCount();
  left = abs(left);  // spinning left, the left wheel counts backward
  right = abs(right);
  Serial.print(move);
  Serial.print(" ");
  Serial.print(number);
  Serial.print(": target ");
  Serial.print(target);
  Serial.print(", left ");
  Serial.print(left);
  Serial.print(", right ");
  Serial.print(right);
  Serial.print(", past the target by ");
  Serial.print((left + right) / 2 - target);
  Serial.print(", left ahead by ");
  Serial.print(left - right);
  if (!finished) {
    Serial.print(" (gave up)");
  }
  Serial.println();
}
