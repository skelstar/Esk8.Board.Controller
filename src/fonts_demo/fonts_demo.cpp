/*
  Example for TFT_eSPI library

  Created by Bodmer 31/12/16

  This example draws all fonts (as used by the Adafruit_GFX library) onto the
  screen. These fonts are called the GFX Free Fonts (GFXFF) in this library.

  The fonts are referenced by a short name, see the Free_Fonts.h file
  attached to this sketch.

  Other True Type fonts could be converted using the utility within the
  "fontconvert" folder inside the library. This converted has also been
  copied from the Adafruit_GFX library. 

  Since these fonts are a recent addition Adafruit do not have a tutorial
  available yet on how to use the fontconvert utility.   Linux users will
  no doubt figure it out!  In the meantime there are 48 font files to use
  in sizes from 9 point to 24 point, and in normal, bold, and italic or
  oblique styles.

  This example sketch uses both the print class and drawString() functions
  to plot text to the screen.

  Make sure LOAD_GFXFF is defined in the User_Setup.h file within the
  library folder.

  --------------------------- NOTE ----------------------------------------
  The free font encoding format does not lend itself easily to plotting
  the background without flicker. For values that changes on screen it is
  better to use Fonts 1- 8 which are encoded specifically for rapid
  drawing with background.
  -------------------------------------------------------------------------

  #########################################################################
  ###### DON'T FORGET TO UPDATE THE User_Setup.h FILE IN THE LIBRARY ######
  ######       TO SELECT YOUR DISPLAY TYPE AND ENABLE FONTS          ######
  #########################################################################
*/

#define TEXT "Batt: 34.2" // Text that will be printed on screen in any font

// #include "Free_Fonts.h" // Include the header file attached to this sketch

#include <SPI.h>
#include <TFT_eSPI.h>
// #include <Fonts/Custom/Orbitron_Light_24.h>
// #include <Fonts/Custom/Orbitron_Light_32.h>
#include <fonts/Orbitron_Med_12.h>
#include <fonts/Orbitron_Med_16.h>
#include <fonts/Orbitron_Bold_48.h>

// Use hardware SPI
TFT_eSPI tft = TFT_eSPI();

unsigned long drawTime = 0;

void setup(void)
{

  tft.begin();

  tft.setRotation(1);
}

