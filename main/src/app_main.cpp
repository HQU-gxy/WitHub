#include <sdkconfig.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_spi_flash.h>
#include <etl/random.h>
#include <msd/channel.hpp>
#include <memory>
#include <nvs_handle.hpp>
#include <NimBLEDevice.h>
#include <iostream>
#include <utility>
#include "scan_callback.h"

const auto BLE_NAME = "WitHub";

extern "C" [[noreturn]] void app_main();

[[noreturn]] void app_main() {
  const auto TAG = "main";
  /******** BLE init ********/
  NimBLEDevice::init(BLE_NAME);
  auto &scan    = *NimBLEDevice::getScan();
  auto &scan_cb = *new blue::ScanCallback();
  scan.setScanCallbacks(&scan_cb);
  scan.setInterval(1349);
  scan.setWindow(449);
  scan.setActiveScan(true);
  constexpr auto scanTime = std::chrono::milliseconds(2500);
  // total time = scan time + sleep time
  constexpr auto scanTotalTime = std::chrono::milliseconds(5000);
  static_assert(scanTotalTime > scanTime);
  ESP_LOGI(TAG, "Initiated");
  for (;;) {
    bool ok = scan.start(scanTime.count(), false);
    if (!ok) {
      ESP_LOGW(TAG, "bad scan");
    }
    vTaskDelay(scanTotalTime.count() / portTICK_PERIOD_MS);
  }
}
