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
#include <string>
#include <iostream>
#include <chrono>
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

using namespace std;

int main(void)
{
    string input;
    double voltage;
    double current;
    double freq;

    
    /*
     * First we must call viOpenDefaultRM to get the resource manager
     * handle.  We will store this handle in defaultRM.
     */
   status=viOpenDefaultRM (&defaultRM);
   if (status < VI_SUCCESS)
   {
      printf("Could not open a session to the VISA Resource Manager!\n");
      exit (EXIT_FAILURE);
   } 
                                                                         
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
   cout << "Connecting to the device\n";
   status = viOpen(defaultRM, "ASRL4::INSTR", VI_NULL, VI_NULL, &instr);
   if (status < VI_SUCCESS)  
   {
        printf ("Cannot open a session to the device.\n");
        goto Close;
   }
  
   /*
    * Set timeout value to 5000 milliseconds (5 seconds).
    */ 
   status = viSetAttribute (instr, VI_ATTR_TMO_VALUE, 5000);

   /* Resetting the device */
   cout << "Resetting the device\n\n"; 
   strcpy(stringinput, "*rst\n");
   status = viWrite(instr, (ViBuf)stringinput, (ViUInt32)strlen(stringinput), &writeCount);
   if (status < VI_SUCCESS)
   {
       cout << "Error writing to the device\n";
       goto Close;
   }


   cout << "Current: ";
   cin >> current;
   cout << "Frequency: ";
   cin >> freq;
   voltage = 20;

   // 
   int half_cycle = 500 / freq;

   bool change = true;
   std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
   /* Set Output Current */
   while (true)
   {
       if ((std::chrono::steady_clock::now() - start).count() / 1000000 == half_cycle)
       {
           // cout << (std::chrono::steady_clock::now() - start).count() / 1000000 << endl;
           start = std::chrono::steady_clock::now();
           current *= -1;
           voltage *= -1;
           // cout << to_string(current).substr(0, 4) << endl;
           change = true;
       }

       if (change)
       {
           input = "func:mode curr;:curr ";
           input.append(to_string(current).substr(0, 5));
           input.append(";:volt ");
           input.append(to_string(voltage).substr(0, 5));
           input.append(";:outp on\n");
           strcpy(stringinput, input.c_str());
           status = viWrite(instr, (ViBuf)stringinput, (ViUInt32)strlen(stringinput), &writeCount);
           if (status < VI_SUCCESS)
           {
               cout << "Error writing to the device\n";
               goto Close;
           }
           cout << "Current set to " << current << endl;
           change = false;

           // Measure the output (under development)

           /*strcpy(stringinput, "func:mode ?;:meas:volt ?;:meas:curr?\n");
           status = viWrite(instr, (ViBuf)stringinput, (ViUInt32)strlen(stringinput), &writeCount);
           if (status < VI_SUCCESS)
           {
               cout << "Error writing to the device\n";
               goto Close;
           }
           status = viRead(instr, buffer, 100, &retCount);
           if (strchr((char*)buffer, 10)) strchr((char*)buffer, 10)[0] = 0;
           if (status < VI_SUCCESS)
           {
               cout << "Error reading a response from the device\n";
           }
           else
           {
               cout << "Volt; Curr: " << retCount << " " << buffer << endl;
           }*/
       }
   }

Close:
   cout << "Closing Sessions\nHit enter to continue.";
   //fflush(stdin);
   getchar();  
   status = viClose(instr);
   status = viClose(defaultRM);

   return 0;
}
