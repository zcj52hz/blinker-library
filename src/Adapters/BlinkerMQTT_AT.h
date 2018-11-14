#ifndef BlinkerMQTT_AT_H
#define BlinkerMQTT_AT_H

#if defined(ESP8266)
    #include <ESP8266mDNS.h>
    #include <ESP8266WiFi.h>
    #include <ESP8266HTTPClient.h>
    #include <ESP8266WebServer.h>
#elif defined(ESP32)
    #include <ESPmDNS.h>
    #include <WiFi.h>
    #include <HTTPClient.h>
#endif
#include "Blinker/BlinkerProtocol.h"
#include "modules/WebSockets/WebSocketsServer.h"
#include "modules/mqtt/Adafruit_MQTT.h"
#include "modules/mqtt/Adafruit_MQTT_Client.h"
#include "modules/ArduinoJson/ArduinoJson.h"


static char MQTT_HOST[BLINKER_MQTT_HOST_SIZE];
static uint16_t MQTT_PORT;
// static char MQTT_DEVICEID[BLINKER_MQTT_DEVICEID_SIZE];
static char MQTT_ID[BLINKER_MQTT_ID_SIZE];
static char MQTT_NAME[BLINKER_MQTT_NAME_SIZE];
static char MQTT_KEY[BLINKER_MQTT_KEY_SIZE];
static char MQTT_PRODUCTINFO[BLINKER_MQTT_PINFO_SIZE];
static char UUID[BLINKER_MQTT_UUID_SIZE];
// static char DEVICE_NAME[BLINKER_MQTT_DEVICENAME_SIZE];
static char DEVICE_NAME[BLINKER_MQTT_DEVICEID_SIZE];
static char *BLINKER_PUB_TOPIC;
static char *BLINKER_SUB_TOPIC;

WiFiClientSecure        client_s;
WiFiClient              client;
Adafruit_MQTT_Client    *mqtt;
// Adafruit_MQTT_Publish   *iotPub;
Adafruit_MQTT_Subscribe *iotSub;

#define WS_SERVERPORT                       81
WebSocketsServer webSocket = WebSocketsServer(WS_SERVERPORT);

static char msgBuf[BLINKER_MAX_READ_SIZE];
static bool isConnect = false;
static bool isAvail = false;
static uint8_t ws_num = 0;
static uint8_t dataFrom = BLINKER_MSG_FROM_MQTT;



#include <SoftwareSerial.h>
#include "HardwareSerial.h"
// #include <Blinker/BlinkerProtocol.h>
#include <EEPROM.h>

SoftwareSerial *SSerialBLE;
// HardwareSerial *HSerialBLE;


static WiFiServer *_server;
static WiFiClient _client;
static IPAddress apIP(192, 168, 4, 1);
#if defined(ESP8266)
static IPAddress netMsk(255, 255, 255, 0);
#endif


static bool isATAvaill = false;


static void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
{

    switch(type)
    {
        case WStype_DISCONNECTED:
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2("Disconnected! ", num);
#endif

            isConnect = false;
            break;
        case WStype_CONNECTED:
            {
                IPAddress ip = webSocket.remoteIP(num);

#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG6("num: ", num, ", Connected from: ", ip, ", url: ", (char *)payload);
#endif

                // send message to client
                webSocket.sendTXT(num, "{\"state\":\"serialConnected\"}\n");

                ws_num = num;

                isConnect = true;
            }
            break;
        case WStype_TEXT:

#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG6("num: ", num, ", get Text: ", (char *)payload, ", length: ", length);
#endif
            if (length < BLINKER_MAX_READ_SIZE) {
                // msgBuf = (char*)malloc((length+1)*sizeof(char));
                // memcpy (msgBuf, (char*)payload, length);
                // buflen = length;
                strcpy(msgBuf, (char*)payload);
                isAvail = true;
            }

            dataFrom = BLINKER_MSG_FROM_WS;

            ws_num = num;

            // send message to client
            // webSocket.sendTXT(num, "message here");

            // send data to all serialConnected clients
            // webSocket.broadcastTXT("message here");
            break;
        case WStype_BIN:
            // BLINKER_LOG4("num: ", num, " get binary length: ", length);
            // hexdump(payload, length);

            // send message to client
            // webSocket.sendBIN(num, payload, length);
            break;
    }
}

enum atState_t {
    AT_NONE,
    AT_TEST,
    AT_QUERY,
    AT_SETTING,
    AT_ACTION
};

enum atMQTT_t {
    MQTT_CONFIG_MODE,
    MQTT_AUTH_KEY,
    MQTT_WIFI_SSID,
    MQTT_WIFI_PSWD
};

enum atStatus_t {
    BL_BEGIN,
    BL_INITED
    // ,
    // BL_SUC,
    // MQTT_WIFI_PSWD
};

class ATdata
{
    public :
        ATdata() {}

        void update(String data) {
            // _data = data;
            // BLINKER_LOG2(BLINKER_F("update data: "), data);
            // _isAT = serialize(data);
            serialize(data);

            BLINKER_LOG2(BLINKER_F("serialize _set: "), _set);
            // return _isAT;
        }

        atState_t state() { return _set; }

        String cmd() { return _atCmd; }

        uint8_t paramNum() { return _paramNum; }

        String getParam(uint8_t num) {
            if (num >= _paramNum) return "";
            else return _param[num];
        }

    private :
        // bool _isAT;
        atState_t _set;
        uint8_t _paramNum;
        // String _data;
        String _atCmd;
        String _param[11];

        void serialize(String _data) {
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2(BLINKER_F("serialize _data: "), _data);
#endif
            _paramNum = 0;
            // _isAT = false;
            _set = AT_NONE;
            int addr_start = 0;//_data.indexOf("+");
            int addr_end = 0;

            String startCmd = _data.substring(0, 2);
            BLINKER_LOG2(BLINKER_F("startCmd: "), startCmd);

            // startCmd = _data.substring(0, 3);
            // BLINKER_LOG2(BLINKER_F("startCmd: "), startCmd);

            BLINKER_LOG2(BLINKER_F("startCmd len: "), _data.length());

            // BLINKER_LOG2(BLINKER_F("serialize addr_start: "), addr_start);
            // BLINKER_LOG2(BLINKER_F("serialize addr_end: "), addr_end);

            // if ((addr_start != -1) && STRING_contains_string(_data, ":"))
            if (startCmd == BLINKER_CMD_AT)
            {
                // _isAT = true;

                // if (_data.length() == 3) {
                //     _set = AT_ACTION;
                //     return true;
                // }

                addr_start = 3;
                // int addr_end1 = _data.indexOf("=");
                // int addr_end2 = _data.indexOf("?");
                // int addr_end3 = _data.indexOf("\n");

                // check "="
                addr_end = _data.indexOf("=");
#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG2(BLINKER_F("serialize addr_end: "), addr_end);
#endif
                if (addr_end != -1)
                {
                    _set = AT_SETTING;

                    _atCmd = _data.substring(addr_start, addr_end);
#ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2(BLINKER_F("serialize _atCmd: "), _atCmd);
#endif
                }
                else
                {
                    addr_end = _data.indexOf("?");
#ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2(BLINKER_F("serialize addr_end: "), addr_end);
#endif
                    if (addr_end != -1)
                    {
                        _set = AT_QUERY;

                        _atCmd = _data.substring(addr_start, addr_end);
#ifdef BLINKER_DEBUG_ALL
                        BLINKER_LOG2(BLINKER_F("serialize _atCmd: "), _atCmd);
#endif
                        return;
                    }
                    else
                    {
                        addr_end = _data.indexOf("\r");
#ifdef BLINKER_DEBUG_ALL
                        BLINKER_LOG2(BLINKER_F("serialize addr_end: "), addr_end);
#endif
                        if (addr_end != -1)
                        {
                            _set = AT_ACTION;

                            if (addr_end == (addr_start-1)) {
                                _atCmd = startCmd;
#ifdef BLINKER_DEBUG_ALL
                                BLINKER_LOG2(BLINKER_F("serialize _atCmd: "), _atCmd);
#endif
                                return;
                            }

                            _atCmd = _data.substring(addr_start, addr_end);
#ifdef BLINKER_DEBUG_ALL
                            BLINKER_LOG2(BLINKER_F("serialize _atCmd: "), _atCmd);
#endif
                            return;
                        }
                    }
                }

                // BLINKER_LOG2(BLINKER_F("serialize _data: "), _data);

                String serData;
                uint16_t dataLen = _data.length() - 1;

                addr_start = 0;

                for (_paramNum = 0; _paramNum < 11; _paramNum++) {
                    addr_start += addr_end;
                    addr_start += 1;
                    serData = _data.substring(addr_start, dataLen);

                    addr_end = serData.indexOf(",");

                    BLINKER_LOG2(BLINKER_F("serialize serData: "), serData);
                    BLINKER_LOG2(BLINKER_F("serialize addr_start: "), addr_start);
                    BLINKER_LOG2(BLINKER_F("serialize addr_end: "), addr_end);

                    if (addr_end == -1) {
                        if (addr_start >= dataLen) return;

                        addr_end = serData.indexOf(" ");

                        if (addr_end != -1) {
                            _param[_paramNum] = serData.substring(0, addr_end);

                            if (_param[_paramNum] == "?" && _paramNum == 0) {
                                _set = AT_TEST;
                            }
#ifdef BLINKER_DEBUG_ALL
                            BLINKER_LOG2(BLINKER_F("_param[_paramNum]0: "), _param[_paramNum]);
                            // BLINKER_LOG2(BLINKER_F("_set: "), _set);
#endif
                            _paramNum++;
                            return;
                        }

                        _param[_paramNum] = serData;

                        if (_param[_paramNum] == "?" && _paramNum == 0) {
                            _set = AT_TEST;
                        }
#ifdef BLINKER_DEBUG_ALL
                        BLINKER_LOG2(BLINKER_F("_param[_paramNum]1: "), _param[_paramNum]);
                        // BLINKER_LOG2(BLINKER_F("_set: "), _set);
#endif
                        _paramNum++;
                        return;
                    }
                    else {
                        _param[_paramNum] = serData.substring(0, addr_end);
                    }
#ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2(BLINKER_F("_param[_paramNum]: "), _param[_paramNum]);
#endif
                }
                // return;
            }
            // else {
            //     // _isAT = false;
            //     return;
            // }
        }
};

