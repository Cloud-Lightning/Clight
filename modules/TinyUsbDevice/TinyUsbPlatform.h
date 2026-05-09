#ifndef TINYUSB_DEVICE_PLATFORM_H_
#define TINYUSB_DEVICE_PLATFORM_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool tinyusb_platform_init(uint8_t rhport);
bool tinyusb_platform_init_host(uint8_t rhport);
void tinyusb_platform_after_init(void);
size_t tinyusb_platform_get_serial(uint16_t desc_str[], size_t max_chars);

#ifdef __cplusplus
}
#endif

#endif
