/*******************************************************************************************************
  Programs for Arduino - Copyright of the author Stuart Robinson - 19/03/20

  This program is supplied as is, it is up to the user of the program to decide if the program is
  suitable for the intended purpose and free from errors. 
*******************************************************************************************************/

/*******************************************************************************************************
  Program Operation - This is a program that demonstrates the detailed setup of a LoRa test receiver.
  The program listens for incoming packets using the lora settings in the 'Settings.h' file. The pins 
  to access the lora device need to be defined in the 'Settings.h' file also.

  There is a printout on the Arduino IDE Serial Monitor of the valid packets received, the packet is 
  assumed to be in ASCII printable text, if it's not ASCII text characters from 0x20 to 0x7F, expect 
  weird things to happen on the Serial Monitor. The LED will flash for each packet received and the 
  buzzer will sound, if fitted.

  Sample serial monitor output;

  7s  Hello World 1234567890*,CRC,DAAB,RSSI,-52dBm,SNR,9dB,Length,23,Packets,5,Errors,0,IRQreg,50

  If there is a packet error it might look like this, which is showing a CRC error,

  968s PacketError,RSSI,-87dBm,SNR,-11dB,Length,23,Packets,613,Errors,2,IRQreg,70,IRQ_HEADER_VALID,IRQ_CRC_ERROR,IRQ_RX_DONE
  
  Serial monitor baud rate is set at 9600.
*******************************************************************************************************/

#define Program_Version "V1.0"

#include <SPI.h>                                 //the lora device is SPI based so load the SPI library
#include "SX128XLT_SW.h"                            //include the appropriate library   


#ifndef BALLOON
#define BALLOON               1
// #define MULTITECH_ISM2400     1
#endif

// Define here the S1280 Mikrobus you have plugged on the TinyGS 2G4 station
#define MIKROBUS0_LAMBDA80
//#define MIKROBUS1_LAMBDA80
//#define MIKROBUS0_E28
//#define MIKROBUS1_E28
//#define MIKROBUS0_NICERF
//#define MIKROBUS1_NICERF

#include "Settings.h"                            //include the setiings file, frequencies, LoRa settings etc   

SX128XLT_SW LT;                                     //create a library class instance called LT

uint32_t RXpacketCount;
uint32_t errors;

uint8_t RXBUFFER[RXBUFFER_SIZE];                 //create the buffer that received packets are copied into

uint8_t RXPacketL;                               //stores length of packet received
int8_t  PacketRSSI;                              //stores RSSI of received packet
int8_t  PacketSNR;                               //stores signal to noise ratio (SNR) of received packet

uint64_t devEUI;

void loop()
{
  RXPacketL = LT.receive(RXBUFFER, RXBUFFER_SIZE, 60000, WAIT_RX); //wait for a packet to arrive with 60seconds (60000mS) timeout

  digitalWrite(LED1, HIGH);                      //something has happened

  if (BUZZER > 0)                                //turn buzzer on
  {
    digitalWrite(BUZZER, HIGH);
  }

  PacketRSSI = LT.readPacketRSSI();              //read the recived RSSI value
  PacketSNR = LT.readPacketSNR();                //read the received SNR value

  if (RXPacketL == 0)                            //if the LT.receive() function detects an error, RXpacketL is 0
  {
    packet_is_Error();
  }
  else
  {
    packet_is_OK();
  }

  if (BUZZER > 0)
  {
    digitalWrite(BUZZER, LOW);                   //buzzer off
  }

  digitalWrite(LED1, LOW);                       //LED off

  Serial.println();
}

static void printf_ba(const uint8_t *ba, size_t len, size_t line_size, const char *line_sep) {
	for (unsigned int i = 0; i < len; i++) {
		if (i != 0 && i % line_size == 0) {
			printf("%s", line_sep);
		}
		printf("%02x ", ba[i]);
	}
	printf("%s", line_sep);
}

//static const boolean isASCII = false;

void packet_is_OK()
{
  uint16_t IRQStatus, localCRC;

  IRQStatus = LT.readIrqStatus();                 //read the LoRa device IRQ status register

  RXpacketCount++;

  printElapsedTime();                             //print elapsed time to Serial Monitor
  Serial.print(F("  "));

  localCRC = LT.CRCCCITT(RXBUFFER, RXPacketL, 0xFFFF);  //calculate the CRC, this is the external CRC calculation of the RXBUFFER
  Serial.print(F("CRC="));                       //contents, not the LoRa device internal CRC
  Serial.print(localCRC, HEX);
  Serial.print(F(",RSSI="));
  Serial.print(PacketRSSI);
  Serial.print(F("dBm,SNR="));
  Serial.print(PacketSNR);
  Serial.print(F("dB,Length="));
  Serial.print(RXPacketL);
  Serial.print(F(",Packets="));
  Serial.print(RXpacketCount);
  Serial.print(F(",Errors="));
  Serial.print(errors);
  Serial.print(F(",IRQreg="));
  Serial.print(IRQStatus, HEX);
  Serial.println();
  LT.printASCIIPacket(RXBUFFER, RXPacketL);       //print the packet as ASCII characters
  Serial.println();
  printf_ba(RXBUFFER, RXPacketL, 16, "\n");
}