static class ATdata * _atData;

class BlinkerTransportStream
{
    public :
        BlinkerTransportStream()
            : stream(NULL), isSerialConnect(false)
        {}

        bool serialAvailable()
        {
            if (!isHWS)
            {
                if (!SSerialBLE->isListening())
                {
                    SSerialBLE->listen();
                    ::delay(100);
                }
            }
            
            if (stream->available())
            {
                strcpy(streamData, (stream->readStringUntil('\n')).c_str());
#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG2(BLINKER_F("handleSerial: "), streamData);
#endif

                // if (!_atData)
                // {
                // if (isATAvaill) free(_atData);

                _atData = new ATdata();

                _atData->update(STRING_format(streamData));
                // }
                // else
                // {
                //     _atData->update(STRING_format(streamData));
                // }
#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG2(BLINKER_F("state: "), _atData->state());
                BLINKER_LOG2(BLINKER_F("cmd: "), _atData->cmd());
                BLINKER_LOG2(BLINKER_F("paramNum: "), _atData->paramNum());
#endif

                if (_atData->state())
                {
                    parseATdata();

                    // free(_atData);
                    // isATAvaill = true;

                    return false;
                }

                free(_atData);

                return true;
            }
            else
            {
                return false;
            }
        }

        void serialBegin(Stream& s, bool state)
        {
            stream = &s;
            stream->setTimeout(BLINKER_STREAM_TIMEOUT);
            isHWS = state;

            serialConnect();

            serialPrint(BLINKER_CMD_BLINKER_MQTT);

            // stream->flush();

            while (_status == BL_BEGIN)
            {
                serialAvailable();
            }
        }

        String serialLastRead() { return STRING_format(streamData); }

        bool serialPrint(String s1, String s2)
        {
            return serialPrint(s1 + s2);
        }

        bool serialPrint(String s)
        {
            // bool state = STRING_contains_string(s, BLINKER_CMD_NOTICE);

            // if (!state) {
            //     state = (STRING_contains_string(s, BLINKER_CMD_STATE) 
            //         && STRING_contains_string(s, BLINKER_CMD_CONNECTED));
            // }

            // if (!state) {
            //     if (!checkPrintSpan()) {
            //         respTime = millis();
            //         return false;
            //     }
            // }

            // respTime = millis();

#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2(BLINKER_F("Response: "), s);
#endif
            if(serialConnected()) {
#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG1(BLINKER_F("Succese..."));
#endif
                stream->println(s);
                return true;
            }
            else {
#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG1(BLINKER_F("Faile... Disconnected"));
#endif
                return false;
            }
        }

        bool serialConnect()
        {
            isSerialConnect = true;
            return serialConnected();
        }

        bool serialConnected() { return isSerialConnect; }

        void serialDisconnect() { isSerialConnect = false; }



        bool connect();

        bool connected() { 
            if (!isMQTTinit) {
                return *isHandle;
            }

            return mqtt->connected()||*isHandle; 
        }

        bool mConnected() {
            if (!isMQTTinit) return false;
            else return mqtt->connected();
        }

        void disconnect() {
            mqtt->disconnect();

            if (*isHandle)
                webSocket.disconnect();
        }
        void ping();
        
        bool available() {
            if (serialAvailable()) mqttPrint(STRING_format(streamData));

            webSocket.loop();

            checkKA();

            if (!mqtt->connected() || (millis() - this->latestTime) > BLINKER_MQTT_PING_TIMEOUT) {
                ping();
            }
            else {
                subscribe();
            }

            if (isAvail) {
                isAvail = false;
                serialPrint(STRING_format(msgBuf));
                return true;
            }
            else {
                return false;
            }
        }

        bool aligenieAvail() {
            if (isAliAvail) {
                isAliAvail = false;

                return true;
            }
            else {
                return false;
            }
        }

        bool extraAvailable() {
            if (isBavail) {
                isBavail = false;
                
                return true;
            }
            else {
                return false;
            }
        }

        void subscribe();
        String lastRead() { return STRING_format(msgBuf); }
        bool print(String data);
        bool mqttPrint(String data);
        bool bPrint(String name, String data);

#if defined(BLINKER_ALIGENIE)
        bool aliPrint(String data);
#endif

        void begin(const char* auth) {
            // _authKey = auth;
            strcpy(_authKey, auth);
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2("_authKey: ", _authKey);
#endif
            // if (connectServer()) {
            //     mDNSInit();
            //     isMQTTinit = true;
            // }
            // else {
                uint32_t re_time = millis();
                bool isConnect = true;
                while(1) {
                    re_time = millis();
                    // ::delay(10000);
                    // BLINKER_ERR_LOG1("Maybe you have put in the wrong AuthKey!");
                    // BLINKER_ERR_LOG1("Or maybe your request is too frequently!");
                    // BLINKER_ERR_LOG1("Or maybe your network is disconnected!");
                    if (connectServer()) {
                        mDNSInit();
                        isMQTTinit = true;
                        return;
                    }
                    // delay(10000);
                    while ((millis() - re_time) < 10000) {
                        if (WiFi.status() != WL_CONNECTED && isConnect) {
                            isConnect = false;
                            WiFi.begin();
                            WiFi.reconnect();
                        }
                        else if (WiFi.status() == WL_CONNECTED && !isConnect) {
                            isConnect = true;
                        }
                        ::delay(10);
                        // WiFi.status();
                    }
                }
            // }
        }

        bool autoPrint(uint32_t id) {
            String payload = "{\"data\":{\"set\":{" + \
                STRING_format("\"trigged\":true,\"autoData\":{") + \
                "\"autoId\":" + STRING_format(id)  + "}}}" + \
                ",\"fromDevice\":\"" + STRING_format(MQTT_ID) + "\"" + \
                ",\"toDevice\":\"" + "autoManager" + "\"}";
                // "\",\"deviceType\":\"" + "type" + "\"}";

#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG1("autoPrint...");
#endif
            if (mqtt->connected()) {
                if ((millis() - linkTime) > BLINKER_LINK_MSG_LIMIT || linkTime == 0) {
                    // linkTime = millis();

                    // Adafruit_MQTT_Publish iotPub = Adafruit_MQTT_Publish(mqtt, BLINKER_PUB_TOPIC);

                    // if (! iotPub.publish(payload.c_str())) {

                    if (! mqtt->publish(BLINKER_PUB_TOPIC, payload.c_str())) {
#ifdef BLINKER_DEBUG_ALL
                        BLINKER_LOG1(payload);
                        BLINKER_LOG1("...Failed");
#endif
                        return false;
                    }
                    else {
#ifdef BLINKER_DEBUG_ALL
                        BLINKER_LOG1(payload);
                        BLINKER_LOG1("...OK!");
#endif
                        linkTime = millis();
                        return true;
                    }
                }
                else {
#ifdef BLINKER_DEBUG_ALL
                    BLINKER_ERR_LOG2("MQTT NOT ALIVE OR MSG LIMIT ", linkTime);
#endif
                    return false;
                }
            }
            else {
                BLINKER_ERR_LOG1("MQTT Disconnected");
                return false;
            }
        }

        bool autoPrint(char *name, char *type, char *data) {
            String payload = "{\"data\":{" + STRING_format(data) + "}," + \ 
                + "\"fromDevice\":\"" + STRING_format(MQTT_ID) + "\"," + \
                + "\"toDevice\":\"" + name + "\"," + \
                + "\"deviceType\":\"" + type + "\"}";

#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG1("autoPrint...");
#endif
            if (mqtt->connected()) {
                if ((millis() - linkTime) > BLINKER_LINK_MSG_LIMIT || linkTime == 0) {
                    linkTime = millis();
#ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2(payload, ("...OK!"));
#endif
                    return true;
                }
                else {
#ifdef BLINKER_DEBUG_ALL
                    BLINKER_ERR_LOG2("MQTT NOT ALIVE OR MSG LIMIT ", linkTime);
#endif
                    return false;
                }
            }
            else {
                BLINKER_ERR_LOG1("MQTT Disconnected");
                return false;
            }
        }

        bool autoPrint(char *name1, char *type1, char *data1
            , char *name2, char *type2, char *data2)
        {
            String payload = "{\"data\":{" + STRING_format(data1) + "}," + \ 
                + "\"fromDevice\":\"" + STRING_format(MQTT_ID) + "\"," + \
                + "\"toDevice\":\"" + name1 + "\"," + \
                + "\"deviceType\":\"" + type1 + "\"}";

#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG1("autoPrint...");
#endif
            if (mqtt->connected()) {
                if ((millis() - linkTime) > BLINKER_LINK_MSG_LIMIT || linkTime == 0) {
                    linkTime = millis();
#ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2(payload, ("...OK!"));
#endif
                    payload = "{\"data\":{" + STRING_format(data2) + "}," + \ 
                        + "\"fromDevice\":\"" + STRING_format(MQTT_ID) + "\"," + \
                        + "\"toDevice\":\"" + name2 + "\"," + \
                        + "\"deviceType\":\"" + type2 + "\"}";

#ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2(payload, ("...OK!"));
#endif                    
                    return true;
                }
                else {
#ifdef BLINKER_DEBUG_ALL
                    BLINKER_ERR_LOG2("MQTT NOT ALIVE OR MSG LIMIT ", linkTime);
#endif
                    return false;
                }
            }
            else {
                BLINKER_ERR_LOG1("MQTT Disconnected");
                return false;
            }
        }

