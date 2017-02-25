#include "DIYLEDScroller.h"

int led2 = D7;

//Setup Entry Point
void setup(){
  pinMode(led2, OUTPUT);
}


//Main Loop Entry Point
void loop(){
  digitalWrite(led2, HIGH);

  // We'll leave it on for 1 second...
  delay(500);

  // Then we'll turn it off...
  digitalWrite(led2, LOW);

  // Wait 1 second...
  delay(200);
}
