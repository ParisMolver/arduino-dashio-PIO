/*
 DashIO.cpp - Library for the DashIO comms protocol.
 Created by C. Tuffnell
 November 17, 2020
 
 MIT License

 Copyright (c) 2020 Craig Tuffnell, DashIO Connect Limited

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

#include "DashIO.h"
#include "DashJSON.h"

// Control type IDs
#define CONNECT_ID "CONNECT"
#define WHO_ID "WHO"
#define STATUS_ID "STATUS"
#define CONFIG_ID "CFG"

#define DEVICE_ID "DVCE"
#define DEVICE_VIEW_ID "DVVW"
#define LABEL_ID "LBL"
#define BUTTON_ID "BTTN"
#define MENU_ID "MENU"
#define BUTTON_GROUP_ID "BTGP"
#define EVENT_LOG_ID "LOG"
#define SLIDER_ID "SLDR"
#define BAR_ID "BAR"
#define KNOB_ID "KNOB"
#define KNOB_DIAL_ID "KBDL"
#define TEXT_BOX_ID "TEXT"
#define SELECTOR_ID "SLCTR"
#define GRAPH_ID "GRPH"
#define TIME_GRAPH_ID "TGRPH"
#define DIRECTION_ID "DIR"
#define DIAL_ID "DIAL"
#define MAP_ID "MAP"
#define BASIC_CONFIG_ID "BAS"

#define DEVICE_NAME_ID "NAME"
#define WIFI_SETUP_ID "WIFI"
#define TCP_SETUP_ID "TCP"
#define DASHIO_SETUP_ID "DASHIO"
#define MQTT_SETUP_ID "MQTT"
#define POPUP_MESSAGE_ID "MSSG"

// Connection type IDs
#define MQTT_CONNECTION_ID "MQTT"
#define BLE_CONNECTION_ID "BLE"
#define TCP_CONNECTION_ID "TCP"

// Alarm
#define ALARM_ID "ALM"

// Button control states
#define BUTTON_ON "ON"
#define BUTTON_OFF "OFF"

// Graph control line types
#define LINE_ID "LINE"
#define BAR_GRAPH_ID "BAR"
#define SEGMENTED_BAR_ID "SEGBAR"
#define PEAK_BAR_ID "PEAKBAR"

// MQTT topic tips
#define DATA_TOPIC_TIP     "data"
#define CONTROL_TOPIC_TIP  "control"
#define ALARM_TOPIC_TIP    "alarm"
#define ANNOUNCE_TOPIC_TIP "announce"
#define WILL_TOPIC_TIP     "data"

// MQTT basic messages
#define MQTT_ONLINE_MSSG  "\tONLINE\n"
#define MQTT_OFFLINE_MSSG "\tOFFLINE\n"

#define MAX_STRING_LEN 64
#define MAX_DEVICE_NAME_LEN 32
#define MAX_DEVICE_TYPE_LEN 32

const char END_DELIM = '\n';
const char DELIM = '\t';

DashioConnection::DashioConnection(ConnectionType connType) {
    deviceID.reserve(MAX_STRING_LEN);
    idStr.reserve(MAX_STRING_LEN);
    payloadStr.reserve(MAX_STRING_LEN);
    payloadStr2.reserve(MAX_STRING_LEN);
    
    connectionType = connType;
};

void DashioConnection::processMessage(const String& message) {
    if (message.length() > 0) {
        if (messageReceived) {
            Serial.println(F("Incoming message overflow. Can't process:"));
            Serial.println(message);
        } else {
            for (unsigned int i = 0; i < message.length(); i++) {
                char chr = message[i];
                if (processChar(chr)) {
                    messageReceived = true;
                }
            }
        }
    }
}

bool DashioConnection::processChar(char chr) {
    bool messageEnd = false;
    if ((chr == DELIM) || (chr == END_DELIM)) {
        if ((readStr.length() > 0) || (segmentCount == 1)) { // segmentCount == 1 allows for empty second field ??? maybe should be 2 for empty third field now that we've added deviceID at the front
        switch (segmentCount) {
        case 0:
            if (readStr == WHO_ID) {
                deviceID = "UNKNOWN";
                control = who;
            } else {
                deviceID = readStr;
                control = unknown;
            }
                  
            idStr = "";
            payloadStr = "";
            break;
        case 1:
            if (readStr == WHO_ID) {
                control = who;
            } else if (readStr == CONNECT_ID) {
                control = connect;
            } else if (readStr == STATUS_ID) {
                control = status;
            } else if (readStr == CONFIG_ID) {
                control = config;
            } else if (readStr == BUTTON_ID) {
                control = button;
            } else if (readStr == SLIDER_ID) {
                control = slider;
            } else if (readStr == KNOB_ID) {
                control = knob;
            } else if (readStr == TEXT_BOX_ID) {
                control = textBox;
            } else if (readStr == TIME_GRAPH_ID) {
                control = timeGraph;
            } else if (readStr == MENU_ID) {
                control = menu;
            } else if (readStr == BUTTON_GROUP_ID) {
                control = buttonGroup;
            } else if (readStr == EVENT_LOG_ID) {
                control = eventLog;
            } else if (readStr == SELECTOR_ID) {
                control = selector;
            } else if (readStr == DEVICE_NAME_ID) {
                control = deviceName;
            } else if (readStr == WIFI_SETUP_ID) {
                control = wifiSetup;
            } else if (readStr == TCP_SETUP_ID) {
                control = tcpSetup;
            } else if (readStr == DASHIO_SETUP_ID) {
                control = dashioSetup;
            } else if (readStr == MQTT_SETUP_ID) {
                control = mqttSetup;
            } else {
                control = unknown;
                segmentCount == -1;
            }
            break;
        case 2:
            idStr = readStr;
            break;
        case 3:
            payloadStr = readStr;
        break;
            case 4:
            payloadStr2 = readStr;
            break;
        default:
                segmentCount = 0;
        }

        if (segmentCount >= 0) {
        segmentCount++;
        if (chr == END_DELIM) { // End of message, so process message
          messageEnd = true;
            segmentCount = -1; // Wait for next start of message
        }
        }
    } else {
        segmentCount = 0; // Must have no data before DELIM or a DELIM + DELIM, so must be start of message
    }
    readStr = "";
    } else {
        readStr += chr;
    }
    return messageEnd;
}

String DashioConnection::getReceivedMessageForPrint(const String& controlStr) {
    String message((char *)0);
    message.reserve(200);

    message += F("**** ");
    switch (connectionType) {
        case BLE_CONN:
            message += "BLE";
            break;
        case TCP_CONN:
            message += "TCP";
            break;
        case MQTT_CONN:
            message += "MQTT";
            break;
    }
    message += F("Received ****\n");
    message += deviceID;
    message += F("  ");
    message += controlStr;
    message += F("  ");
    message += idStr;
    message += F("  ");
    message += payloadStr;
    message += F("  ");
    message += payloadStr2;

    return message;
}

/* --------------- */

