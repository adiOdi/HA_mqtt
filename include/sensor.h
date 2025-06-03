#if !defined(SENSOR_H)
#define SENSOR_H
#include <Arduino.h>
#include <cstdint>
#include <ArduinoJson.h>

class rd_03d
{
public:
  typedef struct
  {
    bool detected;
    float distance;
    float angle;
    int16_t x;
    int16_t y;
    int16_t speed;
    uint16_t resolution;
  } TargetData;
  typedef struct
  {
    TargetData target_1;
    TargetData target_2;
    TargetData target_3;
    bool isNewData;
  } Data;
  void setup(const int rx_pin, const int tx_pin);
  String getTargetDataString(TargetData data);
  void setup();
  void update();
  Data *getData();
  inline void setSensor(int x, int y, float rotation)
  {
    sensorX = x;
    sensorY = y;
    sensorRotation = rotation;
  }
  void printData(TargetData &data);
  void transformFromSensorSpace(TargetData &target);
  bool isOccupied(JsonDocument json);
  bool isMoving();

private:
  void processTargetData(uint8_t *buffer, int index, TargetData &target);
  void processRadarData();
  bool equal(uint8_t *a, const uint8_t *b);
  void sendMessage(const uint8_t *message);
  Data data = {};
  uint8_t RX_BUF[64] = {0};
  uint8_t RX_count = 0;
  uint8_t RX_temp = 0;
  float sensorRotation;
  int sensorX, sensorY;
  int counter;
  const uint8_t Multi_Target_Detection_CMD[12] = {0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 0x90, 0x00, 0x04, 0x03, 0x02, 0x01};
  const uint8_t Single_Target_Detection_CMD[12] = {0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 0x80, 0x00, 0x04, 0x03, 0x02, 0x01};
  const uint8_t OPEN_COMMAND[14] = {0xFD, 0xFC, 0xFB, 0xFA, 0x04, 0x00, 0xFF, 0x00, 0x01, 0x00, 0x04, 0x03, 0x02, 0x01};
  const uint8_t RADAR_ACK_OPEN[18] = {0xFD, 0xFC, 0xFB, 0xFA, 0x08, 0x00, 0xFF, 0x01, 0x00, 0x00, 0x01, 0x00, 0x40, 0x00, 0x04, 0x03, 0x02, 0x01};
  const uint8_t CLOSE_COMMAND[12] = {0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 0xFE, 0x00, 0x04, 0x03, 0x02, 0x01};
  const uint8_t RADAR_ACK_CLOSE[14] = {0xFD, 0xFC, 0xFB, 0xFA, 0x04, 0x00, 0xFE, 0x01, 0x00, 0x00, 0x04, 0x03, 0x02, 0x01};
  const uint8_t RADAR_ACK_MULTI[14] = {0xFD, 0xFC, 0xFB, 0xFA, 0x04, 0x00, 0x90, 0x01, 0x00, 0x00, 0x04, 0x03, 0x02, 0x01};
  typedef enum
  {
    ACK_CLOSE,
    ACK_MULTI,
    ACK_OPEN,
    NOACK
  } radar_ack_message;
  radar_ack_message readMessage();
  // Single-Target Detection Command
  // const uint8_t Single_Target_Detection_CMD[12] = {0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 0x80, 0x00, 0x04, 0x03, 0x02, 0x01};
  // Multi-Target Detection Command
  // const uint8_t Multi_Target_Detection_CMD[12] = {0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 0x90, 0x00, 0x04, 0x03, 0x02, 0x01};
};

#endif // SENSOR_H
