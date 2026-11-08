# Poppy

Hamster-wheel tracker: an ESP32 with a hall-effect sensor counts wheel spins
and publishes them to AWS IoT Core, which forwards them to Lambda and DynamoDB.

## Layout

- `firmware/hall_led/`: ESP32 Arduino sketch
- `lambdas/`: AWS Lambda functions
- `infra/`: IoT rule, IAM policies, DynamoDB schema
- `docs/`: architecture notes

## Firmware setup

1. `cp firmware/hall_led/secrets.h.example firmware/hall_led/secrets.h`
2. Fill in WiFi credentials, the AWS IoT endpoint and the device certificates.
3. Open `firmware/hall_led/hall_led.ino` in the Arduino IDE (ESP32 core + PubSubClient) and upload.
