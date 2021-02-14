/* M370_PRIMARY

    Main m370 firmware for ESP32

    Note: I2C pins are different than the defaults:
    - I2C libraries may need to be modified to allow this
    - this is the reason that they are included with this sketch

    Note: LED functionality requires  the  <FastLED.h> library
    - to install go to  sketch->include library->Manage Libraries
    - and search  for FastLED
   _______

   version history
   20_06_17: added digital input class
   20_06_17: added LED functions from NIME/class/ledExample
   20_06_17: moved external libraries to subfolder
   20_06_10: added support for encoders
   20_05_01: added support for digital inputs
   20_04_21: added initial support for LSM6DS3
   20_04_20: added analog debugging - prints analogRead values to console
   20_04_20: added support for wifi
   20_04_12: added ultrasonic support
             commented out MPR121 on line 88.
             You can uncomment if you want to use MPR121
             but may cause your ESP32 to crash if  MPR121 is not connected
   20_04_01: added MPR121 functions
   20_03_25: added array notation and input
   20_01_20: created
*/

//external libraries are saved in the /src folder. This allows us to make
//minor modifications  to work with the m370 framework
#include "SparkFunLSM6DS3.h"
#include <Wire.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include "Esp32Encoder.h"

byte SERIAL_ENABLE = 1; //enables communication over USB
byte WIFI_ENABLE = 0; //enables communication over USB

// WiFi network name and password:
const char * ssid = "MLE";
const char * password = "mitmusictech";

const byte SERIAL_DEBUG = 0; //for debugging serial communication over USB
const byte WIFI_DEBUG = 0; //for debugging wifi communiication
const byte ANALOG_DEBUG = 0; //for debugging analog inputs using arduino console

LSM6DS3 IMU; //Default constructor is I2C, addr 0x6B

//some version of M370.h will be included in all your firmware
//it declares global variables and objects
#include "M370.h"

/*********************************************
  IMU SETUP
*********************************************/
int  imuInterval = 100;

IMUclass accels[3] =  {
  IMUclass( 0, "accelX", imuInterval, 8, MEAN ),
  IMUclass( 1, "accelY", imuInterval, 8, MEAN ),
  IMUclass( 2, "accelZ", imuInterval, 8, MEAN )
};

IMUclass gyros[3] =  {
  IMUclass( 3, "gyroX", imuInterval, 8, MEAN ),
  IMUclass( 4, "gyroY", imuInterval, 8, MEAN ),
  IMUclass( 5, "gyroZ", imuInterval, 8, MEAN )
};

IMUclass temps =  {
  IMUclass( 6, "temp", imuInterval, 8, MEAN )
};

byte ACCEL_ENABLE = 0;
byte GYRO_ENABLE = 0;
byte TEMP_ENABLE = 0;

/*********************************************
  CAPACITIVE SETUP
*********************************************/
//declare MPR121 object
Adafruit_MPR121 _MPR121 = Adafruit_MPR121();

byte chargeCurrent = 63; //from 0-63, def 63
byte chargeTime = 1; //from 0-7, def 1
byte NUM_ELECTRODES = 0;
int capSenseInterval = 100;

Cap capSense[12] = {
  Cap ( capSenseInterval ),
  Cap ( capSenseInterval ),
  Cap ( capSenseInterval ),
  Cap ( capSenseInterval ), //4
  Cap ( capSenseInterval ),
  Cap ( capSenseInterval ),
  Cap ( capSenseInterval ),
  Cap ( capSenseInterval ),//8
  Cap ( capSenseInterval ),
  Cap ( capSenseInterval ),
  Cap ( capSenseInterval ),
  Cap ( capSenseInterval ) //12
};

/*********************************************
  ANALOG SETUP
*********************************************/
//we can choose how fast to send analog sensors here
int analogSendRate = 250;

//Sensor objects can have multiple kinds of arguments:
//argument 1 is the physical pin on the PCB
//argument 2 is the OSC address of the sensor
//argument 3 (optional) is how fast the data is sent in MS
//argument 4 is oversampling (val from 1 to 32)
//argument 5 is how to process oversampling
const byte numSensors = 22;

