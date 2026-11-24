// Tunes your robot's PID controllers from the MicroBlue app, using the Arduino
// UNO R4 WiFi's built-in Bluetooth. Like PIDSquare, it drives a 40 cm square,
// but you type the gains on your phone and press Run, with no upload in
// between, and a chart shows the wheels' speed during each move. Tune one
// controller at a time, in this order:
//
//   0. Run it with every gain at 0. The wheels turn slower than asked, each
//      move rolls past its target, more the faster it goes, and nothing
//      keeps the wheels together.
//   1. Speed: set the speed PID so the left and right lines on the chart
//      follow the target, whatever the battery and the robot's weight.
//      Without it, the robot only guesses the power and goes slower. Heading
//      works without it, but position needs it.
//   2. Heading: raise its kp until each side is straight and "Left ahead by"
//      stays near 0. Add kd if the robot wobbles, ki if a small difference
//      stays.
//   3. Position: raise its kp until each move stops near its target, with
//      "Past the target" near 0. Add kd if it rolls past and comes back. If
//      it stops short, raise the speed PID's ki.
//   4. Wheel spacing: once each turn stops on its target, if the robot still
//      turns too far or too little, set the wheel spacing to the spacing in
//      use times 90, divided by the angle it turned. Tires slip a little as
//      the robot spins, so it's often a few mm more than what you measure.
//
// Then copy your values into setup() in your other sketches: a "Speed PID in
// use" of 4, 13, 0 is shield.setSpeedPID(4, 13, 0); and a "Wheel spacing in
// use" of 101 is shield.setWheelSpacing(101);
//
// Starting points to try, not the answer for every robot: speed 4, 13, 0,
// then heading 0.3 and position 0.2. Without the speed controller, heading
// takes a bigger kp: about 1.2.
//
// The app's layout for this sketch is in this library's README, as a QR code
// to scan with Layouts -> + -> Scan QR Code. Or build it on the Panel:
//
//   Button "Run the square", ID run
//   Chart "Wheel speed: target, left, right", ID wheels, unit RPM
//   Value "Past the target", ID past, unit pulses
//   Value "Left ahead by", ID ahead, unit pulses
//   Value "RPM in use" and Text "Set RPM", both ID rpm
//   Value "Speed PID in use" and Text "Set speed PID", both ID speed
//   Value "Heading PID in use" and Text "Set heading PID", both ID heading
//   Value "Position PID in use" and Text "Set position PID", both ID position
//   Value "Wheel spacing in use", unit mm, and Text "Set wheel spacing", both
//     ID spacing
//
// Type each PID's gains as up to three numbers, kp, ki and kd: 4, 13, 0.
// Left-out ones are 0, so 0.3 is kp 0.3. Type the wheel spacing in mm: 101.5.
// The Values show what the robot uses, after a reset too. Settings sent during
// a square count from the next one.
// The Console keeps every move's result, and Record there saves them as a
// table.
//
// The app can't stop the robot in the middle of a square: if the app
// disconnects or goes in the background, the square stops after the move it's
// on. To stop it at once, press the board's reset button.
//
// Wiring, the same on the v3 and v2 shield, by the labels on each encoder's port:
//   left wheel (M1):  A to pin 2, B to pin A2
//   right wheel (M2): A to pin 3, B to pin A3
//   5V (or VCC) to 5V, GND to GND
// Run the EncoderTest example first: it checks the wiring and says which of
// the lines in setup() below to uncomment.
//
// Requires the MicroBlue library 2.1 or later -> install it via Sketch -> Include Library -> Manage Libraries.
//
// Developed by A+ Mobile Solutions Inc
// Licensed under the MIT License. See LICENSE for details.

#include <MicroBlue.h>
#include <SnappyXOShield.h>

MicroBlueManager manager;  // talks to the MicroBlue app over the built-in Bluetooth
SnappyXOShield shield(SnappyXOShield::V3);  // the shield version printed on the board: V3 or V2

// Set from the app. Each PID's gains are kp, ki and kd, all 0 to start: off.
float speedGains[3], headingGains[3], positionGains[3];
int rpm = 60;             // the wheels' speed, in turns per minute
float wheelSpacing = 96;  // from the middle of one wheel to the middle of the other, in mm

// The robot's other measurements
const float WHEEL_DIAMETER_MM = 61;
const float GEAR_RATIO = 120;           // how many times the motor turns for each turn of the wheel
const int PULSES_PER_MOTOR_TURN = 8;    // how many pulses the encoder gives for each turn of the motor: its PPR
const float PULSES_PER_WHEEL_TURN = PULSES_PER_MOTOR_TURN * GEAR_RATIO;
const float RATED_RPM = 160;            // the wheel's speed at full power, from the motor's datasheet

const float SIDE_CM = 40;          // the square's sides
const int MIN_RPM = 10;            // the slowest the app can set; the fastest is RATED_RPM
const float MIN_SPACING_MM = 40;   // the narrowest wheel spacing the app can set
const float MAX_SPACING_MM = 300;  // and the widest

