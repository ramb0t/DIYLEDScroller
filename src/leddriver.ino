/* RobThePyro 25/2/2017
 * Low level driver functions for reverse engineered
 * LED Marquee sign hardware
 * Loosely based on https://github.com/osresearch/sparktime/blob/master/redscroller/
 * 18 * 5x7 modules = 90x7 screen
 * On the spark core:
 * D0 CLR/MR
 * D1 D1
 * D2 D0
 * D3 D2
 * D4 D6/S2
 * D5 D4/S1
 * D6 D3/S0
 * D7 D5/D
 *
 * A0 G2B
 * A1 A
 * A2 B
 * A3 C
 * A4 G2A
 * A5
 * A6
 * A7
*/

#include "DIYLEDScroller.h"


// Setup the pin mapping
#define SPARK_CORE
#ifdef SPARK_CORE
// spark core

// Demux control
#define DEMUX_EN_G2A	A4  //active low!
#define DEMUX_EN_G2B	A0  //active low!
#define DEMUX_SEL_A   A1
#define DEMUX_SEL_B   A2
#define DEMUX_SEL_C   A3

// Col latches control
#define COL_LATCH_D   D7  //active low!
#define COL_LATCH_S0  D6
#define COL_LATCH_S1  D5
#define COL_LATCH_S2  D4

// Row Latch Data Pins
#define ROW0		D7 // D2
#define ROW1		D6 // D1
#define ROW2		D5 // D3
#define ROW3		D4 // D6
#define ROW4		D3 // D5
#define ROW5		D2 // D7
#define ROW6		D1 // D4

#define RST     D0
#else
// Some other mcu
#define pin	10

#endif

//Operation Defines
#define COL_MAX   15
#define COL_DELAY 500  // microsecond delay bettween col draws, brightness essentially?

//#define ROW_MAX   7  // use HEIGHT rather
#define BLOCK_SIZE  6

// bitmasks for the demux
#define A_MASK  00000001
#define B_MASK  00000010
#define C_MASK  00000100

// bitmasks for the col latches
#define S0_MASK  00000001
#define S1_MASK  00000010
#define S2_MASK  00000100


// LED pixel matrix array
static uint8_t fb[HEIGHT][WIDTH];

void ledmatrix_setup()
{
  //Setup pins
  pinMode(RST, OUTPUT);
  digitalWrite(RST, LOW); // Pull reset line low for init
  //digitalWrite(RST, HIGH);

  pinMode(DEMUX_EN_G2A, OUTPUT);
  pinMode(DEMUX_EN_G2B, OUTPUT);
  pinMode(DEMUX_SEL_A, OUTPUT);
  pinMode(DEMUX_SEL_B, OUTPUT);
  pinMode(DEMUX_SEL_C, OUTPUT);

  pinMode(COL_LATCH_D, OUTPUT);
  pinMode(COL_LATCH_S0, OUTPUT);
  pinMode(COL_LATCH_S1, OUTPUT);
  pinMode(COL_LATCH_S2, OUTPUT);

	pinMode(ROW0, OUTPUT);
	pinMode(ROW1, OUTPUT);
  pinMode(ROW2, OUTPUT);
  pinMode(ROW3, OUTPUT);
  pinMode(ROW4, OUTPUT);
  pinMode(ROW5, OUTPUT);
  pinMode(ROW6, OUTPUT);

  digitalWrite(DEMUX_EN_G2A, LOW); // 74LS138 3to8 demux. can keep this LOW and just use G2B ? G1 pulled high on pcb..

  // init the matrix to default
	for (int y = 0 ; y < HEIGHT ; y++)
	{
		for (int x = 0 ; x < WIDTH ; x++)
		{
			fb[y][x] = (x % 8) < y ? 0 : 1;
			//fb[y][x] = x < WIDTH/2; //(x^y) & 1;
		}
	}
} // end ledmatrix_setup()

