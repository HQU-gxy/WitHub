#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_spi_flash.h"
#include "etl/random.h"
#include "msd/channel.hpp"
#include <memory>
#include <nvs_handle.hpp>
#include "NimBLEDevice.h"
#include <iostream>
#include <utility>

extern "C" void app_main();

void app_main() {
  const auto TAG = "main";
  ESP_LOGI(TAG, "Hello world!");
  vTaskDelete(nullptr);
}
