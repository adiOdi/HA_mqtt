#include "sensor.h"
#include <Arduino.h>
// #include <cmath>
// #include <cstring>

#define BAUD_RATE 256000

void rd_03d::setup(const int rx_pin, const int tx_pin)
{
  sensorX = 0;
  sensorY = 0;
  sensorRotation = 0.0;
  counter = 0;
  // Serial.begin(9600); // Debugging
  Serial1.setRxBufferSize(64);
  Serial1.begin(BAUD_RATE, SERIAL_8N1, rx_pin, tx_pin);
  // Set buffer size
  Serial.println("RD-03D Radar Module Initialized");

  // Send multi-target detection command
  Serial1.write(Multi_Target_Detection_CMD, sizeof(Multi_Target_Detection_CMD));
  delay(200);
  Serial.println("Multi-target detection mode activated.");

  RX_count = 0;
  Serial1.flush();
}

void rd_03d::update()
{
  // Read data from Serial1
  while (Serial1.available())
  {
    RX_temp = Serial1.read();
    RX_BUF[RX_count++] = RX_temp;

    // Prevent buffer overflow
    if (RX_count >= sizeof(RX_BUF))
    {
      RX_count = sizeof(RX_BUF) - 1;
    }

    // Check for end of frame (0xCC, 0x55)
    if ((RX_count > 1) && (RX_BUF[RX_count - 1] == 0xCC) && (RX_BUF[RX_count - 2] == 0x55))
    {
      processRadarData();
      break;
      // if (data.target_1.detected)
      //   printData(data.target_1);
      // if (data.target_2.detected)
      //   printData(data.target_2);
      // if (data.target_3.detected)
      //   printData(data.target_3);
    }
  }
}
bool rd_03d::isOccupied(JsonDocument json)
{
  if (data.target_1.detected &&
      (int)json["x"] <= data.target_1.x &&
      (int)json["y"] <= data.target_1.y &&
      (int)json["x"] + (int)json["width"] >= data.target_1.x &&
      (int)json["y"] + (int)json["height"] >= data.target_1.y)
    return true;
  if (data.target_2.detected &&
      (int)json["x"] <= data.target_2.x &&
      (int)json["y"] <= data.target_2.y &&
      (int)json["x"] + (int)json["width"] >= data.target_2.x &&
      (int)json["y"] + (int)json["height"] >= data.target_2.y)
    return true;
  if (data.target_3.detected &&
      (int)json["x"] <= data.target_3.x &&
      (int)json["y"] <= data.target_3.y &&
      (int)json["x"] + (int)json["width"] >= data.target_3.x &&
      (int)json["y"] + (int)json["height"] >= data.target_3.y)
    return true;
  return false;
}

bool rd_03d::isMoving()
{
  if (data.target_1.detected && abs(data.target_1.speed) > 8)
    return true;
  if (data.target_2.detected && abs(data.target_2.speed) > 8)
    return true;
  if (data.target_3.detected && abs(data.target_3.speed) > 8)
    return true;
  return false;
}

rd_03d::Data *rd_03d::getData()
{
  return &data;
}

