
#include "freertos/FreeRTOS.h"

#include "freertos/task.h"

#include "Arduino.h"

#include "sensor.hpp"

#include "wifi_mqtt.hpp"

#include "StreamString.h"

#include "JsonWriter.h"

#include "JhLora.hpp"

class MohenCollect : public Sensor
{
    MohenMqtt route_wifi;

    // portMUX_TYPE MohenDataMux = portMUX_INITIALIZER_UNLOCKED;

    struct cache
    {
        uint16_t pm1_0, pm2_5, pm10_0;
        uint16_t ch20_ppb, ch20_fullrange, ch20_ug_m3;
        uint16_t co2_ppm;
        float Temperatura, Humidity;

        cache()
        {
            reset();
        }
        void reset()
        {
            memset(this, 0, sizeof(*this));
        }
        template <class Type>
        void fixed(Type & old, Type & now, uint16_t & flag, uint16_t value)
        {
            if (now != old)
            {
                old = now;
                flag |= value;
            }
        }
    } Data;

    uint16_t DataExistFlag = 0;

    void OnDefCH20(uint16_t ppb, uint16_t fullrange)
    {
        Serial.printf("ZE08CH20_event ppb:%hd fullrange:%hd \n", ppb, fullrange);
        Data.fixed<uint16_t>(Data.ch20_ppb, ppb, DataExistFlag, 0x0001);
        Data.fixed<uint16_t>(Data.ch20_fullrange, fullrange, DataExistFlag, 0x0001);
        // Data.ch20_ppb = ppb;
        // Data.ch20_fullrange = fullrange;
        // DataExistFlag |= 0x1;
        // MohenData.fixed<uint16_t>(MohenData.ch20_ppb, ppb);
        // MohenData.fixed<uint16_t>(MohenData.ch20_fullrange, fullrange);
    }

    void OnAskCH20(uint16_t ppb, uint16_t ug_m3)
    {
        Serial.printf("ZE08CH20_event ppb:%hd ug/m3:%hd \n", ppb, ug_m3);
        Data.fixed<uint16_t>(Data.ch20_ppb, ppb, DataExistFlag, 0x0002);
        Data.fixed<uint16_t>(Data.ch20_ug_m3, ug_m3, DataExistFlag, 0x0002);
        // Data.ch20_ppb = ppb;
        // Data.ch20_ug_m3 = ug_m3;
        // DataExistFlag |= 0x2;
        // MohenData.fixed<uint16_t>(MohenData.ch20_ppb, ppb);
        // MohenData.fixed<uint16_t>(MohenData.ch20_ug_m3, ug_m3);
    }

    void OnAskCO2(uint16_t ppm)
    {
        Serial.printf("MHZ14A_event ppm:%hd \n", ppm);
        Data.fixed<uint16_t>(Data.co2_ppm, ppm, DataExistFlag, 0x0004);
        // Data.co2_ppm = ppm;
        // DataExistFlag |= 0x4;
        // MohenData.fixed<uint16_t>(MohenData.co2_ppm, ppm);
    }

    void OnPM(uint16_t pm1_0, uint16_t pm2_5, uint16_t pm10_0)
    {
        Serial.printf("ZH03_event pm1_0:%hd pm2_5:%hd pm10_0:%hd \n", pm1_0, pm2_5, pm10_0);

        Data.fixed<uint16_t>(Data.pm1_0, pm1_0, DataExistFlag, 0x0008);
        Data.fixed<uint16_t>(Data.pm2_5, pm2_5, DataExistFlag, 0x0008);
        Data.fixed<uint16_t>(Data.pm10_0, pm10_0, DataExistFlag, 0x0008);
        // Data.pm1_0 = pm1_0;
        // Data.pm2_5 = pm2_5;
        // Data.pm10_0 = pm10_0;
        // DataExistFlag |= 0x8;
    }

    void OnDHT12(float Temperatura, float Humidity)
    {
        if(Humidity > 0 && Humidity > 0)
        {
            Serial.printf("DHT12 Temperatura:%f Humidity:%f \n", Temperatura, Humidity);
            Data.fixed<float>(Data.Humidity, Humidity, DataExistFlag, 0x0010);
            Data.fixed<float>(Data.Temperatura, Temperatura, DataExistFlag, 0x0010);
        }
    }

