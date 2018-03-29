
#include <Arduino.h>

#include <WiFi.h>

// Enable MqttClient logs
#define MQTT_LOG_ENABLED 0

// Include library
#include <MqttClient.h>

#define LOG_PRINTFLN(fmt, ...) logfln(fmt, ##__VA_ARGS__)
#define LOG_SIZE_MAX 128
void logfln(const char *fmt, ...)
{
    char buf[LOG_SIZE_MAX];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, LOG_SIZE_MAX, fmt, ap);
    va_end(ap);
    Serial.println(buf);
}

char MQTT_TOPIC_STATUS[] = "mohen/sensor";
char MQTT_TOPIC_SUB[] = "mohen/sensor";

char WIFI_SSID[32] = "tgoffice";
char WIFI_PAK[32] = "tu9u2017";
char MQTT_IP[16] = "192.168.2.150";
uint16_t MQTT_PORT = 1883;

class MohenMqtt : public WiFiClient, public MqttClient::System
{
  public:

    String Mac;

    bool NetworkState = false;

    MohenMqtt()
    {
        Mac = String((uint32_t)ESP.getEfuseMac(), 16);
    }

  private:

    uint8_t ReConnectCount = 0, ReWifiCount = 0;

    unsigned long millis() const
    {
        return ::millis();
    }

    void yield(void)
    {
        ::yield();
    }

    MqttClient::Logger *mqttLogger;
    MqttClient::Network *mqttNetwork;
    MqttClient::Buffer *mqttSendBuffer;
    MqttClient::Buffer *mqttRecvBuffer;
    MqttClient::MessageHandlers *mqttMessageHandlers;
    MqttClient *mqtt;

  public:

    bool CheckWiFiConnect()
    {
        if(0 != ReConnectCount)
        {
            if(WiFi.isConnected())
            {
                // ReConnectCount = 0xFF;
                return true;
            }
            else
            {
                WiFi.reconnect();
                ReConnectCount--;
            }
        }
        else
        {
            if(0 != ReWifiCount)
            {
                if(WiFi.isConnected())
                {
                    LOG_PRINTFLN("Connected to WiFi");
                    LOG_PRINTFLN("IP: %s", WiFi.localIP().toString().c_str());
                    ReConnectCount = 60;
                    return true;
                }
                else
                {
                    ReWifiCount--;
                }
            }
            else
            {
                // WiFi.persistent(true);
                // WiFi.mode(WIFI_STA);
                // WiFi.disconnect();
                // WiFi.scanNetworks will return the number of networks found
                int n = WiFi.scanNetworks();
                Serial.println("scan done");
                if (n == 0)
                {
                    Serial.println("no networks found");
                }
                else
                {
                    Serial.print(n);
                    Serial.println(" networks found");
                    for (int i = 0; i < n; ++i)
                    {
                        // if(WiFi.SSID(i) == WIFI_SSID && (WiFi.encryptionType(i) != WIFI_AUTH_OPEN))
                        if(WiFi.SSID(i) == WIFI_SSID)
                        {
                            // WiFi.setHostname(("ESP32_" + String((long)ESP.getEfuseMac(), HEX)).c_str());
                            WiFi.begin(WIFI_SSID, WIFI_PAK);
                            WiFi.printDiag(Serial);
                            LOG_PRINTFLN("Wait Connecting to WiFi");
                            ReWifiCount = 60;
                        }
                    }
                }
            }
        }
        delay(500);
        return false;
    }

    bool CheckWiFiClientConnect()
    {
        if (WiFiClient::connected())
        {
            return true;
        }
        // 撤销上一次遗留操作进行重连
        if (mqtt->isConnected())
        {
            LOG_PRINTFLN("mqtt->isConnected");
            mqtt->disconnect();
            // WiFiClient::stop();
        }
        LOG_PRINTFLN("WiFiClient::connect");
        if(1 == WiFiClient::connect(MQTT_IP, MQTT_PORT)) // 1 连接可用 0 不可
        {
            delay(500);
            return true;
        }
        return false;
    }

    bool CheckMqttClientConnect()
    {
        if (mqtt->isConnected())
        {
            return true;
        }
        MqttClient::ConnectResult connectResult;
        MQTTPacket_connectData options = MQTTPacket_connectData_initializer;
        options.MQTTVersion = 3;
        options.clientID.cstring = (char *)(Mac.c_str());
        options.cleansession = true;
        options.keepAliveInterval = 15; // 15 seconds
        // Setup LWT
        // options.willFlag = true;
        // options.will.topicName.cstring = MQTT_TOPIC_STATUS;
        // options.will.message.cstring = (char *)(String(Mac, HEX) + " disconnected").c_str();
        // options.will.retained = true;
        // options.will.qos = MqttClient::QOS1;
        MqttClient::Error::type rc = mqtt->connect(options, connectResult);
        LOG_PRINTFLN("check_MqttClient_connect result: %i", rc);
        delay(500);
        return (rc == MqttClient::Error::SUCCESS); // 1 为无错 0 为出错
    }