void packet_is_Error()
{
  uint16_t IRQStatus;
  IRQStatus = LT.readIrqStatus();                   //read the LoRa device IRQ status register

  printElapsedTime();                               //print elapsed time to Serial Monitor

  if (IRQStatus & IRQ_RX_TIMEOUT)                   //check for an RX timeout
  {
    Serial.print(F(" RXTimeout"));
  }
  else
  {
    errors++;
    Serial.print(F(" PacketError"));
    Serial.print(F(",RSSI,"));
    Serial.print(PacketRSSI);
    Serial.print(F("dBm,SNR,"));
    Serial.print(PacketSNR);
    Serial.print(F("dB,Length,"));
    Serial.print(LT.readRXPacketL());               //get the device packet length
    Serial.print(F(",Packets,"));
    Serial.print(RXpacketCount);
    Serial.print(F(",Errors,"));
    Serial.print(errors);
    Serial.print(F(",IRQreg,"));
    Serial.print(IRQStatus, HEX);
    LT.printIrqStatus();                            //print the names of the IRQ registers set
  }

  delay(250);                                       //gives a longer buzzer and LED flash for error 
  
}


void printElapsedTime()
{
  float seconds;
  seconds = millis() / 1000;
  Serial.print(seconds, 0);
  Serial.print(F("s"));
}


void led_Flash(uint16_t flashes, uint16_t delaymS)
{
  uint16_t index;

  for (index = 1; index <= flashes; index++)
  {
    digitalWrite(LED1, HIGH);
    delay(delaymS);
    digitalWrite(LED1, LOW);
    delay(delaymS);
  }
}


void setup()
{
#ifdef ARDUINO_ARCH_ESP32  
  devEUI = ESP.getEfuseMac();
  printf("DevEUI: %llX\n", devEUI);
#else
  devEUI = 0x0102030405060708;
#endif

  pinMode(LED1, OUTPUT);                        //setup pin as output for indicator LED
  led_Flash(2, 125);                            //two quick LED flashes to indicate program start

  Serial.begin(115200);
  Serial.println();
  Serial.print(F(__TIME__));
  Serial.print(F(" "));
  Serial.println(F(__DATE__));
  Serial.println(F(Program_Version));
  Serial.println();
  Serial.println(F("TinyGS 2G4 :: LoRa_Receiver_Detailed_Setup Starting"));
  Serial.println();

  if (BUZZER > 0)
  {
    pinMode(BUZZER, OUTPUT);
    digitalWrite(BUZZER, HIGH);
    delay(50);
    digitalWrite(BUZZER, LOW);
  }

  SPI.begin();

  //SPI beginTranscation is normally part of library routines, but if it is disabled in the library
  //a single instance is needed here, so uncomment the program line below
  //SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));

  //setup hardware pins used by device, then check if device is found
  if (LT.begin(NSS, NRESET, RFBUSY, DIO1, DIO2, DIO3, RX_EN, TX_EN, LORA_DEVICE))
  {
    Serial.println(F("LoRa Device found"));
    led_Flash(2, 125);
    delay(1000);
  }
  else
  {
    Serial.println(F("No device responding"));
    while (1)
    {
      led_Flash(50, 50);                                       //long fast speed LED flash indicates device error
    }
  }

  //The function call list below shows the complete setup for the LoRa device using the information defined in the
  //Settings.h file.
  //The 'Setup LoRa device' list below can be replaced with a single function call;
  //LT.setupLoRa(Frequency, Offset, SpreadingFactor, Bandwidth, CodeRate);

  //***************************************************************************************************
  //Setup LoRa device
  //***************************************************************************************************
  LT.setMode(MODE_STDBY_RC);
  LT.setRegulatorMode(USE_LDO);
  LT.setPacketType(PACKET_TYPE_LORA);
  LT.setSyncWord(LORA_MAC_PUBLIC_SYNCWORD);
  LT.setRfFrequency(Frequency, Offset);
  LT.setBufferBaseAddress(0, 0);
  LT.setModulationParams(SpreadingFactor, Bandwidth, CodeRate);
  LT.setPacketParams(LenInSymb, LORA_PACKET_VARIABLE_LENGTH, 255, LORA_CRC_ON, LORA_IQ_UPLINK, 0, 0);
  LT.setDioIrqParams(IRQ_RADIO_ALL, (IRQ_TX_DONE + IRQ_RX_TX_TIMEOUT), 0, 0);
  //***************************************************************************************************


  Serial.println();
  LT.printModemSettingsSW();                                     //reads and prints the configured LoRa settings, useful check
  Serial.println();
  LT.printOperatingSettings();                                 //reads and prints the configured operting settings, useful check
  Serial.println();
  Serial.println();
  LT.printRegisters(0x900, 0x9FF);                             //print contents of device registers, normally 0x900 to 0x9FF
  Serial.println();
  Serial.println();

  Serial.print(F("Receiver ready - RXBUFFER_SIZE "));
  Serial.println(RXBUFFER_SIZE);
  Serial.println();
}
