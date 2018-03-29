
#include <Arduino.h>

#include <lora_client.hpp>

struct JhLoraClient : LoraClient
{
    enum LoraFlag
    {
        LoraTx,
        LoraTxing,
        LoraRx,
        LoraRxing,
    };

    LoraFlag LoraState;

    uint16_t RunDelay = 5000;
    uint8_t Verify = 0;

    void Interrupt()
    {
        Logout.printf("InterruptLora: %d\n", LoraState);
        switch(LoraState)
        {
            case LoraTxing:
            {
                SPIRead(LR_RegIrqFlags);
                LoraSx1278::LoRaClearIrq();
                LoraSx1278::Standby();
                Logout.println("TX Done");
                LoraState = LoraRxing;
                EntryRx();
                break;
            }
            case LoraRxing:
            {
                Logout.println("RX Done");
                uint8_t recvlen = 0, Result = false, Temp = 0;
                recvlen = PacketRx(&Temp, sizeof(Temp));
                if (recvlen && recvlen <= sizeof(Temp))
                {
                    Logout.printf("RSSI is %d dBm Pkt RSSI is %d dBm\n", (-164 + SPIRead(LR_RegRssiValue)), (-164 + SPIRead(LR_RegPktRssiValue)));
                    if(Verify == Temp)
                    {
                        TimeOut = 0;
                        Result = true;
                        LoraState = LoraTx;
                    }
                    LoraClient::LoraTranAdjust(Result, &RunDelay);
                }
                break;
            }
        }
    }

    unsigned long TimeOut = 0;

    bool Send(uint8_t buffer[], uint8_t buflen)
    {
        Logout.printf("Lora RunDelay %d\n", RunDelay);
        if(LoraTx == LoraState || (millis() - TimeOut) > 5000 + RunDelay)
        {
            TimeOut = millis();

            Verify = 0;
            for(uint8_t i = 0; i < buflen; i++) Verify += buffer[i];


            LoraClient::LoraTranAdjust(LoraTx == LoraState, &RunDelay);
            // delay(RunDelay);

            if(EntryTx(buflen))
            {
                PacketTx(buffer, buflen);

                Logout.printf("Result : %d Send : %s\n", buflen, buffer);
            }

            LoraState = LoraTxing;
            return true;
        }
		return false;
    }
};

JhLoraClient LoraDevice;

static void InterruptLoraRecvd(void)
{
    LoraDevice.Interrupt();
}

void LoraDeviceSetup()
{
    LoraDevice.LoraState = JhLoraClient::LoraTx;
    LoraDevice.ConfigSx1278(5, 26, 27);
    LoraDevice.SetOnDIORise(26, InterruptLoraRecvd, true);
    // CAD
}
