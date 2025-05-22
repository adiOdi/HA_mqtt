
#include <Arduino.h>
#define BAUD_RATE 256000

#define RX_PIN 16
#define TX_PIN 17

const uint8_t Multi_Target_Detection_CMD[12] = {0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 0x90, 0x00, 0x04, 0x03, 0x02, 0x01};
const uint8_t Single_Target_Detection_CMD[12] = {0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 0x80, 0x00, 0x04, 0x03, 0x02, 0x01};
const uint8_t OPEN_COMMAND[14] = {0xFD, 0xFC, 0xFB, 0xFA, 0x04, 0x00, 0xFF, 0x00, 0x01, 0x00, 0x04, 0x03, 0x02, 0x01};
const uint8_t RADAR_ACK_OPEN[18] = {0xFD, 0xFC, 0xFB, 0xFA, 0x08, 0x00, 0xFF, 0x01, 0x00, 0x00, 0x01, 0x00, 0x40, 0x00, 0x04, 0x03, 0x02, 0x01};
const uint8_t CLOSE_COMMAND[12] = {0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 0xFE, 0x00, 0x04, 0x03, 0x02, 0x01};
const uint8_t RADAR_ACK_CLOSE[14] = {0xFD, 0xFC, 0xFB, 0xFA, 0x04, 0x00, 0xFE, 0x01, 0x00, 0x00, 0x04, 0x03, 0x02, 0x01};

uint8_t RX_BUF[64] = {0};
uint8_t RX_count = 0;
uint8_t RX_temp = 0;

bool equal(uint8_t *a, const uint8_t *b)
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

void readMessage()
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
  Serial.print("reading: ");
  if (equal(RX_BUF, RADAR_ACK_OPEN))
    Serial.print("GOT RADAR_ACK_OPEN: ");
  if (equal(RX_BUF, RADAR_ACK_CLOSE))
    Serial.print("GOT RADAR_ACK_CLOSE: ");
  for (size_t i = 0; i < RX_count; i++)
  {
    Serial.printf("%#04X ", RX_BUF[i]);
  }
  Serial.println();
  delay(200);
  Serial.flush();
  RX_count = 0;
}

void sendMessage(const uint8_t *message)
{
  Serial1.write(message, 10 + message[4]);
  delay(200);
  Serial1.flush();
}
void setup()
{
  Serial.begin(BAUD_RATE);
  // initialize LED digital pin as an output.
  // pinMode(LED_BUILTIN, OUTPUT);
  Serial1.setRxBufferSize(64);
  Serial1.begin(BAUD_RATE, SERIAL_8N1, RX_PIN, TX_PIN);

  Serial.println("\nopen.");
  sendMessage(OPEN_COMMAND);
  readMessage();
  Serial.println("Multi_Target_Detection_CMD:");
  sendMessage(Multi_Target_Detection_CMD);
  readMessage();
  Serial.println("close");
  sendMessage(CLOSE_COMMAND);
  readMessage();
}

void loop()
{
}
