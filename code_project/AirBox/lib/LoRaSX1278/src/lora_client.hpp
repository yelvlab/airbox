
#include "lora.hpp"

class LoraClient : public LoraSx1278
{
  public:

    Stream & Logout = Serial;

    void test_setup()
    {
        Serial.begin(115200);

        ConfigSx1278(5, 39, 32);
        
        // ConfigSx1278(15, 4, 16);
    }

    void test_loop()
    {
        Logout.println("LoRa-Sender");
        static int sent_count = 0;
        static uint8_t TxData[64];
        sprintf((char *)TxData, "%011d", sent_count);
        Logout.printf("Sending is : %s\n", TxData);
        EntryTx(11); // Enter TX mode
        PacketTx(TxData, 11);
        sent_count++;
        
        return ;

        Logout.println("LoRa-Receiver");
        EntryRx();
        uint8_t recvlen = 0;
        delay(100);
        static uint8_t RxData[64];
        recvlen = PacketRx(RxData, sizeof(RxData));
        if (recvlen && recvlen <= sizeof(RxData))
        {
            Logout.printf("RSSI is %d dBm\nPkt RSSI is %d dBm\n", (-164 + SPIRead(LR_RegRssiValue)), (-164 + SPIRead(LR_RegPktRssiValue)));
            Logout.printf("Recv Len %d Data : %s\n", recvlen, RxData);
        }
    }

    void SetOnDIORise(uint8_t DIO, void(*Callback)(void), bool Flag)
    {
        pinMode(DIO, INPUT);
        if(Flag)
        {
            attachInterrupt(digitalPinToInterrupt(DIO), Callback, RISING);
        }
        else
        {
            detachInterrupt(digitalPinToInterrupt(DIO));
        }
    }

    uint8_t ConfigSx1278(uint8_t NSS, uint8_t DIO0, uint8_t RST, uint8_t FrequencyMode = 1)
    {
        pinMode(NSS, OUTPUT);
        pinMode(RST, OUTPUT);

        LoraSx1278::setup(NSS, DIO0, RST, FrequencyMode);

        SPI.begin();//(13,12,14,27);

        digitalWrite(RST, HIGH);
        delay(10);
        digitalWrite(RST, LOW);
        delay(10);
        digitalWrite(RST, HIGH);
        delay(10);

        mode = 0x01;         // lora mode
        Freq_Sel = 0x00;     // 433Mhz
        Power_Sel = 0x00;    //
        SF = 12;             // SF12
        BandWide_Sel = 0x07; // 125Khz
        Fsk_Rate_Sel = 0x00;

        LoraSx1278::Sleep(); // modem must be in sleep mode

        uint8_t ret = SPIRead(RegVersion);
        if (ret == 0x12)
        {
            LoraSx1278::EntryLoRa(); //lora mode
            // SPIWrite(0x5904); // change digital regulator from 1.6V to 1.47V: see errata note
            SPIBurstWrite(LR_RegFrMsb, sx1278FreqTbl[Freq_Sel], 3); //set the frequency parameter
            // SPIWrite(LR_RegPaConfig, 0x8F), SPIWrite(REG_LR_PADAC, 0x87);
            // set the base parameters
            SPIWrite(LR_RegPaConfig, sx1278PowerTbl[Power_Sel]); // set the output power parameter(0xFF)
            SPIWrite(LR_RegOcp, 0x0B); // OCP disabled
            SPIWrite(LR_RegLna, 0x20); // 0x23 for HF, 00111000
            uint8_t mc1 = (sx1278LoRaBwTbl[BandWide_Sel] << 4) | (CR << 1);
            //mc2 = {SF << 4 ) | (CRC<<2 ) | 0x03 ; //CRC on, RxTimeout MSB = 0x03
            uint8_t mc2 = (SF << 4) | (CRC << 2);
            uint8_t mc3 = 0x00;
            if (SF == 6) // SP6 only can be run under Implict mode
            {
                // Implicit, Enable CRC Enable (0x02) & Error Coding rate 4/5 (0x01), 4/6 (0x02), 4/7 (0x03), 4/8 (0x04)
                mc1 = mc1 | 0x01; // Implicit mode
                uint8_t tmp = SPIRead(LR_RegDetectOptimize); // Reg:0x31
                tmp &= 0xF8;
                tmp |= 0x05; // SP6 LoRa Detection Optimize
                SPIWrite(LR_RegDetectOptimize, tmp);
                SPIWrite(LR_RegDetectionThreshold, 0x0C); // SP6 Detection Threshold(Reg:0x37)
            }
            if ((SF == 11) || (SF == 12)) //LowDataRateOptimize
            {
                mc3 = 0x8;
            }
            SPIWrite(LR_RegModemConfig1, mc1);
            SPIWrite(LR_RegModemConfig2, mc2);
            SPIWrite(LR_RegModemConfig3, mc3); // LNA AGC off
            // SPIWrite(LR_RegSymbTimeoutLsb, 0xFF);   // RxTimeout LSB = 0xFF, i.e. set the maximum Timeout value
            SPIWrite(LR_RegSymbTimeoutLsb, 0x10);
            SPIWrite(LR_RegPreambleMsb, 0x00); //
            SPIWrite(LR_RegPreambleLsb, 8);    // Preamble length = 12
            SPIWrite(LR_RegSyncWord, 0x12);
            SPIWrite(LR_RegFifoRxBaseAddr, 0x00), SPIWrite(LR_RegFifoTxBaseAddr, 0x80);
            LoraSx1278::Standby();
            Logout.print("Config - finished method, opmode is now : ");
            Logout.println(SPIRead(LR_RegOpMode), BIN);
            uint8_t showOpMode = SPIRead(LR_RegOpMode);
            Logout.println((showOpMode && 0x80 == 0) ? "OpMode is FSK" : "OpMode is LoRa");
            Logout.print("OpMode Regsiters = "), Logout.println(showOpMode, HEX);
            Logout.flush();
        }
        return ret;
    }

