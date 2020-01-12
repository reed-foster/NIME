/*general_protocol
 * 
 * arduino sketch for use with an ESP32, sending data to a PC
 * or a raspberry pi.
 * 
 * Ian Hattwick
 * Fred Kelly
 * Massachusetts Institute of Technology, 2019
 * _______
 * 
 * define whether to send data over serial or wifi using wifiEnable and serialEnable
 * - these will be set by physical switches later
 * - the same data is sent over both interfaces (except for the debugging serial
 *   information)
 *   
 * _______
 * 
 * version history
 * 07_30: test patch, sends test data over both wifi and serial
 */

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_LSM9DS1.h>
#include <Adafruit_Sensor.h>
#include <WiFi.h>
#include <WiFiUdp.h>

byte led_val = 0;
byte wifiEnable = 1;
byte serialEnable = 1;
byte WifiSend();


const byte LED_PIN = 22;
const int potPin0 = 37;
const int potPin1 = 38;
const int potPin2 = 34;
const int btnPin0 = 19;
const int btnPin1 = 22;
const int btnPin2 = 21;

int potValue0 = 0;
int potValue1 = 0;
int potValue2 = 0;
int btnValue0 = 0;
int btnValue1 = 0;
int btnValue2 = 0;

sensors_event_t accelEvt, magEvt, gyroEvt, tempEvt;
float pitch;
float roll;

int runAvgBuf0[64];
int runAvgBuf1[64];
int runAvgBuf2[64];

int curTime = 0;

int lastSentBtn0 = 0;
int lastSentBtn1 = 0;
int lastSentBtn2 = 0;

int lastSentFdr0 = 0;
int lastSentFdr1 = 0;
int lastSentFdr2 = 0;

int lastSentImuRaw0 = 0;
int lastSentImuAngles0 = 0;

int lastReadImu0 = 0;
int lastCalcImu0 = 0;



Adafruit_LSM9DS1 lsm = Adafruit_LSM9DS1();

void setupSensor()
{
  lsm.begin();
  
  // 1.) Set the accelerometer range
  lsm.setupAccel(lsm.LSM9DS1_ACCELRANGE_2G);
  //lsm.setupAccel(lsm.LSM9DS1_ACCELRANGE_4G);
  //lsm.setupAccel(lsm.LSM9DS1_ACCELRANGE_8G);
  //lsm.setupAccel(lsm.LSM9DS1_ACCELRANGE_16G);
  
  // 2.) Set the magnetometer sensitivity
  lsm.setupMag(lsm.LSM9DS1_MAGGAIN_4GAUSS);
  //lsm.setupMag(lsm.LSM9DS1_MAGGAIN_8GAUSS);
  //lsm.setupMag(lsm.LSM9DS1_MAGGAIN_12GAUSS);
  //lsm.setupMag(lsm.LSM9DS1_MAGGAIN_16GAUSS);

  // 3.) Setup the gyroscope
  lsm.setupGyro(lsm.LSM9DS1_GYROSCALE_245DPS);
  //lsm.setupGyro(lsm.LSM9DS1_GYROSCALE_500DPS);
  //lsm.setupGyro(lsm.LSM9DS1_GYROSCALE_2000DPS);
}

int oversample(int adcPin, int numSamples) {
  int total = 0;
  for (int i = 0; i < numSamples; i++) {
    total += analogRead(adcPin);
  }
  return total / numSamples;
}

void readImu(sensors_event_t *a, sensors_event_t *m, sensors_event_t *g, sensors_event_t *temp,
             float *pitch, float *roll, int interval, int *lastTime) {
  if (curTime - *lastTime < interval) {
    return;
  }
  else {
    *lastTime = curTime; // update the contents of that memory address to curTime
  }
  lsm.read();
  lsm.getEvent(a, m, g, temp);
  
  float dt = (float)interval / 1000;
  float gyroPitch, gyroRoll, accelPitch, accelRoll; 
  gyroPitch = *pitch + g->gyro.x * dt;
  gyroRoll = *roll + g->gyro.y * dt; 
  accelPitch = atan2f(a->acceleration.y, a->acceleration.z) * 180 / PI;
  accelRoll = atan2f(a->acceleration.x, a->acceleration.z) * 180 / PI;
  
  *pitch = 0.98 * gyroPitch + 0.02 * accelPitch;
  *roll = 0.98 * gyroRoll + 0.02 * accelRoll;      
}


int runningAverage(int adcPin, int buf[], int bufferSize) {
  int avgs = 0;
  static int index;
  int curReading = analogRead(adcPin);
  if (index >= bufferSize) {
    index = 0;
  }
  buf[index] = curReading;
  for (int i = 0; i < bufferSize; i++) {
    avgs = avgs + buf[i];
  }
  index++;
  return avgs / bufferSize;
}

void setup() {
  pinMode(btnPin0, INPUT_PULLUP);
  pinMode(btnPin1, INPUT_PULLUP);
  pinMode(btnPin2, INPUT_PULLUP);
  if( serialEnable)SerialSetup();
  if( wifiEnable ) WiFiSetup();
  MsgsSetup();
  setupSensor();
}

void loop() {
  curTime = millis();
  readImu(&accelEvt, &magEvt, &gyroEvt, &tempEvt, &pitch, &roll, 1, &lastReadImu0);
  sendImuAngles(0, pitch, roll, 100, &lastSentImuAngles0);
  // sendImuRaw(0, accelEvt, magEvt, gyroEvt, 1000, &lastSentImuRaw0);
  delay(0);
}