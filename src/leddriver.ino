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
#define ROW0		D2
#define ROW1		D1
#define ROW2		D3
#define ROW3		D6
#define ROW4		D5
#define ROW5		D7
#define ROW6		D4

#define RST     D0
#else
// Some other mcu
#define pin	10

#endif

//Operation Defines
#define COL_MAX   15
#define COL_DELAY 500  // microsecond delay bettween col draws

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
  if(address & A_MASK){
    digitalWrite(DEMUX_SEL_A, HIGH);
  }else{
    digitalWrite(DEMUX_SEL_A, LOW);
  }

  if(address & B_MASK){
    digitalWrite(DEMUX_SEL_B, HIGH);
  }else{
    digitalWrite(DEMUX_SEL_B, LOW);
  }

  if(address & C_MASK){
    digitalWrite(DEMUX_SEL_C, HIGH);
  }else{
    digitalWrite(DEMUX_SEL_C, LOW);
  }
}

// inline function that sets the row data pin from the led matrix
// based on the provided row pin
static inline void row_data(
  const int row_pin,
  const uint8_t data
){
  data > 0 ? digitalWrite(row_pin, LOW) : digitalWrite(row_pin, HIGH);
}

// inline function that selects the correct col on the current latch
static inline void col_select(const unsigned col){
  // select the first col latch
  demux_select(6); // this should already be set from last instruction..
  // activate only this col on the col latch
  for(unsigned col_add = 0; col_add < 8 ; col_add++){
    // select address
    col_add & S0_MASK > 0 ? digitalWrite(COL_LATCH_S0, HIGH) : digitalWrite(COL_LATCH_S0, LOW);
    col_add & S1_MASK > 0 ? digitalWrite(COL_LATCH_S1, HIGH) : digitalWrite(COL_LATCH_S1, LOW);
    col_add & S2_MASK > 0 ? digitalWrite(COL_LATCH_S2, HIGH) : digitalWrite(COL_LATCH_S2, LOW);
    // write data
    col_add == col ? digitalWrite(COL_LATCH_D, LOW) : digitalWrite(COL_LATCH_D, HIGH); // active low
  }

  // select the second col latch
  demux_select(7);
  // activate only this col on the col latch
  for(unsigned col_add = 0; col_add < 8 ; col_add++){
    // select address
    col_add & S0_MASK > 0 ? digitalWrite(COL_LATCH_S0, HIGH) : digitalWrite(COL_LATCH_S0, LOW);
    col_add & S1_MASK > 0 ? digitalWrite(COL_LATCH_S1, HIGH) : digitalWrite(COL_LATCH_S1, LOW);
    col_add & S2_MASK > 0 ? digitalWrite(COL_LATCH_S2, HIGH) : digitalWrite(COL_LATCH_S2, LOW);
    // write data (offset of 8 cols for second latch)
    (col_add + 8) == col ? digitalWrite(COL_LATCH_D, LOW) : digitalWrite(COL_LATCH_D, HIGH); // active low
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
    // 1: (init) Select the correct block row latch address with the demux (pulls latch clock low)
    // 2: Write the row data from the matrix out on the data pins
    // 3: Select the *next* row latch address (high going clock edge to latch the row latch data!)
    // 4: GOTO 2 and repeat until all row data is latched!

    // 1: init the demux row latch address
    demux_select(0); // first block
    digitalWrite(DEMUX_EN_G2B, LOW); // activate mux
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

      // 3: select the *next* block col address and create this row latch clock pulse
      demux_select(block+1);

    } // all row data latched in

    // select the correct col with the two col latches
    col_select(col);

    // deactivate the demux
    digitalWrite(DEMUX_EN_G2B, HIGH);
    // wait for the POV col delay time
    delayMicroseconds(COL_DELAY);

  } // rinse and repeat for each col :D

  // set reset
  digitalWrite(RST, LOW);

}
