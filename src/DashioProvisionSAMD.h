/*
 MIT License

 Copyright (c) 2021 Craig Tuffnell, DashIO Connect Limited

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
*/

#if defined ARDUINO_SAMD_NANO_33_IOT || defined ARDUINO_SAMD_MKRWIFI1010
//#ifdef ARDUINO_ARCH_SAMD

#ifndef DashioProvisionSAMD_h
#define DashioProvisionSAMD_h

#include "Arduino.h"
#include "DashIO.h"
#include <FlashStorage.h>

typedef struct {
    char deviceName[32];
    char wifiSSID[32];
    char wifiPassword[63];
    char dashUserName[32];
    char dashPassword[32];
    char saved;
} DeviceData;

class DashioProvision {
public:
    DashioDevice *dashioDevice;
    
    char wifiSSID[32];
    char wifiPassword[63];
    char dashUserName[32];
    char dashPassword[32];

    DashioProvision(DashioDevice *_dashioDevice);
    void load(DeviceData *defaultDeviceData, void (*_onProvisionCallback)(ConnectionType connectionType, const String& message, bool commsChanged));
    void processMessage(MessageData *messageData);

private:
    void (*onProvisionCallback)(ConnectionType connectionType, const String& message, bool commsChanged);

    void save();
    void load();
    void update(DeviceData *deviceData);
};

#endif
#endif