// The app clears its Values when it connects, and listens only a moment
// later. So the robot sends what it uses once a second, this many times.
const int REPORTS_AFTER_CONNECTING = 5;
int reportsLeft = 0;
unsigned long lastReportMs = 0;
bool wasConnected = false;

int runs = 0;  // squares driven since reset

void setup() {
  Serial.begin(9600);      // shows each move's result too, with the USB cable plugged in
  shield.begin();          // sets up the motor pins
  shield.beginEncoders();  // starts counting the encoders' pulses

  // Uncomment the lines EncoderTest printed for this robot:
  // shield.invertLeft();
  // shield.invertRight();
  // shield.swapMotors();

  shield.setWheelDiameter(WHEEL_DIAMETER_MM);
  shield.setWheelSpacing(wheelSpacing);
  shield.setGearRatio(GEAR_RATIO);
  shield.setEncoderPulses(PULSES_PER_MOTOR_TURN);
  shield.setRatedRPM(RATED_RPM);

  shield.showMoves(sendToChart);  // the app's chart, during each move

  manager.begin("SnappyXO Robot");  // the name the app shows when you connect: change it to tell robots apart
}

void loop() {
  bool connected = manager.isConnected();
  if (connected && !wasConnected) {
    reportsLeft = REPORTS_AFTER_CONNECTING;
    lastReportMs = millis();
  }
  wasConnected = connected;
  if (!connected) {
    return;
  }

  if (reportsLeft > 0 && millis() - lastReportMs >= 1000) {
    sendSettings();
    reportsLeft--;
    lastReportMs = millis();
  }

  MicroBlueMessage msg = manager.read();  // the latest message from the app, if any
  if (msg.id == "run") {
    if (msg.value == "1") {  // pressed; "0" is let go
      driveSquare();
    }
  } else {
    changeSetting(msg);
  }
}

// Drives the square with the settings from the app, and shows how each move
// went
void driveSquare() {
  runs++;
  sendSettings();  // so the Console keeps them with the results
  say("Run " + String(runs) + " at " + rpm + " RPM");

  long sidePulses = mmToPulses(SIDE_CM * 10);
  // Spinning 90 degrees, each wheel rolls a quarter of a circle as wide as the
  // wheel spacing
  long turnPulses = mmToPulses(PI * wheelSpacing / 4);

  for (int side = 1; side <= 4; side++) {
    shield.resetCounts();
    bool finished = shield.driveCm(SIDE_CM, rpm);
    showResults("Side", side, sidePulses, finished);
    if (!appStillThere()) {
      break;
    }
    delay(500);

    shield.resetCounts();
    finished = shield.spinTurn(-90, rpm);  // spin left
    showResults("Turn", side, turnPulses, finished);
    if (!appStillThere()) {
      break;
    }
    delay(500);
  }

  // What the app sent during the square counts now, except pressing Run again
  for (MicroBlueMessage msg = manager.read(); msg.hasId(); msg = manager.read()) {
    if (msg.id != "run") {
      changeSetting(msg);
    }
  }
}

// False, after saying so, once the app has disconnected or its heartbeat has
// stopped (the phone out of range, the app closed or in the background)
bool appStillThere() {
  if (manager.isConnected() && !manager.heartbeatLost()) {
    return true;
  }
  Serial.println(F("Stopped: the app is gone"));
  return false;
}

// Sends the wheels' speeds to the app's chart, ten times a second during each
// move: the speed asked for, then how fast the left and right wheels turn, in
// RPM. It runs in the middle of the move, so it only sends.
void sendToChart(const MoveStatus &move) {
  // The speed asked for: the same for both wheels, but for the heading
  // controller's difference
  long target = lround((move.leftTarget + move.rightTarget) / 2);
  manager.write("wheels", String(target) + "," + lround(move.leftSpeed) + "," + lround(move.rightSpeed));
}

// Shows how a move went, in the app and on the Serial Monitor. The move has
// just finished.
void showResults(const char *move, int number, long target, bool finished) {
  delay(300);  // until the robot stands still, counting how far it rolls on
  long left = shield.leftCount();
  long right = shield.rightCount();
  left = abs(left);  // spinning left, the left wheel counts backward
  right = abs(right);
  long past = (left + right) / 2 - target;
  long ahead = left - right;

  manager.write("past", past);
  manager.write("ahead", ahead);
  String line = String(move) + " " + number + ": target " + target + ", left " + left + ", right " + right +
                ", past " + past + ", left ahead " + ahead;
  if (!finished) {
    line += " (gave up)";
  }
  say(line);
}

// Shows a line in the app's Console and on the Serial Monitor. Keep it under
// about 90 characters, for the app.
void say(const String &line) {
  manager.write("log", line);
  Serial.println(line);
}