void rd_03d::printData(TargetData &data)
{
  Serial.printf("d: %3.0fcm, a: %3.0fdeg, x: %3.0fcm, y: %3.0fcm, speed: %3.0f, res: %3.0fcm\n", data.distance / 10.0, (float)data.angle,
                data.x / 10.0, data.y / 10.0, (float)data.speed, data.resolution / 10.0);
}
String rd_03d::getTargetDataString(TargetData data)
{
  // int x = sensorX + (cos(sensorRotation) * data.y + sin(sensorRotation) * data.x) / 10;
  // int y = sensorY + (sin(sensorRotation) * data.y - cos(sensorRotation) * data.x) / 10;
  String str = "{\"x\":" + String(data.x) + ", \"y\":" + String(data.y) + "}";
  return str;
}
void rd_03d::transformFromSensorSpace(TargetData &target)
{
  int x = sensorX + (cos(sensorRotation) * target.y - sin(sensorRotation) * target.x) / 10;
  int y = sensorY + (sin(sensorRotation) * target.y + cos(sensorRotation) * target.x) / 10;
  target.x = x;
  target.y = y;
}
int16_t convertBufferToInt16(uint8_t *buffer, int index)
{
  int16_t target_tmp = (buffer[index] | (buffer[index + 1] << 8));
  int16_t result = target_tmp & 0x7fff;
  if (!(target_tmp & 0x8000))
    result *= -1;
  return result;
}
void rd_03d::processTargetData(uint8_t *buffer, int index, TargetData &target)
{
  target.x = convertBufferToInt16(buffer, index);
  target.y = (buffer[index + 2] | (buffer[index + 3] << 8)) - 0x8000;
  target.speed = convertBufferToInt16(buffer, index + 4);
  target.resolution = (buffer[index + 6] | (buffer[index + 7] << 8));
  if (buffer[index] | buffer[index + 1] | buffer[index + 2] | buffer[index + 4])
  {
    target.detected = 1;
    target.distance = sqrt(pow(target.x, 2) + pow(target.y, 2));
    target.angle = atan2(target.y, target.x) * 180.0 / PI;
    transformFromSensorSpace(target);
  }
  else
  {
    target.detected = 0;
    target.distance = 0;
    target.angle = 0;
  }
}
void rd_03d::processRadarData()
{
  // output data
  // printBuffer();

  /* RX_BUF: 0xAA 0xFF 0x03 0x00                   Header
   *  0x05 0x01 0x19 0x82 0x00 0x00 0x68 0x01      target1
   *  0xE3 0x81 0x33 0x88 0x20 0x80 0x68 0x01      target2
   *  0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00      target3
   *  0x55 0xCC
   */

  if (RX_count >= 32)
  {
    // Extract data for Target 1
    processTargetData(RX_BUF, 4, data.target_1);
    processTargetData(RX_BUF, 12, data.target_2);
    processTargetData(RX_BUF, 20, data.target_3);
    counter++;
    if (counter >= 3)
    {
      counter = 0;
      if (RX_BUF[4] | RX_BUF[5] | RX_BUF[6] | RX_BUF[7])
      {
        Serial.println("target 1: ");
        printData(data.target_1);
      }
      else
      {
        Serial.println("no target detected");
      }

      if (RX_BUF[12] | RX_BUF[13] | RX_BUF[14] | RX_BUF[15])
      {
        Serial.println("target 2: ");
        printData(data.target_2);
      }
      if (RX_BUF[18] | RX_BUF[19] | RX_BUF[20] | RX_BUF[21])
      {
        Serial.println("target 3: ");
        printData(data.target_3);
      }
    }

    // data.target_1.x = convertBufferToInt16(RX_BUF, 4) - 0x200;
    // data.target_1.y = (RX_BUF[6] | (RX_BUF[7] << 8)) - 0x8000;
    // data.target_1.speed = convertBufferToInt16(RX_BUF, 8) - 0x10;
    // data.target_1.resolution = (RX_BUF[10] | (RX_BUF[11] << 8));
    // if (data.target_1.x == 0 && data.target_1.y == 0 && data.target_1.speed == 0 && data.target_1.resolution == 0)
    // {
    //   data.target_1.detected = 0;
    //   data.target_1.distance = 0;
    //   data.target_1.angle = 0;
    // }
    // else
    // {
    //   data.target_1.detected = 1;
    //   data.target_1.distance = sqrt(pow(data.target_1.x, 2) + pow(data.target_1.y, 2));
    //   data.target_1.angle = atan2(data.target_1.y, data.target_1.x) * 180.0 / PI;
    // }
    // // Extract data for Target 2
    // data.target_2.x = convertBufferToInt16(RX_BUF, 12) - 0x200;
    // data.target_2.y = (RX_BUF[14] | (RX_BUF[15] << 8)) - 0x8000;
    // data.target_2.speed = convertBufferToInt16(RX_BUF, 16) - 0x10;
    // data.target_2.resolution = (RX_BUF[18] | (RX_BUF[19] << 8));
    // if (data.target_2.x == 0 && data.target_2.y == 0 && data.target_2.speed == 0 && data.target_2.resolution == 0)
    // {
    //   data.target_2.detected = 0;
    //   data.target_2.distance = 0;
    //   data.target_2.angle = 0;
    // }
    // else
    // {
    //   data.target_2.detected = 1;
    //   data.target_2.distance = sqrt(pow(data.target_2.x, 2) + pow(data.target_2.y, 2));
    //   data.target_2.angle = atan2(data.target_2.y, data.target_2.x) * 180.0 / PI;
    // }
    // // Extract data for Target 3
    // data.target_3.x = convertBufferToInt16(RX_BUF, 20) - 0x200;
    // data.target_3.y = (RX_BUF[22] | (RX_BUF[23] << 8)) - 0x8000;
    // data.target_3.speed = convertBufferToInt16(RX_BUF, 24) - 0x10;
    // data.target_3.resolution = (RX_BUF[26] | (RX_BUF[27] << 8));
    // if (data.target_3.x == 0 && data.target_3.y == 0 && data.target_3.speed == 0 && data.target_3.resolution == 0)
    // {
    //   data.target_3.detected = 0;
    //   data.target_3.distance = 0;
    //   data.target_3.angle = 0;
    // }
    // else
    // {
    //   data.target_3.detected = 1;
    //   data.target_3.distance = sqrt(pow(data.target_3.x, 2) + pow(data.target_3.y, 2));
    //   data.target_3.angle = atan2(data.target_3.y, data.target_3.x) * 180.0 / PI;
    // }
    // Reset buffer and counter
    memset(RX_BUF, 0x00, sizeof(RX_BUF));
    RX_count = 0;
    data.isNewData = 1;
  }
}
