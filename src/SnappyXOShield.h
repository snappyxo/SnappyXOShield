// Developed by A+ Mobile Solutions Inc
// Licensed under the MIT License. See LICENSE for details.

#ifndef SNAPPYXOSHIELD_H
#define SNAPPYXOSHIELD_H

#include <Arduino.h>

// Drives the two DC motors on the SnappyXO Motor Shield.
//
//   SnappyXOShield shield(SnappyXOShield::V3);  // v3 shield
//   SnappyXOShield shield(SnappyXOShield::V2);  // v2 shield
//
// There's no default, so every sketch says which shield it's written for.
// Call begin() once in setup(). Speeds run from -255 (full backward) to 255
// (full forward), and 0 stops.
class SnappyXOShield {
   public:
	// Shield version, printed on the board
	enum Version { V2 = 2, V3 = 3 };

	explicit SnappyXOShield(Version version);

	// Only here so that SnappyXOShield shield; without a version fails to
	// compile with this message instead of a cryptic one
	template <bool versionGiven = false>
	SnappyXOShield() {
		static_assert(versionGiven,
					  "Say which shield you have: SnappyXOShield "
					  "shield(SnappyXOShield::V3); or SnappyXOShield "
					  "shield(SnappyXOShield::V2);");
	}

	// Sets the motor pins as outputs and stops both motors. Call once in
	// setup().
	void begin();

	// For a wheel that turns the wrong way. invertLeft() and invertRight()
	// reverse the motor on the shield's left or right output; swapMotors()
	// swaps the two outputs, for motors plugged in the other way round.
	void invertLeft(bool invert = true);
	void invertRight(bool invert = true);
	void swapMotors(bool swap = true);

	// Both wheels at speed (0-255, full speed when left out)
	void forward(int speed = 255);
	void reverse(int speed = 255);

	// Spin in place: one wheel forward, the other backward
	void spinLeft(int speed = 255);
	void spinRight(int speed = 255);

	// Each wheel's speed, -255 (full backward) to 255 (full forward)
	void setMotors(int left, int right);

	// Joystick-style driving for a two-wheeled car. throttle runs from -255
	// (backward) to 255 (forward), steering from -255 (left) to 255 (right).
	// With no throttle, steering spins the car in place. To make it easier to
	// drive, values under 10 count as 0, and steering is squared (small pushes
	// turn gently) and limited to the turn speed.
	void drive(int throttle, int steering);
	void drive(int throttle, int steering, int deadband);

	// How much of the motors' power drive() uses for turning, 0-100 percent
	// (60 when not set). Lower makes steering gentler, 100 is the full range.
	void setTurnSpeed(int percent);

	void stop();   // Coast: cut the power and roll to a stop
	void brake();  // Hard stop: lock both motors

   private:
	uint8_t _leftIn1, _leftIn2, _leftEnable;
	uint8_t _rightIn1, _rightIn2, _rightEnable;
	bool _invertLeft, _invertRight, _swap;

	void _writeMotor(uint8_t forwardPin, uint8_t backwardPin, uint8_t enablePin,
					 int speed);
};

#endif
