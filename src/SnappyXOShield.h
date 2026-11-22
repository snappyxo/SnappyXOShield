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
// (full forward), and 0 stops. With encoder motors, also call beginEncoders()
// to measure the wheels, drive a distance and turn by an angle.
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
	// reverse the motor on the shield's left output (M1 on the board) or
	// right output (M2); swapMotors() swaps the two outputs, for motors
	// plugged in the other way round.
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
	// With no throttle, steering spins the car in place.
	//
	// By default both inputs are squared (sign preserved) before driving, so
	// small stick motions near 0 barely move the car while larger motions
	// still reach full speed. Pass false to drive an axis linearly instead.
	void drive(int throttle, int steering, bool squareThrottle = true,
			   bool squareSteering = true);

	void stop();   // Coast: cut the power and roll to a stop
	void brake();  // Hard stop: lock both motors

	// ---- Encoder motors (optional) ----
	// For motors with built-in encoders, on either shield, by the labels on
	// each encoder's port:
	//
	//   left wheel (M1):  A to pin 2, B to pin A2
	//   right wheel (M2): A to pin 3, B to pin A3
	//
	// Sketches that don't call beginEncoders() leave these pins alone.

	// Starts counting the encoder pulses. Call once in setup(), after
	// begin(). If the board can't be interrupted by the shield's encoder A
	// pins, the encoders stay off and the moves return false.
	void beginEncoders();

	// How far each wheel has turned since beginEncoders() or resetCounts(),
	// in pulses: 8 per turn of the motor, so 960 per turn of the wheel with a
	// 120:1 gearbox. Counts up when the wheel rolls forward, down when it rolls
	// backward.
	long leftCount();
	long rightCount();
	void resetCounts();

	// Each wheel's speed in turns per minute, negative when rolling backward
	float leftRPM();
	float rightRPM();

	// Drives straight for a distance, then brakes. A negative distance drives
	// backward. rpm is the wheels' speed in turns per minute, 60 if left out.
	// Waits until the robot has stopped. Returns false, after stopping the
	// motors, if beginEncoders() wasn't called, a wheel stopped turning for a
	// second without its power going up (an encoder unplugged, a wheel stuck,
	// or too little power), a wheel turned the wrong way (an encoder's B
	// unplugged, or its A and B swapped), or the position controller didn't
	// settle. The sign of rpm is ignored: the distance or angle gives the
	// direction.
	//
	// Without PID control (below), a move guesses the motor power for rpm
	// from setRatedRPM(), so the wheels may turn slower. It brakes when it
	// reaches the distance, so it rolls a little past, and nothing keeps the
	// wheels together.
	bool driveInches(float inches, int rpm = 60);
	bool driveCm(float cm, int rpm = 60);

	// Turn by an angle, then stop: positive degrees turn right, negative turn
	// left, like drive()'s steering. Otherwise like driveInches().
	//
	// spinTurn() spins in place, one wheel forward and the other backward.
	// pivotTurn() rolls one wheel forward around the other, which stays still:
	// the left wheel to turn right, the right wheel to turn left.
	bool spinTurn(float degrees, int rpm = 60);
	bool pivotTurn(float degrees, int rpm = 60);

	// The robot's measurements, for the calls above. The defaults fit the
	// SnappyXO robot: 61 mm wheels, 96 mm apart, and a 120:1 gearbox rated at
	// 160 RPM.
	void setWheelDiameter(float mm);
	// From the middle of one wheel to the middle of the other. Only the turns
	// use it: if they fall short, make it bigger; if they turn too far,
	// smaller.
	void setWheelSpacing(float mm);
	// How many times the motor turns for one turn of the wheel: 120 for a
	// 120:1 gearbox
	void setGearRatio(float ratio);
	// The wheel's speed at full power, in RPM after the gearbox, from the
	// motor's datasheet: 160 for the DFRobot FIT0450 at 6 V. Without
	// setSpeedPID(), the moves guess the power for a speed from it. A real
	// robot is usually slower: for a closer guess, measure yours with
	// setMotors(255, 255) and leftRPM().
	void setRatedRPM(float rpm);

	// ---- PID control for the moves (optional) ----
	// Each robot is different, so these are off until a sketch sets its own
	// robot's gains: kp, then ki and kd, which can be left out for P only.
	// All 0 turns a controller off again. A call with a negative gain is
	// ignored.

	// Slows down near the end of a move and settles on the target, backing up
	// if it rolls past. The error is the pulses still to go, and it asks for
	// a wheel speed in RPM.
	void setPositionPID(float kp, float ki = 0, float kd = 0);
	// Keeps the wheels together, so the robot drives straight and spins on
	// the spot. The error is how many pulses one wheel is ahead of the other,
	// and it asks for a difference in wheel speed, in RPM.
	void setHeadingPID(float kp, float ki = 0, float kd = 0);
	// Holds each wheel at the speed asked for, whatever the battery or the
	// robot's weight, instead of guessing the power. The error is the target
	// speed minus the measured one, in RPM. The position controller needs
	// it: near the end, it asks for speeds too slow for a guessed power to
	// turn the wheels at all.
	void setSpeedPID(float kp, float ki = 0, float kd = 0);

	// Prints what the controllers see during moves, ten times a second, for
	// the Serial Plotter: shield.plotMoves(Serial);
	void plotMoves(Print &output);

   private:
	// The pins of one motor output: the motor's, then its encoder's
	struct OutputPins {
		uint8_t in1, in2, enable, encoderA, encoderB;
	};
	// The pins one shield version uses: its left output, M1 on the board,
	// and its right output, M2
	struct PinMap {
		OutputPins left, right;
	};
	// One row per version, in order from V2, kept in flash
	static const PinMap PINS[];
	// This object's row. It's in flash: read it with pgm_read_byte().
	const PinMap *_pinMap() const;

	// Each object's own
	uint8_t _version;
	uint8_t _leftIn1, _leftIn2, _leftEnable;
	uint8_t _rightIn1, _rightIn2, _rightEnable;
	bool _invertLeft, _invertRight, _swap;

	// Encoder motors. Shared by all objects, since a robot has one pair of
	// encoders, and kept with the encoder code, so only sketches that use
	// encoder motors need room for them.
	static bool _encodersOn;
	static float _wheelDiameter, _wheelSpacing, _gearRatio, _ratedRPM;
	static float _positionGains[3], _headingGains[3], _speedGains[3];  // kp, ki, kd
	static Print *_plot;

	void _writeMotor(uint8_t forwardPin, uint8_t backwardPin, uint8_t enablePin,
					 int speed);

	// Which encoder belongs to each wheel: 0, the left output's (M1), or 1,
	// the right output's (M2)
	uint8_t _leftEncoder();
	uint8_t _rightEncoder();

	// Turns the wheels for the moves above
	bool _moveWheels(long pulses, int8_t leftDir, int8_t rightDir, int rpm);
};

#endif
