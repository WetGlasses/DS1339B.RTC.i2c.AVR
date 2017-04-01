/*

Library for DS1339B RTC chip

Written By Shadman Sakib
2RA Technology Ltd.

This is a minimalistic library and not depended on any other 3rd party library. Required i2c and serial functions are included in this.
Just include this header file only. There is no .c file. So, dont worry. Tested with Atmel studio 7.
This library is built using the raw informations from the datasheet and documentations on i2c.
All the mechanisms, methodologies and references from datasheet are cited as much as possible.
So, one can use this file as a tutorial too ^_^
Have fun.


Need these:
-----------

#define F_CPU

#include <avr/io.h>
#include <stdlib.h>
#include <util/delay.h>
#include <avr/interrupt.h>

LCD:
----
#include "hd44780_settings.h"
#include "hd44780.h"
#include "hd44780.c"

*/

////////////////////////////////////////	i2c Main Method		////////////////////////////////////////////////////////////

/*
	I2C Reading steps:
 
1.	Start Bit
2.	Transmit the slave address with write bit: R/W = 0
3.	Write register address.
4.	Start bit (restart)
5.	Transmit the slave address with read bit: R/W = 1
6.	Read data byte.
7.	STOP bit

*/


/*

	I2C writing steps:

1.	send START
2.	write slave address with last bit as WRITE ACCESS(0)
3.	write sub-address: this is the address of the register you what to write to; if not applicable skip to 4.
4.	write data
5.	send STOP

*/



////////////////////////////////////////////////// Address Map Definitions for DS1339B	///////////////////////////////////////////////

#define slave_add_write	0xd0
#define slave_add_read	0xd1

#define sec_add	0x00
#define min_add	0x01
#define hr_add	0x02
#define day_add	0x03

#define DD_add	0x04
#define MM_add	0x05
#define YY_add	0x06
#define Device_ID 0x0A







//////////////////////////////////	Common functions for all i2c devices	//////////////////////////////////////////////

void i2c_init (void);					//	Must initialize in the main function. Setup the whole thing
void i2c_start(void);					//	Start the communication. "Start bit"
void i2c_stop(void);					//	Stops the communication. "Stop bit"
int i2c_read(void);					//	Reading last incoming data from i2c data register.  
void i2c_write(char data);


///////////////////////////////////	Functions for DS1339B i2c RTC	///////////////////////////////////////////////////////////////////////


						///	Time functions	///

int RTC_sec(void);					//	Reading second
int RTC_min(void);					//	Reading minuite
int RTC_hr(void);					//	Reading hour
int RTC_day(void);					//	Reading day

void Time_update(void);					//	Updates time (sec, min, hour, day) to a string


						///	Calendar functions	///

int RTC_DD(void);
int RTC_MM(void);
int RTC_YY(void);

void Date_update(void);					//	Updates calender (DD, MM, YY) to a string



/////////////////////////////////////////////	Functions for  Setup RTC	///////////////////////////////////////


void set_RTC(void);							//	Setup new time and date

void set_Date(void);							//	Naamei porichoy, isn't it? :v 
void set_Time(void);							//	Resets time and date from a global string


char int2bcd(int input);						//	int to BCD converter. Needed everywhere. 


char give_me_number(char ki_chao);


/////////////////////////////////////////////	Global variables for RTC	///////////////////////////////////////

char time[16];							//	Stores time data for update (16 because of the full length of LCD)
char date[16];							//	Stores date data for update



/////////////////////////////////////////////	Global variables for Ethernet and new time for resetting RTC	///////////////////////////////////////


int byte_count = 0;									//	For incoming data decoding
char LAN_Packet[10];									//	Lan Data
char New_time_date[13] = "215000150514";			//	New Setup goes here. [hh-mm-ss-DD-MM-YY]



/////////////////////////////////////////////	Global variables for Random temporary works	////////////////////////////////////////////////////////

int temp_time;

///////////////////////////////////////////////		I2C	Functions	//////////////////////////////////////////// 


