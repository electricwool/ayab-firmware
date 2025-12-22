#include "ads1015.h"

// Config register bit masks
#define ADS1015_REG_CONFIG_OS_SINGLE    0x8000  // Write: Set to start a single-conversion
#define ADS1015_REG_CONFIG_OS_BUSY      0x0000  // Read: Bit = 0 when conversion is in progress
#define ADS1015_REG_CONFIG_OS_NOTBUSY   0x8000  // Read: Bit = 1 when device is not performing a conversion

#define ADS1015_REG_CONFIG_MODE_MASK    0x0100
#define ADS1015_REG_CONFIG_DR_MASK      0x00E0
#define ADS1015_REG_CONFIG_CMODE_MASK   0x0010
#define ADS1015_REG_CONFIG_CPOL_MASK    0x0008
#define ADS1015_REG_CONFIG_CLAT_MASK    0x0004
#define ADS1015_REG_CONFIG_CQUE_MASK    0x0003

Ads1015::Ads1015(hardwareAbstraction::HalInterface *hal, uint8_t i2cAddress)
    : _hal(hal),
      _i2cAddress(i2cAddress),
      _gain(Gain::GAIN_TWO),
      _dataRate(DataRate::SPS_1600),
      _mode(Mode::SINGLE) {
}

bool Ads1015::begin() {
  // Check if device is present on I2C bus
  return _hal->i2c->detect(_i2cAddress);
}

void Ads1015::setGain(Gain gain) {
  _gain = gain;
}

Ads1015::Gain Ads1015::getGain() {
  return _gain;
}

void Ads1015::setDataRate(DataRate rate) {
  _dataRate = rate;
}

void Ads1015::setMode(Mode mode) {
  _mode = mode;
}

int16_t Ads1015::readADC(uint8_t channel) {
  if (channel > 3) {
    return 0;
  }
  return readADC_SingleEnded(channel);
}

int16_t Ads1015::readADC_Differential_0_1() {
  return readADC_Differential(Mux::DIFF_0_1);
}

int16_t Ads1015::readADC_Differential_0_3() {
  return readADC_Differential(Mux::DIFF_0_3);
}

int16_t Ads1015::readADC_Differential_1_3() {
  return readADC_Differential(Mux::DIFF_1_3);
}

int16_t Ads1015::readADC_Differential_2_3() {
  return readADC_Differential(Mux::DIFF_2_3);
}

float Ads1015::readVoltage(uint8_t channel) {
  int16_t raw = readADC(channel);
  return computeVolts(raw);
}

float Ads1015::computeVolts(int16_t counts) {
  // ADS1015 is 12-bit, left-aligned in 16-bit result
  // Shift right by 4 bits to get actual 12-bit value
  float fsRange;
  
  switch (_gain) {
    case Gain::GAIN_TWOTHIRDS:
      fsRange = 6.144f;
      break;
    case Gain::GAIN_ONE:
      fsRange = 4.096f;
      break;
    case Gain::GAIN_TWO:
      fsRange = 2.048f;
      break;
    case Gain::GAIN_FOUR:
      fsRange = 1.024f;
      break;
    case Gain::GAIN_EIGHT:
      fsRange = 0.512f;
      break;
    case Gain::GAIN_SIXTEEN:
      fsRange = 0.256f;
      break;
    default:
      fsRange = 2.048f;
  }
  
  // ADS1015 is 12-bit: -2048 to +2047
  // But the value is left-aligned in 16-bit, so we shift right by 4
  return (counts >> 4) * (fsRange / 2048.0f);
}

