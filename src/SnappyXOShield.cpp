#include "SnappyXOShield.h"

// drive() treats throttle and steering below this as 0
static const int DRIVE_DEADBAND = 10;

SnappyXOShield::SnappyXOShield(Version version)
	: _invertLeft(false), _invertRight(false), _swap(false), _turnSpeed(60) {
	if (version == V2) {
		_leftIn1 = 4, _leftIn2 = 5, _leftEnable = 6;
		_rightIn1 = 12, _rightIn2 = 13, _rightEnable = 11;
	} else {
		_leftIn1 = 4, _leftIn2 = 7, _leftEnable = 5;
		_rightIn1 = 8, _rightIn2 = 9, _rightEnable = 6;
	}
}

void SnappyXOShield::begin() {
	pinMode(_leftIn1, OUTPUT);
	pinMode(_leftIn2, OUTPUT);
	pinMode(_leftEnable, OUTPUT);
	pinMode(_rightIn1, OUTPUT);
	pinMode(_rightIn2, OUTPUT);
	pinMode(_rightEnable, OUTPUT);
	stop();
}

void SnappyXOShield::invertLeft(bool invert) { _invertLeft = invert; }
void SnappyXOShield::invertRight(bool invert) { _invertRight = invert; }
void SnappyXOShield::swapMotors(bool swap) { _swap = swap; }

void SnappyXOShield::forward(int speed) { setMotors(speed, speed); }
void SnappyXOShield::reverse(int speed) { setMotors(-speed, -speed); }
void SnappyXOShield::spinLeft(int speed) { setMotors(-speed, speed); }
void SnappyXOShield::spinRight(int speed) { setMotors(speed, -speed); }

void SnappyXOShield::setMotors(int left, int right) {
	left = constrain(left, -255, 255);
	right = constrain(right, -255, 255);

	if (_swap) {
		int t = left;
		left = right;
		right = t;
	}
	if (_invertLeft) left = -left;
	if (_invertRight) right = -right;

	// Forward is IN1 high on the left motor, but IN2 high on the right motor
	writeMotor(_leftIn1, _leftIn2, _leftEnable, left);
	writeMotor(_rightIn2, _rightIn1, _rightEnable, right);
}

void SnappyXOShield::drive(int throttle, int steering) {
	throttle = constrain(throttle, -255, 255);
	steering = constrain(steering, -255, 255);

	// Ignore small movements around the joystick's center, so a resting
	// thumb doesn't creep or turn the robot
	if (abs(throttle) < DRIVE_DEADBAND) throttle = 0;
	if (abs(steering) < DRIVE_DEADBAND) steering = 0;

	// Square the steering, keeping its sign: small pushes turn gently for fine
	// corrections, and a full push still turns fully. Then scale it to the
	// turn speed. long, because these products don't fit in the UNO R3's
	// 16-bit int.
	long turn = (long)steering * abs(steering) / 255 * _turnSpeed / 100;

	long left = throttle + turn;
	long right = throttle - turn;

	// Past full speed, slow both wheels by the same factor so the turn keeps
	// its shape
	long biggest = max(abs(left), abs(right));
	if (biggest > 255) {
		left = left * 255 / biggest;
		right = right * 255 / biggest;
	}

	setMotors(left, right);
}

void SnappyXOShield::setTurnSpeed(int percent) {
	_turnSpeed = constrain(percent, 0, 100);
}

void SnappyXOShield::stop() {
	analogWrite(_leftEnable, 0);
	analogWrite(_rightEnable, 0);
}

void SnappyXOShield::brake() {
	digitalWrite(_leftIn1, HIGH);
	digitalWrite(_leftIn2, HIGH);
	analogWrite(_leftEnable, 255);
	digitalWrite(_rightIn1, HIGH);
	digitalWrite(_rightIn2, HIGH);
	analogWrite(_rightEnable, 255);
}

void SnappyXOShield::writeMotor(uint8_t forwardPin, uint8_t backwardPin,
								uint8_t enablePin, int speed) {
	digitalWrite(forwardPin, speed >= 0 ? HIGH : LOW);
	digitalWrite(backwardPin, speed >= 0 ? LOW : HIGH);
	analogWrite(enablePin, abs(speed));
}
