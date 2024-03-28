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
#define M_PI 3.14159265358979323846

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

class PowerSupply {
public:
    ViSession defaultRM;
    ViSession instr;
    ViStatus status;
    ViUInt32 retCount;
    ViUInt32 writeCount;
    unsigned char buffer[100];
    char stringinput[512];

    PowerSupply(const char* descriptor) {
        status = viOpenDefaultRM(&this->defaultRM);
        if (status < VI_SUCCESS) {
            printf("Could not open a session to the VISA Resource Manager!\n");
            exit(EXIT_FAILURE);
        }
        std::cout << "Connecting to the device\n";
        status = viOpen(this->defaultRM, descriptor, VI_NULL, VI_NULL, &this->instr);
        if (status < VI_SUCCESS) {
            printf("Cannot open a session to the device.\n");
        }
        status = viSetAttribute(instr, VI_ATTR_TMO_VALUE, 5000);
    }

    void command() {
        status = viWrite(instr, (ViBuf)this->stringinput, (ViUInt32)strlen(this->stringinput), &this->writeCount);
        if (status < VI_SUCCESS) {
            std::cout << "Error writing to the device\n";
        }
        memset(stringinput, 0, 512 * sizeof(char));
    }

    void reset() {
        std::cout << "Resetting the device\n\n";
        strcpy(stringinput, "*rst\n");
        command();
    }

    void setCurrent(float current, float voltageLimit) {
        sprintf(stringinput, "func:mode curr;:curr %f;:volt %f;:outp on\n", current, voltageLimit);
        command();
    }

    void setCurrentList(float* currentList, int length, float voltageLimit, float dwell, int count) {
        sprintf(stringinput, "list:cle;:list:dwel %f;:func:mode curr;:volt %d\n", dwell, voltageLimit);
        command();
        for (int i = 0; i < length; i++) {
            if (i % 8 == 0) {
                strcpy(stringinput, "list:volt ");
            }
            strcat(stringinput, std::to_string(currentList[i]).substr(0, 5).c_str());
            if (i % 8 != 7 && i != length - 1) {
                strcat(stringinput, ",");
            } else {
                strcat(stringinput, "\n");
                command();
            }
        }
        sprintf(stringinput, "list:coun %d;:outp on;:curr:mode list\n", count);
        command();
    }

class MagnetSystem {
public:
    PowerSupply PSX;
    PowerSupply PSY;
    PowerSupply PSZ;
    DWORD dwResult;
    XINPUT_STATE state;
    const int numSteps = 48;
    float voltageLimit = 20;
    float zCurrent = 0;
    float xyCurrent = 0;
    float cosLUT[numSteps];
    float sinLUT[numSteps];
    float xHoppingLUT[numSteps * 2];
    float yHoppingLUT[numSteps * 2];

    MagnetSystem(const char* descriptorX, const char* descriptorY, const char* descriptorZ, 
                float zCurrent, float xyCurrent, float voltageLimit, int numSteps) {
        this->PSX = PowerSupply(descriptorX);
        this->PSY = PowerSupply(descriptorY);
        this->PSZ = PowerSupply(descriptorZ);
        PSX.reset();
        PSY.reset();
        PSZ.reset();
        this->zCurrent = zCurrent;
        this->xyCurrent = xyCurrent;
        this->voltageLimit = voltageLimit;
        this->numSteps = numSteps;
        fillTrigLUTs(xyCurrent);
    }

    void fillTrigLUTs(float curr) {
        for (int i = 0; i < numSteps; i++) {
            this->currentCosLUT[i] = cos((i * M_PI) / numSteps) * curr;
            this->currentSinLUT[i] = sin((i * M_PI) / numSteps) * curr;
        }
    }

    void fillHoppingLUTs(int direction) {

    }

    void initializeController() {
        ZeroMemory(&this->state, sizeof(this->XINPUT_STATE));
        dwResult = XInputGetState(0, &this->state);
        if (dwResult == ERROR_SUCCESS) {
            std::std::cout << "Controller is connected!\n";
        } else {
            std::std::cout << "Controller " << 0 << " is not connected!\n";
        }
    }

    void joystickControl() {
        float LX = state.Gamepad.sThumbLX;
        std::cout << "Left Joystick X-Value " << LX << "\n";
        PSX.setCurrent((LX / 32768) * this->xyCurrent, this->voltageLimit);
        float LY = state.Gamepad.sThumbLY;
        std::cout << "Left Joystick Y-Value " << LY << "\n";
        PSX.setCurrent((LY / 32768) * this->xyCurrent, this->voltageLimit);
    }

    void triggerControl() {
        float RT = state.Gamepad.bRightTrigger;
        float LT = state.Gamepad.bLeftTrigger;
        if (RT > 50 || LT > 50) {
            PSZ.setCurrent(this->zCurrent * -1, this->voltageLimit);
        } else if (RT < 50) {
            PSZ.setCurrent(this->zCurrent, this->voltageLimit);
        }
    }

    void xButtonControl() {
        float time = 0;
        float angle = 0;
        while (X == 16384) {  
            angle = 2 * M_PI * this->freq * time;
            PSX.setCurrent(this->xyCurrent * sin(angle), this->voltageLimit);
            PSY.setCurrent(this->xyCurrent * cos(angle), this->voltageLimit);

            //Determine status of x-button for next iteration of while loop
            dwResult = XInputGetState(0, &state);
            X = state.Gamepad.wButtons;
            time += 0.08;
        }
    }
};

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


void loop(float freq, float voltageLimit, float zfield, float xyfield) {
    //X-button
    float time = 0; //seconds
    while (X == 16384) {   
        current = xyfield * (cos(2 * M_PI * freq * time));
        PSY.setCurrent(current, voltageLimit);
        
        current = xyfield * (sin(2 * M_PI * freq * time));
        PSX.setCurrent(current, voltageLimit);

        //Determine status of x-button for next iteration of while loop
        dwResult = XInputGetState(0, &state);
        X = state.Gamepad.wButtons;

        time += 0.08;
    }

    //float T = 1.0 / freq;
    //float TS = T / (2.0 * numSteps);

    while (RightButton == 8) {
        
    }
    if (Start == 16) {
        PSX.reset();
        PSY.reset();
        PSZ.reset();
        run = false;
    }
    dwResult = XInputGetState(0, &state);
    Start = state.Gamepad.wButtons;
}



int main(void) {
    DWORD dwResult;
    XINPUT_STATE state;
    string input;
    float voltage = 20;
    float current = 0;
    float freq;
    float period;


    /*Initializing the device to zero */

    std::cout << "Current set to " << current << " Amps" << endl;
    std::cout << "freq: ";
    std::cin >> freq;

    // Z-FIELD 
    float zfield;
    std::cout << "Z-field Current: ";
    std::cin >> zfield;

    // XY-FIELD 
    float xyfield;
    std::cout << "XY-field Current: ";
    std::cin >> xyfield;

    loop(freq, voltage, zfield, xyfield);
}        