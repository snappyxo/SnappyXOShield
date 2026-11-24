// Developed by A+ Mobile Solutions Inc
// Licensed under the MIT License. See LICENSE for details.

// The optional encoder motors: counting their pulses, measuring the wheels'
// speed, and driving a distance or turning by an angle with them, with
// optional PID control.

#include "SnappyXOShield.h"

namespace {
// Each encoder's A output pulses as the motor turns, and its B output, a
// quarter of a pulse ahead or behind, tells which way. Encoder 0 belongs to
// the motor on the shield's left output (M1), encoder 1 to the right one
// (M2). Their pins come from the shield version's row in PINS, set by
// beginEncoders().
uint8_t aPins[2], bPins[2];

// How many pulses the encoder gives for each turn of the motor, set by
// setEncoderPulses(). The gearbox then turns the wheel once every
// setGearRatio() turns of the motor. 0 until set. Kept here rather than with
// the other settings, since the interrupts read it.
uint16_t pulsesPerMotorTurn = 0;

// With no pulse for this long, a wheel counts as stopped
const unsigned long STOPPED_US = 200000UL;

// How often the moves' controllers run, in milliseconds and in seconds
const unsigned long CONTROL_MS = 20;
const float DT = CONTROL_MS / 1000.0;

// The controllers measure each wheel's speed as the pulses it made in the
// last 100 ms: this many control steps
const uint8_t SPEED_STEPS = 100 / CONTROL_MS;
const float SPEED_SECONDS = SPEED_STEPS * DT;  // 0.1

// With the position controller on, a move is done once it has stayed within
// SETTLE_PULSES of the target for SETTLE_MS. It gives up if it still hasn't
// SETTLE_GIVE_UP_MS after first getting there.
const long SETTLE_PULSES = 5;
const unsigned long SETTLE_MS = 100;
const unsigned long SETTLE_GIVE_UP_MS = 3000;

// When a wheel that should turn makes no pulses for this long, and the power
// it's asked for hasn't gone up in that time either, the move gives up: an
// unplugged encoder, a stuck wheel, or too little power. While a controller
// is still pushing harder, it gets the time to.
const unsigned long GIVE_UP_MS = 1000;

// When a wheel has gone this many pulses the wrong way since the move began,
// its encoder counts backward and the move gives up: its B unplugged, or its
// A and B swapped.
const long WRONG_WAY_PULSES = 100;

// showMoves() shows a move every this many control steps: 10 a second
const uint8_t SHOW_EVERY = 5;

// Changed by the interrupts, so volatile
volatile long counts[2];
volatile int8_t directions[2];          // the way the last pulse went, 1 or -1
volatile unsigned long lastPulseUs[2];  // micros() at the last pulse
volatile unsigned long turnStartUs[2];  // micros() when this motor turn began
volatile uint16_t pulsesThisTurn[2];    // pulses since then
volatile unsigned long lastTurnUs[2];   // the last motor turn's time, 0: none

// The interrupt service routine (ISR): runs on every pulse of encoder i, the
// moment its A output goes high
void encoderISR(uint8_t i) {
	unsigned long now = micros();
	// B is still low if A leads, the way the left motor turns rolling its
	// wheel forward, and already high the other way.
	int8_t dir = digitalRead(bPins[i]) == LOW ? 1 : -1;
	counts[i] += dir;

	// Times a full turn of the motor, so the uneven spacing of the encoder's
	// magnets evens out. Starts over after a stop or a change of
	// direction.
	if (dir != directions[i] || now - lastPulseUs[i] > STOPPED_US) {
		directions[i] = dir;
		turnStartUs[i] = now;
		pulsesThisTurn[i] = 0;
		lastTurnUs[i] = 0;
	} else if (++pulsesThisTurn[i] >= pulsesPerMotorTurn) {
		lastTurnUs[i] = now - turnStartUs[i];
		turnStartUs[i] = now;
		pulsesThisTurn[i] = 0;
	}
	lastPulseUs[i] = now;
}

void encoder0ISR() {
	encoderISR(0);
}
void encoder1ISR() {
	encoderISR(1);
}

// The interrupts pause while these read, since an UNO R3 reads a long in
// several steps and a pulse in between would garble it.

long readCount(uint8_t i) {
	noInterrupts();
	long count = counts[i];
	interrupts();
	return count;
}

// How fast encoder i's motor turns, in pulses a second: positive the way its
// count goes up, negative the other way
float readSpeed(uint8_t i) {
	noInterrupts();
	unsigned long turnUs = lastTurnUs[i];
	unsigned long lastPulse = lastPulseUs[i];
	int8_t dir = directions[i];
	interrupts();

	unsigned long sinceUs = micros() - lastPulse;
	if (turnUs == 0 || sinceUs > STOPPED_US) {
		return 0;
	}
	// The last motor turn's pulses took turnUs microseconds
	float rate = pulsesPerMotorTurn * 1000000.0 / turnUs;
	// With no pulse for twice the usual gap since, the motor has slowed down
	float slowest = 2000000.0 / sinceUs;
	return dir * min(rate, slowest);
}

// How many pulses a wheel makes rolling this far
long mmToPulses(float mm, float diameter, float ratio) {
	float wheelTurns = mm / (PI * diameter);
	return wheelTurns * ratio * pulsesPerMotorTurn + 0.5;
}

// ---- Watching for a wheel that stopped turning ----

struct Watch {
	long progress;              // where the wheel was last time
	unsigned long movedMs;      // when it last turned
	float mostPower;            // the most power it was asked for since then
	unsigned long morePowerMs;  // when that last went up
};

void restart(Watch &watch, unsigned long now) {
	watch.movedMs = now;
	watch.mostPower = 0;
	watch.morePowerMs = now;
}

// True when the wheel hasn't turned for GIVE_UP_MS while asked to, and the
// power it's asked for hasn't gone up in that time either
bool stuck(Watch &watch, long progress, float power, unsigned long now) {
	if (progress != watch.progress) {
		watch.progress = progress;
		restart(watch, now);
		return false;
	}
	if (fabs(power) > watch.mostPower + 1) {
		watch.mostPower = fabs(power);
		watch.morePowerMs = now;
	}
	return power != 0 && now - watch.movedMs > GIVE_UP_MS &&
		   now - watch.morePowerMs > GIVE_UP_MS;
}

// ---- PID ----

bool isOn(const float gains[3]) {
	return gains[0] != 0 || gains[1] != 0 || gains[2] != 0;
}

void setGains(float gains[3], float kp, float ki, float kd) {
	if (kp < 0 || ki < 0 || kd < 0) {
		return;
	}
	gains[0] = kp;
	gains[1] = ki;
	gains[2] = kd;
}

// One step of a PID controller with gains kp, ki, kd: from the error and how
// fast the error is changing (per second), the output, between low and high.
// integral is the controller's memory from one step to the next, already
// times ki. It only grows while the output isn't stuck at a limit, so it
// can't wind up.
float pidStep(const float gains[3], float &integral, float error,
			  float errorRate, float low, float high) {
	float newIntegral = constrain(integral + gains[1] * error * DT, low, high);
	float output = gains[0] * error + newIntegral + gains[2] * errorRate;
	if (output > high) {
		if (error < 0) {
			integral = newIntegral;  // only if it brings the output back
		}
		return high;
	}
	if (output < low) {
		if (error > 0) {
			integral = newIntegral;
		}
		return low;
	}
	integral = newIntegral;
	return output;
}
}  // namespace