void i2c_init (void)
{
	TWSR = 0;						//	Status register. Prescaling is 1. So, all are 0s :P 
	TWBR = 32;						//	Two wire bit rate - equation in data sheet. Page 204. SHould not be less than 10
	TWCR = (1 << TWEN);					//	NACK
	_delay_ms(10);
}


void i2c_start(void)
{
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);		//	data sheet page 211, 213.
												//	Setting the interrupt will clear it like some other flags.. You know-- AVR's magic mushroom :v
	while (!(TWCR & (1<<TWINT)));				//	Wait for the execution completion flag (interrupt goes high by hardware)
}


void i2c_stop(void)
{
	TWCR = (1<<TWINT)|(1<<TWSTO)|(1<<TWEN);			//	data sheet page 211, 213.
												//	Setting the interrupt will clear it like some other flags.. You know-- AVR's magic mushroom :v
	_delay_ms(10);								//	Wait for the execution completion flag (interrupt goes high by hardware)
}


int i2c_read(void)
{
	int data;
	TWCR = (1 << TWINT) | (1 << TWEN);
	_delay_ms(10);
	data = TWDR;						//	Read the last received data from the i2c Data Register.
	return data;						//	Yes, TWRB holds both last received and to-be transmitted data.
}


void i2c_write(char data)
{
	TWDR = data;						//	Write the required transmitting value in the i2c Data Register
	TWCR = (1 << TWINT) | (1 << TWEN);			//	Go..Go..Go... :D
	while (!(TWCR & (1<<TWINT)));				//	Wait for the execution completion flag (interrupt goes high by hardware)
}



///////////////////////////////////////////////		DS1339B	Functions	//////////////////////////////////////////// 

char give_me_number(char ki_chao)
{
	char result;
	
	if (ki_chao == 'h')			// Hour
	{
		result = (10*(time[0]-48)+ (time[1]-48));
	} 
	else if (ki_chao == 'm')	// Min
	{
		result = (10*(time[3]-48)+ (time[4]-48));
	}
	else if (ki_chao == 'M')	// Month
	{
		result = (10*(date[3]-48)+ (date[4]-48));
	}
	else if (ki_chao == 'D')	// Day
	{
		result = (10*(date[0]-48)+ (date[1]-48));
	}
	else if (ki_chao == 'Y')	// Year
	{
		result = (10*(date[6]-48)+ (date[7]-48));
	}
	else if (ki_chao == 's')	// second
	{
		result = (10*(time[6]-48)+ (time[7]-48));
	}
	else
	{
		result = 0;
	}
	return result;
}

///////////////////////////////////////////////		Time  Read Functions		//////////////////////////////////////////// 

int RTC_sec()
{
	int sec;
	i2c_start();
	i2c_write(slave_add_write);
	i2c_write(sec_add);
	i2c_start();							//	Restart is required for i2c communication :O
	i2c_write(slave_add_read);
	sec = i2c_read();
	i2c_stop();
	time[5]=':';
	time[7]=48+(sec & 0b00001111);
	time[6]=48+((sec & 0b01110000)>>4);
	return sec;
}


int RTC_min()
{
		int min;
		i2c_start();
		i2c_write(slave_add_write);
		i2c_write(min_add);
		i2c_start();						//	Restart is required for i2c communication :O
		i2c_write(slave_add_read);
		min = i2c_read();
		i2c_stop();
		time[2]=':';
		time[4]=48+(min & 0b00001111);
		time[3]=48+((min & 0b01110000)>>4);
		return min;
}


int RTC_hr()
{
		int hr;
		i2c_start();
		i2c_write(slave_add_write);
		i2c_write(hr_add);
		i2c_start();						//	Restart is required for i2c communication :O
		i2c_write(slave_add_read);
		hr = i2c_read();
		i2c_stop();
		time[1]=48+(hr & 0b00001111);
		time[0]=48+((hr & 0b01110000)>>4);
		return hr;
}


int RTC_day()
{
	return 0;
}


void Time_update(void)
{
	temp_time=RTC_sec();
	temp_time=RTC_min();
	temp_time=RTC_hr();
	
}


