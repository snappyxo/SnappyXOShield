// Sets up a robot with encoder motors. It turns each wheel, asks you what you
// saw, and works out what's wired right and what isn't. It also turns each
// wheel backward, to check that its encoder tells the two ways apart. At the
// end it prints the lines your sketches need in setup(), if any.
//
// Lift the robot so its wheels turn freely, then open the Serial Monitor at
// 9600 baud. Answer each question by typing a letter in the Serial Monitor's
// message box and pressing Enter.
//
// Wiring, the same on the v3 and v2 shield, by the labels on each encoder's port:
//   left wheel (M1):  A to pin 2, B to pin A2
//   right wheel (M2): A to pin 3, B to pin A3
//   5V (or VCC) to 5V, GND to GND
// A2 and A3 are the board's pins of that name, not the encoder's A.
//
// Developed by A+ Mobile Solutions Inc
// Licensed under the MIT License. See LICENSE for details.

#include <SnappyXOShield.h>

SnappyXOShield shield(SnappyXOShield::V3);  // the shield version printed on the board: V3 or V2

// What the test found this robot needs, printed at the end
bool swapped = false;
bool invertedLeft = false;
bool invertedRight = false;

void setup() {
  Serial.begin(9600);
  shield.begin();          // sets up the motor pins
  shield.beginEncoders();  // starts counting the encoders' pulses

  // The motor's numbers, from its datasheet, for the speed the test shows
  shield.setGearRatio(120);    // how many times the motor turns for each turn of the wheel
  shield.setEncoderPulses(8);  // how many pulses the encoder gives for each turn of the motor: its PPR

  waitForStart();
  Serial.println(F("Forward is the way that would roll the robot ahead."));

  if (checkWheel(true) && checkWheel(false)) {
    printSetupLines();
  }
  Serial.println();
  Serial.println(F("Press the board's reset button to run the test again."));
}

void loop() {
  // Nothing to repeat: press reset to run the test again
}

// Turns one wheel until it rolls forward and its encoder counts forward,
// fixing what the library can along the way. Returns false after saying what
// to rewire, when the wiring needs changing.
bool checkWheel(bool left) {
  const char *name = left ? "left" : "right";
  const char *otherName = left ? "right" : "left";
  bool invertedThisWheel = false;
  bool doubtedEncoder = false;

  while (true) {
    // The shield output and encoder pins this wheel should be on. After
    // swapMotors(), they're the other side's.
    bool onLeftSide = left != swapped;
    const char *aPin = onLeftSide ? "2" : "3";
    const char *bPin = onLeftSide ? "A2" : "A3";

    Serial.println();
    Serial.print(F("Watch the "));
    Serial.print(name);
    Serial.println(F(" wheel..."));
    delay(1500);

    // Turns it forward for a second
    shield.resetCounts();
    shield.setMotors(left ? 150 : 0, left ? 0 : 150);
    delay(1000);
    float rpm = left ? shield.leftRPM() : shield.rightRPM();  // measured while it's still turning
    shield.stop();
    delay(500);  // until it stands still
    long count = left ? shield.leftCount() : shield.rightCount();
    long otherCount = left ? shield.rightCount() : shield.leftCount();

    Serial.println(F("Which wheel turned, and which way? Type a letter, then press Enter:"));
    Serial.print(F("  f  the "));
    Serial.print(name);
    Serial.println(F(" wheel, forward"));
    Serial.print(F("  b  the "));
    Serial.print(name);
    Serial.println(F(" wheel, backward"));
    Serial.print(F("  o  the "));
    Serial.print(otherName);
    Serial.println(F(" wheel"));
    Serial.println(F("  n  neither"));
    Serial.println(F("  a  I missed it: turn it again"));
    char answer = ask("fbona");

    if (answer == 'a') {
      continue;
    }

    if (answer == 'n') {
      Serial.print(F("Check the motor plugged into the shield's "));
      Serial.print(onLeftSide ? F("left output (M1)") : F("right output (M2)"));
      Serial.println(F(", and the battery. Then press reset."));
      return false;
    }

    if (answer == 'o') {
      if (left && !swapped) {
        Serial.println(F("The motors are plugged in the other way round: the left wheel's into M2. shield.swapMotors() swaps them back, and their encoders with them. Trying again with it."));
        shield.swapMotors();
        swapped = true;
        continue;
      }
      Serial.println(F("Plug the left wheel's motor into M1, the shield's left output, and the right wheel's into M2. Then press reset."));
      return false;
    }

    // The wheel rolled forward (f) or backward (b)
    int8_t rolled = answer == 'f' ? 1 : -1;

    if (rolled < 0 && invertedThisWheel) {
      // Turned around since last time, so it can't have rolled backward twice
      Serial.println(F("It turned the other way this time than last time. Watch it once more."));
      continue;
    }

    // Did the encoder see it?
    if (abs(otherCount) > 100 && abs(otherCount) > abs(count)) {
      if (swapped) {
        Serial.println(F("The motors are plugged in the other way round, but their encoders aren't. Swap the two motors' plugs on the shield, then press reset."));
      } else {
        Serial.print(F("The "));
        Serial.print(name);
        Serial.print(F(" wheel's encoder is wired to the "));
        Serial.print(otherName);
        Serial.println(F(" wheel's pins. Swap the encoder wires on pins 2 and A2 with the ones on pins 3 and A3, then press reset."));
      }
      return false;
    }
    if (abs(count) <= 100) {
      Serial.print(F("No pulses from the "));
      Serial.print(name);
      Serial.print(F(" wheel's encoder. Check that its A goes to pin "));
      Serial.print(aPin);
      Serial.println(F(", its 5V to 5V and its GND to GND. Then press reset."));
      return false;
    }
    if ((count > 0 ? 1 : -1) != rolled) {
      if (!doubtedEncoder) {
        // Before blaming the wiring, make sure which way the wheel turned
        doubtedEncoder = true;
        Serial.println(F("Its encoder counted the other way. Watch it once more, to be sure which way it turns."));
        continue;
      }
      // With A and B swapped, it counts the wrong way both ways round. With
      // B not connected, it counts the same way whichever way it turns.
      long backCount = turnBackward(left);
      if ((backCount > 0) == (count > 0) || abs(backCount) <= 100) {
        printBNotConnected(name, bPin);
        return false;
      }
      Serial.print(F("The "));
      Serial.print(name);
      Serial.print(F(" wheel's encoder counts the wrong way: its A and B are swapped. A goes to pin "));
      Serial.print(aPin);
      Serial.print(F(", B to pin "));
      Serial.print(bPin);
      Serial.println(F(". Fix that, then press reset."));
      return false;
    }

    if (rolled < 0) {
      // The motor's on backward: reverse the shield output it's on
      if (onLeftSide) {
        shield.invertLeft();
        invertedLeft = true;
      } else {
        shield.invertRight();
        invertedRight = true;
      }
      invertedThisWheel = true;
      Serial.print(F("Its encoder is wired right. The motor turns backward, which shield."));
      Serial.print(onLeftSide ? "invertLeft()" : "invertRight()");
      Serial.println(F(" fixes. Trying again with it."));
      continue;
    }

    // It counts forward going forward. Going backward, it should count
    // backward: not if its B isn't connected.
    if (turnBackward(left) >= -100) {
      printBNotConnected(name, bPin);
      return false;
    }

    Serial.print(F("OK: the "));
    Serial.print(name);
    Serial.print(F(" wheel rolls forward, and its encoder counted "));
    Serial.print(count);
    Serial.print(F(" pulses forward at "));
    Serial.print(rpm);
    Serial.println(F(" RPM, and backward going backward."));
    return true;
  }
}

