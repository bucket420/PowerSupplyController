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
# define M_PI           3.14159265358979323846  /* pi */
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <iostream>
#include <chrono>
#include "visa.h"
#include <thread>
#include <Windows.h>
#include "Xinput.h"
#include <cmath>
#include <math.h>




#pragma comment(lib,"XInput.lib")
#pragma comment(lib,"Xinput9_1_0.lib")

static ViSession defaultRM;
static ViSession instr;
static ViStatus status;
static ViUInt32 retCount;
static ViUInt32 writeCount;
static unsigned char buffer[100];
static char stringinput[512];

static ViSession defaultRM2;
static ViSession instr2;
static ViStatus status2;
static ViUInt32 retCount2;
static ViUInt32 writeCount2;
static unsigned char buffer2[100];
static char stringinput2[512];

static ViSession defaultRM3;
static ViSession instr3;
static ViStatus status3;
static ViUInt32 retCount3;
static ViUInt32 writeCount3;
static unsigned char buffer3[100];
static char stringinput3[512];


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

void zero() {
    /*Initializing the device to zero */

    cout << "\nResetting the device to zero Amp output\n\n";
    double voltage = 20;
    double current = 0;
    string input = "func:mode curr;:curr ";
    input.append(to_string(current).substr(0, 5));
    input.append(";:volt ");
    input.append(to_string(voltage).substr(0, 5));
    input.append(";:outp on\n");
    strcpy(stringinput, input.c_str());
    strcpy(stringinput2, input.c_str());
    strcpy(stringinput3, input.c_str());
    status = viWrite(instr, (ViBuf)stringinput, (ViUInt32)strlen(stringinput), &writeCount);
    status2 = viWrite(instr2, (ViBuf)stringinput2, (ViUInt32)strlen(stringinput2), &writeCount2);
    status3 = viWrite(instr3, (ViBuf)stringinput3, (ViUInt32)strlen(stringinput3), &writeCount3);
    if (status < VI_SUCCESS)
    {
        cout << "Error writing to the device\n";
        cout << "Closing Sessions\nHit enter to continue.";
        //fflush(stdin);
        getchar();
        status = viClose(instr);
        status = viClose(defaultRM);
    }
    if (status2 < VI_SUCCESS)
    {
        cout << "Error writing to the device\n";
        cout << "Closing Sessions\nHit enter to continue.";
        //fflush(stdin);
        getchar();
        status2 = viClose(instr2);
        status2 = viClose(defaultRM2);
    }
    if (status3 < VI_SUCCESS)
    {
        cout << "Error writing to the device\n";
        cout << "Closing Sessions\nHit enter to continue.";
        //fflush(stdin);
        getchar();
        status3 = viClose(instr3);
        status3 = viClose(defaultRM3);
    }
}



void supply1(double voltage, double current) {
  
    // Power supply 7 for the X-Field

    string input = "func:mode curr;:curr ";
    input.append(to_string(current).substr(0, 5));
    input.append(";:volt ");
    input.append(to_string(voltage).substr(0, 5));
    input.append(";:outp on\n");
    strcpy(stringinput, input.c_str());
    status = viWrite(instr, (ViBuf)stringinput, (ViUInt32)strlen(stringinput), &writeCount);
    if (status < VI_SUCCESS)
    {
        //cout << "Error writing to the device\n";
        //cout << "Closing Sessions\nHit enter to continue.";
        //fflush(stdin);
        getchar();
        status = viClose(instr);
        status = viClose(defaultRM);
    }
}

void supply2(double voltage, double current) {
    // Power supply 8 for the Y-Field 
    string input = "func:mode curr;:curr ";
    input.append(to_string(current).substr(0, 5));
    input.append(";:volt ");
    input.append(to_string(voltage).substr(0, 5));
    input.append(";:outp on\n");
    strcpy(stringinput2, input.c_str());
    status2 = viWrite(instr2, (ViBuf)stringinput2, (ViUInt32)strlen(stringinput2), &writeCount2);
    if (status2 < VI_SUCCESS)
    {
        //cout << "Error writing to the device\n";
        //cout << "Closing Sessions\nHit enter to continue.";
        //fflush(stdin);
        getchar();
        status2 = viClose(instr2);
        status2 = viClose(defaultRM2);
    }
    //cout << "Supply 2 Current set to " << current << endl;
}

