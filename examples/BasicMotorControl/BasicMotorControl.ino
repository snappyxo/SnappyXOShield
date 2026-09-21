#include <SnappyXOShield.h>

void setup() {
  // initMotors() sets all motor pins as outputs and configures inversion options.
  //
  // initMotors(invertL, invertR, invertLR)
  //   invertL   - set true to reverse the left motor direction
  //   invertR   - set true to reverse the right motor direction
  //   invertLR  - set true to swap the left and right motors entirely
  //
  // All parameters are optional and default to false.
  // Examples:
  //   initMotors()                     - no inversion. Probably what you want if you copied the examples exactly.
  //   initMotors(true, false, false)   - left motor reversed
  //   initMotors(false, true, false)   - right motor reversed
  //   initMotors(false, false, true)   - left and right motors swapped
  initMotors();
}

void loop() {
  forward();    // both motors full speed forward (255)
  delay(1000);

  reverse();    // both motors full speed backward (255)
  delay(1000);

  turnLeft();   // left motor backward, right motor forward - spins left in place
  delay(500);

  turnRight();  // left motor forward, right motor backward - spins right in place
  delay(500);

  pause();      // coasting stop - cuts power, motors slow down gradually
  delay(1000);
}