// inline function to set the demux address
static inline void demux_select( const uint8_t address){
  // the demux select inputs take binary as: CBA with C MSB
  // lets use bitmasking of the address to call the correct address lines

  switch(address){
    case 0:
      digitalWriteFast(DEMUX_SEL_A, LOW);
      digitalWriteFast(DEMUX_SEL_B, LOW);
      digitalWriteFast(DEMUX_SEL_C, LOW);
    break;
    case 1:
      digitalWriteFast(DEMUX_SEL_A, HIGH);
      digitalWriteFast(DEMUX_SEL_B, LOW);
      digitalWriteFast(DEMUX_SEL_C, LOW);
    break;
    case 2:
      digitalWriteFast(DEMUX_SEL_A, LOW);
      digitalWriteFast(DEMUX_SEL_B, HIGH);
      digitalWriteFast(DEMUX_SEL_C, LOW);
    break;
    case 3:
      digitalWriteFast(DEMUX_SEL_A, HIGH);
      digitalWriteFast(DEMUX_SEL_B, HIGH);
      digitalWriteFast(DEMUX_SEL_C, LOW);
    break;
    case 4:
      digitalWriteFast(DEMUX_SEL_A, LOW);
      digitalWriteFast(DEMUX_SEL_B, LOW);
      digitalWriteFast(DEMUX_SEL_C, HIGH);
    break;
    case 5:
      digitalWriteFast(DEMUX_SEL_A, HIGH);
      digitalWriteFast(DEMUX_SEL_B, LOW);
      digitalWriteFast(DEMUX_SEL_C, HIGH);
    break;
    case 6:
      digitalWriteFast(DEMUX_SEL_A, LOW);
      digitalWriteFast(DEMUX_SEL_B, HIGH);
      digitalWriteFast(DEMUX_SEL_C, HIGH);
    break;
    case 7:
      digitalWriteFast(DEMUX_SEL_A, HIGH);
      digitalWriteFast(DEMUX_SEL_B, HIGH);
      digitalWriteFast(DEMUX_SEL_C, HIGH);
    break;

    default:
      digitalWriteFast(DEMUX_SEL_A, LOW);
      digitalWriteFast(DEMUX_SEL_B, LOW);
      digitalWriteFast(DEMUX_SEL_C, LOW);
    break;
  }
  /*if(address & A_MASK){
    digitalWriteFast(DEMUX_SEL_A, HIGH);
  }else{
    digitalWriteFast(DEMUX_SEL_A, LOW);
  }

  if(address & B_MASK){
    digitalWriteFast(DEMUX_SEL_B, HIGH);
  }else{
    digitalWriteFast(DEMUX_SEL_B, LOW);
  }

  if(address & C_MASK){
    digitalWriteFast(DEMUX_SEL_C, HIGH);
  }else{
    digitalWriteFast(DEMUX_SEL_C, LOW);
  }*/
}

// inline function to toggle the demux for the col latches
static inline void toggle_demux(){
  // activate the demux
  digitalWriteFast(DEMUX_EN_G2B, LOW);
  // deactivate the demux
  digitalWriteFast(DEMUX_EN_G2B, HIGH);
}

// inline function that sets the row data pin from the led matrix
// based on the provided row pin
static inline void row_data(
  const int row_pin,
  const uint8_t data
){
  data != 0 ? digitalWriteFast(row_pin, HIGH) : digitalWriteFast(row_pin, LOW);
}