        String deviceName() { return DEVICE_NAME;/*MQTT_ID;*/ }

        String authKey() { return _authKey;/*MQTT_ID;*/ }

        bool init() { return isMQTTinit; }

        bool reRegister() { return connectServer(); }

    private :

        void parseATdata()
        {
            if (_atData->cmd() == BLINKER_CMD_AT) {
                serialPrint(BLINKER_CMD_OK);
            }
            else if (_atData->cmd() == BLINKER_CMD_RST) {
                serialPrint(BLINKER_CMD_OK);
            }
            else if (_atData->cmd() == BLINKER_CMD_BLINKER_MQTT) {
                // serialPrint(BLINKER_CMD_OK);

                BLINKER_LOG1(BLINKER_CMD_BLINKER_MQTT);

                atState_t at_state = _atData->state();

                BLINKER_LOG1(at_state);

                String reqData;

                switch (at_state)
                {
                    case AT_NONE:
                        // serialPrint();
                        break;
                    case AT_TEST:
                        reqData = STRING_format(BLINKER_CMD_AT) + \
                                "+" + STRING_format(BLINKER_CMD_BLINKER_MQTT) + \
                                "=<MQTT_CONFIG_MODE>,<MQTT_AUTH_KEY>" + \
                                "[,<MQTT_WIFI_SSID>,<MQTT_WIFI_PSWD>]";
                        serialPrint(reqData);
                        break;
                    case AT_QUERY:
                        reqData = "+" + STRING_format(BLINKER_CMD_BLINKER_MQTT) + \
                                "=" + getMode(_wlanMode) + \
                                "," + STRING_format(_authKey) + \
                                "," + WiFi.SSID() + \
                                "," + WiFi.psk();
                        serialPrint(reqData);
                        break;
                    case AT_SETTING:
#ifdef BLINKER_DEBUG_ALL
                        BLINKER_LOG2(BLINKER_F("MQTT_CONFIG_MODE: "), _atData->getParam(MQTT_CONFIG_MODE));
                        BLINKER_LOG2(BLINKER_F("MQTT_AUTH_KEY: "),  _atData->getParam(MQTT_AUTH_KEY));
                        BLINKER_LOG2(BLINKER_F("MQTT_WIFI_SSID: "), _atData->getParam(MQTT_WIFI_SSID));
                        BLINKER_LOG2(BLINKER_F("MQTT_WIFI_PSWD: "), _atData->getParam(MQTT_WIFI_PSWD));
#endif

                        if ((_atData->getParam(MQTT_CONFIG_MODE)).toInt() == BLINKER_CMD_COMCONFIG_NUM)
                        {
                            BLINKER_LOG1(BLINKER_F("BLINKER_CMD_COMWLAN"));

                            connectWiFi((_atData->getParam(MQTT_WIFI_SSID)).c_str(), 
                                        (_atData->getParam(MQTT_WIFI_PSWD)).c_str());
                            
                            begin((_atData->getParam(MQTT_AUTH_KEY)).c_str());
                            _status = BL_INITED;
                            _wlanMode = BLINKER_CMD_COMCONFIG_NUM;
                        }
                        else if ((_atData->getParam(MQTT_CONFIG_MODE)).toInt() == BLINKER_CMD_SMARTCONFIG_NUM)
                        {
                            BLINKER_LOG1(BLINKER_F("BLINKER_CMD_SMARTCONFIG"));

                            if (!autoInit()) smartconfig();

                            begin((_atData->getParam(MQTT_AUTH_KEY)).c_str());
                            _status = BL_INITED;
                            _wlanMode = BLINKER_CMD_SMARTCONFIG_NUM;
                        }
                        else if ((_atData->getParam(MQTT_CONFIG_MODE)).toInt() == BLINKER_CMD_APCONFIG_NUM)
                        {
                            BLINKER_LOG1(BLINKER_F("BLINKER_CMD_APCONFIG"));

                            if (!autoInit())
                            {
                                softAPinit();
                                while(WiFi.status() != WL_CONNECTED) {
                                    serverClient();

                                    ::delay(10);
                                }
                            }

                            begin((_atData->getParam(MQTT_AUTH_KEY)).c_str());
                            _status = BL_INITED;
                            _wlanMode = BLINKER_CMD_APCONFIG_NUM;
                        }
                        else {
                            return;
                        }
                        serialPrint(BLINKER_CMD_OK);
                        break;
                    case AT_ACTION:
                        // serialPrint();
                        break;
                    default :
                        break;
                }
            }
        }

        String getMode(uint8_t mode)
        {
            switch (mode)
            {
                case BLINKER_CMD_COMCONFIG_NUM :
                    return BLINKER_CMD_COMCONFIG;
                case BLINKER_CMD_SMARTCONFIG_NUM :
                    return BLINKER_CMD_SMARTCONFIG;
                case BLINKER_CMD_APCONFIG_NUM :
                    return BLINKER_CMD_APCONFIG;
                default :
                    return BLINKER_CMD_COMCONFIG;
            }
        }

        bool isMQTTinit = false;

        bool connectServer();

