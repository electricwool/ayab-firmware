/*!
 * \file ads1015hall.h
 * \brief ADS1015-based Hall Effect Sensor Driver for AYAB Firmware
 * 
 * This module implements support for dual analog hall effect sensors using
 * the ADS1015 12-bit I2C ADC with hardware comparator and ALERT pin.
 * 
 * Hardware Configuration:
 *   - Two bipolar hall effect sensors (left and right position sensors)
 *   - Each sensor outputs differential analog voltage based on magnetic pole
 *   - ADS1015 reads differential voltages and triggers ALERT on threshold crossing
 *   - Microcontroller reads values over I2C when ALERT is asserted
 * 
 * ADS1015 Configuration:
 *   - Gain: 2/3x (±6.144V range) for widest voltage detection
 *   - Data Rate: 3300 SPS (maximum speed for fast carriage detection)
 *   - Mode: Continuous conversion with comparator
 *   - Comparator: Window mode with configurable thresholds
 *   - Alert Pin: Active LOW when voltage crosses thresholds
 * 
 * Voltage Thresholds:
 *   - Configured via pin_definitions.json in human-readable format (e.g., 2.50V, -1.25V)
 *   - preBuild.py converts to 12-bit ADS1015 values during compilation
 *   - Supports both positive and negative voltages (bipolar sensors)
 * 
 * Carriage Detection:
 *   - K (Knit): Single North pole - positive voltage spike
 *   - L (Lace): Single South pole - negative voltage spike  
 *   - G (Garter): South then North - negative followed by positive spikes
 * 
 * \author AYAB Contributors
 * \copyright GPL-3.0 License
 */

#ifndef ADS1015HALL_H
#define ADS1015HALL_H

#include "api.h"
#include "encoder.h"
#include <Arduino.h>
#include "hal.h"
#include "../../devices/ads1015/ads1015.h"

// Forward declarations
class Encoder;

/*!
 * \brief ADS1015-based Hall Sensor Driver
 * 
 * Implements async hall sensor reading using ADS1015 ADC with ALERT pin.
 * Compatible with existing HallSensor interface for drop-in replacement.
 */
class Ads1015Hall {
 public:
  /*!
   * \brief Configuration for ADS1015 hall sensors
   */
  class Config {
   public:
    int16_t position;           ///< Sensor position on needle bed
    int16_t thresholdLow;       ///< Low threshold (12-bit ADS1015 value)
    int16_t thresholdHigh;      ///< High threshold (12-bit ADS1015 value)
    uint8_t flags;              ///< Sensor flags (future use)
  };

  /*!
   * \brief Constructor for ADS1015 hall sensor
   * 
   * \param hal Hardware abstraction layer interface
   * \param ads1015 Pointer to configured ADS1015 device instance
   * \param alertPin GPIO pin connected to ADS1015 ALERT/RDY
   * \param channel ADS1015 input channel (0-3 for single-ended, or differential pair)
   * \param isDifferential True if using differential input
   */
  Ads1015Hall(hardwareAbstraction::HalInterface *hal, 
              Ads1015 *ads1015,
              uint8_t alertPin,
              uint8_t channel,
              bool isDifferential = true);
  
  ~Ads1015Hall() = default;

  /*!
   * \brief Initialize ADS1015 and configure comparator thresholds
   */
  void init();

  /*!
   * \brief Configure sensor position and thresholds
   * 
   * \param config Pointer to configuration structure
   */
  void config(Config *config);

  /*!
   * \brief Get last sensor reading
   * 
   * \return Raw 12-bit ADC value (left-aligned in 16-bit, so appears as 16-bit value)
   */
  uint16_t getSensorValue();

  /*!
   * \brief Check if sensor is currently active (magnet detected)
   * 
   * \return True if within detection window
   */
  bool isActive();

  /*!
   * \brief Get configured sensor position
   * 
   * \return Sensor position on needle bed
   */
  int16_t getSensorPosition();

  /*!
   * \brief Get detected carriage position
   * 
   * \return Encoder position where carriage was detected
   */
  int16_t getDetectedPosition();

  /*!
   * \brief Get detected carriage type
   * 
   * \return Type of carriage detected
   */
  CarriageType getDetectedCarriage();

  /*!
   * \brief Get belt phase at detection
   * 
   * \return True if belt was in shifted phase
   */
  bool getDetectedBeltPhase();

  /*!
   * \brief Run detection state machine (call from main loop)
   * 
   * Checks ALERT pin and reads sensor values when triggered.
   * Runs carriage detection algorithm.
   * 
   * \param encoder Encoder instance for position tracking
   * \param direction Current carriage direction
   * \param beltPhase Current belt phase
   * \return True if carriage was detected
   */
  bool isDetected(Encoder *encoder, Direction direction, bool beltPhase);

  /*!
   * \brief ISR handler for ALERT pin interrupt
   * 
   * Sets doorbell flag for main loop processing.
   */
  void alertISR();

 private:
  /*!
   * \brief Detection state machine states
   */
  enum class State { 
    ST_IDLE,    ///< Waiting for initial detection
    ST_HUNT,    ///< Tracking magnet passage
    ST_ESCAPE   ///< Waiting for carriage to leave
  };

  /*!
   * \brief Extremum tracking for carriage identification
   */
  class Extremum {
   public:
    int16_t value;      ///< ADC value at extremum
    int16_t position;   ///< Encoder position at extremum
    bool isFirst;       ///< True if this was detected first
  };

  /*!
   * \brief Read current sensor value from ADS1015
   */
  void readSensor();

  /*!
   * \brief Reset detection state machine
   */
  void resetDetector();

  /*!
   * \brief Analyze extrema to identify carriage type
   * 
   * \return True if carriage was successfully identified
   */
  bool detectCarriage();

  /*!
   * \brief Update ADS1015 comparator thresholds
   */
  void updateComparatorThresholds();

  // Hardware interface
  hardwareAbstraction::HalInterface *_hal;
  Ads1015 *_ads1015;
  uint8_t _alertPin;
  uint8_t _channel;
  bool _isDifferential;

  // Configuration
  Config *_config;

  // Current sensor reading
  int16_t _sensorValue;
  volatile bool _alertTriggered;  // Set by ISR, cleared by main loop

  // Detection state
  State _state;
  int16_t _detectedPosition;
  CarriageType _detectedCarriage;
  bool _detectedBeltPhase;
  Direction _detectedDirection;

  // Extremum tracking
  Extremum _minimum;
  Extremum _maximum;
  uint8_t _needlesToGo;

  // Constants
  static constexpr uint8_t MAX_DET_NEEDLES = 3;
  static constexpr int16_t NONE = 0x7FFF;  // Use max positive int16 as "none" marker
};

#endif // ADS1015HALL_H
