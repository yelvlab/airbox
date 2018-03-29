
#include "sx1278.hpp"

#include <Esp.h>

#include <SPI.h>

#include <stdint.h>

// Parameter Table Definition

static uint8_t sx1278FreqTbl[1][3] =
    {
        {0x6C, 0x80, 0x00}, //434Mhz
};

static uint8_t sx1278PowerTbl[4] =
    {
        0xFF,
        0xFC,
        0xF9,
        0xF6,
};

static uint8_t sx1278LoRaBwTbl[10] =
    {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9 // 7.8Khz, 10.4KHz, 15.6KHz, 20.8KHz, 31.2KHz, 41.7KHz, 62.5KHz, 125KHz, 250KHz, 500KHz
};

struct LoraSx1278
{
    uint8_t mode;          //lora --1 / FSK --0
    uint8_t Freq_Sel;      //
    uint8_t Power_Sel;     //
    uint8_t Lora_Rate_Sel; //
    uint8_t BandWide_Sel;  //
    uint8_t Fsk_Rate_Sel;  //
    uint8_t SF;            // 扩频因子

    SPISettings __settings = SPISettings(10E6, MSBFIRST, SPI_MODE0); // PUBLIC

    uint8_t NSS, DIO0, RST, FrequencyMode;

    // default
    void setup(uint8_t NSS, uint8_t DIO0, uint8_t RST, uint8_t frequency_mode = 1)
    {
        this->NSS = NSS, this->DIO0 = DIO0, this->RST = RST, FrequencyMode = frequency_mode;
    }

    // ----------------------------------------------------------------------------
    // The SS (Chip select) pin is used to make sure the RFM95 is selected
    // ----------------------------------------------------------------------------
    inline void selectreceiver()
    {
        digitalWrite(NSS, LOW);
    }

    // ----------------------------------------------------------------------------
    // ... or unselected
    // ----------------------------------------------------------------------------
    inline void unselectreceiver()
    {
        digitalWrite(NSS, HIGH);
    }

    uint8_t SPIRead(uint8_t addr)
    {
        selectreceiver();
        SPI.beginTransaction(__settings);
        SPI.transfer(addr & 0x7F);
        uint8_t res = SPI.transfer(0x00);
        SPI.endTransaction();
        unselectreceiver();
        return res;
    }

    void SPIWrite(uint8_t addr, uint8_t WrPara)
    {
        selectreceiver();
        SPI.beginTransaction(__settings);
        SPI.transfer(addr | 0x80);
        SPI.transfer(WrPara);
        SPI.endTransaction();
        unselectreceiver();
    }

    void SPIBurstRead(uint8_t addr, uint8_t *ptr, uint8_t len)
    {
        if (!len) return;
        selectreceiver();
        SPI.beginTransaction(__settings);
        SPI.transfer(addr & 0x7F);
        for (uint8_t i = 0; i != len; i++) ptr[i] = SPI.transfer(0x00);
        SPI.endTransaction();
        unselectreceiver();
    }

    void SPIBurstWrite(uint8_t addr, uint8_t *ptr, uint8_t len)
    {
        if (!len) return;
        selectreceiver();
        SPI.beginTransaction(__settings);
        SPI.transfer(addr | 0x80);
        for (uint8_t i = 0; i != len; i++) SPI.transfer(ptr[i]);
        SPI.endTransaction();
        unselectreceiver();
    }

    void Standby()
    {
        if (FrequencyMode)
        {
            SPIWrite(LR_RegOpMode, 0x09); // Standby & Low Frequency mode
        }
        else
        {
            SPIWrite(LR_RegOpMode, 0x01); // standby / high frfequency mode
        }
        delay(10);
    }

    void Sleep()
    {
        if (FrequencyMode)
        {
            SPIWrite(LR_RegOpMode, 0x08); // Sleep & Low Frequency mode
        }
        else
        {
            SPIWrite(LR_RegOpMode, 0x00); // Sleep / high frequency mode
        }
        delay(25);
    }

    void EntryLoRa()
    {
        if (FrequencyMode)
        {
            SPIWrite(LR_RegOpMode, 0x88); // Low frequency mode
        }
        else
        {
            SPIWrite(LR_RegOpMode, 0x80); // high frequency mode
        }
    }

    void LoRaClearIrq(void)
    {
        SPIWrite(LR_RegIrqFlags, 0xFF);
    }

    inline uint8_t LoRaReadRSSI(void)
    {
        return (SPIRead(LR_RegRssiValue) + 127 - 137);
    }

    inline uint8_t FSKReadRSSI(void)
    {
        return 127 - (SPIRead(0x11) >> 1);
    }
};
