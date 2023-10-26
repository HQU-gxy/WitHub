Need certain IDF version ([v5.1.1](https://github.com/espressif/esp-idf/tree/v5.1.1)) to build the project.

In your IDF directory.

```bash
cd $IDF_PATH
# I assume your origin is 
# https://github.com/espressif/esp-idf.git
git fetch origin v5.1.1
git checkout v5.1.1
git submodule update --init --recursive
./install.sh
source export.sh
```

I could make full use of the flash... See also [Partition Tables](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/partition-tables.html).

## Wit

- [WITMOTION/WitBluetooth_BWT901BLE5_0](https://github.com/WITMOTION/WitBluetooth_BWT901BLE5_0)
- [蓝牙5.0通讯协议](https://wit-motion.yuque.com/wumwnr/docs/gpare3)