void Ads1015::startComparator_SingleEnded(uint8_t channel, int16_t threshold) {
  if (channel > 3) {
    return;
  }

  // Set high threshold
  writeRegister(REG_HI_THRESH, threshold << 4);
  // Set low threshold to 0
  writeRegister(REG_LO_THRESH, 0);

  // Build config with comparator enabled
  uint16_t config = ADS1015_REG_CONFIG_OS_SINGLE;  // Start single conversion
  
  // Set mux for single-ended channel
  config |= static_cast<uint16_t>(Mux::SINGLE_0) + (channel << 12);
  config |= static_cast<uint16_t>(_gain);
  config |= static_cast<uint16_t>(Mode::CONTINUOUS);  // Continuous mode for comparator
  config |= static_cast<uint16_t>(_dataRate);
  config |= static_cast<uint16_t>(ComparatorMode::TRADITIONAL);
  config |= static_cast<uint16_t>(ComparatorPolarity::ACTIVE_LOW);
  config |= static_cast<uint16_t>(ComparatorLatch::NON_LATCHING);
  config |= static_cast<uint16_t>(ComparatorQueue::QUEUE_1);  // Assert after 1 conversion

  writeRegister(REG_CONFIG, config);
}

void Ads1015::setComparatorThresholds(int16_t low, int16_t high) {
  // Thresholds are 12-bit values, left-aligned in 16-bit registers
  writeRegister(REG_LO_THRESH, low << 4);
  writeRegister(REG_HI_THRESH, high << 4);
}

int16_t Ads1015::getLastConversionResults() {
  return static_cast<int16_t>(readRegister(REG_CONVERSION));
}

int16_t Ads1015::readADC_SingleEnded(uint8_t channel) {
  if (channel > 3) {
    return 0;
  }

  // Build configuration word
  uint16_t config = ADS1015_REG_CONFIG_OS_SINGLE;  // Start single conversion
  
  // Set mux for single-ended channel
  config |= static_cast<uint16_t>(Mux::SINGLE_0) + (channel << 12);
  config |= static_cast<uint16_t>(_gain);
  config |= static_cast<uint16_t>(_mode);
  config |= static_cast<uint16_t>(_dataRate);
  config |= static_cast<uint16_t>(ComparatorQueue::DISABLE);  // Disable comparator

  // Write config to start conversion
  writeRegister(REG_CONFIG, config);

  // Wait for conversion to complete (in single-shot mode)
  if (_mode == Mode::SINGLE) {
    // Poll the conversion ready bit
    // For ADS1015 at max rate (3300 SPS), conversion takes ~303us
    // Add some margin
    _hal->delayMicroseconds(1000);
    
    // Poll until conversion is complete
    while ((readRegister(REG_CONFIG) & ADS1015_REG_CONFIG_OS_NOTBUSY) == 0) {
      _hal->delayMicroseconds(100);
    }
  }

  // Read conversion result
  return static_cast<int16_t>(readRegister(REG_CONVERSION));
}

int16_t Ads1015::readADC_Differential(Mux mux) {
  // Build configuration word
  uint16_t config = ADS1015_REG_CONFIG_OS_SINGLE;  // Start single conversion
  
  config |= static_cast<uint16_t>(mux);
  config |= static_cast<uint16_t>(_gain);
  config |= static_cast<uint16_t>(_mode);
  config |= static_cast<uint16_t>(_dataRate);
  config |= static_cast<uint16_t>(ComparatorQueue::DISABLE);  // Disable comparator

  // Write config to start conversion
  writeRegister(REG_CONFIG, config);

  // Wait for conversion to complete (in single-shot mode)
  if (_mode == Mode::SINGLE) {
    _hal->delayMicroseconds(1000);
    
    // Poll until conversion is complete
    while ((readRegister(REG_CONFIG) & ADS1015_REG_CONFIG_OS_NOTBUSY) == 0) {
      _hal->delayMicroseconds(100);
    }
  }

  // Read conversion result
  return static_cast<int16_t>(readRegister(REG_CONVERSION));
}

void Ads1015::writeRegister(uint8_t reg, uint16_t value) {
  _hal->i2c->write16(_i2cAddress, reg, value);
}

uint16_t Ads1015::readRegister(uint8_t reg) {
  return _hal->i2c->read16(_i2cAddress, reg);
}
