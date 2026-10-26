// The BasicMotorControl example without the SnappyXOShield library. Its loop()
// is the same, but forward(), drive() and the other functions are written out
// below, so you can see what the library does inside. Compare the two
// sketches side by side.
//
// Each motor has three pins on the shield's motor driver. IN1 and IN2 pick the
// direction, and PWM sets the speed with analogWrite() (0-255):
//
//   IN1    IN2    PWM      the motor
//   HIGH   LOW    speed    turns forward
//   LOW    HIGH   speed    turns backward
//   any    any    0        coasts to a stop   (stop())
//   HIGH   HIGH   255      brakes hard        (brake())
//
// Developed by A+ Mobile Solutions Inc
// Licensed under the MIT License. See LICENSE for details.

// SnappyXO Motor Shield v3 pins. For a v2 shield: left 4, 5, 6 and right 12, 13, 11.
// If a wheel turns backward when it should go forward, swap its IN1 and IN2
// numbers: that's what shield.invertLeft() and shield.invertRight() do.
const int LEFT_IN1 = 4;
const int LEFT_IN2 = 7;
const int LEFT_PWM = 5;
const int RIGHT_IN1 = 8;
const int RIGHT_IN2 = 9;
const int RIGHT_PWM = 6;

// How much of the motors' power drive() uses for turning, 0-100 percent
// (shield.setTurnSpeed() in the library). Lower makes steering gentler.
const int TURN_SPEED = 60;

// Turns one motor at a speed from -255 (full backward) to 255 (full forward)
void setMotor(int forwardPin, int backwardPin, int pwmPin, int speed) {
  speed = constrain(speed, -255, 255);
  if (speed >= 0) {
    digitalWrite(forwardPin, HIGH);
    digitalWrite(backwardPin, LOW);
  } else {
    digitalWrite(forwardPin, LOW);
    digitalWrite(backwardPin, HIGH);
  }
  analogWrite(pwmPin, abs(speed));  // the speed without its sign: 0-255
}

// Sets each wheel's speed, -255 to 255. Every move below ends up here.
// The two motors face opposite ways on the robot, so for both wheels to roll
// forward the right motor has to turn the other way from the left one. That's
// why forward is IN1 on the left motor but IN2 on the right motor.
void setMotors(int left, int right) {
  setMotor(LEFT_IN1, LEFT_IN2, LEFT_PWM, left);
  setMotor(RIGHT_IN2, RIGHT_IN1, RIGHT_PWM, right);
}

// Both wheels at the same speed (0-255, full speed when left out)
void forward(int speed = 255) { setMotors(speed, speed); }
void reverse(int speed = 255) { setMotors(-speed, -speed); }

// Spin in place: one wheel forward, the other backward
void spinLeft(int speed = 255) { setMotors(-speed, speed); }
void spinRight(int speed = 255) { setMotors(speed, -speed); }

// Joystick-style driving for a two-wheeled car: throttle from -255 (backward)
// to 255 (forward), steering from -255 (left) to 255 (right)
void drive(int throttle, int steering) {
  throttle = constrain(throttle, -255, 255);
  steering = constrain(steering, -255, 255);

  // 1. Small values near the joystick's center count as 0, so a resting thumb
  //    doesn't creep or turn the robot
  if (abs(throttle) < 10) throttle = 0;
  if (abs(steering) < 10) steering = 0;

  // 2. Square the steering, keeping its sign: half a push turns a quarter as
  //    hard, so small pushes give fine corrections
  // 3. Use only TURN_SPEED percent of the power for turning
  // long, because steering * steering doesn't fit in the UNO R3's 16-bit int
  long turn = (long)steering * abs(steering) / 255 * TURN_SPEED / 100;

  // 4. Mix: the left wheel gets throttle + turn, the right wheel throttle - turn.
  //    Turning right speeds up the left wheel and slows down the right one.
  long left = throttle + turn;
  long right = throttle - turn;

  // 5. If a wheel would go past full speed, slow both wheels by the same
  //    factor, so the robot still follows the same curve
  long biggest = max(abs(left), abs(right));
  if (biggest > 255) {
    left = left * 255 / biggest;
    right = right * 255 / biggest;
  }

  setMotors(left, right);
}

// Coast: cut the power and roll to a stop
void stop() {
  analogWrite(LEFT_PWM, 0);
  analogWrite(RIGHT_PWM, 0);
}

// Hard stop: lock both motors
void brake() {
  digitalWrite(LEFT_IN1, HIGH);
  digitalWrite(LEFT_IN2, HIGH);
  analogWrite(LEFT_PWM, 255);
  digitalWrite(RIGHT_IN1, HIGH);
  digitalWrite(RIGHT_IN2, HIGH);
  analogWrite(RIGHT_PWM, 255);
}

void setup() {
  // What shield.begin() does: every motor pin is an output, and the robot starts stopped
  pinMode(LEFT_IN1, OUTPUT);
  pinMode(LEFT_IN2, OUTPUT);
  pinMode(LEFT_PWM, OUTPUT);
  pinMode(RIGHT_IN1, OUTPUT);
  pinMode(RIGHT_IN2, OUTPUT);
  pinMode(RIGHT_PWM, OUTPUT);
  stop();
}

void loop() {
  forward();  // both wheels forward at full speed (255)
  delay(1000);

  reverse(150);  // both wheels backward at speed 150 (0-255)
  delay(1000);

  spinLeft();  // left wheel backward, right wheel forward: spins left in place
  delay(500);

  spinRight();  // left wheel forward, right wheel backward: spins right in place
  delay(500);

  setMotors(255, 100);  // each wheel's speed, -255 to 255: left faster, so it curves right
  delay(1000);

  drive(200, -100);  // joystick-style: throttle 200 (forward), steering -100 (left)
  delay(1000);

  stop();  // cuts the power, the robot coasts to a stop
  delay(1000);
}
