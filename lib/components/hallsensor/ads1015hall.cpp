/*!
 * \file ads1015hall.cpp
 * \brief ADS1015-based Hall Effect Sensor Driver Implementation
 * 
 * Platform-agnostic implementation using HAL abstraction.
 * Compatible with existing HallSensor interface.
 */

#include "ads1015hall.h"
#include <stdlib.h>
#include "../../platform/common/atomic.h"

// Detection window constants
#define MAX_DET_NEEDLES 3
#define NONE 0x7FFF

// Global instance for ISR access
static Ads1015Hall *ads1015HallInstance = nullptr;

// Static ISR wrapper
static void ads1015AlertISRWrapper() {
  if (ads1015HallInstance != nullptr) {
    ads1015HallInstance->alertISR();
  }
}

Ads1015Hall::Ads1015Hall(hardwareAbstraction::HalInterface *hal, 
                         Ads1015 *ads1015,
                         uint8_t alertPin,
                         uint8_t channel,
                         bool isDifferential)
    : _hal(hal),
      _ads1015(ads1015),
      _alertPin(alertPin),
      _channel(channel),
      _isDifferential(isDifferential),
      _config(nullptr),
      _sensorValue(0),
      _alertTriggered(false) {
  
  // Set global instance for ISR access
  ads1015HallInstance = this;
  
  resetDetector();
}

void Ads1015Hall::init() {
  // Initialize ADS1015
  if (!_ads1015->begin()) {
    // ADS1015 not detected on I2C bus
    return;
  }

  // Configure ADS1015 for optimal hall sensor reading
  _ads1015->setGain(Ads1015::Gain::GAIN_TWOTHIRDS);  // ±6.144V range
  _ads1015->setDataRate(Ads1015::DataRate::SPS_3300); // Maximum speed
  _ads1015->setMode(Ads1015::Mode::CONTINUOUS);      // Continuous conversion

  // Configure ALERT pin as input with pull-up
  _hal->pinMode(_alertPin, INPUT_PULLUP);
  
  // Attach interrupt on ALERT pin (active LOW)
  _hal->attachInterrupt(_alertPin, ads1015AlertISRWrapper, CHANGE);

  // Initial sensor read
  readSensor();
}

void Ads1015Hall::config(Config *config) {
  resetDetector();
  _config = config;
  
  if (_config != nullptr) {
    updateComparatorThresholds();
  }
}

void Ads1015Hall::updateComparatorThresholds() {
  if (_config == nullptr) {
    return;
  }

  // Set comparator thresholds
  // ADS1015 thresholds are 12-bit values, left-aligned in 16-bit registers
  _ads1015->setComparatorThresholds(_config->thresholdLow, _config->thresholdHigh);

  // Start comparator in continuous mode on the configured channel
  if (_isDifferential) {
    // Configure for differential reading based on channel pair
    // Channel 0 = AIN0-AIN1, 1 = AIN0-AIN3, 2 = AIN1-AIN3, 3 = AIN2-AIN3
    switch (_channel) {
      case 0:
        _ads1015->startComparator_SingleEnded(0, _config->thresholdHigh);
        break;
      case 1:
        _ads1015->startComparator_SingleEnded(1, _config->thresholdHigh);
        break;
      case 2:
        _ads1015->startComparator_SingleEnded(2, _config->thresholdHigh);
        break;
      case 3:
        _ads1015->startComparator_SingleEnded(3, _config->thresholdHigh);
        break;
    }
  } else {
    // Single-ended mode
    _ads1015->startComparator_SingleEnded(_channel, _config->thresholdHigh);
  }
}

uint16_t Ads1015Hall::getSensorValue() { 
  // Convert 12-bit ADS1015 value (in 16-bit container) to 10-bit equivalent
  // for compatibility with existing code that expects 10-bit ADC values
  // ADS1015 returns values left-aligned in 16-bit, so shift right 4 bits for 12-bit,
  // then shift right 2 more bits to get 10-bit equivalent
  return (_sensorValue >> 6) & 0x3FF;
}

bool Ads1015Hall::isActive() {
  return (_minimum.value != NONE) || (_maximum.value != NONE);
}

int16_t Ads1015Hall::getSensorPosition() {
  if (_config == nullptr) {
    return NONE;
  }
  return _config->position;
}

int16_t Ads1015Hall::getDetectedPosition() { 
  return _detectedPosition; 
}

CarriageType Ads1015Hall::getDetectedCarriage() { 
  return _detectedCarriage; 
}

bool Ads1015Hall::getDetectedBeltPhase() { 
  return _detectedBeltPhase; 
}

void Ads1015Hall::alertISR() {
  // ISR - keep minimal, just set flag
  _alertTriggered = true;
}

