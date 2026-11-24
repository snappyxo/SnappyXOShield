// Developed by A+ Mobile Solutions Inc
// Licensed under the MIT License. See LICENSE for details.

#include "SnappyXOShield.h"

// The pins of each shield version, in order from V2. A new version is a new
// row here and a new name in Version.
const SnappyXOShield::PinMap SnappyXOShield::PINS[] PROGMEM = {
	//  IN1  IN2  EN  encoder A, B
	{{4, 5, 6, 2, A2},      // V2: left output, M1
	 {12, 13, 11, 3, A3}},  //     right output, M2
	{{4, 7, 5, 2, A2},      // V3: left output, M1
	 {8, 9, 6, 3, A3}},     //     right output, M2
};

const SnappyXOShield::PinMap *SnappyXOShield::_pinMap() const {
	return &PINS[_version - V2];
}

SnappyXOShield::SnappyXOShield(Version version)
	: _version(version), _invertLeft(false), _invertRight(false), _swap(false) {
	const PinMap *pins = _pinMap();
	_leftIn1 = pgm_read_byte(&pins->left.in1);
	_leftIn2 = pgm_read_byte(&pins->left.in2);
	_leftEnable = pgm_read_byte(&pins->left.enable);
	_rightIn1 = pgm_read_byte(&pins->right.in1);
	_rightIn2 = pgm_read_byte(&pins->right.in2);
	_rightEnable = pgm_read_byte(&pins->right.enable);
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

	// Forward is IN1 high on the left motor (M1), but IN2 high on the right
	// motor (M2)
	_writeMotor(_leftIn1, _leftIn2, _leftEnable, left);
	_writeMotor(_rightIn2, _rightIn1, _rightEnable, right);
}

namespace {
// Squares v while keeping its sign and its -255..255 range.
int squareInput(int v) {
	int32_t sq = (int32_t)v * v;
	return v < 0 ? -sq / 255 : sq / 255;
}
}  // namespace

void SnappyXOShield::drive(int throttle, int steering, bool squareThrottle,
						   bool squareSteering) {
	// Limited first: squared, a bigger input would overflow an UNO R3's int
	throttle = constrain(throttle, -255, 255);
	steering = constrain(steering, -255, 255);

	if (squareThrottle) {
		throttle = squareInput(throttle);
	}
	if (squareSteering) {
		steering = squareInput(steering);
	}

	int32_t left = throttle + steering;
	int32_t right = throttle - steering;

	int32_t maxVal = max(abs(left), abs(right));
	if (maxVal > 255) {
		left = (left * 255) / maxVal;
		right = (right * 255) / maxVal;
	}

	setMotors(left, right);
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

void SnappyXOShield::_writeMotor(uint8_t forwardPin, uint8_t backwardPin,
								 uint8_t enablePin, int speed) {
	digitalWrite(forwardPin, speed >= 0 ? HIGH : LOW);
	digitalWrite(backwardPin, speed >= 0 ? LOW : HIGH);
	analogWrite(enablePin, abs(speed));
}