Sensor sensors[22] = {
  Sensor ( p0, "/analog/0", analogSendRate, 8, MEAN ),
  Sensor ( p1, "/analog/1", analogSendRate, 8, MEAN ),
  Sensor ( p2, "/analog/2", analogSendRate, 8, MEAN ),
  Sensor ( p3, "/analog/3", analogSendRate, 8, MEAN ),
  Sensor ( p4, "/analog/4", analogSendRate, 8, MEAN ),
  Sensor ( p5, "/analog/5", analogSendRate, 8, MEAN ),
  Sensor ( p6, "/analog/6", analogSendRate, 8, MEAN ),
  Sensor ( p7, "/analog/7", analogSendRate, 8, MEAN ),
  Sensor ( p8, "/analog/8", analogSendRate, 8, MEAN ),
  Sensor ( p9, "/analog/9", analogSendRate, 8, MEAN ),

  Sensor ( BUTTON_0, "/analog/8", analogSendRate, 8, MEAN ),
  Sensor ( BUTTON_1, "/analog/9", analogSendRate, 8, MEAN ),

  Sensor ( pCS0, "/button/0", analogSendRate, 8, MEAN ),
  Sensor ( pCS1, "/button/1", analogSendRate, 8, MEAN),
  Sensor ( dac1, "/button/0", analogSendRate, 8, MEAN ),
  Sensor ( dac2, "/button/1", analogSendRate, 8, MEAN),

  Sensor ( pCLK, "/button/0", analogSendRate, 8, MEAN ),
  Sensor ( pMISO, "/button/1", analogSendRate, 8, MEAN),
  Sensor ( pSDA, "/button/0", analogSendRate, 8, MEAN ),
  Sensor ( pSCL, "/button/1", analogSendRate, 8, MEAN),
  Sensor ( pMOSI, "/button/0", analogSendRate, 8, MEAN ),
  Sensor ( MIDI, "/button/1", analogSendRate, 8, MEAN)
};

/*********************************************
  DIGITAL INPUT SETUP
*********************************************/
//we can choose how fast to send analog sensors here
int digitalSendRate = 1;

//Sensor objects can have multiple kinds of arguments:
//argument 1 is the physical pin on the PCB
//argument 2 is the OSC address of the sensor
//argument 3 (optional) is how fast the data is sent in MS
//argument 4 is oversampling (val from 1 to 32)
//argument 5 is how to process oversampling
const byte numDigitalInputs = 4;

DigitalIn dIn[numDigitalInputs] = {
  DigitalIn ( pMISO, "/digital/0", digitalSendRate, 8 ),
  DigitalIn ( pMOSI, "/digital/1", digitalSendRate, 8 ),
  DigitalIn ( pCLK, "/digital/2", digitalSendRate, 8 ),
  DigitalIn ( pCS0, "/digital/3", digitalSendRate, 8 )
};

/*********************************************
  ENCODERS SETUP
*********************************************/
//encoders rely on the  ESP32Encoder library being installed in  ~/documents/Arduino/libraries
//Esp32Encoder rotaryEncoder = Esp32Encoder(18,2,4);//A,B,Button
//optional divider argument:
const byte NUM_ENCODERS = 3;

Esp32Encoder enc[3] = {
  Esp32Encoder(p0, p3, -1, 4), //A,B,Button, Divider
  Esp32Encoder(p4, pCS1, -1, 4), //A,B,Button, Divider
  Esp32Encoder(p6, p8, -1, 4) //A,B,Button, Divider
};

/*********************************************
  SETUP
*********************************************/
void setup() {
  //Be sure to select  either USB or WiFi using enables at top of script
  Serial.begin(115200);
  SerialSetup();
  //WiFiSetup();
  IMUSetup();
  ledSetup();

  MPR121setup(); //<- UNCOMMENT TO USE MPR121
  //MPR121test(); //comment out for normal use
  
  delay(500);
  
  EncoderSetup();

  //for(byte i=0;i < numDigitalInputs; i++) dIn[i].setup();
}

/*********************************************
  LOOP
*********************************************/
void loop() {
  static uint32_t = 0;
  int interval =500;
  static byte pin = 0;

  if(millis()-timer>interval){
    timer=millis();

    switch(pin){
    case 0:  val = IMU.readRawAccelX(); break;
    case 1:  val = IMU.readRawAccelY(); break;
    case 2:  val = IMU.readRawAccelZ(); break;
    case 3:  val = IMU.readRawGyroX(); break;
    case 4:  val = IMU.readRawGyroY(); break;
    case 5:  val = IMU.readRawGyroZ(); break;
    case 6:  val = IMU.readTempC() * 2; break;
    }
    pin>4 ? pin=0:pin++;
    Serial.print(pin);
    Serial.print(" ");
    Serial.println(val);
  }
}

/*********************************************
  ADDITIONAL FUNCTIONS
*********************************************/

void sendCapValues() {
  static uint32_t timer = 0;
  int interval = 50;

  if (millis() - timer > interval) {
    timer = millis();

    totalCapacitance = 4096;
    //make sure totalCap >= 0
    for (byte i = 0; i < NUM_ELECTRODES; i++) totalCapacitance += (capSense[i].outVal - 4096);
    SlipOutByte(101);
    SlipOutInt(totalCapacitance);
    SerialOutSlip();
  }
}