DashioDevice::DashioDevice(const String& deviceType, const String& deviceName) {
    type.reserve(MAX_DEVICE_TYPE_LEN);
    type = deviceType;

    name.reserve(MAX_DEVICE_NAME_LEN);
    name = deviceName;
}

DashioDevice::DashioDevice() {
    type.reserve(MAX_DEVICE_TYPE_LEN);
    name.reserve(MAX_DEVICE_NAME_LEN);
}

void DashioDevice::setDeviceID(const String& deviceIdentifier) {
    deviceID.reserve(MAX_STRING_LEN);

    deviceID = deviceIdentifier;
}

void DashioDevice::setDeviceID(uint8_t m_address[6]) {
    char buffer[20];
    String macStr((char *)0);
    macStr.reserve(20);

    sprintf(buffer, "%x", m_address[0]);
    if (m_address[0] < 16) {
        macStr += "0";
    }
    macStr += buffer;
    sprintf(buffer, "%x", m_address[1]);
    if (m_address[1] < 16) {
        macStr += "0";
    }
    macStr += buffer;
    sprintf(buffer, "%x", m_address[2]);
    if (m_address[2] < 16) {
        macStr += "0";
    }
    macStr += buffer;
    sprintf(buffer, "%x", m_address[3]);
    if (m_address[3] < 16) {
        macStr += "0";
    }
    macStr += buffer;
    sprintf(buffer, "%x", m_address[4]);
    if (m_address[4] < 16) {
        macStr += "0";
    }
    macStr += buffer;
    sprintf(buffer, "%x", m_address[5]);
    if (m_address[5] < 16) {
        macStr += "0";
    }
    macStr += buffer;

    deviceID = macStr.c_str();
};

String DashioDevice::getOnlineMessage() {
    String message = String(DELIM);
    message += deviceID;
    message += MQTT_ONLINE_MSSG;
    return message;
}

String DashioDevice::getOfflineMessage() {
    String message = String(DELIM);
    message += deviceID;
    message += MQTT_OFFLINE_MSSG;
    return message;
}

