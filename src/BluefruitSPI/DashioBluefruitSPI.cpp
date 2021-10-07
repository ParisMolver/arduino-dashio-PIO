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

#include "DashioBluefruitSPI.h"

DashioBluefruit_BLE::DashioBluefruit_BLE(DashioDevice *_dashioDevice, bool _printMessages) : messageData(BLE_CONN),
                                                                                             bluefruit(BLUEFRUIT_SPI_SCK, BLUEFRUIT_SPI_MISO,
                                                                                                       BLUEFRUIT_SPI_MOSI, BLUEFRUIT_SPI_CS,
                                                                                                       BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST) {
    dashioDevice = _dashioDevice;
    printMessages = _printMessages;
}

void DashioBluefruit_BLE::sendMessage(const String& writeStr) {
    if (bluefruit.isConnected()) {
        bluefruit.print(writeStr);
        bluefruit.flush();
        
        if (printMessages) {
            Serial.println(F("---- BLE Sent ----"));
            Serial.println(writeStr);
            Serial.println();
        }
    }
}

void DashioBluefruit_BLE::run() {
    while(bluefruit.available()) {
        char data;
        data = (char)bluefruit.read(); // Read individual characters.
    
        if (messageData.processChar(data)) {
            if (printMessages) {
                Serial.println(messageData.getReceivedMessageForPrint(dashioDevice->getControlTypeStr(messageData.control)));
            }

            switch (messageData.control) {
            case who:
                sendMessage(dashioDevice->getWhoMessage());
                break;
            case connect:
                sendMessage(dashioDevice->getConnectMessage());
                break;
            default:
                if (processBLEmessageCallback != NULL) {
                    processBLEmessageCallback(&messageData);
                }
                break;
            }
        }
    }
}

void DashioBluefruit_BLE::setCallback(void (*processIncomingMessage)(MessageData *messageData)) {
    processBLEmessageCallback = processIncomingMessage;
}

void DashioBluefruit_BLE::begin(bool factoryResetEnable, bool useMacForDeviceID) {
  // Initialise the module
    Serial.print(F("Initialising Bluefruit LE module: "));
    if (!bluefruit.begin(VERBOSE_MODE)) {
        Serial.print(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring"));
        while(1);
    } else {
        Serial.println(F("OK!"));
    }

    // Perform a factory reset to make sure everything is in a known state
    if (factoryResetEnable) {
        Serial.println(F("Performing a factory reset: "));
        if (!bluefruit.factoryReset()) {
        Serial.print(F("Couldn't factory reset"));
        while(1);
        }
    }
  
    bluefruit.echo(false); // Disable command echo from Bluefruit
    Serial.println("Requesting Bluefruit info:");
    bluefruit.info(); // Print Bluefruit information
    bluefruit.verbose(false);  // debug info is a little annoying after this point!
    
    // LED Activity command is only supported from 0.6.6
    if (bluefruit.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION)) {
        // Change Mode LED Activity
        Serial.println(F("Change LED activity to " MODE_LED_BEHAVIOUR));
        bluefruit.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR);
    }

    if (useMacForDeviceID) {
        // Get deviceID for mac address
        bluefruit.println("AT+BLEGETADDR");
        delay(1000); // Wait a while for reply
        dashioDevice->deviceID = "";
        while(bluefruit.available() > 0) {
            char c = bluefruit.read();
            if ((c == '\n') || (c == '\r')) { // Before the OK
                break;
            }
            dashioDevice->deviceID += c;
        }
        Serial.print(F("DeviceID: "));
        Serial.println(dashioDevice->deviceID);
    }

    // Set local name name
    Serial.print(F("Set local name to DashIO_"));
    Serial.println(dashioDevice->type);
    String commandMsg = F("AT+GAPDEVNAME=DashIO_");
    commandMsg += dashioDevice->type;
    bluefruit.sendCommandCheckOK(commandMsg.c_str());
    
    // Set Bluefruit to DATA mode
    Serial.println(F("Switching to DATA mode!"));
    bluefruit.setMode(BLUEFRUIT_MODE_DATA);
    
    Serial.println();
}
