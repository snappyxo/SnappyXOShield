// Drives a two-wheeled robot forward, backward and around in a loop.
// Also a quick check that both motors turn the right way.

#include <SnappyXOShield.h>

SnappyXOShield shield(SnappyXOShield::V3);  // the shield version printed on the board: V3 or V2

void setup() {
  shield.begin();  // sets up the motor pins

  // If a wheel turns the wrong way, uncomment the line that fixes it:
  // shield.invertLeft();   // the left motor turns backward when it should go forward
  // shield.invertRight();  // the right motor turns backward when it should go forward
  // shield.swapMotors();   // the left and right motors are plugged in the other way round
}

void loop() {
  shield.forward();  // both wheels forward at full speed (255)
  delay(1000);

  shield.reverse(150);  // both wheels backward at speed 150 (0-255)
  delay(1000);

  shield.spinLeft();  // left wheel backward, right wheel forward: spins left in place
  delay(500);

  shield.spinRight();  // left wheel forward, right wheel backward: spins right in place
  delay(500);

  shield.setMotors(255, 100);  // each wheel's speed, -255 to 255: left faster, so it curves right
  delay(1000);

  shield.drive(200, -100);  // joystick-style: throttle 200 (forward), steering -100 (left)
  delay(1000);

  shield.stop();  // cuts the power, the robot coasts to a stop
  delay(1000);
}
