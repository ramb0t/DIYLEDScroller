#include "DIYLEDScroller.h"

#define ONE_DAY_MILLIS (24*60*60*1000)
#define SCREEN_LENGTH 18


bool cloudflag;
String cloudstring;
String cvEvent;
String cvData;


static int
show_string(String s)
{
  cloudstring = s;
  cloudflag = true;

	return s.length();
}

static unsigned
draw_num(
	unsigned y,
	unsigned n,
	unsigned digits
)
{

	y += digits * 5;
	for(unsigned i = 0 ; i < digits ; i++)
	{
		if (y >= WIDTH)
			return WIDTH;
		y -= 5;
		draw_char(y, '0' + (n % 10));
		n /= 10;
	}

	return y + digits * 5;
}

static unsigned
draw_num_small(
	unsigned y,
	unsigned n,
	unsigned digits
)
{
	y += digits * 4;
	for(unsigned i = 0 ; i < digits ; i++)
	{
		if (y >= WIDTH)
			return WIDTH;

		y -= 4;
		draw_small_digit(y, (n % 10), 0xFF);
		n /= 10;
	}

	return y + digits * 4;
}


uint8_t valx;
uint8_t valy;
uint8_t val;

//Setup Entry Point
void setup(){
  //Pin Setup
  pinMode(BUZZ_PIN, OUTPUT);

  // Cloud functions
  Spark.function("show", show_string);

  // Cloud Variables registration
  Spark.variable("cvEvent",cvEvent);
  Spark.variable("cvData",cvData);

  ledmatrix_setup();
  draw_clear();
  draw_string("DIYLEDScroller");
  for(unsigned i = 0 ; i < 255 ; i++) ledmatrix_draw();
  Time.zone(+2);
  valx = 0;
  valy = 0;
  val = 0;
  cloudflag = false;
  //We "Subscribe" to our IFTTT event called Button so that we get events for it
   Particle.subscribe("CiandriZButton", myHandler);

   digitalWrite(BUZZ_PIN, HIGH);
   delay(300);
   digitalWrite(BUZZ_PIN, LOW);

}


//Main Loop Entry Point
void loop(){
  // Check if we have a cloud string
  if (cloudflag){
    // Beep!
    digitalWrite(BUZZ_PIN, HIGH);
    delay(75);
    digitalWrite(BUZZ_PIN, LOW);
    delay(100);
    digitalWrite(BUZZ_PIN, HIGH);
    delay(75);
    digitalWrite(BUZZ_PIN, LOW);
    delay(100);
    digitalWrite(BUZZ_PIN, HIGH);
    delay(75);
    digitalWrite(BUZZ_PIN, LOW);

    delay(500);

    draw_clear();

    // Scolling code!
    // first determine if the string is longer than the max size?
    if(cloudstring.length() < SCREEN_LENGTH ){ // shorter than max...

      // just do the 'old code' xD
      // loop through string and draw each char
      for(unsigned i = 0 ; i < cloudstring.length() && i*5 < 90 ; i++) // 18 chars max :)
        draw_char(i*5, cloudstring.charAt(i));

      //crude delay for'some' time...
      for(unsigned i = 0 ; i < 255 ; i++) ledmatrix_draw();
      for(unsigned i = 0 ; i < 255 ; i++) ledmatrix_draw();

    }else{ // longer than max, need to scroll!
      // append 18 blank chars on the end for scrolling effect
      cloudstring = cloudstring + "                  ";
      cloudstring = "                  " + cloudstring;

      for(unsigned j = 0 ; j < 3 ; j ++){ // outter repeat
        for(unsigned c = 0 ; c <= cloudstring.length() - SCREEN_LENGTH ; c++){ // loop through the excess chars...

          // get the substring
          String sbuf = cloudstring.substring(c,c + SCREEN_LENGTH);
          // loop through string and draw each char
        	for(unsigned i = 0 ; i < sbuf.length() && i*5 < 90 ; i++){ // 18 chars max :)
                draw_char(i*5, sbuf.charAt(i));
          }

          //crude delay for'some' time...
          for(unsigned i = 0 ; i < 15 ; i++) ledmatrix_draw();
          draw_clear(); // not sure if needed?
        }
      } // end repeat
    }


    // finally clear the flag.
    cloudflag=false;
  }

  static uint32_t last_sync;
	const uint32_t now_millis = millis();
	static uint32_t last_millis;

  // sync the time once a day
	if (now_millis - last_sync > ONE_DAY_MILLIS)
	{
		Spark.syncTime();
		last_sync = millis();
		return;
	}

	//if (now_millis > last_millis + 900)
	{
		last_millis = now_millis;

		// 012345678901234567
		// YYYYMMDD HHMMSS.
		unsigned y = 9; // pad the first few chars

		unsigned year = Time.year();
		unsigned mon = Time.month();
		unsigned day = Time.day();

		draw_clear();
    // small logo display
		draw_char(0, '[');
		draw_char(5, ']');

    // draw the date
		y = draw_num_small(y, year, 4) + 2;
		ledmatrix_set_col(y-2, 0x08, 0xFF);
		y = draw_num_small(y, mon, 2) + 2;
		ledmatrix_set_col(y-2, 0x08, 0xFF);
		y = draw_num_small(y, day, 2) + 2;

		unsigned hour = Time.hour();
		unsigned min = Time.minute();
		unsigned sec = Time.second();

		//y += 2;
		y--;

		// draw the HH:MM:SS with colons inbetween
		y = draw_num(y, hour, 2) + 2;
		ledmatrix_set_col(y-2, 0x14, 0xFF);
		y = draw_num(y, min, 2) + 2;
		ledmatrix_set_col(y-2, 0x14, 0xFF);
		y = draw_num(y, sec, 2) + 2;
	}

	//for (int i = 0 ; i < 128 ; i++)
	{
		unsigned ms = (millis() / 10) % 100;
		unsigned y = 81;
		draw_num_small(y, ms, 2);

		ledmatrix_draw();
	}



  //for(unsigned i = 0 ; i < 4 ; i++) ledmatrix_draw();

  //ledmatrix_test(valx, valy);
  //ledmatrix_test2(val);

  /*val++;
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
  }*/
}



//The function that handles the event from IFTTT
void myHandler(const char *event, const char *data){
    cvEvent = event;
    cvData = data;

    show_string(data);

}