        void mDNSInit()
        {
#if defined(ESP8266)
            if (!MDNS.begin(DEVICE_NAME, WiFi.localIP())) {
#elif defined(ESP32)
            if (!MDNS.begin(DEVICE_NAME)) {
#endif
                while(1) {
                    ::delay(100);
                }
            }

            BLINKER_LOG1(("mDNS responder started"));
            
            MDNS.addService(BLINKER_MDNS_SERVICE_BLINKER, "tcp", WS_SERVERPORT);
            MDNS.addServiceTxt(BLINKER_MDNS_SERVICE_BLINKER, "tcp", "deviceName", String(DEVICE_NAME));

            webSocket.begin();
            webSocket.onEvent(webSocketEvent);
            BLINKER_LOG1(("webSocket server started"));
            BLINKER_LOG4("ws://", DEVICE_NAME, ".local:", WS_SERVERPORT);
        }

        void checkKA() {
            if (millis() - kaTime >= BLINKER_MQTT_KEEPALIVE)
                isAlive = false;
        }

        bool checkAliKA() {
            if (millis() - aliKaTime >= 10000)
                return false;
            else
                return true;
        }

        bool checkCanPrint() {
            if ((millis() - printTime >= BLINKER_MQTT_MSG_LIMIT && isAlive) || printTime == 0) {
                return true;
            }
            else {
#ifdef BLINKER_DEBUG_ALL
                BLINKER_ERR_LOG1("MQTT NOT ALIVE OR MSG LIMIT");
#endif
                checkKA();

                return false;
            }
        }

        bool checkCanBprint() {
            if ((millis() - bPrintTime >= BLINKER_BRIDGE_MSG_LIMIT) || bPrintTime == 0) {
                return true;
            }
            else {
#ifdef BLINKER_DEBUG_ALL
                BLINKER_ERR_LOG1("MQTT NOT ALIVE OR MSG LIMIT");
#endif
                return false;
            }
        }

        bool checkPrintSpan() {
            if (millis() - respTime < BLINKER_PRINT_MSG_LIMIT) {
                if (respTimes > BLINKER_PRINT_MSG_LIMIT) {
#ifdef BLINKER_DEBUG_ALL
                    BLINKER_ERR_LOG1("WEBSOCKETS CLIENT NOT ALIVE OR MSG LIMIT");
#endif
                    return false;
                }
                else {
                    respTimes++;
                    return true;
                }
            }
            else {
                respTimes = 0;
                return true;
            }
        }

        bool checkAliPrintSpan() {
            if (millis() - respAliTime < BLINKER_PRINT_MSG_LIMIT/2) {
                if (respAliTimes > BLINKER_PRINT_MSG_LIMIT/2) {
#ifdef BLINKER_DEBUG_ALL
                    BLINKER_ERR_LOG1("ALIGENIE NOT ALIVE OR MSG LIMIT");
#endif
                    return false;
                }
                else {
                    respAliTimes++;
                    return true;
                }
            }
            else {
                respAliTimes = 0;
                return true;
            }
        }

// #if defined(BLINKER_APCONFIG)
        void softAPinit()
        {
            _server = new WiFiServer(80);

            WiFi.mode(WIFI_AP);
            String softAP_ssid = ("DiyArduino_") + macDeviceName();
            
#if defined(ESP8266)
            WiFi.hostname(softAP_ssid);
            WiFi.softAPConfig(apIP, apIP, netMsk);
#elif defined(ESP32)
            WiFi.setHostname(softAP_ssid.c_str());
            WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
#endif

            WiFi.softAP(softAP_ssid.c_str(), ("12345678"));
            delay(100);

            _server->begin();
            BLINKER_LOG2(BLINKER_F("AP IP address: "), WiFi.softAPIP());
            BLINKER_LOG1(BLINKER_F("HTTP _server started"));
            BLINKER_LOG2(BLINKER_F("URL: http://"), WiFi.softAPIP());
        }

        void serverClient()
        {
            if (!_client)
            {
                _client = _server->available();
            }
            else
            {
                if (_client.status() == CLOSED)
                {
                    _client.stop();
                    BLINKER_LOG1(BLINKER_F("Connection closed on _client"));
                }
                else
                {
                    if (_client.available())
                    {
                        String data = _client.readStringUntil('\r');

                        // data = data.substring(4, data.length() - 9);
                        _client.flush();

                        // BLINKER_LOG2("clientData: ", data);

                        if (STRING_contains_string(data, "ssid") && STRING_contains_string(data, "pswd")) {

                            String msg = ("{\"hello\":\"world\"}");
                            
                            String s= "HTTP/1.1 200 OK\r\nContent-Type: application/json;charset=utf-8\r\n";
                            s += String("Content-Length: " + String(msg.length()) + "\r\n" +  
                                "Connection: Keep Alive\r\n\r\n" +  
                                msg + "\r\n");

                            _client.print(s);
                            
                            _client.stop();

                            parseUrl(data);
                        }
                    }
                }
            }
        }

        bool parseUrl(String data)
        {
            BLINKER_LOG2(BLINKER_F("APCONFIG data: "), data);
            DynamicJsonBuffer jsonBuffer;
            JsonObject& wifi_data = jsonBuffer.parseObject(data);

            if (!wifi_data.success()) {
                return false;
            }
                            
            String _ssid = wifi_data["ssid"];
            String _pswd = wifi_data["pswd"];

            BLINKER_LOG2(BLINKER_F("ssid: "), _ssid);
            BLINKER_LOG2(BLINKER_F("pswd: "), _pswd);

            free(_server);
            connectWiFi(_ssid, _pswd);
            return true;
        }
// #endif

        bool autoInit() {
            WiFi.mode(WIFI_STA);
            String _hostname = ("DiyArduino_") + macDeviceName();

#if defined(ESP8266)
            WiFi.hostname(_hostname.c_str());
#elif defined(ESP32)
            WiFi.setHostname(_hostname.c_str());
#endif

            WiFi.begin();
            ::delay(500);
            BLINKER_LOG3(BLINKER_F("Waiting for WiFi "),BLINKER_WIFI_AUTO_INIT_TIMEOUT / 1000,
                BLINKER_F("s, will enter SMARTCONFIG or APCONFIG while WiFi not connect!"));
            uint8_t _times = 0;
            while (WiFi.status() != WL_CONNECTED) {
                ::delay(500);
                if (_times > BLINKER_WIFI_AUTO_INIT_TIMEOUT / 500) break;
                _times++;
            }

            if (WiFi.status() != WL_CONNECTED) return false;
            else {
                BLINKER_LOG1(BLINKER_F("WiFi Connected."));

                BLINKER_LOG1(BLINKER_F("IP Address: "));
                BLINKER_LOG1(WiFi.localIP());

                // mDNSInit();
                return true;
            }
        }

// #if defined(BLINKER_ESP_SMARTCONFIG)
        void smartconfig() {
            WiFi.mode(WIFI_STA);
            String _hostname = ("DiyArduino_") + macDeviceName();

#if defined(ESP8266)
            WiFi.hostname(_hostname.c_str());
#elif defined(ESP32)
            WiFi.setHostname(_hostname.c_str());
#endif

            WiFi.beginSmartConfig();
            
            BLINKER_LOG1(BLINKER_F("Waiting for SmartConfig."));
            while (!WiFi.smartConfigDone()) {
                ::delay(500);
            }

            BLINKER_LOG1(BLINKER_F("SmartConfig received."));
            
            BLINKER_LOG1(BLINKER_F("Waiting for WiFi"));
            while (WiFi.status() != WL_CONNECTED) {
                ::delay(500);
            }

            BLINKER_LOG1(BLINKER_F("WiFi Connected."));

            BLINKER_LOG1(BLINKER_F("IP Address: "));
            BLINKER_LOG1(WiFi.localIP());

            // mDNSInit();
        }
// #endif
        // void mDNSInit()
        // {
        //     if (!MDNS.begin(macDeviceName().c_str(), WiFi.localIP())) {
        //         while(1) {
        //             ::delay(100);
        //         }
        //     }

        //     BLINKER_LOG1(("mDNS responder started"));
            
        //     MDNS.addService("DiyArduino", "tcp", WS_SERVERPORT);

        //     this->conn.begin(macDeviceName());
        // }

        void connectWiFi(String _ssid, String _pswd)
        {
            connectWiFi(_ssid.c_str(), _pswd.c_str());
        }

        void connectWiFi(const char* _ssid, const char* _pswd)
        {
            uint32_t connectTime = millis();

            BLINKER_LOG2(BLINKER_F("Connecting to "), _ssid);

            WiFi.mode(WIFI_STA);
            String _hostname = ("DiyArduinoMQTT_") + macDeviceName();

#if defined(ESP8266)
            WiFi.hostname(_hostname.c_str());
#elif defined(ESP32)
            WiFi.setHostname(_hostname.c_str());
#endif

            if (_pswd && strlen(_pswd)) {
                WiFi.begin(_ssid, _pswd);
            }
            else {
                WiFi.begin(_ssid);
            }

            while (WiFi.status() != WL_CONNECTED) {
                ::delay(50);

                if (millis() - connectTime > BLINKER_CONNECT_TIMEOUT_MS && WiFi.status() != WL_CONNECTED) {
                    connectTime = millis();
                    BLINKER_LOG1(BLINKER_F("WiFi connect timeout, please check ssid and pswd!"));
                    BLINKER_LOG1(BLINKER_F("Retring WiFi connect again!"));
                }
            }
            BLINKER_LOG1(BLINKER_F("Connected"));

            IPAddress myip = WiFi.localIP();
            BLINKER_LOG2(BLINKER_F("Your IP is: "), myip);

            // mDNSInit();
        }

    protected :
        atStatus_t  _status = BL_BEGIN;
        uint8_t     _wlanMode = BLINKER_CMD_COMCONFIG_NUM;

        char        _authKey[BLINKER_AUTHKEY_SIZE];
        bool*       isHandle = &isConnect;
        bool        isAlive = false;
        bool        isBavail = false;
        uint32_t    latestTime;
        uint32_t    printTime = 0;
        uint32_t    bPrintTime = 0;
        uint32_t    kaTime = 0;
        uint32_t    linkTime = 0;
        uint8_t     respTimes = 0;
        uint32_t    respTime = 0;
        uint8_t     respAliTimes = 0;
        uint32_t    respAliTime = 0;

        uint32_t    aliKaTime = 0;
        bool        isAliAlive = false;
        bool        isAliAvail = false;
        String      mqtt_broker;

    // protected :
        Stream* stream;
        char    streamData[BLINKER_MAX_READ_SIZE];
        bool    isSerialConnect;
        bool    isHWS = false;
//         uint8_t respTimes = 0;
//         uint32_t    respTime = 0;

//         bool checkPrintSpan() {
//             if (millis() - respTime < BLINKER_PRINT_MSG_LIMIT) {
//                 if (respTimes > BLINKER_PRINT_MSG_LIMIT) {
// #ifdef BLINKER_DEBUG_ALL
//                     BLINKER_ERR_LOG1("DEVICE NOT CONNECT OR MSG LIMIT");
// #endif
//                     return false;
//                 }
//                 else {
//                     respTimes++;
//                     return true;
//                 }
//             }
//             else {
//                 respTimes = 0;
//                 return true;
//             }
//         }
};

bool BlinkerTransportStream::connectServer() {
    const int httpsPort = 443;
#if defined(ESP8266)
    const char* host = "iotdev.clz.me";
    const char* fingerprint = "84 5f a4 8a 70 5e 79 7e f5 b3 b4 20 45 c8 35 55 72 f6 85 5a";

    // WiFiClientSecure client_s;

#ifdef BLINKER_DEBUG_ALL
    BLINKER_LOG2(("connecting to "), host);
#endif

    
    uint8_t connet_times = 0;
    client_s.stop();
    ::delay(100);

    while (1) {
        bool cl_connected = false;
        if (!client_s.connect(host, httpsPort)) {
    // #ifdef BLINKER_DEBUG_ALL
            BLINKER_ERR_LOG1(("server connection failed"));
    // #endif
            // return BLINKER_CMD_FALSE;

            connet_times++;
        }
        else {
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG1(("connection succeed"));
#endif
            // return true;
            cl_connected = true;

            break;
        }

        if (connet_times >= 4 && !cl_connected)  return BLINKER_CMD_FALSE;
    }

#ifndef BLINKER_LAN_DEBUG
    if (client_s.verify(fingerprint, host)) {
    #ifdef BLINKER_DEBUG_ALL
        // _status = DH_VERIFIED;
        BLINKER_LOG1(("Fingerprint verified"));
        // return true;
    #endif
    }
    else {
    #ifdef BLINKER_DEBUG_ALL
        // _status = DH_VERIFY_FAILED;
        // _status = DH_VERIFIED;
        BLINKER_LOG1(("Fingerprint verification failed!"));
        // return false;
    #endif
    }
#endif

    // String url;
    String client_msg;

    String url_iot = "/api/v1/user/device/diy/auth?authKey=" + String(_authKey);

#if defined(BLINKER_ALIGENIE_LIGHT)
    url_iot += "&aliType=light";
#elif defined(BLINKER_ALIGENIE_OUTLET)
    url_iot += "&aliType=outlet";
#elif defined(BLINKER_ALIGENIE_SWITCH)
#elif defined(BLINKER_ALIGENIE_SENSOR)
    url_iot += "&aliType=sensor";
#endif

#ifdef BLINKER_DEBUG_ALL 
    BLINKER_LOG3("HTTPS begin: ", host, url_iot);
#endif

    client_msg = STRING_format("GET " + url_iot + " HTTP/1.1\r\n" +
        "Host: " + host + ":" + STRING_format(httpsPort) + "\r\n" +
        "Connection: close\r\n\r\n");

    client_s.print(client_msg);
#ifdef BLINKER_DEBUG_ALL
    BLINKER_LOG2(("client_msg: "), client_msg);
#endif

    unsigned long timeout = millis();
    while (client_s.available() == 0) {
        if (millis() - timeout > 5000) {
            BLINKER_LOG1((">>> Client Timeout !"));
            client_s.stop();
            return BLINKER_CMD_FALSE;
        }
    }

    String _dataGet;
    String lastGet;
    String lengthOfJson;
    while (client_s.available()) {
        // String line = client_s.readStringUntil('\r');
        _dataGet = client_s.readStringUntil('\n');

        if (_dataGet.startsWith("Content-Length: ")){
            int addr_start = _dataGet.indexOf(' ');
            int addr_end = _dataGet.indexOf('\0', addr_start + 1);
            lengthOfJson = _dataGet.substring(addr_start + 1, addr_end);
        }

        if (_dataGet == "\r") {
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG1(("headers received"));
#endif
            break;
        }
    }

    for(int i=0;i<lengthOfJson.toInt();i++){
        lastGet += (char)client_s.read();
    }

    _dataGet = lastGet;

#ifdef BLINKER_DEBUG_ALL
    BLINKER_LOG2(("_dataGet: "), _dataGet);
#endif

    String payload = _dataGet;

#elif defined(ESP32)
    const char* host = "https://iotdev.clz.me";
    // const char* ca = \ 
    //     "-----BEGIN CERTIFICATE-----\n" \
    //     "MIIEgDCCA2igAwIBAgIQDKTfhr9lmWbWUT0hjX36oDANBgkqhkiG9w0BAQsFADBy\n" \
    //     "MQswCQYDVQQGEwJDTjElMCMGA1UEChMcVHJ1c3RBc2lhIFRlY2hub2xvZ2llcywg\n" \
    //     "SW5jLjEdMBsGA1UECxMURG9tYWluIFZhbGlkYXRlZCBTU0wxHTAbBgNVBAMTFFRy\n" \
    //     "dXN0QXNpYSBUTFMgUlNBIENBMB4XDTE4MDEwNDAwMDAwMFoXDTE5MDEwNDEyMDAw\n" \
    //     "MFowGDEWMBQGA1UEAxMNaW90ZGV2LmNsei5tZTCCASIwDQYJKoZIhvcNAQEBBQAD\n" \
    //     "ggEPADCCAQoCggEBALbOFn7cJ2I/FKMJqIaEr38n4kCuJCCeNf1bWdWvOizmU2A8\n" \
    //     "QeTAr5e6Q3GKeJRdPnc8xXhqkTm4LOhgdZB8KzuVZARtu23D4vj4sVzxgC/zwJlZ\n" \
    //     "MRMxN+cqI37kXE8gGKW46l2H9vcukylJX+cx/tjWDfS2YuyXdFuS/RjhCxLgXzbS\n" \
    //     "cve1W0oBZnBPRSMV0kgxTWj7hEGZNWKIzK95BSCiMN59b+XEu3NWGRb/VzSAiJEy\n" \
    //     "Hy9DcDPBC9TEg+p5itHtdMhy2gq1OwsPgl9HUT0xmDATSNEV2RB3vwviNfu9/Eif\n" \
    //     "ObhsV078zf30TqdiESqISEB68gJ0Otru67ePoTkCAwEAAaOCAWowggFmMB8GA1Ud\n" \
    //     "IwQYMBaAFH/TmfOgRw4xAFZWIo63zJ7dygGKMB0GA1UdDgQWBBR/KLqnke61779P\n" \
    //     "xc9htonQwLOxPDAYBgNVHREEETAPgg1pb3RkZXYuY2x6Lm1lMA4GA1UdDwEB/wQE\n" \
    //     "AwIFoDAdBgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwTAYDVR0gBEUwQzA3\n" \
    //     "BglghkgBhv1sAQIwKjAoBggrBgEFBQcCARYcaHR0cHM6Ly93d3cuZGlnaWNlcnQu\n" \
    //     "Y29tL0NQUzAIBgZngQwBAgEwgYEGCCsGAQUFBwEBBHUwczAlBggrBgEFBQcwAYYZ\n" \
    //     "aHR0cDovL29jc3AyLmRpZ2ljZXJ0LmNvbTBKBggrBgEFBQcwAoY+aHR0cDovL2Nh\n" \
    //     "Y2VydHMuZGlnaXRhbGNlcnR2YWxpZGF0aW9uLmNvbS9UcnVzdEFzaWFUTFNSU0FD\n" \
    //     "QS5jcnQwCQYDVR0TBAIwADANBgkqhkiG9w0BAQsFAAOCAQEAhtM4eyrWB14ajJpQ\n" \
    //     "ibZ5FbzVuvv2Le0FOSoss7UFCDJUYiz2LiV8yOhL4KTY+oVVkqHaYtcFS1CYZNzj\n" \
    //     "6xWcqYZJ+pgsto3WBEgNEEe0uLSiTW6M10hm0LFW9Det3k8fqwSlljqMha3gkpZ6\n" \
    //     "8WB0f2clXOuC+f1SxAOymnGUsSqbU0eFSgevcOIBKR7Hr3YXBXH3jjED76Q52OMS\n" \
    //     "ucfOM9/HB3jN8o/ioQbkI7xyd/DUQtzK6hSArEoYRl3p5H2P4fr9XqmpoZV3i3gQ\n" \
    //     "oOdVycVtpLunyUoVAB2DcOElfDxxXCvDH3XsgoIU216VY03MCaUZf7kZ2GiNL+UX\n" \
    //     "9UBd0Q==\n" \
    //     "-----END CERTIFICATE-----\n";
// #endif

    HTTPClient http;

    String url_iot = String(host) + "/api/v1/user/device/diy/auth?authKey=" + String(_authKey);

#if defined(BLINKER_ALIGENIE_LIGHT)
    url_iot += "&aliType=light";
#elif defined(BLINKER_ALIGENIE_OUTLET)
    url_iot += "&aliType=outlet";
#elif defined(BLINKER_ALIGENIE_SWITCH)
#elif defined(BLINKER_ALIGENIE_SENSOR)
    url_iot += "&aliType=sensor";
#endif

#ifdef BLINKER_DEBUG_ALL 
    BLINKER_LOG2("HTTPS begin: ", url_iot);
#endif

// #if defined(ESP8266)
//     http.begin(url_iot, fingerprint); //HTTP
// #elif defined(ESP32)
    // http.begin(url_iot, ca); TODO
    http.begin(url_iot);
// #endif
    int httpCode = http.GET();

    String payload;

    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
#ifdef BLINKER_DEBUG_ALL 
        BLINKER_LOG2("[HTTP] GET... code: ", httpCode);
#endif

        // file found at server
        if (httpCode == HTTP_CODE_OK) {
            payload = http.getString();
            // BLINKER_LOG1(payload);
        }
    }
    else {
#ifdef BLINKER_DEBUG_ALL 
        BLINKER_LOG2("[HTTP] GET... failed, error: ", http.errorToString(httpCode).c_str());
        payload = http.getString();
        BLINKER_LOG1(payload);
#endif
    }