// The encoder settings, shared by all objects. Defined here, with the encoder
// code, so only sketches that use it link them (library.properties sets
// dot_a_linkage).
bool SnappyXOShield::_encodersOn = false;
// The robot's measurements: 0 until the sketch sets them
float SnappyXOShield::_wheelDiameter = 0;  // mm
float SnappyXOShield::_wheelSpacing = 0;   // mm
float SnappyXOShield::_gearRatio = 0;
float SnappyXOShield::_ratedRPM = 0;  // the wheel's speed at full power
float SnappyXOShield::_speedGains[3];  // all 0: off
float SnappyXOShield::_headingGains[3];
float SnappyXOShield::_positionGains[3];
// Where showMoves() shows the moves, if anywhere
void (*SnappyXOShield::_showMove)(const MoveStatus &move) = nullptr;
Print *SnappyXOShield::_output = nullptr;

void SnappyXOShield::beginEncoders() {
	// This shield version's encoder pins. Their A pins must be able to
	// interrupt the sketch on this board.
	const PinMap *pins = _pinMap();
	const OutputPins *outputs[2] = {&pins->left, &pins->right};
	for (uint8_t i = 0; i < 2; i++) {
		aPins[i] = pgm_read_byte(&outputs[i]->encoderA);
		bPins[i] = pgm_read_byte(&outputs[i]->encoderB);
#ifdef NOT_AN_INTERRUPT
		// Cores that know which pins can't interrupt say so. The UNO R4's
		// core takes any pin here.
		if (digitalPinToInterrupt(aPins[i]) == NOT_AN_INTERRUPT) {
			return;  // the encoders stay off, and the moves return false
		}
#endif
	}
	for (uint8_t i = 0; i < 2; i++) {
		// Pulled up, so an unplugged encoder reads nothing instead of random
		// pulses
		pinMode(aPins[i], INPUT_PULLUP);
		pinMode(bPins[i], INPUT_PULLUP);
	}
	resetCounts();
	attachInterrupt(digitalPinToInterrupt(aPins[0]), encoder0ISR, RISING);
	attachInterrupt(digitalPinToInterrupt(aPins[1]), encoder1ISR, RISING);
	_encodersOn = true;
}