void loop()
{

  // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
  // Show all 48 fonts in centre of screen ( x,y coordinate 160,120)
  // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

  // Where font sizes increase the screen is not cleared as the larger fonts overwrite
  // the smaller one with the background colour.

  // Set text datum to middle centre
  tft.setTextDatum(MR_DATUM);

  // Set text colour to orange with black background
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  int x = 320, y = 40;

  tft.fillScreen(TFT_BLACK);                  // Clear screen
  tft.setFreeFont(&Orbitron_Medium_12);       // Select the font
  tft.drawString("Orbitron Medium 12", x, y); // Print the string name of the font
  tft.drawString(TEXT, x, y + 40);            // Print the string name of the font
  delay(2000);

  tft.fillScreen(TFT_BLACK);                  // Clear screen
  tft.setFreeFont(&Orbitron_Medium_16);       // Select the font
  tft.drawString("Orbitron Medium 16", x, y); // Print the string name of the font
  tft.drawString(TEXT, x, y + 40);            // Print the string name of the font
  delay(2000);

  tft.fillScreen(TFT_BLACK);                 // Clear screen
  tft.setFreeFont(&Orbitron_Light_24);       // Select the font
  tft.drawString("Orbitron Light 24", x, y); // Print the string name of the font
  tft.drawString(TEXT, x, y + 40);           // Print the string name of the font
  delay(2000);

  tft.fillScreen(TFT_BLACK);                 // Clear screen
  tft.setFreeFont(&Orbitron_Light_32);       // Select the font
  tft.drawString("Orbitron Light 32", x, y); // Print the string name of the font
  tft.drawString(TEXT, x, y + 40);           // Print the string name of the font
  delay(2000);

  // tft.fillScreen(TFT_BLACK);              // Clear screen
  // tft.setFreeFont(&Roboto_Thin_24);       // Select the font
  // tft.drawString("Roboto_Thin_24", x, y); // Print the string name of the font
  // tft.drawString(TEXT, x, y + 40);        // Print the string name of the font
  // delay(2000);

  tft.fillScreen(TFT_BLACK);                // Clear screen
  tft.setFreeFont(&Orbitron_Bold_48);       // Select the font
  tft.drawString("Orbitron_Bold_48", x, y); // Print the string name of the font
  tft.drawString(TEXT, x, y + 50);          // Print the string name of the font
  delay(2000);

  //tft.fillScreen(TFT_BLACK);
  // tft.setFreeFont(FF18);                // Select the font
  // tft.drawString(sFF2, x, y, GFXFF); // Print the string name of the font
  // tft.setFreeFont(FF2);
  // tft.drawString(TEXT, x, y + 40, GFXFF);
  // delay(2000);
  // //tft.fillScreen(TFT_BLACK);
  // tft.setFreeFont(FF18);                // Select the font
  // tft.drawString(sFF3, x, y, GFXFF); // Print the string name of the font
  // tft.setFreeFont(FF3);
  // tft.drawString(TEXT, x, y + 40, GFXFF);
  // delay(2000);
  // //tft.fillScreen(TFT_BLACK);
  // tft.setFreeFont(FF18);                // Select the font
  // tft.drawString(sFF4, x, y, GFXFF); // Print the string name of the font
  // tft.setFreeFont(FF4);
  // tft.drawString(TEXT, x, y + 40, GFXFF);
  // delay(2000);

  // tft.fillScreen(TFT_BLACK);
  // tft.setFreeFont(FF18);                // Select the font
  // tft.drawString(sFF5, x, y, GFXFF); // Print the string name of the font
  // tft.setFreeFont(FF5);
  // tft.drawString(TEXT, x, y + 40, GFXFF);
  // delay(2000);
  // //tft.fillScreen(TFT_BLACK);
  // tft.setFreeFont(FF18);                // Select the font
  // tft.drawString(sFF6, x, y, GFXFF); // Print the string name of the font
  // tft.setFreeFont(FF6);
  // tft.drawString(TEXT, x, y + 40, GFXFF);
  // delay(2000);
  // //tft.fillScreen(TFT_BLACK);
  // tft.setFreeFont(FF18);                // Select the font
  // tft.drawString(sFF7, x, y, GFXFF); // Print the string name of the font
  // tft.setFreeFont(FF7);
  // tft.drawString(TEXT, x, y + 40, GFXFF);
  // delay(2000);
  // //tft.fillScreen(TFT_BLACK);
  // tft.setFreeFont(FF18);                // Select the font
  // tft.drawString(sFF8, x, y, GFXFF); // Print the string name of the font
  // tft.setFreeFont(FF8);
  // tft.drawString(TEXT, x, y + 40, GFXFF);
  // delay(2000);

  // tft.fillScreen(TFT_BLACK);
  // tft.setFreeFont(FF18);                // Select the font
  // tft.drawString(sFF9, x, y, GFXFF); // Print the string name of the font
  // tft.setFreeFont(FF9);
  // tft.drawString(TEXT, x, y + 40, GFXFF);
  // delay(2000);
  // //tft.fillScreen(TFT_BLACK);
  // tft.setFreeFont(FF18);                 // Select the font
  // tft.drawString(sFF10, x, y, GFXFF); // Print the string name of the font
  // tft.setFreeFont(FF10);
  // tft.drawString(TEXT, x, y + 40, GFXFF);
  // delay(2000);
  // //tft.fillScreen(TFT_BLACK);
  // tft.setFreeFont(FF18);                 // Select the font
  // tft.drawString(sFF11, x, y, GFXFF); // Print the string name of the font
  // tft.setFreeFont(FF11);
  // tft.drawString(TEXT, x, y + 40, GFXFF);
  // delay(2000);
  // //tft.fillScreen(TFT_BLACK);
  // tft.setFreeFont(FF18);                 // Select the font
  // tft.drawString(sFF12, x, y, GFXFF); // Print the string name of the font
  // tft.setFreeFont(FF12);
  // tft.drawString(TEXT, x, y + 40, GFXFF);
  // delay(2000);

  // tft.fillScreen(TFT_BLACK);
  // tft.setFreeFont(FF18);                 // Select the font
  // tft.drawString(sFF13, x, y, GFXFF); // Print the string name of the font
  // tft.setFreeFont(FF13);
  // tft.drawString(TEXT, x, y + 40, GFXFF);
  // delay(2000);
  // //tft.fillScreen(TFT_BLACK);
  // tft.setFreeFont(FF18);                 // Select the font
  // tft.drawString(sFF14, x, y, GFXFF); // Print the string name of the font
  // tft.setFreeFont(FF14);
  // tft.drawString(TEXT, x, y + 40, GFXFF);
  // delay(2000);
  // //tft.fillScreen(TFT_BLACK);
  // tft.setFreeFont(FF18);                 // Select the font
  // tft.drawString(sFF15, x, y, GFXFF); // Print the string name of the font
  // tft.setFreeFont(FF15);
  // tft.drawString(TEXT, x, y + 40, GFXFF);
  // delay(2000);
  // //tft.fillScreen(TFT_BLACK);
  // tft.setFreeFont(FF18);                 // Select the font
  // tft.drawString(sFF16, x, y, GFXFF); // Print the string name of the font
  // tft.setFreeFont(FF16);
  // tft.drawString(TEXT, x, y + 40, GFXFF);
  // delay(2000);

  // tft.fillScreen(TFT_BLACK);
  // tft.setFreeFont(FF18);                 // Select the font
  // tft.drawString(sFF17, x, y, GFXFF); // Print the string name of the font
  // tft.setFreeFont(FF17);
  // tft.drawString(TEXT, x, y + 40, GFXFF);
  // delay(2000);
  // //tft.fillScreen(TFT_BLACK);
  // tft.setFreeFont(FF18);                 // Select the font
  // tft.drawString(sFF18, x, y, GFXFF); // Print the string name of the font
  // tft.setFreeFont(FF18);
  // tft.drawString(TEXT, x, y + 40, GFXFF);
  // delay(2000);
  // //tft.fillScreen(TFT_BLACK);
  // tft.setFreeFont(FF18);                 // Select the font
  // tft.drawString(sFF19, x, y, GFXFF); // Print the string name of the font
  // tft.setFreeFont(FF19);
  // tft.drawString(TEXT, x, y + 40, GFXFF);
  // delay(2000);
  // //tft.fillScreen(TFT_BLACK);
  // tft.setFreeFont(FF18);                 // Select the font
  // tft.drawString(sFF20, x, y, GFXFF); // Print the string name of the font
  // tft.setFreeFont(FF20);
  // tft.drawString(TEXT, x, y + 40, GFXFF);
  // delay(2000);

  // tft.fillScreen(TFT_BLACK);
  // tft.setFreeFont(FF18);                 // Select the font
  // tft.drawString(sFF21, x, y, GFXFF); // Print the string name of the font
  // tft.setFreeFont(FF21);
  // tft.drawString(TEXT, x, y + 40, GFXFF);
  // delay(2000);
  // //tft.fillScreen(TFT_BLACK);
  // tft.setFreeFont(FF18);                 // Select the font
  // tft.drawString(sFF22, x, y, GFXFF); // Print the string name of the font
  // tft.setFreeFont(FF22);
  // tft.drawString(TEXT, x, y + 40, GFXFF);
  // delay(2000);
  // //tft.fillScreen(TFT_BLACK);
  // tft.setFreeFont(FF18);                 // Select the font
  // tft.drawString(sFF23, x, y, GFXFF); // Print the string name of the font
  // tft.setFreeFont(FF23);
  // tft.drawString(TEXT, x, y + 40, GFXFF);
  // delay(2000);
  // //tft.fillScreen(TFT_BLACK);
  // tft.setFreeFont(FF18);                 // Select the font
  // tft.drawString(sFF24, x, y, GFXFF); // Print the string name of the font
  // tft.setFreeFont(FF24);
  // tft.drawString(TEXT, x, y + 40, GFXFF);
  // delay(2000);

  // tft.fillScreen(TFT_BLACK);
  // tft.setFreeFont(FF18);                 // Select the font
  // tft.drawString(sFF25, x, y, GFXFF); // Print the string name of the font
  // tft.setFreeFont(FF25);
  // tft.drawString(TEXT, x, y + 40, GFXFF);
  // delay(2000);
  // //tft.fillScreen(TFT_BLACK);
  // tft.setFreeFont(FF18);                 // Select the font
  // tft.drawString(sFF26, x, y, GFXFF); // Print the string name of the font
  // tft.setFreeFont(FF26);
  // tft.drawString(TEXT, x, y + 40, GFXFF);
  // delay(2000);
  // //tft.fillScreen(TFT_BLACK);
  // tft.setFreeFont(FF18);                 // Select the font
  // tft.drawString(sFF27, x, y, GFXFF); // Print the string name of the font
  // tft.setFreeFont(FF27);
  // tft.drawString(TEXT, x, y + 40, GFXFF);
  // delay(2000);
  // //tft.fillScreen(TFT_BLACK);
  // tft.setFreeFont(FF18);                 // Select the font
  // tft.drawString(sFF28, x, y, GFXFF); // Print the string name of the font
  // tft.setFreeFont(FF28);
  // tft.drawString(TEXT, x, y + 40, GFXFF);
  // delay(2000);

  // tft.fillScreen(TFT_BLACK);
  // tft.setFreeFont(FF18);                 // Select the font
  // tft.drawString(sFF29, x, y, GFXFF); // Print the string name of the font
  // tft.setFreeFont(FF29);
  // tft.drawString(TEXT, x, y + 40, GFXFF);
  // delay(2000);
  // //tft.fillScreen(TFT_BLACK);
  // tft.setFreeFont(FF18);                 // Select the font
  // tft.drawString(sFF30, x, y, GFXFF); // Print the string name of the font
  // tft.setFreeFont(FF30);
  // tft.drawString(TEXT, x, y + 40, GFXFF);
  // delay(2000);
  // //tft.fillScreen(TFT_BLACK);
  // tft.setFreeFont(FF18);                 // Select the font
  // tft.drawString(sFF31, x, y, GFXFF); // Print the string name of the font
  // tft.setFreeFont(FF31);
  // tft.drawString(TEXT, x, y + 40, GFXFF);
  // delay(2000);
  // //tft.fillScreen(TFT_BLACK);
  // tft.setFreeFont(FF18);                 // Select the font
  // tft.drawString(sFF32, x, y, GFXFF); // Print the string name of the font
  // tft.setFreeFont(FF32);
  // tft.drawString(TEXT, x, y + 40, GFXFF);
  // delay(2000);

  // tft.fillScreen(TFT_BLACK);
  // tft.setFreeFont(FF18);                 // Select the font
  // tft.drawString(sFF33, x, y, GFXFF); // Print the string name of the font
  // tft.setFreeFont(FF33);
  // tft.drawString(TEXT, x, y + 40, GFXFF);
  // delay(2000);
  // //tft.fillScreen(TFT_BLACK);
  // tft.setFreeFont(FF18);                 // Select the font
  // tft.drawString(sFF34, x, y, GFXFF); // Print the string name of the font
  // tft.setFreeFont(FF34);
  // tft.drawString(TEXT, x, y + 40, GFXFF);
  // delay(2000);
  // //tft.fillScreen(TFT_BLACK);
  // tft.setFreeFont(FF18);                 // Select the font
  // tft.drawString(sFF35, x, y, GFXFF); // Print the string name of the font
  // tft.setFreeFont(FF35);
  // tft.drawString(TEXT, x, y + 40, GFXFF);
  // delay(2000);
  // //tft.fillScreen(TFT_BLACK);
  // tft.setFreeFont(FF18);                 // Select the font
  // tft.drawString(sFF36, x, y, GFXFF); // Print the string name of the font
  // tft.setFreeFont(FF36);
  // tft.drawString(TEXT, x, y + 40, GFXFF);
  // delay(2000);

  // tft.fillScreen(TFT_BLACK);
  // tft.setFreeFont(FF18);                 // Select the font
  // tft.drawString(sFF37, x, y, GFXFF); // Print the string name of the font
  // tft.setFreeFont(FF37);
  // tft.drawString(TEXT, x, y + 40, GFXFF);
  // delay(2000);
  // //tft.fillScreen(TFT_BLACK);
  // tft.setFreeFont(FF18);                 // Select the font
  // tft.drawString(sFF38, x, y, GFXFF); // Print the string name of the font
  // tft.setFreeFont(FF38);
  // tft.drawString(TEXT, x, y + 40, GFXFF);
  // delay(2000);
  // //tft.fillScreen(TFT_BLACK);
  // tft.setFreeFont(FF18);                 // Select the font
  // tft.drawString(sFF39, x, y, GFXFF); // Print the string name of the font
  // tft.setFreeFont(FF39);
  // tft.drawString(TEXT, x, y + 40, GFXFF);
  // delay(2000);
  // //tft.fillScreen(TFT_BLACK);
  // tft.setFreeFont(FF18);                 // Select the font
  // tft.drawString(sFF40, x, y, GFXFF); // Print the string name of the font
  // tft.setFreeFont(FF40);
  // tft.drawString(TEXT, x, y + 40, GFXFF);
  // delay(2000);

  // tft.fillScreen(TFT_BLACK);
  // tft.setFreeFont(FF18);                 // Select the font
  // tft.drawString(sFF41, x, y, GFXFF); // Print the string name of the font
  // tft.setFreeFont(FF41);
  // tft.drawString(TEXT, x, y + 40, GFXFF);
  // delay(2000);
  // //tft.fillScreen(TFT_BLACK);
  // tft.setFreeFont(FF18);                 // Select the font
  // tft.drawString(sFF42, x, y, GFXFF); // Print the string name of the font
  // tft.setFreeFont(FF42);
  // tft.drawString(TEXT, x, y + 40, GFXFF);
  // delay(2000);
  // //tft.fillScreen(TFT_BLACK);
  // tft.setFreeFont(FF18);                 // Select the font
  // tft.drawString(sFF43, x, y, GFXFF); // Print the string name of the font
  // tft.setFreeFont(FF43);
  // tft.drawString(TEXT, x, y + 40, GFXFF);
  // delay(2000);
  // //tft.fillScreen(TFT_BLACK);
  // tft.setFreeFont(FF18);                 // Select the font
  // tft.drawString(sFF44, x, y, GFXFF); // Print the string name of the font
  // tft.setFreeFont(FF44);
  // tft.drawString(TEXT, x, y + 40, GFXFF);
  // delay(2000);

  // tft.fillScreen(TFT_BLACK);
  // tft.setFreeFont(FF18);                 // Select the font
  // tft.drawString(sFF45, x, y, GFXFF); // Print the string name of the font
  // tft.setFreeFont(FF45);
  // tft.drawString(TEXT, x, y + 40, GFXFF);
  // delay(2000);
  // //tft.fillScreen(TFT_BLACK);
  // tft.setFreeFont(FF18);                 // Select the font
  // tft.drawString(sFF46, x, y, GFXFF); // Print the string name of the font
  // tft.setFreeFont(FF46);
  // tft.drawString(TEXT, x, y + 40, GFXFF);
  // delay(2000);
  // //tft.fillScreen(TFT_BLACK);
  // tft.setFreeFont(FF18);                 // Select the font
  // tft.drawString(sFF47, x, y, GFXFF); // Print the string name of the font
  // tft.setFreeFont(FF47);
  // tft.drawString(TEXT, x, y + 40, GFXFF);
  // delay(2000);
  // //tft.fillScreen(TFT_BLACK);
  // tft.setFreeFont(FF18);                 // Select the font
  // tft.drawString(sFF48, x, y, GFXFF); // Print the string name of the font
  // tft.setFreeFont(FF48);
  // tft.drawString(TEXT, x, y + 40, GFXFF);
  // delay(2000);
}

// There follows a crude way of flagging that this example sketch needs fonts which
// have not been enbabled in the User_Setup.h file inside the TFT_HX8357 library.
//
// These lines produce errors during compile time if settings in User_Setup are not correct
//
// The error will be "does not name a type" but ignore this and read the text between ''
// it will indicate which font or feature needs to be enabled
//
// Either delete all the following lines if you do not want warnings, or change the lines
// to suit your sketch modifications.

#ifndef LOAD_GLCD
//ERROR_Please_enable_LOAD_GLCD_in_User_Setup
#endif

#ifndef LOAD_GFXFF
ERROR_Please_enable_LOAD_GFXFF_in_User_Setup !
#endif