    void yield(uint32_t timeout)
    {
        mqtt->yield(timeout);
    }

    bool SetupMqtt()
    {
        // Setup MqttClient
        mqttLogger = new MqttClient::LoggerImpl<HardwareSerial>(Serial);
        if (NULL == mqttLogger)
            return false;
        mqttNetwork = new MqttClient::NetworkClientImpl<WiFiClient>(*this, *this);
        if (NULL == mqttNetwork)
            return false;
        mqttSendBuffer = new MqttClient::ArrayBuffer<1024>();
        if (NULL == mqttSendBuffer)
            return false;
        mqttRecvBuffer = new MqttClient::ArrayBuffer<1024>();
        if (NULL == mqttRecvBuffer)
            return false;
        mqttMessageHandlers = new MqttClient::MessageHandlersImpl<2>();
        if (NULL == mqttMessageHandlers)
            return false;
        // Configure client options
        MqttClient::Options mqttOptions;
        // Set command timeout to 10 seconds
        mqttOptions.commandTimeoutMs = 5000;
        // Make client object
        mqtt = new MqttClient(
            mqttOptions, *mqttLogger, *this, *mqttNetwork, *mqttSendBuffer,
            *mqttRecvBuffer, *mqttMessageHandlers);
        if (NULL == mqtt)
            return false;
        return true;
    }

    static void ProcessMessage(MqttClient::MessageData &md)
    {
        const MqttClient::Message &msg = md.message;
        char payload[msg.payloadLen + 1];
        memcpy(payload, msg.payload, msg.payloadLen);
        payload[msg.payloadLen] = '\0';
        LOG_PRINTFLN("Message arrived: qos %d, retained %d, dup %d, packetid %d, payload:[%s]", msg.qos, msg.retained, msg.dup, msg.id, payload);
    }

    bool Subscribe(const char *topic)
    {
        MqttClient::Error::type rc = mqtt->subscribe(topic, MqttClient::QOS1, ProcessMessage);
        if (rc != MqttClient::Error::SUCCESS)
        {
            LOG_PRINTFLN("Subscribe error: %i", rc);
            LOG_PRINTFLN("Drop connection");
            mqtt->disconnect();
            return false;
        }
        return true;
    }

    ~MohenMqtt()
    {
        if (NULL != mqttLogger)
            delete mqttLogger, mqttLogger = NULL;
        if (NULL != mqttNetwork)
            delete mqttNetwork, mqttNetwork = NULL;
        if (NULL != mqttSendBuffer)
            delete mqttSendBuffer, mqttSendBuffer = NULL;
        if (NULL != mqttRecvBuffer)
            delete mqttRecvBuffer, mqttRecvBuffer = NULL;
        if (NULL != mqttMessageHandlers)
            delete mqttMessageHandlers, mqttMessageHandlers = NULL;
        if (NULL != mqtt)
            delete mqtt, mqtt = NULL;
    }

    void YieldTask(uint32_t timeout = 500L)
    {
        if (NetworkState)
        {
            yield(timeout);
        }
        else
        {
            delay(timeout);
        }
    }

    bool CheckConnect()
    {
        return NetworkState;
    }

    bool KeepConnect()
    {
        LOG_PRINTFLN("CheckWiFiConnect");
        if (CheckWiFiConnect())
        {
            LOG_PRINTFLN("CheckWiFiClientConnect");
            if (CheckWiFiClientConnect())
            {
                LOG_PRINTFLN("CheckMqttClientConnect");
                if (CheckMqttClientConnect())
                {
                    NetworkState = true;
                    delay(500);
                    return true;
                }
                else
                {
                    LOG_PRINTFLN("MqttClient Connect error");
                }
            }
            else
            {
                LOG_PRINTFLN("WiFiClient Connect error");
            }
        }
        else
        {
            LOG_PRINTFLN("WiFi error");
        }
        NetworkState = false;
        return false;
    }

    bool SendData(uint8_t Pack[], int Len)
    {
        LOG_PRINTFLN("SendData");
        if (NetworkState)
        {
            // Publish `connected` message
            MqttClient::Message message;
            message.qos = MqttClient::QOS1;
            message.retained = true;
            message.dup = false;
            message.payload = (void *)Pack;
            message.payloadLen = Len;
            MqttClient::Error::type rc = mqtt->publish(MQTT_TOPIC_STATUS, message);
            if (rc == MqttClient::Error::SUCCESS)
            {
                LOG_PRINTFLN("Publish Success");
                Serial.println((const char *)Pack);
                return true;
            }
            else
            {
                LOG_PRINTFLN("Mqtt Publish error");
            }
        }
        else
        {
            LOG_PRINTFLN("CheckConnect error");
        }
        return false;
    }
};
