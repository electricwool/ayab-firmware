#ifndef ADS1015_H
#define ADS1015_H

#include <Arduino.h>
#include "hal.h"

class Ads1015 {
 public:
  // I2C Address options (ADDR pin configuration)
  static constexpr uint8_t ADDR_GND = 0x48;  // ADDR -> GND
  static constexpr uint8_t ADDR_VDD = 0x49;  // ADDR -> VDD
  static constexpr uint8_t ADDR_SDA = 0x4A;  // ADDR -> SDA
  static constexpr uint8_t ADDR_SCL = 0x4B;  // ADDR -> SCL

  // Register addresses
  static constexpr uint8_t REG_CONVERSION = 0x00;
  static constexpr uint8_t REG_CONFIG     = 0x01;
  static constexpr uint8_t REG_LO_THRESH  = 0x02;
  static constexpr uint8_t REG_HI_THRESH  = 0x03;

  // Programmable Gain Amplifier (PGA) settings
  enum class Gain : uint16_t {
    GAIN_TWOTHIRDS = 0x0000,  // +/- 6.144V range (2/3x gain)
    GAIN_ONE       = 0x0200,  // +/- 4.096V range (1x gain)
    GAIN_TWO       = 0x0400,  // +/- 2.048V range (2x gain, default)
    GAIN_FOUR      = 0x0600,  // +/- 1.024V range (4x gain)
    GAIN_EIGHT     = 0x0800,  // +/- 0.512V range (8x gain)
    GAIN_SIXTEEN   = 0x0A00   // +/- 0.256V range (16x gain)
  };

  // Data rate settings
  enum class DataRate : uint16_t {
    SPS_128  = 0x0000,  // 128 samples per second
    SPS_250  = 0x0020,  // 250 samples per second
    SPS_490  = 0x0040,  // 490 samples per second
    SPS_920  = 0x0060,  // 920 samples per second
    SPS_1600 = 0x0080,  // 1600 samples per second (default)
    SPS_2400 = 0x00A0,  // 2400 samples per second
    SPS_3300 = 0x00C0   // 3300 samples per second
  };

  // Operating mode
  enum class Mode : uint16_t {
    CONTINUOUS = 0x0000,  // Continuous conversion mode
    SINGLE     = 0x0100   // Single-shot mode (default)
  };

  // Comparator mode
  enum class ComparatorMode : uint16_t {
    TRADITIONAL = 0x0000,  // Traditional comparator
    WINDOW      = 0x0010   // Window comparator
  };

  // Comparator polarity
  enum class ComparatorPolarity : uint16_t {
    ACTIVE_LOW  = 0x0000,  // Active low (default)
    ACTIVE_HIGH = 0x0008   // Active high
  };

  // Comparator latching
  enum class ComparatorLatch : uint16_t {
    NON_LATCHING = 0x0000,  // Non-latching (default)
    LATCHING     = 0x0004   // Latching
  };

  // Comparator queue
  enum class ComparatorQueue : uint16_t {
    QUEUE_1 = 0x0000,  // Assert after 1 conversion
    QUEUE_2 = 0x0001,  // Assert after 2 conversions
    QUEUE_4 = 0x0002,  // Assert after 4 conversions
    DISABLE = 0x0003   // Disable comparator (default)
  };

  // Multiplexer configuration for differential/single-ended reads
  enum class Mux : uint16_t {
    DIFF_0_1 = 0x0000,  // Differential: AIN0 - AIN1 (default)
    DIFF_0_3 = 0x1000,  // Differential: AIN0 - AIN3
    DIFF_1_3 = 0x2000,  // Differential: AIN1 - AIN3
    DIFF_2_3 = 0x3000,  // Differential: AIN2 - AIN3
    SINGLE_0 = 0x4000,  // Single-ended: AIN0
    SINGLE_1 = 0x5000,  // Single-ended: AIN1
    SINGLE_2 = 0x6000,  // Single-ended: AIN2
    SINGLE_3 = 0x7000   // Single-ended: AIN3
  };

  Ads1015(hardwareAbstraction::HalInterface *hal, uint8_t i2cAddress = ADDR_GND);
  ~Ads1015() = default;

  // Basic ADC operations
  bool begin();
  int16_t readADC(uint8_t channel);
  int16_t readADC_Differential_0_1();
  int16_t readADC_Differential_0_3();
  int16_t readADC_Differential_1_3();
  int16_t readADC_Differential_2_3();
  
  // Voltage conversion
  float readVoltage(uint8_t channel);
  float computeVolts(int16_t counts);

  // Configuration
  void setGain(Gain gain);
  Gain getGain();
  void setDataRate(DataRate rate);
  void setMode(Mode mode);

  // Comparator setup
  void startComparator_SingleEnded(uint8_t channel, int16_t threshold);
  void setComparatorThresholds(int16_t low, int16_t high);
  int16_t getLastConversionResults();

 private:
  hardwareAbstraction::HalInterface *_hal;
  uint8_t _i2cAddress;
  Gain _gain;
  DataRate _dataRate;
  Mode _mode;

  // Low-level I2C operations
  void writeRegister(uint8_t reg, uint16_t value);
  uint16_t readRegister(uint8_t reg);
  int16_t readADC_SingleEnded(uint8_t channel);
  int16_t readADC_Differential(Mux mux);
};

#endif // ADS1015_H