    http.end();
#endif

#ifdef BLINKER_DEBUG_ALL
    BLINKER_LOG1("reply was:");
    BLINKER_LOG1("==============================");
    BLINKER_LOG1(payload);
    BLINKER_LOG1("==============================");
#endif

    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(payload);

    if (STRING_contains_string(payload, BLINKER_CMD_NOTFOUND) || !root.success() ||
        !STRING_contains_string(payload, BLINKER_CMD_IOTID)) {
        // while(1) {
            BLINKER_ERR_LOG1("Maybe you have put in the wrong AuthKey!");
            BLINKER_ERR_LOG1("Or maybe your request is too frequently!");
            BLINKER_ERR_LOG1("Or maybe your network is disconnected!");
            // ::delay(60000);

            return false;
        // }
    }

    // String _userID = STRING_find_string(payload, "deviceName", "\"", 4);
    // String _userName = STRING_find_string(payload, "iotId", "\"", 4);
    // String _key = STRING_find_string(payload, "iotToken", "\"", 4);
    // String _productInfo = STRING_find_string(payload, "productKey", "\"", 4);
    // String _broker = STRING_find_string(payload, "broker", "\"", 4);
    // String _uuid = STRING_find_string(payload, "uuid", "\"", 4);
    String _userID = root[BLINKER_CMD_DETAIL][BLINKER_CMD_DEVICENAME];
    String _userName = root[BLINKER_CMD_DETAIL][BLINKER_CMD_IOTID];
    String _key = root[BLINKER_CMD_DETAIL][BLINKER_CMD_IOTTOKEN];
    String _productInfo = root[BLINKER_CMD_DETAIL][BLINKER_CMD_PRODUCTKEY];
    String _broker = root[BLINKER_CMD_DETAIL][BLINKER_CMD_BROKER];
    String _uuid = root[BLINKER_CMD_DETAIL][BLINKER_CMD_UUID];

