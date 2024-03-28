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
#include <thread>
#include <Windows.h>
#include "Xinput.h"
#include <cmath>
#include <math.h>

#pragma comment(lib,"XInput.lib")
#pragma comment(lib,"Xinput9_1_0.lib")
#define M_PI 3.14159265358979323846
#define NUM_STEPS 48

class PowerSupply {
public:
    ViSession defaultRM;
    ViSession instr;
    ViStatus status;
    ViUInt32 retCount;
    ViUInt32 writeCount;
    unsigned char buffer[100];
    char stringinput[512];

    PowerSupply() {
        std::cout << "Please provide device descriptor\n";
    }

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
        sprintf(stringinput, "list:cle;:list:dwel %f;:func:mode curr;:volt %f\n", dwell, voltageLimit);
        std::cout << stringinput;
        command();
        for (int i = 0; i < length; i++) {
            if (i % 8 == 0) {
                strcpy(stringinput, "list:curr ");
            }
            strcat(stringinput, std::to_string(currentList[i]).substr(0, 5).c_str());
            if (i % 8 != 7 && i != length - 1) {
                strcat(stringinput, ",");
            } else {
                strcat(stringinput, "\n");
                std::cout << stringinput;
                command();
            }
        }
        sprintf(stringinput, "list:coun %d;:outp on;:curr:mode list\n", count);
        std::cout << stringinput;
        command();
    }
};

class MagnetSystem {
public:
    PowerSupply PSX;
    PowerSupply PSY;
    PowerSupply PSZ;
    DWORD dwResult;
    XINPUT_STATE state;
    float voltageLimit;
    float zCurrent;
    float xyCurrent;
    float freq;
    float cosLUT[NUM_STEPS];
    float sinLUT[NUM_STEPS];
    float xHoppingLUT[NUM_STEPS * 2];
    float yHoppingLUT[NUM_STEPS * 2];
    float zHoppingLUT[2];
    float lastKeyPressed = 0;
    bool active = true;

    MagnetSystem(const char* descriptorX, const char* descriptorY, const char* descriptorZ, 
                float zCurrent, float xyCurrent, float freq, float voltageLimit) {
        PSX = PowerSupply(descriptorX);
        PSY = PowerSupply(descriptorY);
        PSZ = PowerSupply(descriptorZ);
        PSX.reset();
        PSY.reset();
        PSZ.reset();
        this->zCurrent = zCurrent;
        this->xyCurrent = xyCurrent;
        this->voltageLimit = voltageLimit;
        this->freq = freq;
        fillTrigLUTs(xyCurrent);
        zHoppingLUT[0] = zCurrent;
        zHoppingLUT[1] = -zCurrent;
    }

    void fillTrigLUTs(float curr) {
        for (int i = 0; i < NUM_STEPS; i++) {
            this->cosLUT[i] = cos((i * 2 * M_PI) / NUM_STEPS) * curr;
            this->sinLUT[i] = sin((i * 2 * M_PI) / NUM_STEPS) * curr;
        }
    }

    // need reworking
    void fillHoppingLUTs(int direction) {
        switch (direction) {
            case 8: // right
                for (int i = 0; i < NUM_STEPS / 2; i++) {
                    xHoppingLUT[i] = cosLUT[i];
                    yHoppingLUT[i] = -sinLUT[i];
                }
                for (int i = NUM_STEPS / 2; i < NUM_STEPS; i++) {
                    xHoppingLUT[i] = cosLUT[NUM_STEPS / 2];
                    yHoppingLUT[i] = -sinLUT[NUM_STEPS / 2];
                }
                for (int i = NUM_STEPS; i < NUM_STEPS * 3 / 2; i++) {
                    xHoppingLUT[i] = cosLUT[i - NUM_STEPS / 2];
                    yHoppingLUT[i] = -sinLUT[i - NUM_STEPS / 2];
                }
                for (int i = NUM_STEPS * 3 / 2; i < NUM_STEPS * 2; i++) {
                    xHoppingLUT[i] = cosLUT[0];
                    yHoppingLUT[i] = -sinLUT[0];
                }
                break;
            case 1: // up
                break;
            case 4: // left
                for (int i = 0; i < NUM_STEPS / 2; i++) {
                    xHoppingLUT[i] = -cosLUT[i];
                    yHoppingLUT[i] = sinLUT[i];
                }
                for (int i = NUM_STEPS / 2; i < NUM_STEPS; i++) {
                    xHoppingLUT[i] = -cosLUT[NUM_STEPS / 2];
                    yHoppingLUT[i] = sinLUT[NUM_STEPS / 2];
                }
                for (int i = NUM_STEPS; i < NUM_STEPS * 3 / 2; i++) {
                    xHoppingLUT[i] = -cosLUT[i - NUM_STEPS / 2];
                    yHoppingLUT[i] = sinLUT[i - NUM_STEPS / 2];
                }
                for (int i = NUM_STEPS * 3 / 2; i < NUM_STEPS * 2; i++) {
                    xHoppingLUT[i] = -cosLUT[0];
                    yHoppingLUT[i] = sinLUT[0];
                }
                break;
            case 2: // down
                break;
            default: // none
                break;
        }
    }