void supply3(double voltage, double current) {
    // Power supply 9 for the Z-Field 
    string input = "func:mode curr;:curr ";
    input.append(to_string(current).substr(0, 5));
    input.append(";:volt ");
    input.append(to_string(voltage).substr(0, 5));
    input.append(";:outp on\n");
    strcpy(stringinput3, input.c_str());
    status3 = viWrite(instr3, (ViBuf)stringinput3, (ViUInt32)strlen(stringinput3), &writeCount3);
    if (status3 < VI_SUCCESS)
    {
        //cout << "Error writing to the device\n";
        //cout << "Closing Sessions\nHit enter to continue.";
        //fflush(stdin);
        getchar();
        status3 = viClose(instr3);
        status3 = viClose(defaultRM3);
    }
    //cout << "Supply 3 Current set to " << current << endl;
}


void loop(double current, double freq, double voltage, string input, double zfield, double xyfield) {
    // Code that connects to the Xbox controller
    DWORD dwResult;
    XINPUT_STATE state;

    ZeroMemory(&state, sizeof(XINPUT_STATE));

    // Simply get the state of the controller from XInput.
    dwResult = XInputGetState(0, &state);

    if (dwResult == ERROR_SUCCESS)
    {
        // Controller is connected
        std::cout << "Controller is connected!\n";
    }
    else
    {
        // Controller is not connected
        std::cout << "Controller " << 0 << " is not connected!:-(\n";
    }

    boolean run = true; 
    /* Set Output Current */
    while (run)
    {
        dwResult = XInputGetState(0, &state);
        float LT = state.Gamepad.bLeftTrigger;
        std::cout << "Left Trigger Value " << LT << "\n";
        float RT = state.Gamepad.bRightTrigger;
        std::cout << "Right Trigger Value " << RT << "\n";
        float LX = state.Gamepad.sThumbLX;
        std::cout << "Left Joystick X-Value " << LX << "\n";
        float LY = state.Gamepad.sThumbLY;
        std::cout << "Left Joystick Y-Value " << LY << "\n";
        float X = state.Gamepad.wButtons;
        std::cout << "Button Value " << X << "\n";
        float Y = state.Gamepad.wButtons;
        float RightButton = state.Gamepad.wButtons;
        float LeftButton = state.Gamepad.wButtons;
        float DownButton = state.Gamepad.wButtons;
        float UpButton = state.Gamepad.wButtons;
        float Start = state.Gamepad.wButtons;       
        cout << "\n";

        // X-FIELD
        if (LX < 0) {
            current = (LX / -32768) * xyfield * -1;
            supply1(voltage, current);
        }

        else if (LX > 0) {
            current = (LX / 32767) * xyfield;
            supply1(voltage, current);
        }

        else if (LX == 0) {
            current = 0;
            supply1(voltage, current);
        }
     
        // Y-FIELD
        if (LY < 0) {
            current = (LY / -32768) * xyfield * -1;       
            supply2(voltage, current);
        }
        else if (LY > 0 ) {
            current = (LY / 32767) * xyfield;
            supply2(voltage, current);
        }
        else if (LY == 0) {
            current = 0;
            supply2(voltage, current);
        }

        // Z-FIELD
        if (RT > 50 || LT > 50) {
            current = zfield * -1;
            supply3(voltage, current);       
        }
        else if (RT < 50) {
            current = zfield;
            supply3(voltage, current);
        }

        //X-button
        double time = 0; //seconds
        while (X == 16384) {   
            current = xyfield * (cos(2 * M_PI * freq * time));
            supply2(voltage, current);
            
            current = xyfield * (sin(2 * M_PI * freq * time));
            supply1(voltage, current);

            //Determine status of x-button for next iteration of while loop
            dwResult = XInputGetState(0, &state);
            X = state.Gamepad.wButtons;

            time += 0.08;
        }
  
        //Disk Hopping
        // period 
        double T = 1.0 / freq;

        // number of steps for rotation of field halfway around 
        int NumSteps = 24; 

        // Timestep between each small change in field when rotating is occurring 
        double TS = T / (2.0 * NumSteps);

        // Time between Z-field switches, in ms
        int TZ_m = ((T / 2) - TS) * 1000;

        // NumStep+1 array of angles from zero to pi 
        double AngleArray[25];
        
        // NumStep+1 array of cosines of the angles above
        double CosineArray[25];

        // Array of x-components
        double XComps[25];

        // NumStep+1 array of sines of the angles above
        double SineArray[25];

        // Array of y-components
        double YComps[25];

        // counter
        int ihop = 0;

        //Right Disk Hopping
        for (int i = 0; i < 25; i++) {
            AngleArray[i] = -1 * (i * M_PI) / NumSteps;
            CosineArray[i] = cos(AngleArray[i]);
            XComps[i] = CosineArray[i] * xyfield;
            SineArray[i] = sin(AngleArray[i]);
            YComps[i] = SineArray[i] * xyfield;
        }
        while (RightButton == 8) {
            auto start = std::chrono::high_resolution_clock::now();
            // Set z-field to UP
            supply3(voltage, zfield);
            auto end = std::chrono::high_resolution_clock::now(); std::chrono::duration<double> elapsed_seconds = end - start;
            std::cout << "elapsed time (supply3): " << elapsed_seconds.count() << "s" << std::endl;

            /*start = std::chrono::high_resolution_clock::now();
            std::this_thread::sleep_for(std::chrono::milliseconds(TZ_m));
            end = std::chrono::high_resolution_clock::now(); elapsed_seconds = end - start; std::cout
                << "elapsed time for sleeping (expecting " << (double) TZ_m / 1000 <<"): " << elapsed_seconds.count() << "s" << std::endl; start = std::chrono::high_resolution_clock::now();*/

            while (ihop < 25) {
                auto start = std::chrono::high_resolution_clock::now();
                std::this_thread::sleep_for(std::chrono::milliseconds((int) (TS * 1000)));
                end = std::chrono::high_resolution_clock::now(); elapsed_seconds = end - start; std::cout
                    << "elapsed time for sleeping (expecting " << TS << "): " << elapsed_seconds.count() << "s" << std::endl; start = std::chrono::high_resolution_clock::now();

                start = std::chrono::high_resolution_clock::now();
                double XCurrent = XComps[ihop];
                supply1(voltage, XCurrent);
                end = std::chrono::high_resolution_clock::now(); elapsed_seconds = end - start; std::cout
                    << "elapsed time for supply1: " << elapsed_seconds.count() << "s" << std::endl; start = std::chrono::high_resolution_clock::now();

                start = std::chrono::high_resolution_clock::now();
                double YCurrent = YComps[ihop];
                supply2(voltage, YCurrent);
                end = std::chrono::high_resolution_clock::now(); elapsed_seconds = end - start; std::cout
                    << "elapsed time for supply2: " << elapsed_seconds.count() << "s" << std::endl; start = std::chrono::high_resolution_clock::now();
                ihop++;
            }
            start = std::chrono::high_resolution_clock::now();
            dwResult = XInputGetState(0, &state);
            RightButton = state.Gamepad.wButtons;
            end = std::chrono::high_resolution_clock::now(); elapsed_seconds = end - start; std::cout
                << "elapsed time for XBox right button input: " << elapsed_seconds.count() << "s" << std::endl; start = std::chrono::high_resolution_clock::now();

            start = std::chrono::high_resolution_clock::now();
            // Set z-field to DOWN
            supply3(voltage, zfield * -1);
            end = std::chrono::high_resolution_clock::now(); elapsed_seconds = end - start; std::cout
                << "elapsed time for supply3: " << elapsed_seconds.count() << "s" << std::endl; start = std::chrono::high_resolution_clock::now();

            //start = std::chrono::high_resolution_clock::now();
            //std::this_thread::sleep_for(std::chrono::milliseconds(TZ_m));
            //end = std::chrono::high_resolution_clock::now(); elapsed_seconds = end - start; std::cout
            //    << "elapsed time for sleeping (expecting " << (double) TZ_m / 1000 << "): " << elapsed_seconds.count() << "s" << std::endl; start = std::chrono::high_resolution_clock::now();

            if (RightButton == 8) {
                while (ihop > 0) {
                    ihop--;
                    start = std::chrono::high_resolution_clock::now();
                    std::this_thread::sleep_for(std::chrono::milliseconds((int) (TS * 1000)));
                    end = std::chrono::high_resolution_clock::now(); elapsed_seconds = end - start; std::cout
                        << "elapsed time for sleeping (expecting " << TS << "): " << elapsed_seconds.count() << "s" << std::endl; start = std::chrono::high_resolution_clock::now();

                    start = std::chrono::high_resolution_clock::now();
                    double XCurrent = XComps[ihop];
                    supply1(voltage, XCurrent);
                    end = std::chrono::high_resolution_clock::now(); elapsed_seconds = end - start; std::cout
                        << "elapsed time for supply1: " << elapsed_seconds.count() << "s" << std::endl; start = std::chrono::high_resolution_clock::now();

                    start = std::chrono::high_resolution_clock::now();
                    double YCurrent = YComps[ihop];
                    supply2(voltage, YCurrent);
                    end = std::chrono::high_resolution_clock::now(); elapsed_seconds = end - start; std::cout
                        << "elapsed time for supply2: " << elapsed_seconds.count() << "s" << std::endl; start = std::chrono::high_resolution_clock::now();
                }
                start = std::chrono::high_resolution_clock::now(); 
                dwResult = XInputGetState(0, &state);
                RightButton = state.Gamepad.wButtons;
                end = std::chrono::high_resolution_clock::now(); elapsed_seconds = end - start; std::cout
                    << "elapsed time for XBox right button input: " << elapsed_seconds.count() << "s" << std::endl; start = std::chrono::high_resolution_clock::now();

                if (RightButton == 0) {
                    start = std::chrono::high_resolution_clock::now();
                    supply3(voltage, zfield);
                    end = std::chrono::high_resolution_clock::now(); elapsed_seconds = end - start; std::cout
                        << "elapsed time for supply3: " << elapsed_seconds.count() << "s" << std::endl; start = std::chrono::high_resolution_clock::now();

                    //start = std::chrono::high_resolution_clock::now();
                    //std::this_thread::sleep_for(std::chrono::milliseconds(TZ_m));
                    //end = std::chrono::high_resolution_clock::now(); elapsed_seconds = end - start; std::cout
                    //    << "elapsed time for sleeping (expecting " << (double) TZ_m / 1000 << "): " << elapsed_seconds.count() << "s" << std::endl;
                } 
            }
        }
        // Left Disk Hopping

        while (LeftButton == 4) {
            int num = 24;
            for (int i = 0; i < 25; i++) {
                AngleArray[i] = (num * M_PI) / NumSteps;
                CosineArray[i] = cos(AngleArray[i]);
                XComps[i] = CosineArray[i] * xyfield;
                SineArray[i] = sin(AngleArray[i]);
                YComps[i] = SineArray[i] * xyfield;
                num--;
            }
            // Set z-field to UP
            supply3(voltage, zfield);
            std::this_thread::sleep_for(std::chrono::milliseconds(TZ_m));
            while (ihop < 25) {
                Sleep(TS * 1000);
                double XCurrent = XComps[ihop];
                supply1(voltage, XCurrent);
                double YCurrent = YComps[ihop];
                supply2(voltage, YCurrent);
                ihop++;
            }
            dwResult = XInputGetState(0, &state);
            LeftButton = state.Gamepad.wButtons;

            // Set z-field to DOWN
            supply3(voltage, zfield * -1); 
            std::this_thread::sleep_for(std::chrono::milliseconds(TZ_m));
            if (LeftButton == 4) {
                while (ihop > 0) {
                    ihop--;
                    Sleep(TS * 1000);
                    double XCurrent = XComps[ihop];
                    supply1(voltage, XCurrent);
                    double YCurrent = YComps[ihop];
                    supply2(voltage, YCurrent);
                }
                dwResult = XInputGetState(0, &state);
                LeftButton = state.Gamepad.wButtons;
                if (LeftButton == 0) {
                    supply3(voltage, zfield);
                    std::this_thread::sleep_for(std::chrono::milliseconds(TZ_m));
                }  
            }
        }

        // Up Disk Hopping
        while (UpButton == 1) {
            for (int i = 0; i < 25; i++) {
                AngleArray[i] = -1 * (i * M_PI  / NumSteps);
                CosineArray[i] = cos(AngleArray[i]);
                YComps[i] = CosineArray[i] * xyfield;
                SineArray[i] = sin(AngleArray[i]);
                XComps[i] = SineArray[i] * xyfield;
            }

            // Set z-field to UP
            supply3(voltage, zfield);
            std::this_thread::sleep_for(std::chrono::milliseconds(TZ_m));
            while (ihop < 25) {
                Sleep(TS * 1000);
                double XCurrent = XComps[ihop];
                supply1(voltage, XCurrent);
                double YCurrent = YComps[ihop];
                supply2(voltage, YCurrent);
                ihop++;
            }
            dwResult = XInputGetState(0, &state);
            UpButton = state.Gamepad.wButtons;

            // Set z-field to DOWN
            supply3(voltage, zfield * -1);
            std::this_thread::sleep_for(std::chrono::milliseconds(TZ_m));
            if (UpButton == 1) {
                while (ihop > 0) {
                    ihop--;
                    Sleep(TS * 1000);
                    double XCurrent = XComps[ihop];
                    supply1(voltage, XCurrent);
                    double YCurrent = YComps[ihop];
                    supply2(voltage, YCurrent);
                }
                dwResult = XInputGetState(0, &state);
                UpButton = state.Gamepad.wButtons;
                if (UpButton == 0) {
                    supply3(voltage, zfield);
                    std::this_thread::sleep_for(std::chrono::milliseconds(TZ_m));
                }
            }
        }
        //DownButton 
        while (DownButton == 2) {
            int num = 24;
            for (int i = 0; i < 25; i++) {
                AngleArray[i] = (num * M_PI / NumSteps);
                CosineArray[i] = cos(AngleArray[i]);
                YComps[i] = CosineArray[i] * xyfield;
                SineArray[i] = sin(AngleArray[i]);
                XComps[i] = SineArray[i] * xyfield;
                num--;
            }
            // Set z-field to UP
            supply3(voltage, zfield);
            std::this_thread::sleep_for(std::chrono::milliseconds(TZ_m));
            while (ihop < 25) {
                Sleep(TS * 1000);
                double XCurrent = XComps[ihop];
                supply1(voltage, XCurrent);
                double YCurrent = YComps[ihop];
                supply2(voltage, YCurrent);
                ihop++;
            }
            dwResult = XInputGetState(0, &state);
            DownButton = state.Gamepad.wButtons;

            // Set z-field to DOWN
            supply3(voltage, zfield * -1);
            std::this_thread::sleep_for(std::chrono::milliseconds(TZ_m));
            if (DownButton == 2) {
                while (ihop > 0) {
                    ihop--;
                    Sleep(TS * 1000);
                    double XCurrent = XComps[ihop];
                    supply1(voltage, XCurrent);
                    double YCurrent = YComps[ihop];
                    supply2(voltage, YCurrent);
                }
                dwResult = XInputGetState(0, &state);
                DownButton = state.Gamepad.wButtons;
                if (DownButton == 0) {
                    supply3(voltage, zfield);
                    std::this_thread::sleep_for(std::chrono::milliseconds(TZ_m));
                }
            }
        }
        // stop the code by pressing Start on xbox contoller
        if (Start == 16) {
            zero();
            run = false;
        }
        dwResult = XInputGetState(0, &state);
        Start = state.Gamepad.wButtons;
    }
}