// inline function that selects the correct col on the current latch
static inline void col_select(const unsigned col){
  // select the first col latch
  demux_select(6); // this should already be set from last instruction..

  // activate only this col on the col latch
  for(unsigned col_add = 0; col_add < 8 ; col_add++){
    // select address
    /*col_add & S0_MASK > 0 ? digitalWriteFast(COL_LATCH_S0, HIGH) : digitalWriteFast(COL_LATCH_S0, LOW);
    col_add & S1_MASK > 0 ? digitalWriteFast(COL_LATCH_S1, HIGH) : digitalWriteFast(COL_LATCH_S1, LOW);
    col_add & S2_MASK > 0 ? digitalWriteFast(COL_LATCH_S2, HIGH) : digitalWriteFast(COL_LATCH_S2, LOW);*/
    switch(col_add){
      case 0:
        digitalWriteFast(COL_LATCH_S0, LOW);
        digitalWriteFast(COL_LATCH_S1, LOW);
        digitalWriteFast(COL_LATCH_S2, LOW);
      break;
      case 1:
        digitalWriteFast(COL_LATCH_S0, HIGH);
        digitalWriteFast(COL_LATCH_S1, LOW);
        digitalWriteFast(COL_LATCH_S2, LOW);
      break;
      case 2:
        digitalWriteFast(COL_LATCH_S0, LOW);
        digitalWriteFast(COL_LATCH_S1, HIGH);
        digitalWriteFast(COL_LATCH_S2, LOW);
      break;
      case 3:
        digitalWriteFast(COL_LATCH_S0, HIGH);
        digitalWriteFast(COL_LATCH_S1, HIGH);
        digitalWriteFast(COL_LATCH_S2, LOW);
      break;
      case 4:
        digitalWriteFast(COL_LATCH_S0, LOW);
        digitalWriteFast(COL_LATCH_S1, LOW);
        digitalWriteFast(COL_LATCH_S2, HIGH);
      break;
      case 5:
        digitalWriteFast(COL_LATCH_S0, HIGH);
        digitalWriteFast(COL_LATCH_S1, LOW);
        digitalWriteFast(COL_LATCH_S2, HIGH);
      break;
      case 6:
        digitalWriteFast(COL_LATCH_S0, LOW);
        digitalWriteFast(COL_LATCH_S1, HIGH);
        digitalWriteFast(COL_LATCH_S2, HIGH);
      break;
      case 7:
        digitalWriteFast(COL_LATCH_S0, HIGH);
        digitalWriteFast(COL_LATCH_S1, HIGH);
        digitalWriteFast(COL_LATCH_S2, HIGH);
      break;
      default:
        digitalWriteFast(COL_LATCH_S0, LOW);
        digitalWriteFast(COL_LATCH_S1, LOW);
        digitalWriteFast(COL_LATCH_S2, LOW);
      break;
    }
    // write data
    //col_add == col ? digitalWriteFast(COL_LATCH_D, LOW) : digitalWriteFast(COL_LATCH_D, HIGH); // active low
    if(col_add == col){
      digitalWriteFast(COL_LATCH_D,LOW);
    }else{
      digitalWriteFast(COL_LATCH_D,HIGH);
    }
    toggle_demux();
  }

  // select the second col latch
  demux_select(7);
  // activate only this col on the col latch
  for(unsigned col_add = 0; col_add < 7 ; col_add++){ // only go to 7, last output doesn't exist
    // select address
    /*col_add & S0_MASK > 0 ? digitalWriteFast(COL_LATCH_S0, HIGH) : digitalWriteFast(COL_LATCH_S0, LOW);
    col_add & S1_MASK > 0 ? digitalWriteFast(COL_LATCH_S1, HIGH) : digitalWriteFast(COL_LATCH_S1, LOW);
    col_add & S2_MASK > 0 ? digitalWriteFast(COL_LATCH_S2, HIGH) : digitalWriteFast(COL_LATCH_S2, LOW);*/
    switch(col_add){
      case 0:
        digitalWriteFast(COL_LATCH_S0, LOW);
        digitalWriteFast(COL_LATCH_S1, LOW);
        digitalWriteFast(COL_LATCH_S2, LOW);
      break;
      case 1:
        digitalWriteFast(COL_LATCH_S0, HIGH);
        digitalWriteFast(COL_LATCH_S1, LOW);
        digitalWriteFast(COL_LATCH_S2, LOW);
      break;
      case 2:
        digitalWriteFast(COL_LATCH_S0, LOW);
        digitalWriteFast(COL_LATCH_S1, HIGH);
        digitalWriteFast(COL_LATCH_S2, LOW);
      break;
      case 3:
        digitalWriteFast(COL_LATCH_S0, HIGH);
        digitalWriteFast(COL_LATCH_S1, HIGH);
        digitalWriteFast(COL_LATCH_S2, LOW);
      break;
      case 4:
        digitalWriteFast(COL_LATCH_S0, LOW);
        digitalWriteFast(COL_LATCH_S1, LOW);
        digitalWriteFast(COL_LATCH_S2, HIGH);
      break;
      case 5:
        digitalWriteFast(COL_LATCH_S0, HIGH);
        digitalWriteFast(COL_LATCH_S1, LOW);
        digitalWriteFast(COL_LATCH_S2, HIGH);
      break;
      case 6:
        digitalWriteFast(COL_LATCH_S0, LOW);
        digitalWriteFast(COL_LATCH_S1, HIGH);
        digitalWriteFast(COL_LATCH_S2, HIGH);
      break;
      case 7: // should never get here, doesn't exist
        digitalWriteFast(COL_LATCH_S0, HIGH);
        digitalWriteFast(COL_LATCH_S1, HIGH);
        digitalWriteFast(COL_LATCH_S2, HIGH);
      break;
      default:
        digitalWriteFast(COL_LATCH_S0, LOW);
        digitalWriteFast(COL_LATCH_S1, LOW);
        digitalWriteFast(COL_LATCH_S2, LOW);
      break;
    }
    // write data (offset of 8 cols for second latch)
    //(col_add + 8) == col ? digitalWriteFast(COL_LATCH_D, LOW) : digitalWriteFast(COL_LATCH_D, HIGH); // active low
    if((col_add+8) == col){
      digitalWriteFast(COL_LATCH_D,LOW);
    }else{
      digitalWriteFast(COL_LATCH_D,HIGH);
    }
    toggle_demux();
  }


}