// Encoder 0 belongs to the motor on the shield's left output (M1), encoder 1
// to the right one (M2). After swapMotors(), the motor on the left output is
// the right wheel, and its encoder with it.
uint8_t SnappyXOShield::_leftEncoder() {
	return _swap ? 1 : 0;
}
uint8_t SnappyXOShield::_rightEncoder() {
	return _swap ? 0 : 1;
}

// The two motors face opposite ways, so rolling forward turns them opposite
// ways too: the left wheel's encoder counts up, the right wheel's down. The
// right wheel's count and speed are flipped to match.

long SnappyXOShield::leftCount() {
	return readCount(_leftEncoder());
}
long SnappyXOShield::rightCount() {
	return -readCount(_rightEncoder());
}

float SnappyXOShield::leftRPM() {
	if (pulsesPerMotorTurn == 0 || _gearRatio == 0) {
		return 0;  // not set
	}
	return readSpeed(_leftEncoder()) * 60 / pulsesPerMotorTurn / _gearRatio;
}
float SnappyXOShield::rightRPM() {
	if (pulsesPerMotorTurn == 0 || _gearRatio == 0) {
		return 0;
	}
	return -readSpeed(_rightEncoder()) * 60 / pulsesPerMotorTurn /
		   _gearRatio;
}

void SnappyXOShield::resetCounts() {
	noInterrupts();
	counts[0] = 0;
	counts[1] = 0;
	interrupts();
}

bool SnappyXOShield::driveInches(float inches, int rpm) {
	return driveCm(inches * 2.54, rpm);
}

bool SnappyXOShield::driveCm(float cm, int rpm) {
	if (!_canMove(false)) {
		stop();
		return false;
	}
	int8_t dir = cm < 0 ? -1 : 1;
	long pulses = mmToPulses(fabs(cm) * 10, _wheelDiameter, _gearRatio);
	return _moveWheels(pulses, dir, dir, rpm);
}

