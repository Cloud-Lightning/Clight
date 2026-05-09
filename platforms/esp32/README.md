# ESP32 Platform Template

The ESP32 template is intentionally minimal.

To build a real ESP32 project:

1. Activate ESP-IDF locally.
2. Run `idf.py set-target esp32s3` or the target required by your board.
3. Add product components and services locally.
4. Keep `sdkconfig`, `build/`, credentials, certificates, and local managed components out of the public framework repository.

Wi-Fi, BLE, HTTP, and provisioning logic belong in modules or product services, not in the basic GPIO/SPI/UART API.
