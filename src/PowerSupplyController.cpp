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
#include <thread>

#pragma comment(lib,"XInput.lib")
#pragma comment(lib,"Xinput9_1_0.lib")
#define M_PI 3.14159265358979323846
#define NUM_STEPS 48



/*
    Class representing a power supply.
*/
class PowerSupply {
public:
    // VISA session variables
    ViSession defaultRM;
    ViSession instr;
    ViStatus status;
    ViUInt32 retCount;
    ViUInt32 writeCount;
    unsigned char buffer[100];
    char command[512];

    // Default constructor
    PowerSupply() {
    }

    // Constructor with descriptor, connecting the power supply to the computer. 
    // The descriptor contains the name of the port the power supply is connected to. 
    // E.g. "ASRL3::INSTR".
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

    // Send the string stored in `command` to the power supply to execute.
    void executeCommand() {
        status = viWrite(instr, (ViBuf)this->command, (ViUInt32)strlen(this->command), &this->writeCount);
        if (status < VI_SUCCESS) {
            std::cout << "Error writing to the device\n\n";
        }
        memset(command, 0, 512 * sizeof(char));
    }

    // Reset the power supply.
    void reset() {
        std::cout << "Resetting the device\n\n";
        strcpy(command, "*rst\n");
        executeCommand();
    }

    // Set the current value and voltage limit of the power supply.
    void setCurrent(float current, float voltageLimit) {
        sprintf(command, "func:mode curr;:curr %f;:volt %f;:outp on\n", current, voltageLimit);
        executeCommand();
    }