// Sends what the robot uses to the app's Values
void sendSettings() {
  manager.write("rpm", rpm);
  manager.write("speed", gainsText(speedGains));
  manager.write("heading", gainsText(headingGains));
  manager.write("position", gainsText(positionGains));
  manager.write("spacing", shortNumber(wheelSpacing));
}

// Takes a new setting from one of the app's Text boxes, or says what's wrong
// with it
void changeSetting(const MicroBlueMessage &msg) {
  if (msg.id == "rpm") {
    if (readRPM(msg.value)) {
      manager.write("rpm", rpm);
    } else {
      say("Type the RPM as a whole number from " + String(MIN_RPM) + " to " + String((int)RATED_RPM));
    }
  } else if (msg.id == "speed") {
    if (changeGains(msg, speedGains)) {
      shield.setSpeedPID(speedGains[0], speedGains[1], speedGains[2]);
    }
  } else if (msg.id == "heading") {
    if (changeGains(msg, headingGains)) {
      shield.setHeadingPID(headingGains[0], headingGains[1], headingGains[2]);
    }
  } else if (msg.id == "position") {
    if (changeGains(msg, positionGains)) {
      shield.setPositionPID(positionGains[0], positionGains[1], positionGains[2]);
    }
  } else if (msg.id == "spacing") {
    if (readSpacing(msg.value)) {
      shield.setWheelSpacing(wheelSpacing);
      manager.write("spacing", shortNumber(wheelSpacing));
    } else {
      say("Type the wheel spacing in mm, from " + String((int)MIN_SPACING_MM) + " to " + String((int)MAX_SPACING_MM));
    }
  }
}

// Reads a PID's new gains from the app into gains, and sends them back to its
// Value. False, after saying what to type, if the text isn't gains.
bool changeGains(const MicroBlueMessage &msg, float gains[3]) {
  if (!readGains(msg.value, gains)) {
    say("Type the " + msg.id + " PID as up to 3 numbers, 0 or more: 4, 13, 0");
    return false;
  }
  manager.write(msg.id, gainsText(gains));
  return true;
}

// Reads the RPM from text like "60": a whole number from MIN_RPM to
// RATED_RPM. False, leaving rpm as it was, if the text isn't that.
bool readRPM(const String &text) {
  const char *start = text.c_str();
  char *end;
  long value = strtol(start, &end, 10);
  while (*end == ' ') {
    end++;
  }
  if (end == start || *end != '\0' || value < MIN_RPM || value > RATED_RPM) {
    return false;
  }
  rpm = value;
  return true;
}

// Reads the wheel spacing from text like "101.5", in mm, kept to 1 decimal.
// False, leaving wheelSpacing as it was, if the text isn't a number from
// MIN_SPACING_MM to MAX_SPACING_MM.
bool readSpacing(const String &text) {
  const char *start = text.c_str();
  char *end;
  float value = strtod(start, &end);
  while (*end == ' ') {
    end++;
  }
  if (end == start || *end != '\0' || isnan(value) || value < MIN_SPACING_MM || value > MAX_SPACING_MM) {
    return false;
  }
  wheelSpacing = round(value * 10) / 10;
  return true;
}

// Reads up to three gains, kp, ki and kd, from text like "4, 13, 0" or "0.3"
// into gains, the left-out ones 0. False, leaving gains as they were, if the
// text isn't that.
bool readGains(const String &text, float gains[3]) {
  float read[3] = {0, 0, 0};
  int count = 0;
  const char *next = text.c_str();
  while (true) {
    while (*next == ' ' || *next == ',') {  // between the numbers
      next++;
    }
    if (*next == '\0') {
      break;
    }
    if (count == 3 || !(isDigit(*next) || *next == '.')) {
      return false;  // a fourth number, a negative one, or not a number
    }
    char *end;
    float gain = strtod(next, &end);
    if (end == next || isinf(gain)) {
      return false;
    }
    // Kept to 4 decimals, as the app's Value shows it
    read[count] = round(gain * 10000) / 10000;
    count++;
    next = end;
  }
  if (count == 0) {
    return false;
  }
  for (int i = 0; i < 3; i++) {
    gains[i] = read[i];
  }
  return true;
}

// Gains as you'd type them: "4, 13, 0"
String gainsText(const float gains[3]) {
  return shortNumber(gains[0]) + ", " + shortNumber(gains[1]) + ", " + shortNumber(gains[2]);
}

// A number with up to 4 decimals and no 0s at the end: 0.25, not 0.2500
String shortNumber(float value) {
  String text = String(value, 4);
  while (text.endsWith("0")) {
    text.remove(text.length() - 1);
  }
  if (text.endsWith(".")) {
    text.remove(text.length() - 1);
  }
  return text;
}

// How many pulses a wheel makes rolling this far
long mmToPulses(float mm) {
  float wheelTurns = mm / (PI * WHEEL_DIAMETER_MM);
  return wheelTurns * PULSES_PER_WHEEL_TURN + 0.5;
}
