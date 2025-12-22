#ifndef I2C_H
#define I2C_H

#include "hal.h"

namespace hardwareAbstraction {

class i2c : public I2cInterface {
 public:
  i2c();
  ~i2c() = default;

  bool detect(uint8_t device) override;
  uint8_t read(uint8_t device, uint8_t address) override;
  void write(uint8_t device, uint8_t value) override;
  void write(uint8_t device, uint8_t address, uint8_t value) override;
  uint16_t read16(uint8_t device, uint8_t address) override;
  void write16(uint8_t device, uint8_t address, uint16_t value) override;
};
}  // namespace hardwareAbstraction

#endif