// function call to draw the display once
void ledmatrix_draw(){
  // Based on topology rev engineering we need to:
  // outter loop cols 0-14 for all 15x7 displays, disp rows for col1, then col2 etc
  // inner loop, latch in row data for each of the 6 row latches

  // clear reset
  digitalWrite(RST, HIGH);

  // loop through cols
  for(unsigned col = 0 ; col < COL_MAX ; col++){
    // Latch in the row data for this col for each of the blocks
    // To latch in the row data we need to:
    //
    // 1: Write the row data from the matrix out on the data pins
    // 2: Select the row latch address
    // 3: toggle the demux (high going clock edge to latch the row latch data!)
    // 4: GOTO 2 and repeat until all row data is latched!

    // 1: init the demux
    digitalWriteFast(DEMUX_EN_G2B, HIGH); // deactivate mux

    for(unsigned block = 0 ; block < BLOCK_SIZE ; block++){

      // calculate the col address for this block
      unsigned block_col = col + (block*COL_MAX);

      // 2: write the row data to the latch
      row_data(ROW0, fb[0][block_col]); // set the data pin for this row
      row_data(ROW1, fb[1][block_col]);
      row_data(ROW2, fb[2][block_col]);
      row_data(ROW3, fb[3][block_col]);
      row_data(ROW4, fb[4][block_col]);
      row_data(ROW5, fb[5][block_col]);
      row_data(ROW6, fb[6][block_col]);

      // 3: select the block address and create this row latch clock pulse
      demux_select(block);
      toggle_demux();

    } // all row data latched in

    // select the correct col with the two col latches
    col_select(col);

    // wait for the POV col delay time
    delayMicroseconds(COL_DELAY);
    // should turn all cols off before we return... (col 15 doesnt exisit so this will make sure no col is activated)
    col_select(15);

  } // rinse and repeat for each col :D


}

/** Set an entire column at once */
void
ledmatrix_set_col(
	const uint8_t col,
	const uint8_t bits,
	const uint8_t bright
)
{
	for (uint8_t i = 0 ; i < HEIGHT ; i++)
		ledmatrix_set(col, i, (bits & (1 << i)) ? bright : 0);
}


void
ledmatrix_set(
	const uint8_t col,
	const uint8_t row,
	const uint8_t bright
)
{
	fb[row][col] = bright;
}

void ledmatrix_test(uint8_t valx, uint8_t valy){
  for (int y = 0 ; y < HEIGHT ; y++)
	{
		for (int x = 0 ; x < WIDTH ; x++)
		{
        fb[y][x] = 0;
		}
	}
  fb[valy][valx] = 1;
}

void ledmatrix_test2(uint8_t val){
  for(int bank = 0; bank < 6; bank++ ){
    for(int col = 0; col < 15; col++){
      for(int row = 0; row < 7 ; row++){
        int bank_col = col + (bank*15);
        int valoffset = val+bank;
        if(valoffset > 6){valoffset = valoffset - 7;}
        if(row == valoffset){
          fb[row][bank_col] = 1;
        }else{fb[row][bank_col] = 0; }

      }
    }
  }

}
