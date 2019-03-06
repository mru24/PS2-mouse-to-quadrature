

#ifndef __MYPS2_H__
#define __MYPS2_H__

#include "Arduino.h"

class MyPS2
{
	public:
		MyPS2(int clk, int data);
		void write(unsigned char data);
		unsigned char read(void);
    bool InitMouse(void);
    bool IsConnected() const {return bIsConnected;}
    
	private:
    bool bIsConnected; // true if passed initialisation OK, or false if timed-out
		int _ps2clk;
		int _ps2data;
   
		void golo(int pin);
		void gohi(int pin);
};

#endif // __MYPS2_H__