bool SnappyXOShield::spinTurn(float degrees, int rpm) {
	if (!_canMove(true)) {
		stop();
		return false;
	}
	// Spinning in place, each wheel rolls around a circle as wide as the wheel
	// spacing
	float mm = fabs(degrees) / 360 * PI * _wheelSpacing;
	// Turning right, the left wheel rolls forward and the right one backward
	int8_t dir = degrees < 0 ? -1 : 1;
	long pulses = mmToPulses(mm, _wheelDiameter, _gearRatio);
	return _moveWheels(pulses, dir, -dir, rpm);
}

bool SnappyXOShield::pivotTurn(float degrees, int rpm) {
	if (!_canMove(true)) {
		stop();
		return false;
	}
	// The moving wheel rolls around the still one, on a circle twice as wide
	// as the wheel spacing
	float mm = fabs(degrees) / 360 * 2 * PI * _wheelSpacing;
	long pulses = mmToPulses(mm, _wheelDiameter, _gearRatio);
	// Turning right, the left wheel rolls forward around the right one
	if (degrees < 0) {
		return _moveWheels(pulses, 0, 1, rpm);
	}
	return _moveWheels(pulses, 1, 0, rpm);
}

void SnappyXOShield::setWheelDiameter(float mm) {
	if (mm > 0) {
		_wheelDiameter = mm;
	}
}
void SnappyXOShield::setWheelSpacing(float mm) {
	if (mm > 0) {
		_wheelSpacing = mm;
	}
}
void SnappyXOShield::setGearRatio(float ratio) {
	if (ratio > 0) {
		_gearRatio = ratio;
	}
}
void SnappyXOShield::setEncoderPulses(int pulses) {
	if (pulses > 0) {
		// The interrupts read it, and an UNO R3 writes it in two steps
		noInterrupts();
		pulsesPerMotorTurn = pulses;
		interrupts();
	}
}
void SnappyXOShield::setRatedRPM(float rpm) {
	if (rpm > 0) {
		_ratedRPM = rpm;
	}
}

void SnappyXOShield::setSpeedPID(float kp, float ki, float kd) {
	setGains(_speedGains, kp, ki, kd);
}
void SnappyXOShield::setHeadingPID(float kp, float ki, float kd) {
	setGains(_headingGains, kp, ki, kd);
}
void SnappyXOShield::setPositionPID(float kp, float ki, float kd) {
	setGains(_positionGains, kp, ki, kd);
}
void SnappyXOShield::showMoves(Print &output) {
	_output = &output;
	_showMove = _printMove;
}
void SnappyXOShield::showMoves(void (*show)(const MoveStatus &move)) {
	_showMove = show;
}

void SnappyXOShield::_printMove(const MoveStatus &move) {
	_output->print(F("remaining:"));
	_output->print(move.remaining);
	_output->print(F(",ahead:"));
	_output->print(move.ahead);
	// Whole numbers, which is precise enough to plot. Speeds in RPM.
	_output->print(F(",leftTarget:"));
	_output->print(lround(move.leftTarget));
	_output->print(F(",leftSpeed:"));
	_output->print(lround(move.leftSpeed));
	_output->print(F(",rightTarget:"));
	_output->print(lround(move.rightTarget));
	_output->print(F(",rightSpeed:"));
	_output->print(lround(move.rightSpeed));
	_output->print(F(",leftPower:"));
	_output->print(move.leftPower);
	_output->print(F(",rightPower:"));
	_output->println(move.rightPower);
}

bool SnappyXOShield::_canMove(bool turning) {
	// Without the speed controller, the moves guess the power from the rated
	// RPM
	return _encodersOn && _wheelDiameter > 0 && _gearRatio > 0 &&
		   pulsesPerMotorTurn > 0 && (_wheelSpacing > 0 || !turning) &&
		   (_ratedRPM > 0 || isOn(_speedGains));
}