    // Send a list of current values to the power supply.
    // Parameters:
    //     currentList: array of current values to send to the power supply
    //     length: number of entries in the array
    //     dwell: time in seconds to wait between each current value
    //     count: number of times to repeat the list; if 0, continue forever
    void setCurrentList(float* currentList, int length, float voltageLimit, float dwell, int count) {
        sprintf(command, "list:cle;:list:dwel %f;:func:mode curr;:volt %f\n", dwell, voltageLimit);
        std::cout << command;
        executeCommand();
        // The list has to be broken up into smaller parts because the length of the command is limited.
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

    // Send a list of current values to the power supply, creating a waveform that causes the hopping motion.
    // The angle is first set to theta and stays there for T/2, then goes to theta + pi in T/2,
    // then stays at theta + pi for T/2, then goes to theta + 2pi in T/2.
    // Parameters:
    //     LUT: Lookup table for sine OR cosine funtions
    //     count: number of times to repeat the waveform; if 0, continue forever
    //     start: starting index of the LUT, corresponding to the starting angle and the direction of the hopping motion
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

/*
    Class representing the entire magnet system. 
    The magnet system consists of three power supplies, one for each axis. 
    The system can be controlled using an Xbox controller. 
*/
class MagnetSystem {
public:
    // 3 power supplies
    PowerSupply PSX;
    PowerSupply PSY;
    PowerSupply PSZ;

    // Controller variables
    DWORD dwResult;
    XINPUT_STATE state;

    // Max voltage
    float voltageLimit;

    // Currents
    float zCurrent;
    float xyCurrent;

    // Frequency of hopping motion
    float freq;

    // Lookup tables for x and y currents
    float cosLUT[NUM_STEPS];
    float sinLUT[NUM_STEPS];

    // Lookup table for z current, only 2 elements [zCurrent, -zCurrent]
    float zHoppingLUT[2];

    // Last key pressed
    int lastKeyPressed = 0;

    // Is the system running?
    bool active = true;

    // Constructor for the MagnetSystem class.
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

    // Fill the cosine and sine lookup tables with values.
    void fillTrigLUTs(float curr) {
        for (int i = 0; i < NUM_STEPS; i++) {
            this->cosLUT[i] = cos((i * 2 * M_PI) / NUM_STEPS) * curr;
            this->sinLUT[i] = sin((i * 2 * M_PI) / NUM_STEPS) * curr;
        }
    }

    // Initialize the controller.
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

    // Wrapper for multithreaded execution
    // For some reason, ASRL4::INSTR is slower than the other two by ~57ms
    // sleep_for is used to correct that
    void executeCommandWrapper(PowerSupply* ps, bool sleep) {
        if (sleep) {
            std::this_thread::sleep_for(std::chrono::milliseconds(57));
        }
        ps->executeCommand();
    }

    // Control the power supplies using the joystick.
    // The position of the joystick determines angle of the particles.
    void joystickControl() {
        float LX = state.Gamepad.sThumbLX;
        // std::cout << "Left Joystick X-Value " << LX << "\n";
        PSX.setCurrent((LX / 32768) * xyCurrent, voltageLimit);
        float LY = state.Gamepad.sThumbLY;
        // std::cout << "Left Joystick Y-Value " << LY << "\n";
        PSY.setCurrent((LY / 32768) * xyCurrent, voltageLimit);
    }

    // Control the power supplies using the triggers.
    // The z field is flipped when the triggers are pressed or released.
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

    // Control the power supplies using the X button. 
    // Rotate the particles in a circle.
    void xButtonControl() {
        if (state.Gamepad.wButtons == 16384) {
            lastKeyPressed = state.Gamepad.wButtons;

            // Send all the lists to the power supplies
            PSX.setCurrentList(cosLUT, NUM_STEPS, voltageLimit, 1 / freq / NUM_STEPS, 0);
            PSY.setCurrentList(sinLUT, NUM_STEPS, voltageLimit, 1 / freq / NUM_STEPS, 0);

            // Execute the commands concurrently
            std::thread t1(&MagnetSystem::executeCommandWrapper, this, &PSX, true);
            std::thread t2(&MagnetSystem::executeCommandWrapper, this, &PSY, false);

            // Remove threads after finishing
            t1.join();
            t2.join();
        }
        // Keep it running when the button is pressed
        while (state.Gamepad.wButtons == lastKeyPressed && state.Gamepad.wButtons != 0) {
            dwResult = XInputGetState(0, &state);
        }
        // Reset the power supplies when the button is released
        if (state.Gamepad.wButtons == 0 && lastKeyPressed == 16384) {
            PSX.reset();
            PSY.reset();
            lastKeyPressed = 0;
        }
    }

    // Control the power supplies using the start button.
    // Stop all commands and reset the power supplies.
    void startButtonControl() {
        if (state.Gamepad.wButtons == 16) {
            PSX.reset();
            PSY.reset();
            PSZ.reset();
            active = false;
        }
    }

    // Control the power supplies using the direction pad.
    // Move the particles in the direction of the pressed button.
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
            // Send all the lists to the power supplies
            PSX.setHoppingCurrentList(cosLUT, voltageLimit, 1 / freq / NUM_STEPS, 0, 0);
            PSY.setHoppingCurrentList(sinLUT, voltageLimit, 1 / freq / NUM_STEPS, 0, 0);
            PSZ.setCurrentList(zHoppingLUT, 2, voltageLimit, 1 / freq, 0);

            // Execute the commands concurrently
            std::thread t1(&MagnetSystem::executeCommandWrapper, this, &PSX, true);
            std::thread t2(&MagnetSystem::executeCommandWrapper, this, &PSY, false);
            std::thread t3(&MagnetSystem::executeCommandWrapper, this, &PSZ, true);

            // Remove threads after finishing
            t1.join();
            t2.join();
            t3.join();
        }
        // Keep it running when the button is pressed
        while (state.Gamepad.wButtons == lastKeyPressed && state.Gamepad.wButtons != 0) {
            dwResult = XInputGetState(0, &state);
        }
        // Reset the power supplies when the button is released
        if (state.Gamepad.wButtons == 0 && lastKeyPressed > 0 && lastKeyPressed < 9) {
            PSX.reset();
            PSY.reset();
            PSZ.reset();
            lastKeyPressed = 0;
        }
    }

    // Test the hopping function
    void testHopping() {
        PSX.setHoppingCurrentList(cosLUT, voltageLimit, 1 / freq / NUM_STEPS, 0, 0);
        PSY.setHoppingCurrentList(sinLUT, voltageLimit, 1 / freq / NUM_STEPS, 0, 0);
        PSZ.setCurrentList(zHoppingLUT, 2, voltageLimit, 1 / freq, 0);
        std::thread t1(&MagnetSystem::executeCommandWrapper, this, &PSX, true);
        std::thread t2(&MagnetSystem::executeCommandWrapper, this, &PSY, false);
        std::thread t3(&MagnetSystem::executeCommandWrapper, this, &PSZ, true);
        t1.join();
        t2.join();
        t3.join();
    }

    // Run the controller.
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
    std::cout << "z-field current: ";
    std::cin >> zCurrent;
    std::cout << "\n";

    // XY-FIELD 
    float xyCurrent;
    std::cout << "xy-field current: ";
    std::cin >> xyCurrent;
    std::cout << "\n";

    MagnetSystem magnets = MagnetSystem("ASRL3::INSTR", "ASRL4::INSTR", "ASRL5::INSTR",
        zCurrent, xyCurrent, freq, voltageLimit);
    //magnets.initializeController();
    // magnets.run();
    magnets.testHopping();
}