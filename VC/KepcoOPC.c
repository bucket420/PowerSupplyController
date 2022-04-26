/********************************************************************/
/* Kepco Sample Program using National Instruments VISA	            */
/* note : visa32.lib must be included in your project               */
/********************************************************************/
/*              Read and Write to an Instrument Example             */
/*                                                                  */
/* This code demonstrates synchronous read and write commands to a  */
/* GPIB, serial or message-based VXI instrument using VISA.         */
/*                                                                  */
/* The general flow of the code is                                  */
/*      Open Resource Manager                                       */
/*      Open VISA Session to an Instrument                          */
/*      Write the Identification Query Using viWrite                */
/*      Try to Read a Response With viRead                          */
/*      Close the VISA Session                                      */
/********************************************************************/

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_DEPRECATE)
#define _CRT_SECURE_NO_DEPRECATE
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "visa.h"

static ViSession defaultRM;
static ViSession instr;
static ViStatus status; 
static ViUInt32 retCount;
static ViUInt32 writeCount;
static unsigned char buffer[100];
static char stringinput[512];

/*
* In every source code or header file that you use it is necessary to prototype
* your VISA variables at the beginning of the file. You need to declare the VISA
* session, VISA integers, VISA strings, VISA pointers, and VISA floating variables.
* As an example, retCount (above), has been prototyped as a static variable
* to this particular module.   It is an integer of bit length 32.
* If you are uncertain how to declare your VISA prototypes refer to the
* VISA help under the Section titled Type Assignments Table. The VISA
* help is located in your NI-VISA directory or folder.
*/

int send_with_wait(char *cmnd){
	char snd[501];                                 // data to be sent to the power supply
	char rcv[10];                                 // data from power supply
	int j; char status_byte;

	sprintf(snd,"%s;:*OPC?", cmnd);              // Add *OPC? to the command
												// so there is a response from the power supply

	status = viWrite(instr, (ViBuf)snd, (ViUInt32)strlen(snd), &writeCount); // Send the data to the power supply
	if (status < VI_SUCCESS) { printf("Error writing to the device\n"); return 1; }
	printf("waiting...\n");
	for (j = 0; j < 5; j++){								// loop until ready (5 seconds max)
		_sleep(1000);										//  Wait for command to complete
		viReadSTB(instr, &status_byte);
		if ((status_byte & 0x10) == 0x10) j=5;				// by looking for data in string
	}
	status = viRead(instr, rcv, 1000, &retCount); // so the error queue will not receive a 410 error
	if (status < VI_SUCCESS) { printf("Error reading a response from the device\n"); }
}

int main(void)
{
    /*
     * First we must call viOpenDefaultRM to get the resource manager
     * handle.  We will store this handle in defaultRM.
     */
   status=viOpenDefaultRM (&defaultRM);
   if (status < VI_SUCCESS) { printf("Could not open a session to the VISA Resource Manager!\n"); exit (EXIT_FAILURE); } 
                                                                         
    /*
     * Now we will open a VISA session to a device at Primary Address 6.
     * You can use any address for your instrument. In this example we are 
     * using GPIB Primary Address 6.
     *
     * We must use the handle from viOpenDefaultRM and we must   
     * also use a string that indicates which instrument to open.  This
     * is called the instrument descriptor.  The format for this string
     * can be found in the NI-VISA User Manual.
     * After opening a session to the device, we will get a handle for 
     * the instrument which we will use in later VISA functions.  
     * The two parameters in this function which are left blank are
     * reserved for future functionality.  These two parameters are 
     * given the value VI_NULL.
     *
     * This example will also work for serial or LAN instruments by changing 
     * the instrument descriptor from GPIB0::6::INSTR,  e.g. ASRL1::INSTR or
     * TCPIP0::192.168.1.156::5025::SOCKET depending on the descriptor for your instrument.
     */

   status = viOpen (defaultRM,  "ASRL4::INSTR", VI_NULL, VI_NULL, &instr);
   if (status < VI_SUCCESS) { printf ("Cannot open a session to the device.\n"); goto Close; }
  
    /*
     * Set timeout value to 5000 milliseconds (5 seconds).
     */ 
   status = viSetAttribute (instr, VI_ATTR_TMO_VALUE, 5000);
   status = viSetAttribute(instr, VI_ATTR_TERMCHAR_EN, 1);
   status = viSetAttribute(instr, VI_ATTR_TERMCHAR, 0x0A);
  
    /*
     * At this point we now have a session open to the instrument at
     * Primary Address 6.  We can use this session handle to write 
     * an ASCII command to the instrument.  We will use the viWrite function
     * to send the string "*IDN?", asking for the device's identification.  
     */
   //

   strcpy(stringinput, "*IDN?\n");
   status = viWrite(instr, (ViBuf)stringinput, (ViUInt32)strlen(stringinput), &writeCount);
   if (status < VI_SUCCESS) { printf("Error writing to the device\n"); goto Close; }

  
    /*
     * Now we will attempt to read back a response from the device to
     * the identification query that was sent.  We will use the viRead
     * function to acquire the data.  We will try to read back 100 bytes.
     * After the data has been read the response is displayed.
     */

   status = viRead (instr, buffer, 1000, &retCount);
   if (status < VI_SUCCESS) { printf("Error reading a response from the device\n"); }
   else { printf("%*s\n",retCount,buffer); }
   
   /*** Set Output Volt & Curr Values & enable output ***/

   strcpy(stringinput, "sour:volt 5;:sour:curr 1;:outp on\n");
   status = viWrite(instr, (ViBuf)stringinput, (ViUInt32)strlen(stringinput), &writeCount);
   if (status < VI_SUCCESS)  { printf("Error writing to the device\n"); goto Close; }
   printf("ready - hit a key to continue\n");  getchar();

   /*** send  'save' cmnd & wait  for response ***/

   send_with_wait("*sav 1");

   printf("ready - hit a key to continue\n");  getchar();

   /*** send  a reset ***/

   printf("resetting\n"); strcpy(stringinput, "*rst\n");
   status = viWrite(instr, (ViBuf)stringinput, (ViUInt32)strlen(stringinput), &writeCount);
   if (status < VI_SUCCESS) { printf("Error writing to the device\n"); goto Close; }

   printf("ready - hit a key to continue\n");  getchar();

   /*** the following checks the saved setting using the recall cmnd ***/
   printf("recalling settings @1\n");strcpy(stringinput, "*rcl 1\n");
   status = viWrite(instr, (ViBuf)stringinput, (ViUInt32)strlen(stringinput), &writeCount);
   if (status < VI_SUCCESS)  { printf("Error writing to the device\n"); goto Close; }
   printf("ready - hit a key to continue\n");  getchar();

   /*** send  a reset again ***/

   printf("resetting\n"); strcpy(stringinput, "*rst\n");
   status = viWrite(instr, (ViBuf)stringinput, (ViUInt32)strlen(stringinput), &writeCount);
   if (status < VI_SUCCESS) { printf("Error writing to the device\n"); goto Close; }

   printf("ready - hit a key to continue\n");  getchar();
   /*
    * Now we will close the session to the instrument using
    * viClose. This operation frees all system resources.                     
    */

Close:
   printf("Closing Sessions\nHit enter to continue.");
   fflush(stdin);  getchar();  
   status = viClose(instr);
   status = viClose(defaultRM);

   return 0;
}
