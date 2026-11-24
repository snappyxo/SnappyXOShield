// Drives straight for 30 inches with encoder motors, holding both wheels at
// the same speed with a PID controller for each. Open the Serial Plotter at
// 9600 baud to watch the wheels' speeds, then tune Kp, Ki and Kd below until
// both reach the target quickly and stay on it together.
//
// shield.setSpeedPID() runs the same kind of speed controller inside the
// library's moves. This example writes a simpler one out, to show how it
// works: it runs every 100 ms instead of 20, only drives forward, and keeps
// its integral between 0 and 255.
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

// PID gains: starting points to tune
float Kp = 4;     // proportional: pushes harder the further the speed is from the target
float Ki = 13;    // integral: builds up while the speed stays off target, to close the last gap
float Kd = 0.6;   // derivative: holds back sudden changes, against overshooting

const float targetRPM = 60;  // the wheels' speed, in turns per minute
const float dt = 0.1;        // the PID runs every 100 ms, 0.1 seconds

// The robot's measurements
const float WHEEL_DIAMETER_MM = 61;
const float GEAR_RATIO = 120;          // how many times the motor turns for each turn of the wheel
const int PULSES_PER_MOTOR_TURN = 8;   // how many pulses the encoder gives for each turn of the motor: its PPR
const float PULSES_PER_WHEEL_TURN = PULSES_PER_MOTOR_TURN * GEAR_RATIO;

// How far to drive: 30 inches, in pulses of each wheel
const float DISTANCE_MM = 30 * 25.4;
const long targetPulses = DISTANCE_MM / (PI * WHEEL_DIAMETER_MM) * PULSES_PER_WHEEL_TURN;

// Each wheel's PID keeps these from one step to the next
float leftIntegral = 0, rightIntegral = 0;
float leftLastSpeed = 0, rightLastSpeed = 0;

long leftLastCount = 0, rightLastCount = 0;
unsigned long lastMs;
bool done = false;

void setup() {
  Serial.begin(9600);
  shield.begin();          // sets up the motor pins
  shield.beginEncoders();  // starts counting the encoders' pulses

  // Uncomment the lines EncoderTest printed for this robot:
  // shield.invertLeft();   // the left motor (M1) turns backward when it should go forward
  // shield.invertRight();  // the right motor (M2) turns backward when it should go forward
  // shield.swapMotors();   // the left wheel's motor is plugged into M2, the right wheel's into M1

  delay(2000);  // time to put the robot down
  lastMs = millis();
}

void loop() {
  if (done || millis() - lastMs < 100) {
    return;
  }
  lastMs += 100;

  long leftCount = shield.leftCount();
  long rightCount = shield.rightCount();
  if ((leftCount + rightCount) / 2 >= targetPulses) {
    shield.brake();
    done = true;
    return;
  }

  // Each wheel's speed in RPM: the wheel turns it made in the last 100 ms,
  // times 600 for a minute
  float leftSpeed = (leftCount - leftLastCount) / PULSES_PER_WHEEL_TURN * 600;
  float rightSpeed = (rightCount - rightLastCount) / PULSES_PER_WHEEL_TURN * 600;
  leftLastCount = leftCount;
  rightLastCount = rightCount;

  int leftPower = speedController(leftSpeed, leftIntegral, leftLastSpeed);
  int rightPower = speedController(rightSpeed, rightIntegral, rightLastSpeed);
  shield.setMotors(leftPower, rightPower);

  // For the Serial Plotter: a line every 100 ms, a curve for each name
  Serial.print("target:");
  Serial.print(targetRPM);
  Serial.print(",leftSpeed:");
  Serial.print(leftSpeed);
  Serial.print(",rightSpeed:");
  Serial.print(rightSpeed);
  Serial.print(",leftPower:");
  Serial.print(leftPower);
  Serial.print(",rightPower:");
  Serial.println(rightPower);
}

// One step of the speed controller, a PID controller: from a wheel's speed in
// RPM, the motor power (0-255) that brings it to targetRPM. integral and
// lastSpeed belong to that wheel.
int speedController(float speed, float &integral, float &lastSpeed) {
  float error = targetRPM - speed;

  integral += Ki * error * dt;
  integral = constrain(integral, 0, 255);  // so it can't build up past full power

  float change = (speed - lastSpeed) / dt;  // how fast the speed is changing
  lastSpeed = speed;

  return constrain(Kp * error + integral - Kd * change, 0, 255);
}
