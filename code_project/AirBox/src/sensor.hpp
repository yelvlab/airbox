
#include <Arduino.h>

#include <SoftwareSerial.h>

// #include <WEMOS_DHT12.h>

#include <DHT.H>

extern "C" {
#include <SensorWinsen.h>
}

class Sensor
{
    void ZH03_event()
    {
        if (ZH03.available() > 0)
        {
            if (ZH03_DefaultHeader == ZH03.peek())
            {
                if (ZH03.available() >= ZH03_DefaultLen)
                {
                    static uint8_t Pack[ZH03_DefaultLen];
                    for (uint8_t i = 0; i != ZH03_DefaultLen; i++)
                    {
                        Pack[i] = ZH03.read();
                    }
                    uint16_t pm1_0 = 0, pm2_5 = 0, pm10_0 = 0;
                    if (Default == ZH03Parse(Pack, sizeof(Pack), &pm1_0, &pm2_5, &pm10_0))
                    {
                        this->OnPM(pm1_0, pm2_5, pm10_0);
                    }
                    else
                    {
                        Serial.println("ZH03 Default Pack error");
                    }
                }
                else
                {
                    // Serial.println("not ready");
                }
            }
            else if (AskHeader == ZH03.peek())
            {
                if (ZH03.available() >= AskLen)
                {
                    static uint8_t Pack[AskLen];
                    for (uint8_t i = 0; i != AskLen; i++)
                    {
                        Pack[i] = ZH03.read();
                    }
                    uint16_t pm1_0 = 0, pm2_5 = 0, pm10_0 = 0;
                    if (Ask == ZH03Parse(Pack, sizeof(Pack), &pm1_0, &pm2_5, &pm10_0))
                    {
                        this->OnPM(pm1_0, pm2_5, pm10_0);
                    }
                    else
                    {
                        Serial.println("ZH03 Ask Pack error");
                    }
                }
                else
                {
                    // Serial.println("not ready");
                }
            }
            else
            {
                Serial.println("ZH03 tran error");
                ZH03.read(); // throw data
            }

            if (true == ZH03.overflow())
            {
                Serial.println("ZH03 overflow error");
                ZH03.flush();
            }
        }
    }

    void ZE08CH20_event()
    {
        if (ZE08CH20.available() > 0)
        {
            if (AskHeader == ZE08CH20.peek())
            {
                if (ZE08CH20.available() >= AskLen)
                {
                    uint8_t Pack[AskLen];
                    for (uint8_t i = 0; i != AskLen; i++)
                    {
                        Pack[i] = ZE08CH20.read();
                    }
                    uint16_t ppb = 0, fullrangeorugm3 = 0;
                    uint16_t result = ZE08CH20Parse(Pack, sizeof(Pack), &ppb, &fullrangeorugm3);
                    if (Default == result)
                    {
                        this->OnDefCH20(ppb, fullrangeorugm3);
                    }
                    else if (Ask == result)
                    {
                        this->OnAskCH20(ppb, fullrangeorugm3);
                    }
                    else
                    {
                        Serial.println("ZE08CH20 Pack error");
                    }
                }
                else
                {
                    // Serial.println("not ready");
                }
            }
            else
            {
                Serial.println("ZE08CH20 tran error");
                ZE08CH20.read(); // throw data
            }

            if (true == ZE08CH20.overflow())
            {
                Serial.println("ZE08CH20 overflow error");
                ZE08CH20.flush();
            }
        }
    }

    void MHZ14A_event()
    {
        if (MHZ14A.available() > 0)
        {
            if (AskHeader == MHZ14A.peek())
            {
                if (MHZ14A.available() >= AskLen)
                {
                    uint8_t Pack[AskLen];
                    for (uint8_t i = 0; i != AskLen; i++)
                    {
                        Pack[i] = MHZ14A.read();
                    }
                    uint16_t ppm = 0;
                    if (Ask == MHZ14AParse(Pack, sizeof(Pack), &ppm))
                    {
                        this->OnAskCO2(ppm);
                    }
                    else
                    {
                        Serial.println("MHZ14A Pack error");
                    }
                }
                else
                {
                    // Serial.println("not ready");
                }
            }
            else
            {
                Serial.println("MHZ14A tran error");
                MHZ14A.read(); // throw data
            }

            if (true == MHZ14A.overflow())
            {
                Serial.println("MHZ14A overflow error");
                MHZ14A.flush();
            }
        }
    }

    virtual void OnDefCH20(uint16_t ppb, uint16_t fullrange)
    {
        Serial.printf("ZE08CH20_event ppb:%hd fullrange:%hd \n", ppb, fullrange);
    }
    virtual void OnAskCH20(uint16_t ppb, uint16_t ug_m3)
    {
        Serial.printf("ZE08CH20_event ppb:%hd ug/m3:%hd \n", ppb, ug_m3);
    }
    virtual void OnAskCO2(uint16_t ppm)
    {
        Serial.printf("MHZ14A_event ppm:%hd \n", ppm);
    }
    virtual void OnPM(uint16_t pm1_0, uint16_t pm2_5, uint16_t pm10_0)
    {
        Serial.printf("ZH03_event pm1_0:%hd pm2_5:%hd pm10_0:%hd \n", pm1_0, pm2_5, pm10_0);
    }

    virtual void OnDHT12(float Temperatura, float Humidity)
    {
        Serial.printf("DHT12 Temperatura:%f Humidity:%f \n", Temperatura, Humidity);
    }

    SoftwareSerial ZH03;
    SoftwareSerial ZE08CH20;
    SoftwareSerial MHZ14A;
    DHT dht;

  protected:

    Sensor() : MHZ14A(13, 15, false, 144), ZE08CH20(14, 12, false, 144), ZH03(2, 4, false, 144)
    {
        dht.setup(25);
    }

    void start()
    {
        ZH03.begin(9600);
        ZE08CH20.begin(9600);
        MHZ14A.begin(9600);
    }

    void stop()
    {
        ZH03.end();
        ZE08CH20.end();
        MHZ14A.end();
    }

  public:

    void MHZ14A_request()
    {
        for (uint8_t i = 0; i != AskLen; i++)
        {
            MHZ14A.write(WinSenRequestAsk[i]);
        }
    }

    void DHT12_request()
    {
        OnDHT12(dht.getTemperature(), dht.getHumidity());
    }

    void data_event()
    {
        ZH03_event();

        MHZ14A_event();

        ZE08CH20_event();

        while (Serial.available() > 0)
        {
            uint8_t recvd = Serial.read();
            switch (recvd)
            {
            case 0x1:
                while (Serial.available() > 0)
                    ZH03.write(Serial.read());
                break;
            case 0x2:
                while (Serial.available() > 0)
                    ZE08CH20.write(Serial.read());
                break;
            case 0x3:
                while (Serial.available() > 0)
                    MHZ14A.write(Serial.read());
                break;
            }
        }
    }
};