    void initializeController() {
        ZeroMemory(&state, sizeof(XINPUT_STATE));
        dwResult = XInputGetState(0, &state);
        if (dwResult == ERROR_SUCCESS) {
            std::cout << "Controller is connected!\n";
        } else {
            std::cout << "Controller " << 0 << " is not connected!\n";
        }
    }

    void joystickControl() {
        float LX = state.Gamepad.sThumbLX;
        std::cout << "Left Joystick X-Value " << LX << "\n";
        PSX.setCurrent((LX / 32768) * xyCurrent, voltageLimit);
        float LY = state.Gamepad.sThumbLY;
        std::cout << "Left Joystick Y-Value " << LY << "\n";
        PSY.setCurrent((LY / 32768) * xyCurrent, voltageLimit);
    }

    void triggerControl() {
        float RT = state.Gamepad.bRightTrigger;
        float LT = state.Gamepad.bLeftTrigger;
        if (RT > 50 || LT > 50) {
            PSZ.setCurrent(zCurrent * -1, voltageLimit);
        } else if (RT < 50) {
            PSZ.setCurrent(zCurrent, voltageLimit);
        }
    }

    void xButtonControl() {
        float time = 0;
        float angle;
        while (state.Gamepad.wButtons == 16384) {
            angle = 2 * M_PI * this->freq * time;
            PSX.setCurrent(xyCurrent * sin(angle), voltageLimit);
            PSY.setCurrent(xyCurrent * cos(angle), voltageLimit);

            //Determine status of x-button for next iteration of while loop
            dwResult = XInputGetState(0, &state);
            time += 0.08;
        }
    }

    void startButtonControl() {
        if (state.Gamepad.wButtons == 16) {
            PSX.reset();
            PSY.reset();
            PSZ.reset();
            active = false;
        }
    }

    void dirPadControl() {
        if (state.Gamepad.wButtons > 0 && state.Gamepad.wButtons < 9) {
            lastKeyPressed = state.Gamepad.wButtons;
            fillHoppingLUTs(lastKeyPressed);
            PSX.setCurrentList(xHoppingLUT, NUM_STEPS * 2, this->voltageLimit, 1 / freq / NUM_STEPS, 0);
            PSY.setCurrentList(yHoppingLUT, NUM_STEPS * 2, this->voltageLimit, 1 / freq / NUM_STEPS, 0);
            PSZ.setCurrentList(zHoppingLUT, 2, this->voltageLimit, 1 / freq / 2, 0);
        }
        while (state.Gamepad.wButtons == lastKeyPressed && state.Gamepad.wButtons != 0) {
            dwResult = XInputGetState(0, &state);
        }
        if (state.Gamepad.wButtons == 0 && lastKeyPressed > 0 && lastKeyPressed < 9) {
            PSX.reset();
            PSY.reset();
            PSZ.reset();
            lastKeyPressed = 0;
        }
    }

    void run() {
        while (active) {
            dwResult = XInputGetState(0, &state);
            joystickControl();
            triggerControl();
            xButtonControl();
            startButtonControl();
            dirPadControl();
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

int main(void) {
    float voltageLimit = 20;
    float freq;

    /*Initializing the device to zero */

    std::cout << "freq: ";
    std::cin >> freq;

    // Z-FIELD 
    float zCurrent;
    std::cout << "Z-field Current: ";
    std::cin >> zCurrent;

    // XY-FIELD 
    float xyCurrent;
    std::cout << "XY-field Current: ";
    std::cin >> xyCurrent;

    MagnetSystem magnets = MagnetSystem("ASRL3::INSTR", "ASRL4::INSTR", "ASRL5::INSTR", zCurrent, xyCurrent, freq, voltageLimit);
    magnets.initializeController();
    magnets.run();
}        