// Turns one wheel backward for a second, and returns how far its encoder
// counted
long turnBackward(bool left) {
  Serial.println(F("Now backward..."));
  shield.resetCounts();
  shield.setMotors(left ? -150 : 0, left ? 0 : -150);
  delay(1000);
  shield.stop();
  delay(500);  // until it stands still
  return left ? shield.leftCount() : shield.rightCount();
}

void printBNotConnected(const char *name, const char *bPin) {
  Serial.print(F("The "));
  Serial.print(name);
  Serial.print(F(" wheel's encoder counts the same way whichever way the wheel turns: its B isn't connected. Check the wire from its B to pin "));
  Serial.print(bPin);
  Serial.println(F(", then press reset."));
}

// What the sketches for this robot need, from what the test found
void printSetupLines() {
  Serial.println();
  if (!swapped && !invertedLeft && !invertedRight) {
    Serial.println(F("All set: the motors and encoders are wired the standard way, so your sketches need nothing more in setup()."));
    return;
  }
  Serial.println(F("All set. Add these lines to setup() in your sketches, after shield.begin():"));
  if (swapped) {
    Serial.println(F("  shield.swapMotors();"));
  }
  if (invertedLeft) {
    Serial.println(F("  shield.invertLeft();"));
  }
  if (invertedRight) {
    Serial.println(F("  shield.invertRight();"));
  }
}

// Repeats the first instructions until something is typed, in case the
// Serial Monitor was opened after they were printed
void waitForStart() {
  unsigned long lastPromptMs = millis() - 5000;
  while (Serial.available() == 0) {
    if (millis() - lastPromptMs >= 5000) {
      lastPromptMs = millis();
      Serial.println(F("Lift the robot so its wheels turn freely, then type s and press Enter to start."));
    }
  }
}

// Waits until one of the letters in choices is typed, and returns it
char ask(const char *choices) {
  while (Serial.available() > 0) {
    Serial.read();  // drops anything typed before the question
  }
  while (true) {
    if (Serial.available() > 0) {
      char c = Serial.read();
      if (c >= 'A' && c <= 'Z') {
        c += 'a' - 'A';  // F works as well as f
      }
      if (c != '\0' && strchr(choices, c) != NULL) {
        Serial.print(F("> "));
        Serial.println(c);  // what you typed, for the record
        return c;
      }
      if (c != '\n' && c != '\r' && c != ' ') {
        Serial.println(F("Type one of the letters above, then press Enter."));
      }
    }
  }
}
