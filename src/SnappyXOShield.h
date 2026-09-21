#ifndef SNAPPYXOSHIELD_H
#define SNAPPYXOSHIELD_H

#include <Arduino.h>

// Motor Pins
#ifdef SNAPPYXO_SHIELDV2
// Only set this before including this library if you know you have the V2
// shield. If you bought the kit after 2026, you can ignore this part.
const int motorLeft_InputOne = 4, motorLeft_InputTwo = 5, motorLeft_Enable = 6;
const int motorRight_InputOne = 12, motorRight_InputTwo = 13,
		  motorRight_Enable = 11;
#else
const int motorLeft_InputOne = 4, motorLeft_InputTwo = 7, motorLeft_Enable = 5;
const int motorRight_InputOne = 8, motorRight_InputTwo = 9,
		  motorRight_Enable = 6;
#endif

// Inversion flags - set via initMotors(invertL, invertR, invertLR)
static bool motorInvertL = false;
static bool motorInvertR = false;
static bool motorInvertLR = false;

void initMotors(bool invertL = false, bool invertR = false,
				bool invertLR = false) {
	motorInvertL = invertL;
	motorInvertR = invertR;
	motorInvertLR = invertLR;
	pinMode(motorLeft_InputOne, OUTPUT);
	pinMode(motorLeft_InputTwo, OUTPUT);
	pinMode(motorLeft_Enable, OUTPUT);
	pinMode(motorRight_InputOne, OUTPUT);
	pinMode(motorRight_InputTwo, OUTPUT);
	pinMode(motorRight_Enable, OUTPUT);
}

// Range: -255 to 255. Positive = forward, negative = backward.
void _motorWrite(int leftSpeed, int rightSpeed) {
	if (motorInvertLR) {
		int t = leftSpeed;
		leftSpeed = rightSpeed;
		rightSpeed = t;
	}
	if (motorInvertL) leftSpeed = -leftSpeed;
	if (motorInvertR) rightSpeed = -rightSpeed;

	bool leftFwd = leftSpeed >= 0;
	bool rightFwd = rightSpeed >= 0;

	digitalWrite(motorLeft_InputOne, leftFwd ? HIGH : LOW);
	digitalWrite(motorLeft_InputTwo, leftFwd ? LOW : HIGH);
	analogWrite(motorLeft_Enable, abs(leftSpeed));

	digitalWrite(motorRight_InputOne, rightFwd ? LOW : HIGH);
	digitalWrite(motorRight_InputTwo, rightFwd ? HIGH : LOW);
	analogWrite(motorRight_Enable, abs(rightSpeed));
}

void forward() { _motorWrite(255, 255); }
void forward(int speed) { _motorWrite(speed, speed); }
void reverse() { _motorWrite(-255, -255); }
void reverse(int speed) { _motorWrite(-speed, -speed); }
void turnLeft() { _motorWrite(-255, 255); }
void turnLeft(int speed) { _motorWrite(-speed, speed); }
void turnRight() { _motorWrite(255, -255); }
void turnRight(int speed) { _motorWrite(speed, -speed); }

// Coasting brake: slows down slowly to a stop
void pause() {
	analogWrite(motorLeft_Enable, 0);
	analogWrite(motorRight_Enable, 0);
}

// Active brake: quickly stops motors
void brake() {
	digitalWrite(motorLeft_InputOne, HIGH);
	digitalWrite(motorLeft_InputTwo, HIGH);
	analogWrite(motorLeft_Enable, 255);
	digitalWrite(motorRight_InputOne, HIGH);
	digitalWrite(motorRight_InputTwo, HIGH);
	analogWrite(motorRight_Enable, 255);
}

// Differential drive: throttle and steering each in range -512 to +512.
// Zero throttle + nonzero steering does in place turns.
void drive(int throttle, int steering) {
	int left = throttle + steering;
	int right = throttle - steering;

	int maxVal = max(abs(left), abs(right));
	if (maxVal > 512) {
		left = left * 512 / maxVal;
		right = right * 512 / maxVal;
	}

	_motorWrite(left * 255 / 512, right * 255 / 512);
}

#endif