    if (_broker == BLINKER_MQTT_BORKER_ALIYUN) {
        // memcpy(DEVICE_NAME, _userID.c_str(), 12);
        strcpy(DEVICE_NAME, _userID.c_str());
        strcpy(MQTT_ID, _userID.c_str());
        strcpy(MQTT_NAME, _userName.c_str());
        strcpy(MQTT_KEY, _key.c_str());
        strcpy(MQTT_PRODUCTINFO, _productInfo.c_str());
        strcpy(MQTT_HOST, BLINKER_MQTT_ALIYUN_HOST);
        MQTT_PORT = BLINKER_MQTT_ALIYUN_PORT;
    }
    else if (_broker == BLINKER_MQTT_BORKER_QCLOUD) {
        // String id2name = _userID.subString(10, _userID.length());
        // memcpy(DEVICE_NAME, _userID.c_str(), 12);
        strcpy(DEVICE_NAME, _userID.c_str());
        String IDtest = _productInfo + _userID;
        strcpy(MQTT_ID, IDtest.c_str());
        String NAMEtest = IDtest + ";" + _userName;
        strcpy(MQTT_NAME, NAMEtest.c_str());
        strcpy(MQTT_KEY, _key.c_str());
        strcpy(MQTT_PRODUCTINFO, _productInfo.c_str());
        strcpy(MQTT_HOST, BLINKER_MQTT_QCLOUD_HOST);
        MQTT_PORT = BLINKER_MQTT_QCLOUD_PORT;
    }
    else if (_broker == BLINKER_MQTT_BORKER_ONENET) {
        // memcpy(DEVICE_NAME, _userID.c_str(), 12);
        strcpy(DEVICE_NAME, _userID.c_str());
        strcpy(MQTT_ID, _userName.c_str());
        strcpy(MQTT_NAME, _productInfo.c_str());
        strcpy(MQTT_KEY, _key.c_str());
        strcpy(MQTT_PRODUCTINFO, _productInfo.c_str());
        strcpy(MQTT_HOST, BLINKER_MQTT_ONENET_HOST);
        MQTT_PORT = BLINKER_MQTT_ONENET_PORT;
    }
    strcpy(UUID, _uuid.c_str());

#ifdef BLINKER_DEBUG_ALL
    BLINKER_LOG1("====================");
    BLINKER_LOG2("DEVICE_NAME: ", DEVICE_NAME);
    BLINKER_LOG2("MQTT_PRODUCTINFO: ", MQTT_PRODUCTINFO);
    BLINKER_LOG2("MQTT_ID: ", MQTT_ID);
    BLINKER_LOG2("MQTT_NAME: ", MQTT_NAME);
    BLINKER_LOG2("MQTT_KEY: ", MQTT_KEY);
    BLINKER_LOG2("MQTT_BROKER: ", _broker);
    BLINKER_LOG2("HOST: ", MQTT_HOST);
    BLINKER_LOG2("PORT: ", MQTT_PORT);
    BLINKER_LOG2("UUID: ", UUID);
    BLINKER_LOG1("====================");
#endif

    if (_broker == BLINKER_MQTT_BORKER_ALIYUN) {
        uint8_t str_len;
        String PUB_TOPIC_STR = "/" + String(MQTT_PRODUCTINFO) + "/" + String(MQTT_ID) + "/s";
        str_len = PUB_TOPIC_STR.length() + 1;
        BLINKER_PUB_TOPIC = (char*)malloc(str_len*sizeof(char));
        memcpy(BLINKER_PUB_TOPIC, PUB_TOPIC_STR.c_str(), str_len);
#ifdef BLINKER_DEBUG_ALL
        BLINKER_LOG2("BLINKER_PUB_TOPIC: ", BLINKER_PUB_TOPIC);
#endif
        String SUB_TOPIC_STR = "/" + String(MQTT_PRODUCTINFO) + "/" + String(MQTT_ID) + "/r";
        str_len = SUB_TOPIC_STR.length() + 1;
        BLINKER_SUB_TOPIC = (char*)malloc(str_len*sizeof(char));
        memcpy(BLINKER_SUB_TOPIC, SUB_TOPIC_STR.c_str(), str_len);
#ifdef BLINKER_DEBUG_ALL
        BLINKER_LOG2("BLINKER_SUB_TOPIC: ", BLINKER_SUB_TOPIC);
#endif
    }
    else if (_broker == BLINKER_MQTT_BORKER_QCLOUD) {
        uint8_t str_len;
        String PUB_TOPIC_STR = String(MQTT_PRODUCTINFO) + "/" + String(_userID) + "/s";
        str_len = PUB_TOPIC_STR.length() + 1;
        BLINKER_PUB_TOPIC = (char*)malloc(str_len*sizeof(char));
        memcpy(BLINKER_PUB_TOPIC, PUB_TOPIC_STR.c_str(), str_len);
#ifdef BLINKER_DEBUG_ALL
        BLINKER_LOG2("BLINKER_PUB_TOPIC: ", BLINKER_PUB_TOPIC);
#endif
        String SUB_TOPIC_STR = String(MQTT_PRODUCTINFO) + "/" + String(_userID) + "/r";
        str_len = SUB_TOPIC_STR.length() + 1;
        BLINKER_SUB_TOPIC = (char*)malloc(str_len*sizeof(char));
        memcpy(BLINKER_SUB_TOPIC, SUB_TOPIC_STR.c_str(), str_len);
#ifdef BLINKER_DEBUG_ALL
        BLINKER_LOG2("BLINKER_SUB_TOPIC: ", BLINKER_SUB_TOPIC);
#endif
    }
    else if (_broker == BLINKER_MQTT_BORKER_ONENET) {
        uint8_t str_len;
        String PUB_TOPIC_STR = String(MQTT_PRODUCTINFO) + "/onenet_rule/r";
        str_len = PUB_TOPIC_STR.length() + 1;
        BLINKER_PUB_TOPIC = (char*)malloc(str_len*sizeof(char));
        memcpy(BLINKER_PUB_TOPIC, PUB_TOPIC_STR.c_str(), str_len);
#ifdef BLINKER_DEBUG_ALL
        BLINKER_LOG2("BLINKER_PUB_TOPIC: ", BLINKER_PUB_TOPIC);
#endif
        String SUB_TOPIC_STR = String(MQTT_PRODUCTINFO) + "/" + String(_userID) + "/r";
        str_len = SUB_TOPIC_STR.length() + 1;
        BLINKER_SUB_TOPIC = (char*)malloc(str_len*sizeof(char));
        memcpy(BLINKER_SUB_TOPIC, SUB_TOPIC_STR.c_str(), str_len);
#ifdef BLINKER_DEBUG_ALL
        BLINKER_LOG2("BLINKER_SUB_TOPIC: ", BLINKER_SUB_TOPIC);
#endif
    }

    if (_broker == BLINKER_MQTT_BORKER_ALIYUN) {
        mqtt = new Adafruit_MQTT_Client(&client_s, MQTT_HOST, MQTT_PORT, MQTT_ID, MQTT_NAME, MQTT_KEY);
    }
    else if (_broker == BLINKER_MQTT_BORKER_QCLOUD) {
        mqtt = new Adafruit_MQTT_Client(&client_s, MQTT_HOST, MQTT_PORT, MQTT_ID, MQTT_NAME, MQTT_KEY);
    }
    else if (_broker == BLINKER_MQTT_BORKER_ONENET) {
        mqtt = new Adafruit_MQTT_Client(&client, MQTT_HOST, MQTT_PORT, MQTT_ID, MQTT_NAME, MQTT_KEY);
    }

    // iotPub = new Adafruit_MQTT_Publish(mqtt, BLINKER_PUB_TOPIC);
    iotSub = new Adafruit_MQTT_Subscribe(mqtt, BLINKER_SUB_TOPIC);

    mqtt_broker = _broker;

    // mDNSInit(MQTT_ID);
    this->latestTime = millis();
    mqtt->subscribe(iotSub);
    connect();

    return true;
}

bool BlinkerTransportStream::connect() {
    int8_t ret;

    webSocket.loop();

    if (mqtt->connected()) {
        return true;
    }

    disconnect();

    if ((millis() - latestTime) < 5000) {
        return false;
    }

// #ifdef BLINKER_DEBUG_ALL
    BLINKER_LOG1("Connecting to MQTT... ");
// #endif

    if ((ret = mqtt->connect()) != 0) {
        BLINKER_LOG1(mqtt->connectErrorString(ret));
        BLINKER_LOG1("Retrying MQTT connection in 5 seconds...");

        this->latestTime = millis();
        return false;
    }
// #ifdef BLINKER_DEBUG_ALL
    BLINKER_LOG1("MQTT Connected!");
// #endif

    this->latestTime = millis();

    return true;
}

void BlinkerTransportStream::ping() {
#ifdef BLINKER_DEBUG_ALL
    BLINKER_LOG1("MQTT Ping!");
#endif
    if (!mqtt->ping()) {
        disconnect();
        delay(100);

        connect();
    }
    else {
        this->latestTime = millis();
    }
}

