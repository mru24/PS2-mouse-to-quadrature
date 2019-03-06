// PS2_to_Quadrature_Mouse
//
// This code will turn a PS/2 mouse into a quadrature mouse for Archimedes or BBC Micro.
//
// No components required for this project! Just a PS/2 socket, and whatever connector the
// retro computer requires.
//
// Kris Adcock, Feb 2017

#include "MyPS2.h"

static const int kPin_ClockPS2 = 6;
static const int kPin_DataPS2 = 5;

static const int kPin_X1 = 7;
static const int kPin_X2 = 8;
static const int kPin_Y1 = 9;
static const int kPin_Y2 = 10;
static const int kPin_B1 = 2;
static const int kPin_B2 = 4;
static const int kPin_B3 = 3;

MyPS2 ps2port(kPin_ClockPS2, kPin_DataPS2);

const bool g_bChan1[] = {HIGH, LOW,  LOW, HIGH};
const bool g_bChan2[] = {HIGH, HIGH, LOW, LOW };

unsigned char g_uStageX, g_uStageY; // our current point in the sequence (index into the channel patterns)
unsigned long g_uLastUpdateX; // last time X channel was updated ... in microseconds
unsigned long g_uLastUpdateY;

void setup()
{
  pinMode(kPin_X1, OUTPUT);
  pinMode(kPin_X2, OUTPUT);
  pinMode(kPin_Y1, OUTPUT);
  pinMode(kPin_Y2, OUTPUT);
  pinMode(kPin_B1, OUTPUT);
  pinMode(kPin_B2, OUTPUT);
  pinMode(kPin_B3, OUTPUT);

  digitalWrite(kPin_X1, HIGH);
  digitalWrite(kPin_X2, HIGH);
  digitalWrite(kPin_Y1, HIGH);
  digitalWrite(kPin_Y2, HIGH);
  digitalWrite(kPin_B1, HIGH);
  digitalWrite(kPin_B2, HIGH);
  digitalWrite(kPin_B3, HIGH);

  //Serial.begin(57600);
}

void UpdateX()
{
  digitalWrite(kPin_X1, g_bChan1[g_uStageX]);
  digitalWrite(kPin_X2, g_bChan2[g_uStageX]);
}

void UpdateY()
{
  digitalWrite(kPin_Y1, g_bChan1[g_uStageY]);
  digitalWrite(kPin_Y2, g_bChan2[g_uStageY]);
}

void loop()
{
  while (ps2port.IsConnected() == false)
  {
    // mouse not connected! Wait a second before attempting to reset the mouse ...
    delay(1000);
    g_uStageX = 0;
    g_uStageY = 0;
    UpdateX();
    UpdateY();
    ps2port.InitMouse();
  }

  // get a reading from the mouse
  ps2port.write(0xEB);
  ps2port.read(); // ack byte (which we don't care about - it will be sweet 0xFA)
  
  unsigned char uData = ps2port.read(); // bit pattern of buttons pressed and other things
  unsigned char uX = ps2port.read();    // signed twos-complement of the X movement
  unsigned char uY = ps2port.read();    // ditto, for y

  unsigned long uStartTimeMS = millis();

  // convert unsigned byte of X and Y into actual movements. We use a separate bool to keep track of the
  // movement being negative, as that makes later code slightly simpler.
  
  bool bReverseX = false;
  if (uX & 0x80)
  {
    uX ^= 0xFF;
    ++uX;
    bReverseX = true;
  }

  bool bReverseY = false;
  if (uY & 0x80)
  {
    uY ^= 0xFF;
    ++uY;
    bReverseY = true;
  }

  // uData:
  //   bit 0 - left mouse button
  //   bit 1 - right mouse button
  //   bit 2 - middle mouse button (wheel pressed)
  //   bit 3 - always 1
  //   bit 4 - X sign bit
  //   bit 5 - Y sign bit
  //   bit 6 - X overflow
  //   bit 7 - Y overflow

  // reflect the buttons pressed on the PS/2 mouse ...
  digitalWrite(kPin_B1, (uData & 1) ? LOW : HIGH);
  digitalWrite(kPin_B2, (uData & 4) ? LOW : HIGH);
  digitalWrite(kPin_B3, (uData & 2) ? LOW : HIGH);

  // Now we fake the quadrature pulses. The code seems slightly complicated,
  // because I was originally experimenting with different delays between
  // state changes, depending on the speed the mouse was moving. But this
  // doesn't seem to make much difference! (To the Archimedes, anyway.)

  const int kDelayUS = 10; // microseconds
  // (This is a constant, but if I ever decide to put independent, calculated
  // delays for each axis, then this will be what I replace.)

  while (uX || uY) // keep doing this until both uX and uY are 0 (no more movement to simulate)
  {
    unsigned long uTimeNowUS = micros();

    //Serial.print(uX);
    //Serial.print(" ");
    //Serial.println(uY);

    if (uX && (uTimeNowUS - g_uLastUpdateX >= kDelayUS))
    {
      // time to move X axis on to next position
      if (bReverseX)
      {
        if (g_uStageX == 0) g_uStageX = 3; else --g_uStageX;
      }
      else
      {
        g_uStageX = (g_uStageX + 1) & 3;
      }

      --uX;

      UpdateX();
      g_uLastUpdateX = uTimeNowUS;
    }

    if (uY && (uTimeNowUS - g_uLastUpdateY >= kDelayUS))
    {
      // time to move Y axis on to next position
      if (bReverseY)
      {
        if (g_uStageY == 0) g_uStageY = 3; else --g_uStageY;
      }
      else
      {
        g_uStageY = (g_uStageY + 1) & 3;
      }

      --uY;

      UpdateY();
      g_uLastUpdateY = uTimeNowUS;
    }
  }
  
  while ((millis() - uStartTimeMS) < 5) {delay(1); }
}
