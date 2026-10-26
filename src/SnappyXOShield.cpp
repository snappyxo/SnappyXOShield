// Developed by A+ Mobile Solutions Inc
// Licensed under the MIT License. See LICENSE for details.

#include "SnappyXOShield.h"

SnappyXOShield::SnappyXOShield(Version version)
	: _invertLeft(false), _invertRight(false), _swap(false) {
	if (version == V2) {
		_leftIn1 = 4, _leftIn2 = 5, _leftEnable = 6;
		_rightIn1 = 12, _rightIn2 = 13, _rightEnable = 11;
	} else if (version == V3) {
		_leftIn1 = 4, _leftIn2 = 7, _leftEnable = 5;
		_rightIn1 = 8, _rightIn2 = 9, _rightEnable = 6;
	} else {
		// NOTE: Add any future shield version code here
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

void SnappyXOShield::invertLeft(bool invert) {
	_invertLeft = invert;
}
void SnappyXOShield::invertRight(bool invert) {
	_invertRight = invert;
}
void SnappyXOShield::swapMotors(bool swap) {
	_swap = swap;
}

void SnappyXOShield::forward(int speed) {
	setMotors(speed, speed);
}
void SnappyXOShield::reverse(int speed) {
	setMotors(-speed, -speed);
}
void SnappyXOShield::spinLeft(int speed) {
	setMotors(-speed, speed);
}
void SnappyXOShield::spinRight(int speed) {
	setMotors(speed, -speed);
}

void SnappyXOShield::setMotors(int left, int right) {
	left = constrain(left, -255, 255);
	right = constrain(right, -255, 255);

	if (_swap) {
		int t = left;
		left = right;
		right = t;
	}
	if (_invertLeft) {
		left = -left;
	}
	if (_invertRight) {
		right = -right;
	}

	// Forward is IN1 high on the left motor, but IN2 high on the right motor
	writeMotor(_leftIn1, _leftIn2, _leftEnable, left);
	writeMotor(_rightIn2, _rightIn1, _rightEnable, right);
}

void SnappyXOShield::drive(int throttle, int steering) {
	int left = throttle + steering;
	int right = throttle - steering;

	int maxVal = max(abs(left), abs(right));
	if (maxVal > 512) {
		left = left * 512 / maxVal;
		right = right * 512 / maxVal;
	}

	setMotors(left * 255 / 512, right * 255 / 512);
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