// Turns the wheels by pulses, each its own way (1 forward, -1 backward, 0
// stays still), then brakes. The moves call it once _canMove() is true.
//
// Every CONTROL_MS, up to three layers of control, all in wheel RPM:
//   position: how fast to go, from the pulses still to go. Without it, rpm
//             until the distance, then brake.
//   heading:  how much speed to take off the wheel that's ahead and give the
//             other, from how many pulses it's ahead. Only with both wheels
//             moving.
//   speed:    a speed controller on each wheel finds the power that holds
//             the speed asked for. Without it, the power is guessed from
//             setRatedRPM(): a real robot is usually slower, more so with a
//             low battery or a heavy load.
bool SnappyXOShield::_moveWheels(long pulses, int8_t leftDir, int8_t rightDir,
								 int rpm) {
	bool positionOn = isOn(_positionGains);
	bool headingOn = isOn(_headingGains) && leftDir != 0 && rightDir != 0;
	bool speedOn = isOn(_speedGains);

	// The fastest the position and heading controllers ask a wheel to go
	float limit = abs(rpm);
	// Turns pulses every 100 ms into wheel RPM
	float toRPM = 600.0 / (pulsesPerMotorTurn * _gearRatio);

	// Each controller's memory, starting over for every move
	float positionIntegral = 0, headingIntegral = 0;
	float leftSpeedIntegral = 0, rightSpeedIntegral = 0;

	// The last 100 ms of each wheel's progress and speed, one entry per
	// control step, with oldest the one from 100 ms ago
	long leftHistory[SPEED_STEPS] = {0}, rightHistory[SPEED_STEPS] = {0};
	float leftSpeedHistory[SPEED_STEPS] = {0};
	float rightSpeedHistory[SPEED_STEPS] = {0};
	uint8_t oldest = 0;

	long leftStart = leftCount();
	long rightStart = rightCount();
	unsigned long startMs = millis();
	Watch leftWatch = {0, startMs, 0, startMs};
	Watch rightWatch = {0, startMs, 0, startMs};
	bool arrived = false;                 // within SETTLE_PULSES, or past
	unsigned long arrivedMs = 0;          // when that first happened
	bool settling = false;                // within SETTLE_PULSES now
	unsigned long settlingMs = 0;         // since when
	uint8_t showStep = 0;
	unsigned long lastControlMs = millis() - CONTROL_MS;

	while (true) {
		unsigned long now = millis();
		if (now - lastControlMs < CONTROL_MS) {
			continue;
		}
		lastControlMs = now;

		// How far each wheel has gone the way it should, in pulses
		long left = leftDir * (leftCount() - leftStart);
		long right = rightDir * (rightCount() - rightStart);

		// How fast: the pulses it made in the last 100 ms. And how fast that
		// changed over those 100 ms, per second.
		float leftSpeed = left - leftHistory[oldest];
		float rightSpeed = right - rightHistory[oldest];
		float leftSpeedChange =
			(leftSpeed - leftSpeedHistory[oldest]) / SPEED_SECONDS;
		float rightSpeedChange =
			(rightSpeed - rightSpeedHistory[oldest]) / SPEED_SECONDS;
		leftHistory[oldest] = left;
		rightHistory[oldest] = right;
		leftSpeedHistory[oldest] = leftSpeed;
		rightSpeedHistory[oldest] = rightSpeed;
		oldest = (oldest + 1) % SPEED_STEPS;

		long done;       // how far the moving wheels have gone
		float doneSpeed;  // and how fast, in pulses every 100 ms
		if (leftDir == 0) {
			done = right;
			doneSpeed = rightSpeed;
		} else if (rightDir == 0) {
			done = left;
			doneSpeed = leftSpeed;
		} else {
			done = (left + right) / 2;
			doneSpeed = (leftSpeed + rightSpeed) / 2;
		}
		long remaining = pulses - done;

		if (remaining <= SETTLE_PULSES && !arrived) {
			arrived = true;
			arrivedMs = now;
		}

		// Done yet?
		bool holding = false;  // within SETTLE_PULSES: hold still
		if (!positionOn) {
			if (remaining <= 0) {
				break;
			}
		} else if (labs(remaining) <= SETTLE_PULSES) {
			if (!settling) {
				settling = true;
				settlingMs = now;
			}
			if (now - settlingMs >= SETTLE_MS) {
				break;
			}
			holding = true;
		} else {
			settling = false;
			if (now - arrivedMs > SETTLE_GIVE_UP_MS && arrived) {
				stop();  // rolling back and forth past the target
				return false;
			}
		}

		float leftCommand = 0, rightCommand = 0;  // each wheel's target RPM
		float leftPower = 0, rightPower = 0;
		if (holding) {
			brake();
			// The speed controllers start over if the robot rolls out again
			leftSpeedIntegral = 0;
			rightSpeedIntegral = 0;
			restart(leftWatch, now);
			restart(rightWatch, now);
		} else {
			// A wheel going the wrong way: its encoder counts backward
			if ((leftDir != 0 && left < -WRONG_WAY_PULSES) ||
				(rightDir != 0 && right < -WRONG_WAY_PULSES)) {
				stop();
				return false;
			}

			// Position
			float command = limit;
			if (positionOn) {
				command = pidStep(_positionGains, positionIntegral, remaining,
								  -doneSpeed / SPEED_SECONDS, -limit, limit);
			}

			// Heading
			float steer = 0;
			if (headingOn) {
				steer = pidStep(_headingGains, headingIntegral, left - right,
								(leftSpeed - rightSpeed) / SPEED_SECONDS,
								-limit, limit);
			}

			leftCommand = leftDir != 0 ? command - steer : 0;
			rightCommand = rightDir != 0 ? command + steer : 0;
			if (headingOn) {
				// A wheel past the limit can't go faster, so both come down
				// by the excess, keeping the difference between them
				float over = max(leftCommand, rightCommand) - limit;
				if (over > 0) {
					leftCommand -= over;
					rightCommand -= over;
				}
				float under = -limit - min(leftCommand, rightCommand);
				if (under > 0) {
					leftCommand += under;
					rightCommand += under;
				}
				leftCommand = constrain(leftCommand, -limit, limit);
				rightCommand = constrain(rightCommand, -limit, limit);
			}

			// Speed: measured, or guessed
			if (speedOn) {
				if (leftDir != 0) {
					leftPower = pidStep(_speedGains, leftSpeedIntegral,
										leftCommand - leftSpeed * toRPM,
										-leftSpeedChange * toRPM, -255, 255);
				}
				if (rightDir != 0) {
					rightPower = pidStep(_speedGains, rightSpeedIntegral,
										 rightCommand - rightSpeed * toRPM,
										 -rightSpeedChange * toRPM, -255, 255);
				}
			} else {
				leftPower = constrain(leftCommand / _ratedRPM * 255, -255, 255);
				rightPower =
					constrain(rightCommand / _ratedRPM * 255, -255, 255);
			}
			setMotors(leftDir * (int)leftPower, rightDir * (int)rightPower);

			// A wheel that should turn but doesn't
			if ((leftDir != 0 && stuck(leftWatch, left, leftPower, now)) ||
				(rightDir != 0 && stuck(rightWatch, right, rightPower, now))) {
				stop();
				return false;
			}
		}

		if (_showMove != nullptr && ++showStep >= SHOW_EVERY) {
			showStep = 0;
			MoveStatus move;
			move.remaining = remaining;
			move.ahead = left - right;
			move.leftTarget = leftCommand;
			move.leftSpeed = leftSpeed * toRPM;
			move.rightTarget = rightCommand;
			move.rightSpeed = rightSpeed * toRPM;
			move.leftPower = lround(leftPower);
			move.rightPower = lround(rightPower);
			_showMove(move);
		}
	}

	brake();
	delay(100);  // until the robot stands still
	stop();
	return true;
}