void BlinkerTransportStream::subscribe() {
    Adafruit_MQTT_Subscribe *subscription;
    while ((subscription = mqtt->readSubscription(10))) {
        if (subscription == iotSub) {
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2(("Got: "), (char *)iotSub->lastread);
#endif
            // String dataGet = String((char *)iotSub->lastread);

            // DynamicJsonDocument doc;
            // deserializeJson(doc, String((char *)iotSub->lastread));
            // JsonObject& root = doc.as<JsonObject>();
            DynamicJsonBuffer jsonBuffer;
            JsonObject& root = jsonBuffer.parseObject(String((char *)iotSub->lastread));
	        // JsonObject& root = jsonBuffer.parseObject((char *)iotSub->lastread);

            // if (!root.success()) {
            //     BLINKER_LOG1("json test error");
            //     return;
            // }

            String _uuid = root["fromDevice"];
            String dataGet = root["data"];

            // String _uuid = STRING_find_string(dataGet, "fromDevice", "\"", 3);

            // dataGet = STRING_find_string(dataGet, BLINKER_CMD_DATA, ",\"fromDevice", 2);

            // if (dataGet.indexOf("\"") != -1 && dataGet.indexOf("\"") == 0) {
            //     dataGet = STRING_find_string(dataGet, "\"", "\"", 0);
            // }

            // BLINKER_LOG2("data: ", dataGet);
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2("data: ", dataGet);
            BLINKER_LOG2("fromDevice: ", _uuid);
#endif
            if (strcmp(_uuid.c_str(), UUID) == 0) {
#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG1("Authority uuid");
#endif
                kaTime = millis();
                isAvail = true;
                isAlive = true;
            }
            else if (_uuid == BLINKER_CMD_ALIGENIE) {
#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG1("form AliGenie");
#endif
                aliKaTime = millis();
                isAliAlive = true;
                isAliAvail = true;
            }
            else {
                dataGet = String((char *)iotSub->lastread);

#ifdef BLINKER_DEBUG_ALL
                BLINKER_ERR_LOG2("No authority uuid, check is from bridge/share device, data: ", dataGet);
#endif
                // return;

                isBavail = true;
            }

            memset(msgBuf, 0, BLINKER_MAX_READ_SIZE);
            memcpy(msgBuf, dataGet.c_str(), dataGet.length());
            
            this->latestTime = millis();

            dataFrom = BLINKER_MSG_FROM_MQTT;
        }
    }
}

bool BlinkerTransportStream::mqttPrint(String data) {
#ifdef BLINKER_DEBUG_ALL
    BLINKER_LOG2(BLINKER_F("mqttPrint data: "), data);
#endif
    DynamicJsonBuffer jsonBuffer;
    JsonObject& print_data = jsonBuffer.parseObject(data);

    if (!print_data.success()) {
        return false;
    }

    if (*isHandle && dataFrom == BLINKER_MSG_FROM_WS) {
        bool state = STRING_contains_string(data, BLINKER_CMD_NOTICE) ||
                    (STRING_contains_string(data, BLINKER_CMD_TIMING) && 
                     STRING_contains_string(data, BLINKER_CMD_ENABLE)) ||
                    (STRING_contains_string(data, BLINKER_CMD_LOOP) && 
                     STRING_contains_string(data, BLINKER_CMD_TIMES)) ||
                    (STRING_contains_string(data, BLINKER_CMD_COUNTDOWN) &&
                     STRING_contains_string(data, BLINKER_CMD_TOTALTIME));

        if (!state) {
            state = ((STRING_contains_string(data, BLINKER_CMD_STATE) 
                && STRING_contains_string(data, BLINKER_CMD_ONLINE))
                || (STRING_contains_string(data, BLINKER_CMD_BUILTIN_SWITCH)));

            if (!checkPrintSpan()) {
                respTime = millis();
                return false;
            }
            respTime = millis();
        }

        if (!state) {
            if (!checkPrintSpan()) {
                respTime = millis();
                return false;
            }
        }

        respTime = millis();

#ifdef BLINKER_DEBUG_ALL
        BLINKER_LOG1("WS response: ");
        BLINKER_LOG1(data);
        BLINKER_LOG1("Succese...");
        // BLINKER_LOG1(("Succese..."));
#endif
        webSocket.sendTXT(ws_num, data + BLINKER_CMD_NEWLINE);

        return true;
// #ifdef BLINKER_DEBUG_ALL
//         BLINKER_LOG3("WS response: ", data, "Succese...");
// #endif
    }
    else {
        String payload;
        if (STRING_contains_string(data, BLINKER_CMD_NEWLINE)) {
            // payload = "{\"data\":" + data.substring(0, data.length() - 1) + \
            //         ",\"fromDevice\":\"" + MQTT_ID + \
            //         "\",\"toDevice\":\"" + UUID + \
            //         "\",\"deviceType\":\"OwnApp\"}";

            payload = data.substring(0, data.length() - 1);
        }
        else {
        //     payload = "{\"data\":" + data + \
        //             ",\"fromDevice\":\"" + MQTT_ID + \
        //             "\",\"toDevice\":\"" + UUID + \
        //             "\",\"deviceType\":\"OwnApp\"}";

            payload = data;
        }
    
#ifdef BLINKER_DEBUG_ALL
        BLINKER_LOG1("MQTT Publish...");
#endif
        bool _alive = isAlive;
        bool state = STRING_contains_string(data, BLINKER_CMD_NOTICE) ||
                    (STRING_contains_string(data, BLINKER_CMD_TIMING) && 
                     STRING_contains_string(data, BLINKER_CMD_ENABLE)) ||
                    (STRING_contains_string(data, BLINKER_CMD_LOOP) && 
                     STRING_contains_string(data, BLINKER_CMD_TIMES)) ||
                    (STRING_contains_string(data, BLINKER_CMD_COUNTDOWN) &&
                     STRING_contains_string(data, BLINKER_CMD_TOTALTIME));

        if (!state) {
            state = ((STRING_contains_string(data, BLINKER_CMD_STATE) 
                && STRING_contains_string(data, BLINKER_CMD_ONLINE))
                || (STRING_contains_string(data, BLINKER_CMD_BUILTIN_SWITCH)));

            if (!checkPrintSpan()) {
                return false;
            }
            respTime = millis();
        }

// #ifdef BLINKER_DEBUG_ALL
//         BLINKER_LOG2("state: ", state);

//         BLINKER_LOG2("state: ", STRING_contains_string(data, BLINKER_CMD_TIMING));

//         BLINKER_LOG2("state: ", data.indexOf(BLINKER_CMD_TIMING));

//         BLINKER_LOG2("data: ", data);
// #endif

        if (mqtt->connected()) {
            if (!state) {
                if (!checkCanPrint()) {
                    if (!_alive) {
                        isAlive = false;
                    }
                    return false;
                }
            }

            // Adafruit_MQTT_Publish iotPub = Adafruit_MQTT_Publish(mqtt, BLINKER_PUB_TOPIC);

            // if (!iotPub.publish(payload.c_str())) {

            if (! mqtt->publish(BLINKER_PUB_TOPIC, payload.c_str())) {
#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG1(payload);
                BLINKER_LOG1("...Failed");
#endif
                if (!_alive) {
                    isAlive = false;
                }
                return false;
            }
//             else if (mqtt_broker == BLINKER_MQTT_BORKER_ONENET) {
//                 char buf[BLINKER_MAX_SEND_BUFFER_SIZE];
//                 buf[0] = 0x01;
//                 buf[1] = 0x00;
//                 buf[2] = (uint8_t)payload.length();

//                 memcpy(buf+3, payload.c_str(), payload.length());

//                 if (!iotPub.publish((uint8_t *)buf, payload.length()+3)) {
// #ifdef BLINKER_DEBUG_ALL
//                     BLINKER_LOG1(payload);
//                     BLINKER_LOG1("...Failed");
// #endif
//                     if (!_alive) {
//                         isAlive = false;
//                     }
//                     return false;
//                 } else {
// #ifdef BLINKER_DEBUG_ALL
//                     BLINKER_LOG1(payload);
//                     BLINKER_LOG1("...OK!");
// #endif
//                     if (!state) printTime = millis();

//                     if (!_alive) {
//                         isAlive = false;
//                     }
//                     return true;
//                 }
//             }
            else {
#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG1(payload);
                BLINKER_LOG1("...OK!");
#endif
                if (!state) printTime = millis();

                if (!_alive) {
                    isAlive = false;
                }
                return true;
            }            
        }
        else {
            BLINKER_ERR_LOG1("MQTT Disconnected");
            isAlive = false;
            return false;
        }
    }
}