///////////////////////////////////////////////		Calender Read Functions		//////////////////////////////////////////// 

void Date_update(void)
{
	temp_time=RTC_DD();
	temp_time=RTC_MM();
	temp_time=RTC_YY();
	
}

int RTC_YY()
{
	int yy;
	i2c_start();
	i2c_write(slave_add_write);
	i2c_write(YY_add);
	i2c_start();							//	Restart is required for i2c communication :O
	i2c_write(slave_add_read);
	yy = i2c_read();
	i2c_stop();
	date[5]='/';
	date[7]=48+(yy & 0b00001111);
	date[6]=48+((yy & 0b01110000)>>4);
	return yy;
}


int RTC_MM()
{
	int mm;
	i2c_start();
	i2c_write(slave_add_write);
	i2c_write(MM_add);
	i2c_start();							//	Restart is required for i2c communication :O
	i2c_write(slave_add_read);
	mm = i2c_read();
	i2c_stop();
	date[2]='/';
	date[4]=48+(mm & 0b00001111);
	date[3]=48+((mm & 0b01110000)>>4);
	return mm;
}


int RTC_DD()
{
	int dd;
	i2c_start();
	i2c_write(slave_add_write);
	i2c_write(DD_add);
	i2c_start();							//	Restart is required for i2c communication :O
	i2c_write(slave_add_read);
	dd = i2c_read();
	i2c_stop();
	date[1]=48+(dd & 0b00001111);
	date[0]=48+((dd & 0b01110000)>>4);
	return dd;
}

///////////////////////////////////////////////		Time Setup Function		////////////////////////////////////////////

void set_RTC(void)
{
	set_Time();
	set_Date();
	
}

void set_Date(void)
{
	
	char temp[2];
	
	// Day Setting
	
		i2c_start();
		i2c_write(slave_add_write);
		i2c_write(DD_add);
		temp[0]= New_time_date[6];
		temp[1]= New_time_date[7];
		temp_time = atoi(temp);
		i2c_write(int2bcd(temp_time));
		i2c_stop();
		_delay_ms(10);
		
	// Month Setting
		
		i2c_start();
		i2c_write(slave_add_write);
		i2c_write(MM_add);
		temp[0]= New_time_date[8];
		temp[1]= New_time_date[9];
		temp_time = atoi(temp);
		i2c_write(int2bcd(temp_time));
		i2c_stop();
		_delay_ms(10);
		
	// Year Setting
		
		i2c_start();
		i2c_write(slave_add_write);
		i2c_write(YY_add);
		temp[0]= New_time_date[10];
		temp[1]= New_time_date[11];
		temp_time = atoi(temp);
		i2c_write(int2bcd(temp_time));
		i2c_stop();
}

void set_Time(void)
{

	
	char temp[2];
	
	// Second Setting
	
	i2c_start();
	i2c_write(slave_add_write);
	i2c_write(sec_add);
	temp[0]= New_time_date[4];
	temp[1]= New_time_date[5];
	temp_time = atoi(temp);
	i2c_write(int2bcd(temp_time));
	i2c_stop();
	_delay_ms(10);
	
	// Minute Setting
	
	i2c_start();
	i2c_write(slave_add_write);
	i2c_write(min_add);
	temp[0]= New_time_date[2];
	temp[1]= New_time_date[3];
	temp_time = atoi(temp);
	i2c_write(int2bcd(temp_time));
	i2c_stop();
	_delay_ms(10);
	
	// Hour Setting
	
	i2c_start();
	i2c_write(slave_add_write);
	i2c_write(hr_add);
	temp[0]= New_time_date[0];
	temp[1]= New_time_date[1];
	temp_time = atoi(temp);
	i2c_write(int2bcd(temp_time));
	i2c_stop();
		
}


///////////////////////////////////////////////////////// integer BCD conversion ////////////////////////////////////////////////////////////////////

char int2bcd(int input)
{
	char ans = 0x00;
	
	if (input<10)
	{
		ans = (0x30 << 4) | input;
	} 
	else
	{
		ans = ((input/10)<<4) | (input % 10);
	}
	return ans;
}