String DashioDevice::getWhoMessage() {
    String message = String(DELIM);
    message += deviceID;
    message += String(DELIM);
    message += WHO_ID;
    message += String(DELIM);
    message += type;
    message += String(DELIM);
    message += name;
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getConnectMessage() {
    String message = String(DELIM);
    message += deviceID;
    message += String(DELIM);
    message += CONNECT_ID;
    message += String(END_DELIM);
    return  message;
}

String DashioDevice::getPopupMessage(const String& header, const String& body, const String& caption) {
    String message = String(DELIM);
    message += deviceID;
    message += String(DELIM);
    message += POPUP_MESSAGE_ID;
    message += String(DELIM);
    message += header;
    if (body != "") {
        message += (String(DELIM) + body);
        if (caption != "") {
            message += String(DELIM);
            message += caption;
        }
    }
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getDeviceNameMessage() {
    String message = String(DELIM);
    message += deviceID;
    message += String(DELIM);
    message += DEVICE_NAME_ID;
    message += String(DELIM);
    message += name;
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getWifiUpdateAckMessage() {
    String message = String(DELIM);
    message += deviceID;
    message += String(DELIM);
    message += WIFI_SETUP_ID;
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getTCPUpdateAckMessage() {
    String message = String(DELIM);
    message += deviceID;
    message += String(DELIM);
    message += TCP_SETUP_ID;
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getDashioUpdateAckMessage() {
    String message = String(DELIM);
    message += deviceID;
    message += String(DELIM);
    message += DASHIO_SETUP_ID;
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getMQTTUpdateAckMessage() {
    String message = String(DELIM);
    message += deviceID;
    message += String(DELIM);
    message += MQTT_SETUP_ID;
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getAlarmMessage(const String& controlID, const String& title, const String& description) {
    String message = String(DELIM);
    message += deviceID;
    message += String(DELIM);
    message += controlID;
    message += String(DELIM);
    message += title;
    message += String(DELIM);
    message += description;
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getAlarmMessage(Notification alarm) {
    return getAlarmMessage(alarm.identifier, alarm.title, alarm.description);
}

String DashioDevice::getButtonMessage(const String& controlID, bool on, const String& iconName, const String& text) {
    String writeStr = String(DELIM);
    writeStr += deviceID;
    writeStr += String(DELIM);
    writeStr += BUTTON_ID;
    writeStr += String(DELIM);
    writeStr += controlID;
    writeStr += String(DELIM);
    if (on) {
        writeStr += BUTTON_ON;
    } else {
        writeStr += BUTTON_OFF;
    }
    if (text != "") {
        writeStr += String(DELIM);
        writeStr += iconName;
        writeStr += String(DELIM);
        writeStr += text;
    } else {
        if (iconName != "") {
            writeStr += String(DELIM);
            writeStr += iconName;
        }
    }
    writeStr += String(END_DELIM);
    return writeStr;
}

String DashioDevice::getTextBoxMessage(const String& controlID, const String& text) {
    String message = String(DELIM);
    message += deviceID;
    message += String(DELIM);
    message += TEXT_BOX_ID;
    message += String(DELIM);
    message += controlID;
    message += String(DELIM);
    message += text;
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getSelectorMessage(const String& controlID, int index) {
    String message = String(DELIM);
    message += deviceID;
    message += String(DELIM);
    message += SELECTOR_ID;
    message += String(DELIM);
    message += controlID;
    message += String(DELIM);
    message += String(index);
    message += String(END_DELIM);
  return message;
}

String DashioDevice::getSelectorMessage(const String& controlID, int index, String* selectionItems, int rows) {
    String writeStr = String(DELIM);
    writeStr += deviceID;
    writeStr += String(DELIM);
    writeStr += SELECTOR_ID;
    writeStr += String(DELIM);
    writeStr += controlID;
    writeStr += String(DELIM);
    writeStr += String(index);
    for (int i = 0; i < rows; i++) {
        writeStr += String(DELIM);
        writeStr += selectionItems[i];
    }
    writeStr += String(END_DELIM);
    return writeStr;
}

String DashioDevice::getSliderMessage(const String& controlID, int value) {
    String message = String(DELIM);
    message += deviceID;
    message += String(DELIM);
    message += SLIDER_ID;
    message += String(DELIM);
    message += controlID;
    message += String(DELIM);
    message += String(value);
    message += String(END_DELIM);
  return message;
}

String DashioDevice::getSingleBarMessage(const String& controlID, int value) {
    String message = String(DELIM);
    message += deviceID;
    message += String(DELIM);
    message += String(DELIM);
    message += BAR_ID;
    message += String(DELIM);
    message += controlID;
    message += String(DELIM);
    message += String(value);
    message += String(END_DELIM);
  return message;
}

String DashioDevice::getKnobMessage(const String& controlID, int value) {
    String message = String(DELIM);
    message += deviceID;
    message += String(DELIM);
    message += KNOB_ID;
    message += String(DELIM);
    message += controlID;
    message += String(DELIM);
    message += String(value);
    message += String(END_DELIM);
  return message;
}

String DashioDevice::getKnobDialMessage(const String& controlID, int value) {
    String message = String(DELIM);
    message += deviceID;
    message += String(DELIM);
    message += KNOB_DIAL_ID;
    message += String(DELIM);
    message += controlID;
    message += String(DELIM);
    message += String(value);
    message += String(END_DELIM);
  return message;
}

String DashioDevice::getDirectionMessage(const String& controlID, int value, const String& text) {
    String writeStr = String(DELIM);
    writeStr += deviceID;
    writeStr += String(DELIM);
    writeStr += DIRECTION_ID;
    writeStr += String(DELIM);
    writeStr += controlID;
    writeStr + String(DELIM);
    writeStr + String(value);
    if (text != "") {
        writeStr += String(DELIM);
        writeStr += text;
    }
    writeStr += String(END_DELIM);
    return writeStr;
}

String DashioDevice::getDialMessage(const String& controlID, const String& text) {
    String message = String(DELIM);
    message += deviceID;
    message += String(DELIM);
    message += DIAL_ID;
    message += String(DELIM);
    message += controlID;
    message += String(DELIM);
    message += text;
    message += String(END_DELIM);
  return message;
}

String DashioDevice::getMapMessage(const String& controlID, const String& latitude, const String& longitude, const String& message) {
    String writeStr = String(DELIM);
    writeStr += deviceID;
    writeStr += String(DELIM);
    writeStr += MAP_ID;
    writeStr += String(DELIM);
    writeStr += controlID;
    writeStr += String(DELIM);
    writeStr += latitude;
    writeStr += String(DELIM);
    writeStr += longitude;
    writeStr += String(DELIM);
    writeStr += message;
    writeStr += String(END_DELIM);
  return writeStr;
}

String DashioDevice::getEventLogMessage(const String& controlID, const String& timeStr, const String& color, String text[], int dataLength) {
    String writeStr = String(DELIM);
    writeStr += deviceID ;
    writeStr += String(DELIM);
    writeStr += EVENT_LOG_ID;
    writeStr += String(DELIM);
    writeStr += controlID;
    writeStr += String(DELIM);
    writeStr += timeStr;
    writeStr += String(DELIM);
    writeStr += color;
    for (int i = 0; i < dataLength; i++) {
        writeStr += String(DELIM);
        writeStr += text[i];
    }
    writeStr += String(END_DELIM);
    return writeStr;
}

String DashioDevice::getDoubleBarMessage(const String& controlID, int value1, int value2) {
    int barValues[2];
    barValues[0] = value1;
    barValues[1] = value2;
    return getIntArray(BAR_ID, controlID, barValues, 2);
}

String DashioDevice::getBasicConfigData(ControlType controlType, const String& controlID, const String& controlTitle) {
    String message = String(DELIM);
    message += getControlTypeStr(controlType);
    message += String(DELIM);
    message += controlID;
    message += String(DELIM);
    message += controlTitle;
    return message;
}

String DashioDevice::getBasicConfigMessage(ControlType controlType, const String& controlID, const String& controlTitle) {
    String message = String(DELIM);
    message += deviceID;
    message += String(DELIM);
    message += CONFIG_ID;
    message += String(DELIM);
    message += BASIC_CONFIG_ID;
    message += getBasicConfigData(controlType, controlID, controlTitle);
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getBasicConfigMessage(const String& configData) {
    String message = String(DELIM);
    message += deviceID;
    message += String(DELIM);
    message += CONFIG_ID;
    message += String(DELIM);
    message += BASIC_CONFIG_ID;
    message += configData;
    message += String(END_DELIM);
    return  message;
}

String DashioDevice::getFullConfigMessage(ControlType controlType, const String& configData) {
    String message = String(DELIM);
    message += deviceID;
    message += String(DELIM);
    message += CONFIG_ID;
    message += String(DELIM);
    message += getControlTypeStr(controlType);
    message += String(DELIM);
    message += configData;
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getGraphLineInts(const String& controlID, const String& graphLineID, const String& lineName, LineType lineType, const String& color, int lineData[], int dataLength) {
    String writeStr = String(DELIM);
    writeStr += deviceID;
    writeStr += String(DELIM);
    writeStr += GRAPH_ID;
    writeStr += String(DELIM);
    writeStr += controlID;
    writeStr += String(DELIM);
    writeStr += graphLineID;
    writeStr += String(DELIM);
    writeStr += lineName;
    writeStr += String(DELIM);
    writeStr += getLineTypeStr(lineType);
    writeStr += String(DELIM);
    writeStr += color;
    for (int i = 0; i < dataLength; i++) {
        writeStr += String(DELIM);
        writeStr += String(lineData[i]);
    }
    writeStr += String(END_DELIM);
    return writeStr;
}

String DashioDevice::getGraphLineFloats(const String& controlID, const String& graphLineID, const String& lineName, LineType lineType, const String& color, float lineData[], int dataLength) {
    String writeStr = String(DELIM);
    writeStr += deviceID;
    writeStr += String(DELIM);
    writeStr += GRAPH_ID;
    writeStr += String(DELIM);
    writeStr += controlID;
    writeStr += String(DELIM);
    writeStr += graphLineID;
    writeStr += String(DELIM);
    writeStr += lineName;
    writeStr += String(DELIM);
    writeStr += getLineTypeStr(lineType);
    writeStr += String(DELIM);
    writeStr += color;
    for (int i = 0; i < dataLength; i++) {
        char lineDataBuffer[8];
        String floatStr = dtostrf(lineData[i], 5, 2, lineDataBuffer);
        writeStr += String(DELIM);
        writeStr += floatStr;
    }
    writeStr += String(END_DELIM);
    return writeStr;
}

String DashioDevice::getTimeGraphLineFloats(const String& controlID, const String& graphLineID, const String& lineName, LineType lineType, const String& color, String times[], float lineData[], int dataLength, bool breakLine) {
    String writeStr = String(DELIM);
    writeStr += deviceID;
    writeStr += String(DELIM);
    writeStr += TIME_GRAPH_ID;
    writeStr += String(DELIM);
    writeStr += controlID;
    writeStr += String(DELIM);
    writeStr += graphLineID;
    writeStr += String(DELIM);
    writeStr += lineName;
    writeStr += String(DELIM);
    writeStr += getLineTypeStr(lineType);
    writeStr += String(DELIM);
    writeStr += color;
    if (breakLine && (dataLength > 0)) {
        writeStr += String(DELIM);
        writeStr += times[0];
        writeStr += ",";
        writeStr += "B";
    }
    for (int i = 0; i < dataLength; i++) {
        char lineDataBuffer[8];
        String lineDataStr = dtostrf(lineData[i], 5, 2, lineDataBuffer);
        writeStr += String(DELIM);
        writeStr += times[i];
        writeStr += ",";
        writeStr += lineDataStr;
    }
    writeStr += String(END_DELIM);
    return writeStr;
}

String DashioDevice::getTimeGraphLineBools(const String& controlID, const String& graphLineID, const String& lineName, LineType lineType, const String& color, String times[], bool lineData[], int dataLength) {
    String writeStr = String(DELIM);
    writeStr += deviceID;
    writeStr += String(DELIM);
    writeStr += TIME_GRAPH_ID;
    writeStr += String(DELIM);
    writeStr += controlID;
    writeStr += String(DELIM);
    writeStr += graphLineID;
    writeStr += String(DELIM);
    writeStr += lineName;
    writeStr += String(DELIM);
    writeStr += getLineTypeStr(lineType);
    writeStr += String(DELIM);
    writeStr += color;
    for (int i = 0; i < dataLength; i++) {
        writeStr += String(DELIM);
        writeStr += times[i];
        writeStr += ",";
        if (lineData[i]) {
            writeStr += "T";
        } else {
            writeStr += "F";
        }
    }
    writeStr += String(END_DELIM);
    return writeStr;
}

String DashioDevice::getControlTypeStr(ControlType controltype) {
    switch (controltype) {
        case connect: return CONNECT_ID;
        case who: return WHO_ID;
        case status: return STATUS_ID;
        case config: return CONFIG_ID;
              
        case device: return DEVICE_ID;
        case deviceView: return DEVICE_VIEW_ID;
        case label: return LABEL_ID;
        case button: return BUTTON_ID;
        case menu: return MENU_ID;
        case buttonGroup: return BUTTON_GROUP_ID;
        case eventLog: return EVENT_LOG_ID;
        case slider: return SLIDER_ID;
        case knob: return KNOB_ID;
        case dial: return DIAL_ID;
        case direction: return DIRECTION_ID;
        case textBox: return TEXT_BOX_ID;
        case selector: return SELECTOR_ID;
        case graph: return GRAPH_ID;
        case timeGraph: return TIME_GRAPH_ID;
        case mapper: return MAP_ID;
              
        case deviceName: return DEVICE_NAME_ID;
        case wifiSetup: return WIFI_SETUP_ID;
        case tcpSetup: return TCP_SETUP_ID;
        case dashioSetup: return DASHIO_SETUP_ID;
        case mqttSetup: return MQTT_SETUP_ID;
        case popupMessage: return POPUP_MESSAGE_ID;

        case mqttConn: return MQTT_CONNECTION_ID;
        case bleConn: return BLE_CONNECTION_ID;
        case tcpConn: return TCP_CONNECTION_ID;
              
        case alarmNotify: return ALARM_ID;

        case pushToken: return "";
        case unknown: return "";
    }
    return "";
}

String DashioDevice::getMQTTSubscribeTopic(const String& userName) {
    mqttSubscrberTopic = getMQTTTopic(userName, control_topic);
    return mqttSubscrberTopic;
}

String DashioDevice::getMQTTTopic(const String& userName, MQTTTopicType topic) {
    String tip;
    switch (topic) {
        case data_topic:
            tip = DATA_TOPIC_TIP;
            break;
        case control_topic:
            tip = CONTROL_TOPIC_TIP;
            break;
        case alarm_topic:
            tip = ALARM_TOPIC_TIP;
            break;
        case announce_topic:
            tip = ANNOUNCE_TOPIC_TIP;
            break;
        case will_topic:
            tip = WILL_TOPIC_TIP;
            break;
    }
    return userName + "/" + deviceID + "/" + tip;
}

String DashioDevice::getLineTypeStr(LineType lineType) {
    switch (lineType) {
        case line:
            return LINE_ID;
        case bar:
            return BAR_GRAPH_ID;
        case segBar:
            return SEGMENTED_BAR_ID;
        case peakBar:
            return PEAK_BAR_ID;
        default:
            return LINE_ID;
    }
}

String DashioDevice::getIntArray(const String& controlType, const String& ID, int idata[], int dataLength) {
    String writeStr = String(DELIM);
    writeStr += deviceID;
    writeStr + String(DELIM);
    writeStr + controlType;
    writeStr + String(DELIM);
    writeStr + ID;
    for (int i = 0; i < dataLength; i++) {
        writeStr += String(DELIM);
        writeStr += String(idata[i]);
    }
    writeStr += String(END_DELIM);
    return writeStr;
}

String DashioDevice::getFloatArray(const String& controlType, const String& ID, float fdata[], int dataLength) {
    String writeStr = String(DELIM);
    writeStr + deviceID;
    writeStr + String(DELIM);
    writeStr + controlType;
    writeStr + String(DELIM);
    writeStr + ID;
    for (int i = 0; i < dataLength; i++) {
        char fbuffer[8];
        String floatStr = dtostrf(fdata[i], 5, 2, fbuffer);
        writeStr += String(DELIM);
        writeStr +=  floatStr;
    }
    writeStr += String(END_DELIM);
    return writeStr;
}

// Configuration
String DashioDevice::getConfigMessage(DeviceCfg deviceConfigData) {
    DashJSON json;
    json.start();
    json.addKeyInt(F("numDeviceViews"), deviceConfigData.numDeviceViews);
    json.addKeyString(F("deviceSetup"), deviceConfigData.deviceSetup, true);
    return getFullConfigMessage(device, json.jsonStr);
}

String DashioDevice::getConfigMessage(DeviceViewCfg deviceViewData) {
    DashJSON json;
    json.start();
    json.addKeyString(F("controlID"), deviceViewData.controlID);
    json.addKeyString(F("title"), deviceViewData.title);
    json.addKeyString(F("iconName"), deviceViewData.iconName);
    json.addKeyString(F("color"), deviceViewData.color);
    json.addKeyBool(F("shareColumn"), deviceViewData.shareColumn);
    json.addKeyInt(F("numColumns"), deviceViewData.numColumns);

    // Control Default Values
    json.addKeyInt(F("ctrlMaxFontSize"), deviceViewData.ctrlMaxFontSize);
    json.addKeyBool(F("ctrlBorderOn"), deviceViewData.ctrlBorderOn);
    json.addKeyString(F("ctrlBorderColor"), deviceViewData.ctrlBorderColor);
    json.addKeyString(F("ctrlColor"), deviceViewData.ctrlColor);
    json.addKeyString(F("ctrlBkgndColor"), deviceViewData.ctrlBkgndColor);
    json.addKeyInt(F("ctrlBkgndTransparency"), deviceViewData.ctrlBkgndTransparency);

    // Control Title Box Default Values
    json.addKeyInt(F("ctrlTitleFontSize"), deviceViewData.ctrlTitleFontSize);
    json.addKeyString(F("ctrlTitleBoxColor"), deviceViewData.ctrlTitleBoxColor);
    json.addKeyInt(F("ctrlTitleBoxTransparency"), deviceViewData.ctrlTitleBoxTransparency, true);
    return getFullConfigMessage(deviceView, json.jsonStr);
}

String DashioDevice::getConfigMessage(BLEConnCfg connectionData) {
    DashJSON json;
    json.start();
    json.addKeyString(F("serviceUUID"), connectionData.serviceUUID);
    json.addKeyString(F("readUUID"), connectionData.readUUID);
    json.addKeyString(F("writeUUID"), connectionData.writeUUID, true);
    return getFullConfigMessage(bleConn, json.jsonStr);
}

String DashioDevice::getConfigMessage(TCPConnCfg connectionData) {
    DashJSON json;
    json.start();
    json.addKeyString(F("ipAddress"), connectionData.ipAddress);
    json.addKeyInt(F("port"), connectionData.port, true);
    return getFullConfigMessage(tcpConn, json.jsonStr);
}

String DashioDevice::getConfigMessage(MQTTConnCfg connectionData) {
    DashJSON json;
    json.start();
    json.addKeyString(F("userName"), connectionData.userName);
    json.addKeyString(F("hostURL"), connectionData.hostURL, true);
    return getFullConfigMessage(mqttConn, json.jsonStr);
}

String DashioDevice::getConfigMessage(AlarmCfg alarmData) {
    DashJSON json;
    json.start();
    json.addKeyString(F("controlID"), alarmData.controlID);
    json.addKeyString(F("description"), alarmData.description);
    json.addKeyString(F("soundName"), alarmData.soundName, true);
    return getFullConfigMessage(alarmNotify, json.jsonStr);
}

String DashioDevice::getConfigMessage(LabelCfg labelData) {
    DashJSON json;
    json.start();
    json.addKeyString(F("controlID"), labelData.controlID);
    json.addKeyString(F("parentID"), labelData.parentID);
    json.addKeyFloat(F("xPositionRatio"), labelData.graphicsRect.xPositionRatio);
    json.addKeyFloat(F("yPositionRatio"), labelData.graphicsRect.yPositionRatio);
    json.addKeyFloat(F("widthRatio"), labelData.graphicsRect.widthRatio);
    json.addKeyFloat(F("heightRatio"), labelData.graphicsRect.heightRatio);
    json.addKeyString(F("title"), labelData.title);
    json.addKeyString(F("titlePosition"), getTitlePositionStr(labelData.titlePosition));

    json.addKeyString(F("style"), getLabelStyle(labelData.style));
    json.addKeyString(F("color"), labelData.color, true);
    return getFullConfigMessage(label, json.jsonStr);
}

String DashioDevice::getConfigMessage(ButtonCfg buttonData) {
    DashJSON json;
    json.start();
    json.addKeyString(F("controlID"), buttonData.controlID);
    json.addKeyString(F("parentID"), buttonData.parentID);
    json.addKeyFloat(F("xPositionRatio"), buttonData.graphicsRect.xPositionRatio);
    json.addKeyFloat(F("yPositionRatio"), buttonData.graphicsRect.yPositionRatio);
    json.addKeyFloat(F("widthRatio"), buttonData.graphicsRect.widthRatio);
    json.addKeyFloat(F("heightRatio"), buttonData.graphicsRect.heightRatio);
    json.addKeyString(F("title"), buttonData.title);
    json.addKeyString(F("titlePosition"), getTitlePositionStr(buttonData.titlePosition));

    json.addKeyBool(F("buttonEnabled"), buttonData.buttonEnabled);
    json.addKeyString(F("iconName"), buttonData.iconName);
    json.addKeyString(F("text"), buttonData.text);
    json.addKeyString(F("offColor"), buttonData.offColor);
    json.addKeyString(F("onColor"), buttonData.onColor, true);
    return getFullConfigMessage(button, json.jsonStr);
}

String DashioDevice::getConfigMessage(MenuCfg menuData) {
    DashJSON json;
    json.start();
    json.addKeyString(F("controlID"), menuData.controlID);
    json.addKeyString(F("parentID"), menuData.parentID);
    json.addKeyFloat(F("xPositionRatio"), menuData.graphicsRect.xPositionRatio);
    json.addKeyFloat(F("yPositionRatio"), menuData.graphicsRect.yPositionRatio);
    json.addKeyFloat(F("widthRatio"), menuData.graphicsRect.widthRatio);
    json.addKeyFloat(F("heightRatio"), menuData.graphicsRect.heightRatio);
    json.addKeyString(F("title"), menuData.title);
    json.addKeyString(F("titlePosition"), getTitlePositionStr(menuData.titlePosition));

    json.addKeyString(F("iconName"), menuData.iconName);
    json.addKeyString(F("text"), menuData.text, true);
    return getFullConfigMessage(menu, json.jsonStr);
}

String DashioDevice::getConfigMessage(ButtonGroupCfg groupData) {
    DashJSON json;
    json.start();
    json.addKeyString(F("controlID"), groupData.controlID);
    json.addKeyString(F("parentID"), groupData.parentID);
    json.addKeyFloat(F("xPositionRatio"), groupData.graphicsRect.xPositionRatio);
    json.addKeyFloat(F("yPositionRatio"), groupData.graphicsRect.yPositionRatio);
    json.addKeyFloat(F("widthRatio"), groupData.graphicsRect.widthRatio);
    json.addKeyFloat(F("heightRatio"), groupData.graphicsRect.heightRatio);
    json.addKeyString(F("title"), groupData.title);
    json.addKeyString(F("titlePosition"), getTitlePositionStr(groupData.titlePosition));

    json.addKeyString(F("iconName"), groupData.iconName);
    json.addKeyString(F("text"), groupData.text);
    json.addKeyBool(F("gridView"), groupData.gridView, true);
    return getFullConfigMessage(buttonGroup, json.jsonStr);
}

String DashioDevice::getConfigMessage(EventLogCfg eventLogData) {
    DashJSON json;
    json.start();
    json.addKeyString(F("controlID"), eventLogData.controlID);
    json.addKeyString(F("parentID"), eventLogData.parentID);
    json.addKeyFloat(F("xPositionRatio"), eventLogData.graphicsRect.xPositionRatio);
    json.addKeyFloat(F("yPositionRatio"), eventLogData.graphicsRect.yPositionRatio);
    json.addKeyFloat(F("widthRatio"), eventLogData.graphicsRect.widthRatio);
    json.addKeyFloat(F("heightRatio"), eventLogData.graphicsRect.heightRatio);
    json.addKeyString(F("title"), eventLogData.title);
    json.addKeyString(F("titlePosition"), getTitlePositionStr(eventLogData.titlePosition), true);
    return getFullConfigMessage(eventLog, json.jsonStr);
}

String DashioDevice::getConfigMessage(KnobCfg knobData) {
    DashJSON json;
    json.start();
    json.addKeyString(F("controlID"), knobData.controlID);
    json.addKeyString(F("parentID"), knobData.parentID);
    json.addKeyFloat(F("xPositionRatio"), knobData.graphicsRect.xPositionRatio);
    json.addKeyFloat(F("yPositionRatio"), knobData.graphicsRect.yPositionRatio);
    json.addKeyFloat(F("widthRatio"), knobData.graphicsRect.widthRatio);
    json.addKeyFloat(F("heightRatio"), knobData.graphicsRect.heightRatio);
    json.addKeyString(F("title"), knobData.title);
    json.addKeyString(F("titlePosition"), getTitlePositionStr(knobData.titlePosition));

    json.addKeyFloat(F("min"), knobData.min);
    json.addKeyFloat(F("max"), knobData.max);
    json.addKeyFloat(F("redValue"), knobData.redValue);
    json.addKeyBool(F("showMinMax"), knobData.showMinMax);
    json.addKeyString(F("style"), getKnobPresentationStyle(knobData.style));
    json.addKeyString(F("knobColor"), knobData.knobColor);
    json.addKeyBool(F("sendOnlyOnRelease"), knobData.sendOnlyOnRelease);
    json.addKeyBool(F("dialFollowsKnob"), knobData.dialFollowsKnob);
    json.addKeyString(F("dialColor"), knobData.dialColor, true);
    return getFullConfigMessage(knob, json.jsonStr);
}

String DashioDevice::getConfigMessage(DialCfg dialData) {
    DashJSON json;
    json.start();
    json.addKeyString(F("controlID"), dialData.controlID);
    json.addKeyString(F("parentID"), dialData.parentID);
    json.addKeyFloat(F("xPositionRatio"), dialData.graphicsRect.xPositionRatio);
    json.addKeyFloat(F("yPositionRatio"), dialData.graphicsRect.yPositionRatio);
    json.addKeyFloat(F("widthRatio"), dialData.graphicsRect.widthRatio);
    json.addKeyFloat(F("heightRatio"), dialData.graphicsRect.heightRatio);
    json.addKeyString(F("title"), dialData.title);
    json.addKeyString(F("titlePosition"), getTitlePositionStr(dialData.titlePosition));

    json.addKeyFloat(F("min"), dialData.min);
    json.addKeyFloat(F("max"), dialData.max);
    json.addKeyFloat(F("redValue"), dialData.redValue);
    json.addKeyString(F("dialFillColor"), dialData.dialFillColor);
    json.addKeyString(F("pointerColor"), dialData.pointerColor);
    json.addKeyString(F("numberPosition"), getDialNumberPosition(dialData.numberPosition));
    json.addKeyBool(F("showMinMax"), dialData.showMinMax);
    json.addKeyString(F("style"), getDialPresentationStyle(dialData.style));
    json.addKeyString(F("units"), dialData.units);
    json.addKeyInt(F("precision"), dialData.precision, true);
    return getFullConfigMessage(dial, json.jsonStr);
}

String DashioDevice::getConfigMessage(DirectionCfg directionData) {
    DashJSON json;
    json.start();
    json.addKeyString(F("controlID"), directionData.controlID);
    json.addKeyString(F("parentID"), directionData.parentID);
    json.addKeyFloat(F("xPositionRatio"), directionData.graphicsRect.xPositionRatio);
    json.addKeyFloat(F("yPositionRatio"), directionData.graphicsRect.yPositionRatio);
    json.addKeyFloat(F("widthRatio"), directionData.graphicsRect.widthRatio);
    json.addKeyFloat(F("heightRatio"), directionData.graphicsRect.heightRatio);
    json.addKeyString(F("title"), directionData.title);
    json.addKeyString(F("titlePosition"), getTitlePositionStr(directionData.titlePosition));

    json.addKeyString(F("pointerColor"), directionData.pointerColor);
    json.addKeyString(F("style"), getDirectionPresentationStyle(directionData.style));
    json.addKeyInt(F("calAngle"), directionData.calAngle);
    json.addKeyString(F("units"), directionData.units);
    json.addKeyInt(F("precision"), directionData.precision, true);
    return getFullConfigMessage(direction, json.jsonStr);
}

String DashioDevice::getConfigMessage(TextBoxCfg textBoxData) {
    DashJSON json;
    json.start();
    json.addKeyString(F("controlID"), textBoxData.controlID);
    json.addKeyString(F("parentID"), textBoxData.parentID);
    json.addKeyFloat(F("xPositionRatio"), textBoxData.graphicsRect.xPositionRatio);
    json.addKeyFloat(F("yPositionRatio"), textBoxData.graphicsRect.yPositionRatio);
    json.addKeyFloat(F("widthRatio"), textBoxData.graphicsRect.widthRatio);
    json.addKeyFloat(F("heightRatio"), textBoxData.graphicsRect.heightRatio);
    json.addKeyString(F("title"), textBoxData.title);
    json.addKeyString(F("titlePosition"), getTitlePositionStr(textBoxData.titlePosition));

    json.addKeyString(F("format"), getTextFormatStr(textBoxData.format));
    json.addKeyString(F("textAlign"), getTextAlignStr(textBoxData.textAlign));
    json.addKeyString(F("units"), textBoxData.units);
    json.addKeyInt(F("precision"), textBoxData.precision);
    json.addKeyString(F("kbdType"), getKeyboardTypeStr(textBoxData.kbdType));
    json.addKeyBool(F("closeKbdOnSend"), textBoxData.closeKbdOnSend, true);
    return getFullConfigMessage(textBox, json.jsonStr);
}

String DashioDevice::getConfigMessage(SelectorCfg selectorData) {
    DashJSON json;
    json.start();
    json.addKeyString(F("controlID"), selectorData.controlID);
    json.addKeyString(F("parentID"), selectorData.parentID);
    json.addKeyFloat(F("xPositionRatio"), selectorData.graphicsRect.xPositionRatio);
    json.addKeyFloat(F("yPositionRatio"), selectorData.graphicsRect.yPositionRatio);
    json.addKeyFloat(F("widthRatio"), selectorData.graphicsRect.widthRatio);
    json.addKeyFloat(F("heightRatio"), selectorData.graphicsRect.heightRatio);
    json.addKeyString(F("title"), selectorData.title);
    json.addKeyString(F("titlePosition"), getTitlePositionStr(selectorData.titlePosition), true);
    return getFullConfigMessage(selector, json.jsonStr);
}

String DashioDevice::getConfigMessage(SliderCfg sliderData) {
    DashJSON json;
    json.start();
    json.addKeyString(F("controlID"), sliderData.controlID);
    json.addKeyString(F("parentID"), sliderData.parentID);
    json.addKeyFloat(F("xPositionRatio"), sliderData.graphicsRect.xPositionRatio);
    json.addKeyFloat(F("yPositionRatio"), sliderData.graphicsRect.yPositionRatio);
    json.addKeyFloat(F("widthRatio"), sliderData.graphicsRect.widthRatio);
    json.addKeyFloat(F("heightRatio"), sliderData.graphicsRect.heightRatio);
    json.addKeyString(F("title"), sliderData.title);
    json.addKeyString(F("titlePosition"), getTitlePositionStr(sliderData.titlePosition));

    json.addKeyFloat(F("min"), sliderData.min);
    json.addKeyFloat(F("max"), sliderData.max);
    json.addKeyFloat(F("redValue"), sliderData.redValue);
    json.addKeyBool(F("showMinMax"), sliderData.showMinMax);
    json.addKeyBool(F("sliderEnabled"), sliderData.sliderEnabled);
    json.addKeyString(F("knobColor"), sliderData.knobColor);
    json.addKeyBool(F("sendOnlyOnRelease"), sliderData.sendOnlyOnRelease);
    json.addKeyBool(F("barFollowsSlider"), sliderData.barFollowsSlider);
    json.addKeyString(F("barColor"), sliderData.barColor);
    json.addKeyString(F("barStyle"), getBarStyleStr(sliderData.barStyle), true);
    return getFullConfigMessage(slider, json.jsonStr);
}

String DashioDevice::getConfigMessage(GraphCfg graphData) {
    DashJSON json;
    json.start();
    json.addKeyString(F("controlID"), graphData.controlID);
    json.addKeyString(F("parentID"), graphData.parentID);
    json.addKeyFloat(F("xPositionRatio"), graphData.graphicsRect.xPositionRatio);
    json.addKeyFloat(F("yPositionRatio"), graphData.graphicsRect.yPositionRatio);
    json.addKeyFloat(F("widthRatio"), graphData.graphicsRect.widthRatio);
    json.addKeyFloat(F("heightRatio"), graphData.graphicsRect.heightRatio);
    json.addKeyString(F("title"), graphData.title);
    json.addKeyString(F("titlePosition"), getTitlePositionStr(graphData.titlePosition));

    json.addKeyString(F("xAxisLabel"), graphData.xAxisLabel);
    json.addKeyFloat(F("xAxisMin"), graphData.xAxisMin);
    json.addKeyFloat(F("xAxisMax"), graphData.xAxisMax);
    json.addKeyInt(F("xAxisNumBars"), graphData.xAxisNumBars);
    json.addKeyString(F("xAxisLabelsStyle"), getXAxisLabelsStyleStr(graphData.xAxisLabelsStyle));
    json.addKeyString(F("yAxisLabel"), graphData.yAxisLabel);
    json.addKeyFloat(F("yAxisMin"), graphData.yAxisMin);
    json.addKeyFloat(F("yAxisMax"), graphData.yAxisMax);
    json.addKeyInt(F("yAxisNumBars"), graphData.yAxisNumBars, true);
    return getFullConfigMessage(graph, json.jsonStr);
}

String DashioDevice::getConfigMessage(TimeGraphCfg timeGraphData) {
    DashJSON json;
    json.start();
    json.addKeyString(F("controlID"), timeGraphData.controlID);
    json.addKeyString(F("parentID"), timeGraphData.parentID);
    json.addKeyFloat(F("xPositionRatio"), timeGraphData.graphicsRect.xPositionRatio);
    json.addKeyFloat(F("yPositionRatio"), timeGraphData.graphicsRect.yPositionRatio);
    json.addKeyFloat(F("widthRatio"), timeGraphData.graphicsRect.widthRatio);
    json.addKeyFloat(F("heightRatio"), timeGraphData.graphicsRect.heightRatio);
    json.addKeyString(F("title"), timeGraphData.title);
    json.addKeyString(F("titlePosition"), getTitlePositionStr(timeGraphData.titlePosition));

    json.addKeyString(F("yAxisLabel"), timeGraphData.yAxisLabel);
    json.addKeyFloat(F("yAxisMin"), timeGraphData.yAxisMin);
    json.addKeyFloat(F("yAxisMax"), timeGraphData.yAxisMax);
    json.addKeyInt(F("yAxisNumBars"), timeGraphData.yAxisNumBars, true);
    return getFullConfigMessage(timeGraph, json.jsonStr);
}

String DashioDevice::getConfigMessage(MapCfg mapData) {
    DashJSON json;
    json.start();
    json.addKeyString(F("controlID"), mapData.controlID);
    json.addKeyString(F("parentID"), mapData.parentID);
    json.addKeyFloat(F("xPositionRatio"), mapData.graphicsRect.xPositionRatio);
    json.addKeyFloat(F("yPositionRatio"), mapData.graphicsRect.yPositionRatio);
    json.addKeyFloat(F("widthRatio"), mapData.graphicsRect.widthRatio);
    json.addKeyFloat(F("heightRatio"), mapData.graphicsRect.heightRatio);
    json.addKeyString(F("title"), mapData.title);
    json.addKeyString(F("titlePosition"), getTitlePositionStr(mapData.titlePosition), true);
    return getFullConfigMessage(mapper, json.jsonStr);
}


String DashioDevice::getTitlePositionStr(TitlePosition tbp) {
    switch (tbp) {
        case titleTop:
            return "TOP";
        case titleBottom:
            return "BOTTOM";
        default:
            return "NONE";
    }
}

String DashioDevice::getLabelStyle(LabelStyle labelStyle) {
    switch (labelStyle) {
        case basic:
            return "BASIC";
        case border:
            return "GROUP";
        default:
            return "GROUP";
    }
}

String DashioDevice::getDialNumberPosition(DialNumberPosition numberPosition) {
    switch (numberPosition) {
        case numberLeft:
            return "LEFT";
        case numberRight:
            return "RIGHT";
        case numberCenter:
            return "CENTER";
        default:
            return "OFF";
    }
}

String DashioDevice::getKnobPresentationStyle(KnobPresentationStyle presentationStyle) {
    switch (presentationStyle) {
        case knobPan:
            return "PAN";
        default:
            return "NORMAL";
    }
}

String DashioDevice::getDialPresentationStyle(DialPresentationStyle presentationStyle) {
    switch (presentationStyle) {
        case dialPie:
            return "PIE";
        case dialPieInverted:
            return "PIEINV";
        default:
            return "BAR";
    }
}

String DashioDevice::getDirectionPresentationStyle(DirectionPresentationStyle presentationStyle) {
    switch (presentationStyle) {
        case dirDeg:
            return "DEG";
        case dirDegPS:
            return "DEGPS";
        default:
            return "NSEW";
    }
}

String DashioDevice::getTextFormatStr(TextFormat format) {
    switch (format) {
        case numFmt:
            return "NUM";
        case dateTimeFmt:
            return "DATETIME";
        case dateTimeLongFmt:
            return "DTLONG";
        case intvlFmt:
            return "INTVL";
        default:
            return "NONE";
    }
}

String DashioDevice::getKeyboardTypeStr(KeyboardType kbd) {
    switch (kbd) {
        case hexKbd:
            return "HEX";
        case allKbd:
            return "ALL";
        case numKbd:
            return "NUM";
        case intKbd:
            return "INT";
        case dateKbd:
            return "DATE";
        case timeKbd:
            return "TIME";
        case dateTimeKbd:
            return "DATETIME";
        case intvlKbd:
            return "INTVL";
        default:
            return "NONE";
    }
}

String DashioDevice::getTextAlignStr(TextAlign align) {
    switch (align) {
        case textLeft:
            return "LEFT";
        case textRight:
            return "RIGHT";
        default:
            return "CENTER";
    }
}

String DashioDevice::getBarStyleStr(BarStyle barStyle) {
    switch (barStyle) {
        case segmentedBar:
            return "SEG";
        default:
            return "SOLID";
    }
}

String DashioDevice::getXAxisLabelsStyleStr(XAxisLabelsStyle xls) {
    switch (xls) {
        case labelsOnLines:
            return "ON";
        default:
            return "BETWEEN";
    }
}