bool BlinkerTransportStream::print(String data) {
    if (*isHandle && dataFrom == BLINKER_MSG_FROM_WS) {
        bool state = STRING_contains_string(data, BLINKER_CMD_NOTICE) ||
                    (STRING_contains_string(data, BLINKER_CMD_TIMING) && 
                     STRING_contains_string(data, BLINKER_CMD_ENABLE)) ||
                    (STRING_contains_string(data, BLINKER_CMD_LOOP) && 
                     STRING_contains_string(data, BLINKER_CMD_TIMES)) ||
                    (STRING_contains_string(data, BLINKER_CMD_COUNTDOWN) &&
                     STRING_contains_string(data, BLINKER_CMD_TOTALTIME));

        if (!state) {
            state = ((STRING_contains_string(data, BLINKER_CMD_STATE) 
                && STRING_contains_string(data, BLINKER_CMD_ONLINE))
                || (STRING_contains_string(data, BLINKER_CMD_BUILTIN_SWITCH)));

            if (!checkPrintSpan()) {
                respTime = millis();
                return false;
            }
            respTime = millis();
        }

        if (!state) {
            if (!checkPrintSpan()) {
                respTime = millis();
                return false;
            }
        }

        respTime = millis();

#ifdef BLINKER_DEBUG_ALL
        BLINKER_LOG1("WS response: ");
        BLINKER_LOG1(data);
        BLINKER_LOG1("Succese...");
        // BLINKER_LOG1(("Succese..."));
#endif
        webSocket.sendTXT(ws_num, data + BLINKER_CMD_NEWLINE);

        return true;
// #ifdef BLINKER_DEBUG_ALL
//         BLINKER_LOG3("WS response: ", data, "Succese...");
// #endif
    }
    else {
        String payload;
        if (STRING_contains_string(data, BLINKER_CMD_NEWLINE)) {
            payload = "{\"data\":" + data.substring(0, data.length() - 1) + \
                    ",\"fromDevice\":\"" + MQTT_ID + \
                    "\",\"toDevice\":\"" + UUID + \
                    "\",\"deviceType\":\"OwnApp\"}";
        }
        else {
            payload = "{\"data\":" + data + \
                    ",\"fromDevice\":\"" + MQTT_ID + \
                    "\",\"toDevice\":\"" + UUID + \
                    "\",\"deviceType\":\"OwnApp\"}";
        }
    
#ifdef BLINKER_DEBUG_ALL
        BLINKER_LOG1("MQTT Publish...");
#endif
        bool _alive = isAlive;
        bool state = STRING_contains_string(data, BLINKER_CMD_NOTICE) ||
                    (STRING_contains_string(data, BLINKER_CMD_TIMING) && 
                     STRING_contains_string(data, BLINKER_CMD_ENABLE)) ||
                    (STRING_contains_string(data, BLINKER_CMD_LOOP) && 
                     STRING_contains_string(data, BLINKER_CMD_TIMES)) ||
                    (STRING_contains_string(data, BLINKER_CMD_COUNTDOWN) &&
                     STRING_contains_string(data, BLINKER_CMD_TOTALTIME));

        if (!state) {
            state = ((STRING_contains_string(data, BLINKER_CMD_STATE) 
                && STRING_contains_string(data, BLINKER_CMD_ONLINE))
                || (STRING_contains_string(data, BLINKER_CMD_BUILTIN_SWITCH)));

            if (!checkPrintSpan()) {
                return false;
            }
            respTime = millis();
        }

// #ifdef BLINKER_DEBUG_ALL
//         BLINKER_LOG2("state: ", state);

//         BLINKER_LOG2("state: ", STRING_contains_string(data, BLINKER_CMD_TIMING));

//         BLINKER_LOG2("state: ", data.indexOf(BLINKER_CMD_TIMING));

//         BLINKER_LOG2("data: ", data);
// #endif

        if (mqtt->connected()) {
            if (!state) {
                if (!checkCanPrint()) {
                    if (!_alive) {
                        isAlive = false;
                    }
                    return false;
                }
            }

            // Adafruit_MQTT_Publish iotPub = Adafruit_MQTT_Publish(mqtt, BLINKER_PUB_TOPIC);

            // if (!iotPub.publish(payload.c_str())) {

            if (! mqtt->publish(BLINKER_PUB_TOPIC, payload.c_str())) {
#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG1(payload);
                BLINKER_LOG1("...Failed");
#endif
                if (!_alive) {
                    isAlive = false;
                }
                return false;
            }
//             else if (mqtt_broker == BLINKER_MQTT_BORKER_ONENET) {
//                 char buf[BLINKER_MAX_SEND_BUFFER_SIZE];
//                 buf[0] = 0x01;
//                 buf[1] = 0x00;
//                 buf[2] = (uint8_t)payload.length();

//                 memcpy(buf+3, payload.c_str(), payload.length());

//                 if (!iotPub.publish((uint8_t *)buf, payload.length()+3)) {
// #ifdef BLINKER_DEBUG_ALL
//                     BLINKER_LOG1(payload);
//                     BLINKER_LOG1("...Failed");
// #endif
//                     if (!_alive) {
//                         isAlive = false;
//                     }
//                     return false;
//                 } else {
// #ifdef BLINKER_DEBUG_ALL
//                     BLINKER_LOG1(payload);
//                     BLINKER_LOG1("...OK!");
// #endif
//                     if (!state) printTime = millis();

//                     if (!_alive) {
//                         isAlive = false;
//                     }
//                     return true;
//                 }
//             }
            else {
#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG1(payload);
                BLINKER_LOG1("...OK!");
#endif
                if (!state) printTime = millis();

                if (!_alive) {
                    isAlive = false;
                }
                return true;
            }            
        }
        else {
            BLINKER_ERR_LOG1("MQTT Disconnected");
            isAlive = false;
            return false;
        }
    }
}

bool BlinkerTransportStream::bPrint(String name, String data) {
    String payload;
    if (STRING_contains_string(data, BLINKER_CMD_NEWLINE)) {
        payload = "{\"data\":" + data.substring(0, data.length() - 1) + \
                ",\"fromDevice\":\"" + MQTT_ID + \
                "\",\"toDevice\":\"" + name + \
                "\",\"deviceType\":\"DiyBridge\"}";
    }
    else {
        payload = "{\"data\":" + data + \
                ",\"fromDevice\":\"" + MQTT_ID + \
                "\",\"toDevice\":\"" + name + \
                "\",\"deviceType\":\"DiyBridge\"}";
    }

#ifdef BLINKER_DEBUG_ALL
    BLINKER_LOG1("MQTT Publish...");
#endif
    // bool _alive = isAlive;
    // bool state = STRING_contains_string(data, BLINKER_CMD_NOTICE);

    // if (!state) {
    //     state = (STRING_contains_string(data, BLINKER_CMD_STATE) 
    //         && STRING_contains_string(data, BLINKER_CMD_ONLINE));
    // }

    if (mqtt->connected()) {
        // if (!state) {
        if (!checkCanBprint()) {
            // if (!_alive) {
            //     isAlive = false;
            // }
            return false;
        }
        // }

        // Adafruit_MQTT_Publish iotPub = Adafruit_MQTT_Publish(mqtt, BLINKER_PUB_TOPIC);

        // if (! iotPub.publish(payload.c_str())) {

        String bPubTopic = "";

        if (mqtt_broker == BLINKER_MQTT_BORKER_ONENET) {
            bPubTopic = STRING_format(MQTT_PRODUCTINFO) + "/" + name + "/r";
        }
        else {
            bPubTopic = STRING_format(BLINKER_PUB_TOPIC);
        }

        if (! mqtt->publish(bPubTopic.c_str(), payload.c_str())) {
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG1(payload);
            BLINKER_LOG1("...Failed");
#endif
            // if (!_alive) {
            //     isAlive = false;
            // }
            return false;
        }
        else {
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG1(payload);
            BLINKER_LOG1("...OK!");
#endif
            bPrintTime = millis();

            // if (!_alive) {
            //     isAlive = false;
            // }
            return true;
        }            
    }
    else {
        BLINKER_ERR_LOG1("MQTT Disconnected");
        // isAlive = false;
        return false;
    }
    // }
}

#if defined(BLINKER_ALIGENIE)
bool BlinkerTransportStream::aliPrint(String data)
{
    String payload;

    payload = "{\"data\":" + data + \
            ",\"fromDevice\":\"" + MQTT_ID + \
            "\",\"toDevice\":\"AliGenie_r\",\"deviceType\":\"vAssistant\"}";
    
#ifdef BLINKER_DEBUG_ALL
    BLINKER_LOG1("MQTT AliGenie Publish...");
#endif

    if (mqtt->connected()) {
        if (!checkAliKA()) {
            return false;
        }

        if (!checkAliPrintSpan()) {
            respAliTime = millis();
            return false;
        }
        respAliTime = millis();

        // Adafruit_MQTT_Publish iotPub = Adafruit_MQTT_Publish(mqtt, BLINKER_PUB_TOPIC);

        // if (! iotPub.publish(payload.c_str())) {

        if (! mqtt->publish(BLINKER_PUB_TOPIC, payload.c_str())) {
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG1(payload);
            BLINKER_LOG1("...Failed");
#endif
            isAliAlive = false;
            return false;
        }
        else {
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG1(payload);
            BLINKER_LOG1("...OK!");
#endif
            isAliAlive = false;
            return true;
        }      
    }
    else {
        BLINKER_ERR_LOG1("MQTT Disconnected");
        return false;
    }
}
#endif

class BlinkerMQTT_AT
    : public BlinkerProtocol<BlinkerTransportStream>
{
    typedef BlinkerProtocol<BlinkerTransportStream> Base;

    public :
        BlinkerMQTT_AT(BlinkerTransportStream& transp)
            : Base(transp)
        {}

        void begin(uint8_t ss_rx_pin = RX,
                    uint8_t ss_tx_pin = TX,
                    uint32_t ss_baud = 115200)
        {
            if (ss_rx_pin == RX && ss_tx_pin == TX){
                Base::begin();

                ::delay(100);

                Serial.begin(ss_baud);
                this->conn.serialBegin(Serial, true);
                strcpy(Base::_authKey, this->conn.authKey().c_str());
                strcpy(Base::_deviceName, this->conn.deviceName().c_str());
                BLINKER_LOG1(BLINKER_F("BLINKER_MQTT_AT initialized..."));
                return;
            }

            Base::begin();
            SSerialBLE = new SoftwareSerial(ss_rx_pin, ss_tx_pin);
            SSerialBLE->begin(ss_baud);
            this->conn.serialBegin(*SSerialBLE, false);
            strcpy(Base::_authKey, this->conn.authKey().c_str());
            strcpy(Base::_deviceName, this->conn.deviceName().c_str());
            BLINKER_LOG1(BLINKER_F("BLINKER_MQTT_AT initialized..."));
        }

    protected :
        // uint8_t at_rx_pin = RX;
        // uint8_t at_tx_pin = TX;
        // uint32_t at_baud = 9600;
};

#endif