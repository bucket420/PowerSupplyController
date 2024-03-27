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

using namespace std;

const int numSteps = 48;
float currentCosLUT[2 * numSteps];
float currentSinLUT[2 * numSteps];

void createLUT(float curr) {
    for (int i = 0; i < numSteps / 2; i++) {
        currentCosLUT[i] = cos((i * M_PI) / numSteps) * curr;
        currentSinLUT[i] = sin((i * M_PI) / numSteps) * curr;
    }

    for (int i = numSteps / 2; i < numSteps; i++) {
        currentCosLUT[i] = -curr;
        currentCosLUT[i] = curr;
    }

    for (int i = numSteps; i < numSteps * 3 / 2; i++) {
        currentCosLUT[i] = cos(((i - numSteps / 2) * M_PI) / numSteps) * curr;
        currentSinLUT[i] = sin(((i - numSteps / 2) * M_PI) / numSteps) * curr;
    }

    for (int i = numSteps * 3 / 2; i < numSteps * 2; i++) {
        currentCosLUT[i] = curr;
        currentCosLUT[i] = curr;
    }
}

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
        if (status < VI_SUCCESS)
        {
            printf("Could not open a session to the VISA Resource Manager!\n");
            exit(EXIT_FAILURE);
        }
        cout << "Connecting to the device\n";
        status = viOpen(this->defaultRM, descriptor, VI_NULL, VI_NULL, &this->instr);
        if (status < VI_SUCCESS)
        {
            printf("Cannot open a session to the device.\n");
        }
        status = viSetAttribute(instr, VI_ATTR_TMO_VALUE, 5000);
    }

    void command() {
        status = viWrite(instr, (ViBuf)this->stringinput, (ViUInt32)strlen(this->stringinput), &this->writeCount);
        if (status < VI_SUCCESS)
        {
            cout << "Error writing to the device\n";
        }
    }

    void reset() {
        cout << "Resetting the device\n\n";
        memset(stringinput, 0, 512 * sizeof(char));
        strcpy(stringinput, "*rst\n");
        command();
    }

    void setCurrent(float current, float voltageLimit) {
        memset(stringinput, 0, 512 * sizeof(char));
        sprintf(stringinput, "func:mode curr;:curr %f;:volt %f;:outp on\n", current, voltageLimit);
        status = viWrite(instr, (ViBuf)this->stringinput, (ViUInt32)strlen(this->stringinput), &this->writeCount);
        command();
    }

    void hop() {
        input = "curr 5;:list:cle;:list:dwel 0.0005\n";
        strcpy(stringinput, input.c_str());
        status = viWrite(instr, (ViBuf)stringinput, (ViUInt32)strlen(stringinput), &writeCount);
        if (status < VI_SUCCESS)
        {
            cout << "Error writing to the device\n";
        }
        cout << input << "\n";
        for (int i = 0; i < 96; i++) {
            if (i % 8 == 0) {
                input = "list:volt ";
            }
            input.append(to_string(VArray[i]).substr(0, 5));
            if (i % 8 != 7) {
                input.append(",");
            }
            else {
                input.append("\n");
                strcpy(stringinput, input.c_str());
                status = viWrite(instr, (ViBuf)stringinput, (ViUInt32)strlen(stringinput), &writeCount);
                if (status < VI_SUCCESS)
                {
                    cout << "Error writing to the device\n";
                }
                cout << input << "\n";
            }
        }

        input = "list:coun 0;:outp on;:volt:mode list\n";
        cout << input << "\n";
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

    PowerSupply PSX = PowerSupply("ASRL3::INSTR");
    PowerSupply PSY = PowerSupply("ASRL4::INSTR");
    PowerSupply PSZ = PowerSupply("ASRL5::INSTR");
    PSX.reset();
    PSY.reset();
    PSZ.reset();

    float current;
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
            PSX.setCurrent(current, voltageLimit);
        }

        else if (LX > 0) {
            current = (LX / 32767) * xyfield;
            PSX.setCurrent(current, voltageLimit);
        }

        else if (LX == 0) {
            current = 0;
            PSX.setCurrent(current, voltageLimit);
        }
     
        // Y-FIELD
        if (LY < 0) {
            current = (LY / -32768) * xyfield * -1;       
            PSY.setCurrent(current, voltageLimit);
        }
        else if (LY > 0 ) {
            current = (LY / 32767) * xyfield;
            PSY.setCurrent(current, voltageLimit);
        }
        else if (LY == 0) {
            current = 0;
            PSY.setCurrent(current, voltageLimit);
        }

        // Z-FIELD
        if (RT > 50 || LT > 50) {
            current = zfield * -1;
            PSZ.setCurrent(current, voltageLimit);
        }
        else if (RT < 50) {
            current = zfield;
            PSZ.setCurrent(current, voltageLimit);
        }

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
        //// Left Disk Hopping

        //while (LeftButton == 4) {
        //    int num = 24;
        //    for (int i = 0; i < 25; i++) {
        //        AngleArray[i] = (num * M_PI) / NumSteps;
        //        CosineArray[i] = cos(AngleArray[i]);
        //        XComps[i] = CosineArray[i] * xyfield;
        //        SineArray[i] = sin(AngleArray[i]);
        //        YComps[i] = SineArray[i] * xyfield;
        //        num--;
        //    }
        //    // Set z-field to UP
        //    supply3(voltage, zfield);
        //    std::this_thread::sleep_for(std::chrono::milliseconds(TZ_m));
        //    while (ihop < 25) {
        //        Sleep(TS * 1000);
        //        float XCurrent = XComps[ihop];
        //        supply1(voltage, XCurrent);
        //        float YCurrent = YComps[ihop];
        //        supply2(voltage, YCurrent);
        //        ihop++;
        //    }
        //    dwResult = XInputGetState(0, &state);
        //    LeftButton = state.Gamepad.wButtons;

        //    // Set z-field to DOWN
        //    supply3(voltage, zfield * -1); 
        //    std::this_thread::sleep_for(std::chrono::milliseconds(TZ_m));
        //    if (LeftButton == 4) {
        //        while (ihop > 0) {
        //            ihop--;
        //            Sleep(TS * 1000);
        //            float XCurrent = XComps[ihop];
        //            supply1(voltage, XCurrent);
        //            float YCurrent = YComps[ihop];
        //            supply2(voltage, YCurrent);
        //        }
        //        dwResult = XInputGetState(0, &state);
        //        LeftButton = state.Gamepad.wButtons;
        //        if (LeftButton == 0) {
        //            supply3(voltage, zfield);
        //            std::this_thread::sleep_for(std::chrono::milliseconds(TZ_m));
        //        }  
        //    }
        //}

        //// Up Disk Hopping
        //while (UpButton == 1) {
        //    for (int i = 0; i < 25; i++) {
        //        AngleArray[i] = -1 * (i * M_PI  / NumSteps);
        //        CosineArray[i] = cos(AngleArray[i]);
        //        YComps[i] = CosineArray[i] * xyfield;
        //        SineArray[i] = sin(AngleArray[i]);
        //        XComps[i] = SineArray[i] * xyfield;
        //    }

        //    // Set z-field to UP
        //    supply3(voltage, zfield);
        //    std::this_thread::sleep_for(std::chrono::milliseconds(TZ_m));
        //    while (ihop < 25) {
        //        Sleep(TS * 1000);
        //        float XCurrent = XComps[ihop];
        //        supply1(voltage, XCurrent);
        //        float YCurrent = YComps[ihop];
        //        supply2(voltage, YCurrent);
        //        ihop++;
        //    }
        //    dwResult = XInputGetState(0, &state);
        //    UpButton = state.Gamepad.wButtons;

        //    // Set z-field to DOWN
        //    supply3(voltage, zfield * -1);
        //    std::this_thread::sleep_for(std::chrono::milliseconds(TZ_m));
        //    if (UpButton == 1) {
        //        while (ihop > 0) {
        //            ihop--;
        //            Sleep(TS * 1000);
        //            float XCurrent = XComps[ihop];
        //            supply1(voltage, XCurrent);
        //            float YCurrent = YComps[ihop];
        //            supply2(voltage, YCurrent);
        //        }
        //        dwResult = XInputGetState(0, &state);
        //        UpButton = state.Gamepad.wButtons;
        //        if (UpButton == 0) {
        //            supply3(voltage, zfield);
        //            std::this_thread::sleep_for(std::chrono::milliseconds(TZ_m));
        //        }
        //    }
        //}
        ////DownButton 
        //while (DownButton == 2) {
        //    int num = 24;
        //    for (int i = 0; i < 25; i++) {
        //        AngleArray[i] = (num * M_PI / NumSteps);
        //        CosineArray[i] = cos(AngleArray[i]);
        //        YComps[i] = CosineArray[i] * xyfield;
        //        SineArray[i] = sin(AngleArray[i]);
        //        XComps[i] = SineArray[i] * xyfield;
        //        num--;
        //    }
        //    // Set z-field to UP
        //    supply3(voltage, zfield);
        //    std::this_thread::sleep_for(std::chrono::milliseconds(TZ_m));
        //    while (ihop < 25) {
        //        Sleep(TS * 1000);
        //        float XCurrent = XComps[ihop];
        //        supply1(voltage, XCurrent);
        //        float YCurrent = YComps[ihop];
        //        supply2(voltage, YCurrent);
        //        ihop++;
        //    }
        //    dwResult = XInputGetState(0, &state);
        //    DownButton = state.Gamepad.wButtons;

        //    // Set z-field to DOWN
        //    supply3(voltage, zfield * -1);
        //    std::this_thread::sleep_for(std::chrono::milliseconds(TZ_m));
        //    if (DownButton == 2) {
        //        while (ihop > 0) {
        //            ihop--;
        //            Sleep(TS * 1000);
        //            float XCurrent = XComps[ihop];
        //            supply1(voltage, XCurrent);
        //            float YCurrent = YComps[ihop];
        //            supply2(voltage, YCurrent);
        //        }
        //        dwResult = XInputGetState(0, &state);
        //        DownButton = state.Gamepad.wButtons;
        //        if (DownButton == 0) {
        //            supply3(voltage, zfield);
        //            std::this_thread::sleep_for(std::chrono::milliseconds(TZ_m));
        //        }
        //    }
        //}
        // stop the code by pressing Start on xbox contoller
        if (Start == 16) {
            PSX.reset();
            PSY.reset();
            PSZ.reset();
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
    float voltage = 20;
    float current = 0;
    float freq;
    float period;


    /*Initializing the device to zero */

    cout << "Current set to " << current << " Amps" << endl;
    cout << "freq: ";
    cin >> freq;

    // Z-FIELD 
    float zfield;
    cout << "Z-field Current: ";
    cin >> zfield;

    // XY-FIELD 
    float xyfield;
    cout << "XY-field Current: ";
    cin >> xyfield;

    loop(freq, voltage, zfield, xyfield);
}        