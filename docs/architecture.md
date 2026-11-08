# Architecture

```
ESP32 + hall sensor
   │  MQTT over TLS (topic: poppy-wheel)
   ▼
AWS IoT Core ── rule: infra/iot-rule.sql
   ▼
Lambda: poppy-wheel-ingest
   ▼
DynamoDB (DeviceId / SK)
   ▼
Lambda: strava-poster (planned)
```

See also `Project_Poppy.drawio` in the repo root.