    void EntryRx(void)
    {
        LoraSx1278::Standby();
        //SPIWrite(REG_LR_PADAC, 0x84 );    // ??? seems no need to set PADAC at Rx mode !!!!
        SPIWrite(LR_RegHopPeriod, 0x00); // Disable Hopping
        //SPIWrite(REG_LR_DIOMAPPING1, 0x01 );  // ???
        SPIWrite(REG_LR_DIOMAPPING1, 0x00 | 0x3F); // Mapping Dio0 to Rx-Done
        SPIWrite(LR_RegIrqFlagsMask, 0x9f); //enale IRQ for RxDone(bit 6), CRC error(bit 5)
        LoraSx1278::LoRaClearIrq();

        if (SF == 6) SPIWrite(LR_RegPayloadLength, 5); //??? Fix PayloadLength to 5

        SPIWrite(LR_RegFifoAddrPtr, SPIRead(LR_RegFifoRxBaseAddr)); // Prepared Rx FIFO address

        SPIWrite(LR_RegOpMode, 0x8D); // Set the Operating Mode to LoRa (0x80), Rx SINGLE (0x06), Low Frequency Mode (0x08)

        delay(10);
    }

    uint8_t PacketRx(uint8_t * Data, uint8_t Len)
    {
        uint8_t packet_size = 0;
        if (digitalRead(DIO0)) // if RX-Done
        {
            Logout.println("DIO_0 shows packet recieved");

            if (!(SPIRead(LR_RegIrqFlags) & 0x10)) // if not CRC error
            {
                uint8_t addr = SPIRead(LR_RegFifoRxCurrentAddr);
                if (SF == 6)
                    packet_size = 5; //SF6 with fixed payload length
                else
                    packet_size = SPIRead(LR_RegRxNbBytes);
                SPIWrite(LR_RegFifoAddrPtr, addr); // RXBaseAddr --> FiFoAddrPtr

                Logout.printf("Received Packet length = %d\n", packet_size);
                if(Len >= packet_size)
                {
                    SPIBurstRead(LR_RegFifo, Data, packet_size); //LR_RegFifo = 0x00
                }
            }
            else
            {
                SPIWrite(LR_RegIrqFlags, 0x10); //Clear it
                Logout.println("CRC error!");
            }
            LoraSx1278::LoRaClearIrq();
        }
        // rst fifo addr
        SPIWrite(LR_RegFifoAddrPtr, SPIRead(LR_RegFifoRxBaseAddr));
        return packet_size;
    }

    bool EntryTx(uint8_t PayloadLength)
    {
        LoraSx1278::Standby();
        SPIWrite(REG_LR_PADAC, 0x87);    // TX PA-Boost 20dbm
        SPIWrite(LR_RegHopPeriod, 0x00); // Disabled Hopping
        //SPIWrite(REG_LR_DIOMAPPING1, 0x41 );  // ???
        SPIWrite(REG_LR_DIOMAPPING1, 0xFD); // TS_ Mapping DIO0 to Tx-Done
        SPIWrite(LR_RegIrqFlagsMask, 0xF7); // enable IRQ Tx-Done
        LoraSx1278::LoRaClearIrq();
        SPIWrite(LR_RegPayloadLength, PayloadLength); // implict Header, set fixed PayloadLength
        SPIWrite(LR_RegFifoAddrPtr, SPIRead(LR_RegFifoTxBaseAddr)); // Prepared TX FIFO pointer
        delay(10);
        if (PayloadLength == SPIRead(LR_RegPayloadLength))
        {
            return true;
        }
        return false;
    }

    void PacketTx(uint8_t *Data, uint8_t Len)
    {
        LoraSx1278::Standby();
        SPIWrite(LR_RegFifoAddrPtr, SPIRead(LR_RegFifoTxBaseAddr));
        SPIBurstWrite(LR_RegFifo, Data, Len); //LR_RegFifo = 0x00
        SPIWrite(LR_RegOpMode, 0x8b);         //TX mode
        
        return ;

        uint16_t limit = 1000;// Len * 8;
        while (limit--)
        {
            delay(10);
            // if TX-Done
            if (digitalRead(DIO0))
            {
                SPIRead(LR_RegIrqFlags);
                LoraSx1278::LoRaClearIrq();
                LoraSx1278::Standby();
                break;
            }
        }
        return ;// (limit != 0);
    }

    bool LoraTranAdjust( bool Result, uint16_t * Delay )
    {
        const int Limit = 3;
        static unsigned char Layer = 0;

        enum LoraState
        {
            UnStable,
            Stabling,
            Stabled,
        };

        static LoraState State = UnStable;

        switch ( State )
        {
            case UnStable:
            {
                *Delay = 5000, State = Stabling;
                return false;
            }
            case Stabling:
            {
                if ( Result )
                {
                    State = Stabled;
                }
                else
                {
                    *Delay += random(0, 500);
                }
                return true;
            }
            case Stabled:
            {
                if ( false == Result )
                {
                    Layer += 1, *Delay += random(0, 100);
                    if ( Layer > Limit )
                    {
                        State = Stabling;
                    }
                    return true;
                }
                else
                {
                    Layer = 0, *Delay -= random(0, 50);
                    return false;
                }
            }
        }
    }

};
