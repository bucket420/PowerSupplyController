/********************************************************************/
/* Kepco Sample Program using National Instruments VISA	            */
/* note : visa32.lib must be included in your project               */
/********************************************************************/
/*              Read and Write to an Instrument Example             */
/*                                                                  */
/* This code demonstrates synchronous read and write executeCommands to a  */
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
    char command[512];

    PowerSupply() {
    }

    PowerSupply(const char* descriptor) {
        status = viOpenDefaultRM(&this->defaultRM);
        if (status < VI_SUCCESS) {
            printf("Could not open a session to the VISA Resource Manager!\n\n");
            exit(EXIT_FAILURE);
        }
        std::cout << "Connecting to the device\n\n";
        status = viOpen(this->defaultRM, descriptor, VI_NULL, VI_NULL, &this->instr);
        if (status < VI_SUCCESS) {
            printf("Cannot open a session to the device.\n\n");
        }
        status = viSetAttribute(instr, VI_ATTR_TMO_VALUE, 5000);
    }

    void executeCommand() {
        status = viWrite(instr, (ViBuf)this->command, (ViUInt32)strlen(this->command), &this->writeCount);
        if (status < VI_SUCCESS) {
            std::cout << "Error writing to the device\n\n";
        }
        memset(command, 0, 512 * sizeof(char));
    }

    void reset() {
        std::cout << "Resetting the device\n\n";
        strcpy(command, "*rst\n");
        executeCommand();
    }

    void setCurrent(float current, float voltageLimit) {
        sprintf(command, "func:mode curr;:curr %f;:volt %f;:outp on\n", current, voltageLimit);
        executeCommand();
    }

    void setCurrentList(float* currentList, int length, float voltageLimit, float dwell, int count) {
        sprintf(command, "list:cle;:list:dwel %f;:func:mode curr;:volt %f\n", dwell, voltageLimit);
        std::cout << command;
        executeCommand();
        for (int i = 0; i < length; i++) {
            if (i % 8 == 0) {
                strcpy(command, "list:curr ");
            }
            strcat(command, std::to_string(currentList[i]).substr(0, 5).c_str());
            if (i % 8 != 7 && i != length - 1) {
                strcat(command, ",");
            }
            else {
                strcat(command, "\n");
                std::cout << command;
                executeCommand();
            }
        }
        sprintf(command, "list:coun %d;:outp on;:curr:mode list\n", count);
        std::cout << command;
    }

    void setHoppingCurrentList(float* LUT, float voltageLimit, float dwell, int count, int start) {
        sprintf(command, "list:cle;:list:dwel %f;:func:mode curr;:volt %f\n", dwell, voltageLimit);
        std::cout << command;
        executeCommand();
        for (int i = 0; i < NUM_STEPS * 2; i++) {
            if (i % 8 == 0) {
                strcpy(command, "list:curr ");
            }
            if (i < NUM_STEPS / 2) {
                strcat(command, std::to_string(LUT[start]).substr(0, 5).c_str());
            }
            else if (i < NUM_STEPS) {
                strcat(command, std::to_string(LUT[(start + i - NUM_STEPS / 2) % NUM_STEPS]).substr(0, 5).c_str());
            }
            else if (i < NUM_STEPS * 3 / 2) {
                strcat(command, std::to_string(LUT[(start + NUM_STEPS / 2) % NUM_STEPS]).substr(0, 5).c_str());
            }
            else {
                strcat(command, std::to_string(LUT[(start + i - NUM_STEPS) % NUM_STEPS]).substr(0, 5).c_str());
            }
            if (i % 8 != 7 && i != NUM_STEPS * 2 - 1) {
                strcat(command, ",");
            }
            else {
                strcat(command, "\n");
                std::cout << command;
                executeCommand();
            }
        }
        sprintf(command, "list:coun %d;:outp on;:curr:mode list\n", count);
        std::cout << command;
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
    float zHoppingLUT[2];
    int lastKeyPressed = 0;
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

    void initializeController() {
        ZeroMemory(&state, sizeof(XINPUT_STATE));
        dwResult = XInputGetState(0, &state);
        if (dwResult == ERROR_SUCCESS) {
            std::cout << "Controller is connected!\n\n";
        }
        else {
            std::cout << "Controller " << 0 << " is not connected!\n\n";
        }
    }

    void joystickControl() {
        float LX = state.Gamepad.sThumbLX;
        // std::cout << "Left Joystick X-Value " << LX << "\n";
        PSX.setCurrent((LX / 32768) * xyCurrent, voltageLimit);
        float LY = state.Gamepad.sThumbLY;
        // std::cout << "Left Joystick Y-Value " << LY << "\n";
        PSY.setCurrent((LY / 32768) * xyCurrent, voltageLimit);
    }

    void triggerControl() {
        float RT = state.Gamepad.bRightTrigger;
        float LT = state.Gamepad.bLeftTrigger;
        if (RT > 50 || LT > 50) {
            PSZ.setCurrent(zCurrent * -1, voltageLimit);
        }
        else if (RT < 50) {
            PSZ.setCurrent(zCurrent, voltageLimit);
        }
    }

    void xButtonControl() {
        if (state.Gamepad.wButtons == 16384) {
            lastKeyPressed = state.Gamepad.wButtons;
            PSX.setCurrentList(cosLUT, NUM_STEPS, voltageLimit, 1 / freq / NUM_STEPS, 0);
            PSY.setCurrentList(sinLUT, NUM_STEPS, voltageLimit, 1 / freq / NUM_STEPS, 0);
            PSX.executeCommand();
            PSY.executeCommand();
        }
        while (state.Gamepad.wButtons == lastKeyPressed && state.Gamepad.wButtons != 0) {
            dwResult = XInputGetState(0, &state);
        }
        if (state.Gamepad.wButtons == 0 && lastKeyPressed == 16384) {
            PSX.reset();
            PSY.reset();
            lastKeyPressed = 0;
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
            int start = 0;
            switch (lastKeyPressed) {
            case 8: // right
                start = 0;
                break;
            case 1: // up
                start = NUM_STEPS / 4;
                break;
            case 4: // left
                start = NUM_STEPS / 2;
                break;
            case 2: // down
                start = NUM_STEPS * 3 / 4;
                break;
            }
            PSX.setHoppingCurrentList(cosLUT, voltageLimit, 1 / freq / NUM_STEPS, 0, start);
            PSY.setHoppingCurrentList(sinLUT, voltageLimit, 1 / freq / NUM_STEPS, 0, start);
            PSZ.setCurrentList(zHoppingLUT, 2, voltageLimit, 1 / freq, 0);

            //auto begin = std::chrono::high_resolution_clock::now();
            PSY.executeCommand();
            //auto end = std::chrono::high_resolution_clock::now();
            //std::chrono::duration<double> elapsed_seconds = end - begin;
            //std::cout << "Elapsed time (y): " << elapsed_seconds.count() << "s" << std::endl;

            //begin = std::chrono::high_resolution_clock::now();
            PSX.executeCommand();
            //end = std::chrono::high_resolution_clock::now();
            //elapsed_seconds = end - begin;
            //std::cout << "Elapsed time (x): " << elapsed_seconds.count() << "s" << std::endl;

            //begin = std::chrono::high_resolution_clock::now();
            PSZ.executeCommand();
            //end = std::chrono::high_resolution_clock::now();
            //elapsed_seconds = end - begin;
            //std::cout << "Elapsed time (z): " << elapsed_seconds.count() << "s" << std::endl;
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

    void testXY() {
        PSX.setHoppingCurrentList(cosLUT, voltageLimit, 1 / freq / NUM_STEPS, 0, 0);
        PSX.executeCommand();
        /*PSY.setHoppingCurrentList(sinLUT, voltageLimit, 1 / freq / NUM_STEPS, 0, 0);
        PSY.executeCommand();*/
    }

    void run() {
        while (active) {
            dwResult = XInputGetState(0, &state);
            //std::cout << state.Gamepad.wButtons << "\n";
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
    std::cout << "\n";

    // Z-FIELD 
    float zCurrent;
    std::cout << "z-field Current: ";
    std::cin >> zCurrent;
    std::cout << "\n";

    // XY-FIELD 
    float xyCurrent;
    std::cout << "xy-field Current: ";
    std::cin >> xyCurrent;
    std::cout << "\n";

    MagnetSystem magnets = MagnetSystem("ASRL3::INSTR", "ASRL4::INSTR", "ASRL5::INSTR",
        zCurrent, xyCurrent, freq, voltageLimit);
    magnets.initializeController();
    //magnets.testXY();
    magnets.run();
}