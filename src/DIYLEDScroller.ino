#include "DIYLEDScroller.h"

#define ONE_DAY_MILLIS (24*60*60*1000)

static int
show_string(String s)
{
	draw_clear();

	for(unsigned i = 0 ; i < s.length() && i*5 < 90 ; i++)
		draw_char(i*5, s.charAt(i));

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
  Spark.function("show", show_string);
  ledmatrix_setup();
  for(unsigned i = 0 ; i < 255 ; i++) ledmatrix_draw();
  draw_string("DIYLEDScroller");
  for(unsigned i = 0 ; i < 255 ; i++) ledmatrix_draw();
  valx = 0;
  valy = 0;
  val = 0;
}


//Main Loop Entry Point
void loop(){

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
