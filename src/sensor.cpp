#include "sensor.h"
#include <Arduino.h>
// #include <cmath>
// #include <cstring>

#define BAUD_RATE 256000
bool rd_03d::equal(uint8_t *a, const uint8_t *b)
{
  size_t n = 5;
  for (size_t i = 0; i < n; i++)
  {
    if (a[i] != b[i])
    {
      // Serial.println(i);
      return false;
    }

    if (i == 4)
      n = 10 + a[4];
  }
  return true;
}

rd_03d::radar_ack_message rd_03d::readMessage()
{
  while (Serial1.available())
  {
    RX_temp = Serial1.read();
    RX_BUF[RX_count++] = RX_temp;

    // Prevent buffer overflow
    if (RX_count >= sizeof(RX_BUF))
    {
      RX_count = sizeof(RX_BUF) - 1;
    }
  }

  if (equal(RX_BUF, RADAR_ACK_OPEN))
  {
    Serial.println("GOT RADAR_ACK_OPEN");
    RX_count = 0;
    return ACK_OPEN;
  }
  if (equal(RX_BUF, RADAR_ACK_CLOSE))
  {
    Serial.println("GOT RADAR_ACK_CLOSE");
    RX_count = 0;
    return ACK_CLOSE;
  }
  if (equal(RX_BUF, RADAR_ACK_MULTI))
  {
    Serial.println("GOT RADAR_ACK_MULTI");
    RX_count = 0;
    return ACK_MULTI;
  }
  Serial.print("reading: ");
  for (size_t i = 0; i < RX_count; i++)
  {
    Serial.printf("%#04X ", RX_BUF[i]);
  }
  Serial.println();
  delay(200);
  Serial.flush();
  RX_count = 0;
  return NOACK;
}

void rd_03d::sendMessage(const uint8_t *message)
{
  Serial1.write(message, 10 + message[4]);
  delay(200);
  Serial1.flush();
}
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

  do
  {
    Serial.println("\nOPEN_COMMAND");
    sendMessage(OPEN_COMMAND);
  } while (readMessage() != ACK_OPEN);
  do
  {
    Serial.println("Multi_Target_Detection_CMD");
    sendMessage(Multi_Target_Detection_CMD);
  } while (readMessage() != ACK_MULTI);
  do
  {
    Serial.println("CLOSE_COMMAND");
    sendMessage(CLOSE_COMMAND);
  } while (readMessage() != ACK_CLOSE);
  // Send multi-target detection command
  // Serial1.write(Multi_Target_Detection_CMD, sizeof(Multi_Target_Detection_CMD));
  // delay(200);
  // Serial.println("Multi-target detection mode activated.");

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
    // Serial.printf("%#04x ", RX_temp);

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
  if (data.target_1.detected && abs(data.target_1.speed) > 0)
    return true;
  if (data.target_2.detected && abs(data.target_2.speed) > 0)
    return true;
  if (data.target_3.detected && abs(data.target_3.speed) > 0)
    return true;
  return false;
}
int rd_03d::movement()
{
  return abs(data.target_1.speed) + abs(data.target_2.speed) + abs(data.target_3.speed);
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
    // Reset buffer and counter
    memset(RX_BUF, 0x00, sizeof(RX_BUF));
    RX_count = 0;
    data.isNewData = 1;
  }
}
