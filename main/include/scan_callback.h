//
// Created by Kurosu Chan on 2023/10/26.
//

#ifndef WIT_HUB_SCAN_CALLBACK_H
#define WIT_HUB_SCAN_CALLBACK_H

#include <NimBLEDevice.h>

class ScanCallback : public NimBLEScanCallbacks {
public:
  void onResult(NimBLEAdvertisedDevice *advertisedDevice) override{

  };
};

#endif // WIT_HUB_SCAN_CALLBACK_H