    uint16_t GetJson(StreamString &Pack)
    {
        if (0 != DataExistFlag)
        {
            JsonWriter json(&Pack);
            json.beginObject();
            // json.property("DevId", route_wifi.Mac);
            json.property("LoRaFlag", false);
            if (DataExistFlag & 0x1)
            {
                json.property("CH20_PPB", Data.ch20_ppb);
                json.property("CH20_FullRange", Data.ch20_fullrange);
            }
            if (DataExistFlag & 0x2)
            {
                json.property("CH20_PPB", Data.ch20_ppb);
                json.property("CH20_Ug_M3", Data.ch20_ug_m3);
            }
            if (DataExistFlag & 0x4)
            {
                json.property("CO2_PPM", Data.co2_ppm);
            }
            if (DataExistFlag & 0x8)
            {
                json.property("PM1.0", Data.pm1_0);
                json.property("PM2.5", Data.pm2_5);
                json.property("PM10.0", Data.pm10_0);
            }
            if (DataExistFlag & 0x10)
            {
                json.property("Humidity", Data.Humidity);
                json.property("Temperatura", Data.Temperatura);
            }
            json.endObject();
            uint16_t bak = DataExistFlag;
            DataExistFlag = 0;
            return bak;
        }
        return 0;
    }

  public:

    void test()
    {
        OnPM(rand(), rand(), rand());
        OnDHT12(rand(), rand());
    }

    void setup()
    {
        Sensor::start();
        LOG_PRINTFLN("SetupMqtt"), route_wifi.SetupMqtt();
        LoraDeviceSetup();
    }

    void send_event()
    {
        StreamString JsonPack;

        uint16_t DataFlag = GetJson(JsonPack);
        if (0 != DataFlag)
        {
            //portENTER_CRITICAL_ISR(&MohenDataMux);
            if (true == route_wifi.SendData((uint8_t *)JsonPack.c_str(), JsonPack.length()))
            {
                return ;
            }
            // portEXIT_CRITICAL_ISR(&MohenDataMux);

            // Serial.println("route_wifi.SendData fail So To Lora");

            struct lora_data
            {
                uint32_t Mac;
                uint16_t Flag;
                cache Data;
            } loraData = { route_wifi.Mac.toInt(), DataFlag, Data };

            if (false == LoraDevice.Send((uint8_t *)&loraData, sizeof(loraData)))
            {
                  Serial.println("route_lora.SendData fail");
            }

            // route_lora.sx1278_Sleep();
        }
    }

    void recv_event(uint32_t timeout = 200L)
    {
        route_wifi.YieldTask(timeout);
    }

    void poll()
    {
        if(route_wifi.KeepConnect())
        {
            delay(5000);
        }
    }
};

MohenCollect MohenData;

TaskHandle_t KeepTheLine = NULL;

void KeepTheLineTask(void * param)
{
    while (true)
    {
        MohenData.data_event();
        MohenData.send_event();
        // MohenData.test();
        MohenData.recv_event(100);
        static uint32_t last = ESP.getCycleCount();
        uint32_t now = ESP.getCycleCount();
        if(now - last > 600000000)
        {
            MohenData.DHT12_request();
            last = now;
        }
    }
}

void ConfigTask(TaskHandle_t & xTaskHandle, int ID = 2)
{
    xTaskCreate(KeepTheLineTask, "KeepTheLineTask", 4096, NULL, ID, &xTaskHandle);
    configASSERT(xTaskHandle);
    // if( xHandle != NULL )
    // {
    //   vTaskDelete( xHandle );
    // }
}

hw_timer_t * SensorOn = NULL;

void SensorOnTimer(void)
{
    MohenData.MHZ14A_request();
}

void ConfigTimer(hw_timer_t * timer, bool flag)
{
    if (flag)
    {
        timer = timerBegin(0, 80, true);
        if (NULL != timer)
        {
            // attach OnTimer functiOn to our timer.
            timerAttachInterrupt(timer, &SensorOnTimer, true);
            // set alarm to call OnTimer functiOn every secOnd (value in microsecOnds).
            // repeat the alarm (third parameter)
            timerAlarmWrite(timer, 2000000, true);
            // start an alarm
            timerAlarmEnable(timer);
        }
    }
    else
    {
        if (NULL != timer)
        {
            // stop and free timer
            timerEnd(timer);
            timer = NULL;
        }
    }
}

void setup()
{
    Serial.begin(115200);
    MohenData.setup();
    ConfigTask(KeepTheLine, 10);
    ConfigTimer(SensorOn, true);
}

void loop()
{
    MohenData.poll();
}