bool Ads1015Hall::isDetected(Encoder *encoder, Direction direction, bool beltPhase) {
  bool isDetected = false;
  
  if (_config == nullptr) {
    return isDetected;
  }

  int16_t encoder_position = encoder->getPosition();

  // Check if ALERT was triggered and read sensor if needed
  bool shouldRead = false;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    if (_alertTriggered) {
      _alertTriggered = false;
      shouldRead = true;
    }
  }
  
  if (shouldRead) {
    readSensor();
  }

  // Run detection state machine (same algorithm as hallsensor.cpp)
  switch (_state) {
    case State::ST_HUNT:
      // Proceed as long as direction stays the same
      if (direction == _detectedDirection) {
        _needlesToGo--;

        // Record minimum position (South pole / negative voltage)
        if (_sensorValue < _config->thresholdLow) {
          if ((_minimum.value == NONE) || (_sensorValue < _minimum.value)) {
            _minimum.value = _sensorValue;
            _minimum.position = encoder_position;
            _detectedBeltPhase = beltPhase;
            if (_maximum.value == NONE) {
              // Adjust trigger position to the extremum
              _needlesToGo = MAX_DET_NEEDLES;
              _detectedPosition = _minimum.position;
            }
          }
        }

        // Record maximum position (North pole / positive voltage)
        if (_sensorValue > _config->thresholdHigh) {
          if ((_maximum.value == NONE) || (_sensorValue > _maximum.value)) {
            _maximum.value = _sensorValue;
            _maximum.position = encoder_position;
            _detectedBeltPhase = beltPhase;
            if (_minimum.value == NONE) {
              // Adjust trigger position to the extremum
              _needlesToGo = MAX_DET_NEEDLES;
              _detectedPosition = _maximum.position;
            }
          }
        }

        // Select carriage once max needles passed
        if (_needlesToGo == 0) {
          // Detect only if carriage didn't change direction since first detection
          if (abs(encoder_position - _detectedPosition) == MAX_DET_NEEDLES) {
            isDetected = detectCarriage();
          }
          _state = State::ST_ESCAPE;
        }
      } else {
        // Detection not reliable, move out
        _state = State::ST_ESCAPE;
        _detectedPosition = encoder_position;
      }
      // Fall through to ST_ESCAPE
      [[fallthrough]];
      
    case State::ST_ESCAPE:
      // Hide until clearly out of the magnet window (at least 4 needles for G)
      if (abs(encoder_position - _detectedPosition) > 4) {
        resetDetector();
      }
      break;

    default:  // ST_IDLE
      _state = State::ST_HUNT;
      _needlesToGo = MAX_DET_NEEDLES;
      _detectedBeltPhase = beltPhase;
      _detectedPosition = encoder_position;
      _detectedDirection = direction;
      
      if (_sensorValue < _config->thresholdLow) {
        _minimum = {
          .value = _sensorValue,
          .position = _detectedPosition,
          .isFirst = true
        };
      } else if (_sensorValue > _config->thresholdHigh) {
        _maximum = {
          .value = _sensorValue,
          .position = _detectedPosition,
          .isFirst = true
        };
      } else {
        _state = State::ST_IDLE;
      }
      break;
  }
  
  return isDetected;
}

void Ads1015Hall::resetDetector() {
  _state = State::ST_IDLE;
  _minimum = {.value = NONE, .position = 0, .isFirst = false};
  _maximum = {.value = NONE, .position = 0, .isFirst = false};
}

bool Ads1015Hall::detectCarriage() {
  bool isDetected = true;

  if (_minimum.value == NONE) {
    // K Carriage (only a maximum/North pole - positive voltage)
    _detectedCarriage = CarriageType::Knit;
    _detectedPosition = _maximum.position;
  } else if (_maximum.value == NONE) {
    // L Carriage (only a minimum/South pole - negative voltage)
    _detectedCarriage = CarriageType::Lace;
    _detectedPosition = _minimum.position;
  } else if (_minimum.isFirst) {
    // G Carriage (minimum/South followed by maximum/North poles)
    _detectedCarriage = CarriageType::Garter;
    _detectedPosition = _minimum.position;
  } else {
    isDetected = false;
  }

  return isDetected;
}

void Ads1015Hall::readSensor() {
  if (_config == nullptr) {
    _sensorValue = 0;  // Mid-scale equivalent
    return;
  }

  // Read conversion result from ADS1015
  // This automatically resets the ALERT pin
  if (_isDifferential) {
    // Read differential value based on channel configuration
    switch (_channel) {
      case 0:
        _sensorValue = _ads1015->readADC_Differential_0_1();
        break;
      case 1:
        _sensorValue = _ads1015->readADC_Differential_0_3();
        break;
      case 2:
        _sensorValue = _ads1015->readADC_Differential_1_3();
        break;
      case 3:
        _sensorValue = _ads1015->readADC_Differential_2_3();
        break;
      default:
        _sensorValue = 0;
    }
  } else {
    // Read single-ended value
    _sensorValue = _ads1015->readADC(_channel);
  }
}