int main(void)
{
    DWORD dwResult;
    XINPUT_STATE state;
    string input;
    double voltage = 20;
    double current = 0;
    double freq;
    double period;

    /*
     * First we must call viOpenDefaultRM to get the resource manager
     * handle.  We will store this handle in defaultRM.
     */
    status = viOpenDefaultRM(&defaultRM);
    if (status < VI_SUCCESS)
    {
        printf("Could not open a session to the VISA Resource Manager!\n");
        exit(EXIT_FAILURE);
    }
    status2 = viOpenDefaultRM(&defaultRM2);
    if (status2 < VI_SUCCESS)
    {
        printf("Could not open a session to the VISA Resource Manager!\n");
        exit(EXIT_FAILURE);
    }
    status3 = viOpenDefaultRM(&defaultRM3);
    if (status3 < VI_SUCCESS)
    {
        printf("Could not open a session to the VISA Resource Manager!\n");
        exit(EXIT_FAILURE);
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
    cout << "This is version 2\n";
    cout << "Connecting to the device\n";
    status = viOpen(defaultRM, "ASRL3::INSTR", VI_NULL, VI_NULL, &instr);
    status2 = viOpen(defaultRM2, "ASRL4::INSTR", VI_NULL, VI_NULL, &instr2);
    status3 = viOpen(defaultRM3, "ASRL5::INSTR", VI_NULL, VI_NULL, &instr3);

    if (status < VI_SUCCESS)
    {
        printf("Cannot open a session to device1.\n");
        cout << "Closing Sessions\nHit enter to continue.";
        //fflush(stdin);
        getchar();
        status = viClose(instr);
        status = viClose(defaultRM);
    }
    if (status2 < VI_SUCCESS)
    {
        printf("Cannot open a session to device2.\n");
        cout << "Closing Sessions\nHit enter to continue.";
        //fflush(stdin);
        getchar();
        status2 = viClose(instr2);
        status2 = viClose(defaultRM2);
    }
    if (status3 < VI_SUCCESS)
    {
        printf("Cannot open a session to device3.\n");
        cout << "Closing Sessions\nHit enter to continue.";
        //fflush(stdin);
        getchar();
        status3 = viClose(instr3);
        status3 = viClose(defaultRM3);
    }

    /*
     * Set timeout value to 5000 milliseconds (5 seconds).
     */
    status = viSetAttribute(instr, VI_ATTR_TMO_VALUE, 5000);

    /* Resetting the device */
    cout << "Resetting the device\n\n";
    strcpy(stringinput, "*rst\n");
    strcpy(stringinput2, "*rst\n");
    strcpy(stringinput3, "*rst\n");
    status = viWrite(instr, (ViBuf)stringinput, (ViUInt32)strlen(stringinput), &writeCount);
    if (status < VI_SUCCESS)
    {
        cout << "Error writing to the device\n";
        cout << "Closing Sessions\nHit enter to continue.";
        //fflush(stdin);
        getchar();
        status = viClose(instr);
        status = viClose(defaultRM);
    }

    status2 = viWrite(instr2, (ViBuf)stringinput2, (ViUInt32)strlen(stringinput2), &writeCount2);
    if (status2 < VI_SUCCESS)
    {
        cout << "Error writing to the device\n";
        cout << "Closing Sessions\nHit enter to continue.";
        //fflush(stdin);
        getchar();
        status2 = viClose(instr2);
        status2 = viClose(defaultRM2);
    }

    status3 = viWrite(instr3, (ViBuf)stringinput3, (ViUInt32)strlen(stringinput3), &writeCount3);
    if (status3 < VI_SUCCESS)
    {
        cout << "Error writing to the device\n";
        cout << "Closing Sessions\nHit enter to continue.";
        //fflush(stdin);
        getchar();
        status3 = viClose(instr3);
        status3 = viClose(defaultRM3);
    }

    /*Initializing the device to zero */
    zero();

    cout << "Current set to " << current << " Amps" << endl;
    cout << "freq: ";
    cin >> freq;

    // Z-FIELD 
    double zfield;
    cout << "Z-field Current: ";
    cin >> zfield;

    // XY-FIELD 
    double xyfield;
    cout << "XY-field Current: ";
    cin >> xyfield;

    loop(current, freq, voltage, input, zfield, xyfield);
}        