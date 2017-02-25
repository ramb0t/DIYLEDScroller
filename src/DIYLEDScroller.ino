#include "DIYLEDScroller.h"

uint8_t valx;
uint8_t valy;
uint8_t val;

//Setup Entry Point
void setup(){
  ledmatrix_setup();
  for(unsigned i = 0 ; i < 255 ; i++) ledmatrix_draw();
  valx = 0;
  valy = 0;
  val = 0;
}


//Main Loop Entry Point
void loop(){
  for(unsigned i = 0 ; i < 4 ; i++) ledmatrix_draw();

  ledmatrix_test(valx, valy);
  //ledmatrix_test2(val);

  val++;
  if(val > 6){
    val = 0;
  }

  valx++;
  if(valx >= WIDTH){
    valx = 0;
    valy++;
    if(valy >= HEIGHT){
      valy = 0;
      //ledmatrix_setup();
    }
  }
}
