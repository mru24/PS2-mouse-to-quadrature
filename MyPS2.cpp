// MyPS2
//
// Based on the standard Arduino PS/2 library, but made more resilient to disconnects.
//

static const int kTimeout = 5; // ms

#include "MyPS2.h"

MyPS2::MyPS2(int clk, int data)
{
  bIsConnected = false;
	_ps2clk = clk;
	_ps2data = data;
	gohi(_ps2clk);
	gohi(_ps2data);
}

// returns false if failed to initialise.
bool MyPS2::InitMouse()
{
  // we assume that device IS connected to begin with. Any write or read that fails will change this state.
  // We check for that, and abort early.
  bIsConnected = true;

  /* reset */       write(0xff); if (!bIsConnected) {return false;}
  /* ack */         read();      if (!bIsConnected) {return false;} // BAT return code (0xAA or 0xFC)
  /* blank */       read();      if (!bIsConnected) {return false;} // device ID
  /* blank */       read();      if (!bIsConnected) {return false;}
  /* remote mode */ write(0xf0); if (!bIsConnected) {return false;} // set remote mode
  /* ack */         read();      if (!bIsConnected) {return false;}

  // note: apparently "Remote mode" is uncommon. More common is "Stream Mode". Might need to investigate moving to
  // this mode instead.

  delayMicroseconds(100);
  
  return true;
}

/*
 * according to some code I saw, these functions will
 * correctly set the clock and data pins for
 * various conditions.  It's done this way so you don't need
 * pullup resistors.
 */
void MyPS2::gohi(int pin)
{
	pinMode(pin, INPUT);
	digitalWrite(pin, HIGH);
}

void MyPS2::golo(int pin)
{
	pinMode(pin, OUTPUT);
	digitalWrite(pin, LOW);
}

// write a byte to the PS2 device
void MyPS2::write(unsigned char data)
{
	unsigned char i;
	unsigned char parity = 1;
	
	gohi(_ps2data);
	gohi(_ps2clk);
	delayMicroseconds(300);
	golo(_ps2clk);
	delayMicroseconds(300);
	golo(_ps2data);
	delayMicroseconds(10);
	gohi(_ps2clk);	// start bit
  
	// wait for device to take control of clock
  unsigned long timenow = millis();
	while (digitalRead(_ps2clk) == HIGH && (millis() - timenow) < kTimeout) { }
  if (digitalRead(_ps2clk) == HIGH)
  {
    // timed out!
    bIsConnected = false;
    return;
  }

	// clear to send data
	for (i = 0; i < 8; ++i)
	{
		if (data & 0x01)
		{
			gohi(_ps2data);
		} else
		{
			golo(_ps2data);
		}
   
		// wait for clock
    timenow = millis();
	  while (digitalRead(_ps2clk) == LOW && (millis() - timenow) < kTimeout) { }
    if (digitalRead(_ps2clk) == LOW)
    {
      // timed-out!
      bIsConnected = false;
      return;
    }

    timenow = millis();
		while (digitalRead(_ps2clk) == HIGH && (millis() - timenow) < kTimeout) { }
    if (digitalRead(_ps2clk) == HIGH)
    {
      // timed-out!
      bIsConnected = false;
      return;
     }
     
		parity = parity ^ (data & 0x01);
		data = data >> 1;
	}
	// parity bit
	if (parity)
	{
		gohi(_ps2data);
	} else
	{
		golo(_ps2data);
	}
 
	// clock cycle - like ack.
  timenow = millis();
	while (digitalRead(_ps2clk) == LOW && (millis() - timenow) < kTimeout) { }
	if (digitalRead(_ps2clk) == LOW)
  {
    // timed out!
    bIsConnected = false;
    return;
  }
  
  timenow = millis();
	while (digitalRead(_ps2clk) == HIGH && (millis() - timenow) < kTimeout) { }
  if (digitalRead(_ps2clk) == HIGH)
  {
    // timed-out!
    bIsConnected = false;
    return;
  }
  
	// stop bit
	gohi(_ps2data);
	delayMicroseconds(50);
  timenow = millis();
	while (digitalRead(_ps2clk) == HIGH && (millis() - timenow) < kTimeout) { }
  if (digitalRead(_ps2clk) == HIGH)
  {
    // timed out!
    bIsConnected = false;
    return;
  }
		
	// mode switch
  timenow = millis();
	while (  (digitalRead(_ps2clk) == LOW || digitalRead(_ps2data) == LOW) && (millis() - timenow) < kTimeout) { }
  if (digitalRead(_ps2clk) == LOW || digitalRead(_ps2data) == LOW)
  {
    // timed out!
    bIsConnected = false;
    return;
  }
  // hold up incoming data
  golo(_ps2clk);
}


/*
 * read a byte of data from the ps2 device.  Ignores parity.
 */
unsigned char MyPS2::read(void)
{
	unsigned char data = 0x00;
	unsigned char i;
	unsigned char bit = 0x01;

  // start clock
  gohi(_ps2clk);
  gohi(_ps2data);
  delayMicroseconds(50);
  
  unsigned long timenow = millis();
  while (digitalRead(_ps2clk) == HIGH/* && (millis() - timenow) < (kTimeout * 10)*/) { }
  /*if (digitalRead(_ps2clk) == HIGH)
  {
    // timed out!
    bIsConnected = false;
    return 0;
  }*/
  
  delayMicroseconds(5);	// not sure why.
  timenow = millis();
  while (digitalRead(_ps2clk) == LOW && (millis() - timenow) < kTimeout) { } // eat start bit
  if (digitalRead(_ps2clk) == LOW)
  {
    // timed out!
    bIsConnected = false;
    return 0;
  }
  
  for (i = 0; i < 8; ++i)
  {
    timenow = millis();
    while (digitalRead(_ps2clk) == HIGH && (millis() - timenow) < kTimeout) { }
    if (digitalRead(_ps2clk) == HIGH)
    {
      // timed-out!
      bIsConnected = false;
      return 0;
    } 
			
		if (digitalRead(_ps2data) == HIGH)
		{
			data = data | bit;
		}
   
    timenow = millis();
		while (digitalRead(_ps2clk) == LOW && (millis() - timenow) < kTimeout) { }
    if (digitalRead(_ps2clk) == LOW)
    {
      // timed-out!
      bIsConnected = false;
      return 0;
    }

		bit = bit << 1;
	}
 
  // eat parity bit, ignore it.
  timenow = millis();
  while (digitalRead(_ps2clk) == HIGH && (millis() - timenow) < kTimeout) { }
  while (digitalRead(_ps2clk) == LOW && (millis() - timenow) < kTimeout) { }
		
  // eat stop bit
  while (digitalRead(_ps2clk) == HIGH && (millis() - timenow) < kTimeout) { }
  while (digitalRead(_ps2clk) == LOW && (millis() - timenow) < kTimeout) { }

  if ((millis() - timenow) >= kTimeout)
  {
    bIsConnected = false;
    return 0;
  }

  golo(_ps2clk);	// hold incoming data

	return data;
}

