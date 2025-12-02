/*!
 * \file dualhallsensor.cpp
 * \brief Implementation of Dual Hall Effect Sensor Driver
 */

#include "dualhallsensor.h"
#include "../../platform/common/atomic.h"
#include <stdlib.h>

// Global instance for ISR access
DualHallSensor *dualHallSensorInstance = nullptr;

// Static ISR wrappers
void DualHallSensor::staticLeftNorthISR() {
  if (dualHallSensorInstance) {
    dualHallSensorInstance->leftNorthISR();
  }
}

void DualHallSensor::staticLeftSouthISR() {
  if (dualHallSensorInstance) {
    dualHallSensorInstance->leftSouthISR();
  }
}

void DualHallSensor::staticRightNorthISR() {
  if (dualHallSensorInstance) {
    dualHallSensorInstance->rightNorthISR();
  }
}

void DualHallSensor::staticRightSouthISR() {
  if (dualHallSensorInstance) {
    dualHallSensorInstance->rightSouthISR();
  }
}

// Constructor
DualHallSensor::DualHallSensor(hardwareAbstraction::HalInterface *hal,
                               SensorConfig *leftConfig,
                               SensorConfig *rightConfig)
    : _hal(hal),
      _leftConfig(leftConfig),
      _rightConfig(rightConfig),
      _leftPole(Pole::NONE),
      _rightPole(Pole::NONE),
      _isrDoorbell(false),
      _state(State::ST_IDLE),
      _historyCount(0),
      _lastEventTime(0),
      _firstDetectionPosition(0),
      _lastDetectionPosition(0),
      _detectionDirection(Direction::Unknown),
      _needlesRemaining(0) {
  
  // Initialize event buffer
  _eventBuffer.writeIndex = 0;
  _eventBuffer.readIndex = 0;
  _eventBuffer.overflow = false;

  // Initialize detection result
  _detection.type = CarriageType::NoCarriage;
  _detection.position = 0;
  _detection.beltPhase = false;
  _detection.direction = Direction::Unknown;
  _detection.valid = false;
  _detection.confidence = 0;

  // Set global instance for ISR access
  dualHallSensorInstance = this;
}

void DualHallSensor::init() {
  // Configure left sensor pins
  _hal->pinMode(_leftConfig->pinNorth, INPUT);
  _hal->pinMode(_leftConfig->pinSouth, INPUT);

  // Configure right sensor pins
  _hal->pinMode(_rightConfig->pinNorth, INPUT);
  _hal->pinMode(_rightConfig->pinSouth, INPUT);

  // Attach interrupts for all four pins
  _hal->attachInterrupt(_leftConfig->pinNorth, staticLeftNorthISR, CHANGE);
  _hal->attachInterrupt(_leftConfig->pinSouth, staticLeftSouthISR, CHANGE);
  _hal->attachInterrupt(_rightConfig->pinNorth, staticRightNorthISR, CHANGE);
  _hal->attachInterrupt(_rightConfig->pinSouth, staticRightSouthISR, CHANGE);
}

// ISR handlers
void DualHallSensor::leftNorthISR() {
  Pole pole = readSensor(true);
  pushEvent(pole, true);
}

void DualHallSensor::leftSouthISR() {
  Pole pole = readSensor(true);
  pushEvent(pole, true);
}

void DualHallSensor::rightNorthISR() {
  Pole pole = readSensor(false);
  pushEvent(pole, false);
}

void DualHallSensor::rightSouthISR() {
  Pole pole = readSensor(false);
  pushEvent(pole, false);
}

DualHallSensor::Pole DualHallSensor::readSensor(bool isLeft) {
  SensorConfig *config = isLeft ? _leftConfig : _rightConfig;
  
  bool northActive = _hal->digitalRead(config->pinNorth);
  bool southActive = _hal->digitalRead(config->pinSouth);

  // Apply inversion if configured
  if (config->inverted) {
    northActive = !northActive;
    southActive = !southActive;
  }

  // Determine pole from pin states
  if (northActive && southActive) {
    return Pole::BOTH; // Error or transition
  } else if (northActive) {
    return Pole::NORTH;
  } else if (southActive) {
    return Pole::SOUTH;
  } else {
    return Pole::NONE;
  }
}

void DualHallSensor::pushEvent(Pole pole, bool isLeft) {
  uint32_t now = _hal->millis();
  
  // Simple debounce
  if (now - _lastEventTime < DEBOUNCE_MICROS) {
    return;
  }
  _lastEventTime = now;

  // Update current sensor state
  if (isLeft) {
    _leftPole = pole;
  } else {
    _rightPole = pole;
  }

  // Add to event buffer
  uint8_t nextWrite = (_eventBuffer.writeIndex + 1) % EVENT_BUFFER_SIZE;
  if (nextWrite != _eventBuffer.readIndex) {
    DetectionEvent &event = _eventBuffer.events[_eventBuffer.writeIndex];
    event.pole = pole;
    event.timestamp = now;
    // Position and direction will be filled in schedule()
    _eventBuffer.writeIndex = nextWrite;
    
    // Ring doorbell
    _isrDoorbell = true;
  } else {
    _eventBuffer.overflow = true;
  }
}

void DualHallSensor::schedule(Encoder *encoder, Direction direction, bool beltPhase) {
  // Check doorbell
  bool hasEvents;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    hasEvents = _isrDoorbell;
    _isrDoorbell = false;
  }

  if (!hasEvents) {
    return;
  }

  // Process all pending events
  while (processNextEvent(encoder, direction, beltPhase)) {
    // Events processed
  }
}

bool DualHallSensor::processNextEvent(Encoder *encoder, Direction direction, bool beltPhase) {
  // Check if event available
  if (_eventBuffer.readIndex == _eventBuffer.writeIndex) {
    return false;
  }

  // Get event from buffer
  DetectionEvent event;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    event = _eventBuffer.events[_eventBuffer.readIndex];
    _eventBuffer.readIndex = (_eventBuffer.readIndex + 1) % EVENT_BUFFER_SIZE;
  }

  // Fill in encoder position and direction
  event.position = encoder->getPosition();
  event.direction = direction;
  event.beltPhase = beltPhase;

  // State machine
  switch (_state) {
    case State::ST_IDLE:
      // First detection - start hunting
      if (event.pole != Pole::NONE) {
        _state = State::ST_HUNT;
        _firstDetectionPosition = event.position;
        _detectionDirection = direction;
        _needlesRemaining = DETECTION_WINDOW;
        _historyCount = 0;
        
        // Add to history
        if (_historyCount < HISTORY_SIZE) {
          _history[_historyCount++] = event;
        }
      }
      break;

    case State::ST_HUNT:
      // Collect detection events
      if (direction == _detectionDirection) {
        _needlesRemaining--;
        _lastDetectionPosition = event.position;

        // Add to history
        if (_historyCount < HISTORY_SIZE) {
          _history[_historyCount++] = event;
        }

        // End of detection window?
        if (_needlesRemaining == 0) {
          _state = State::ST_ANALYZE;
        }
      } else {
        // Direction changed, reset
        reset();
      }
      break;

    case State::ST_ANALYZE:
      // Analyze pattern to identify carriage
      if (analyzeCarriagePattern()) {
        _detection.position = calculateCarriagePosition();
        _detection.confidence = validateDetection();
        _detection.valid = (_detection.confidence > 50);
        _state = State::ST_CONFIRMED;
      } else {
        // Pattern not recognized
        reset();
      }
      break;

    case State::ST_CONFIRMED:
      // Wait for carriage to exit
      if (event.pole == Pole::NONE && _leftPole == Pole::NONE && _rightPole == Pole::NONE) {
        _state = State::ST_ESCAPE;
      }
      break;

    case State::ST_ESCAPE:
      // Wait for clear sensors before allowing new detection
      if (event.pole == Pole::NONE) {
        reset();
      }
      break;
  }

  return true;
}

bool DualHallSensor::analyzeCarriagePattern() {
  if (_historyCount < 2) {
    return false; // Not enough data
  }

  // Count pole transitions
  uint8_t northCount = 0;
  uint8_t southCount = 0;
  
  for (uint8_t i = 0; i < _historyCount; i++) {
    if (_history[i].pole == Pole::NORTH) {
      northCount++;
    } else if (_history[i].pole == Pole::SOUTH) {
      southCount++;
    }
  }

  // Carriage identification based on magnet pattern
  if (northCount > 0 && southCount == 0) {
    // Only North poles detected -> Knit carriage
    _detection.type = CarriageType::Knit;
    _detection.direction = _detectionDirection;
    _detection.beltPhase = _history[0].beltPhase;
    return true;
  } else if (southCount > 0 && northCount == 0) {
    // Only South poles detected -> Lace carriage
    _detection.type = CarriageType::Lace;
    _detection.direction = _detectionDirection;
    _detection.beltPhase = _history[0].beltPhase;
    return true;
  } else if (northCount > 0 && southCount > 0) {
    // Both poles detected -> Garter carriage (or double-bed)
    // Check sequence: Garter has South then North in travel direction
    bool isSouthFirst = false;
    for (uint8_t i = 0; i < _historyCount; i++) {
      if (_history[i].pole == Pole::SOUTH) {
        isSouthFirst = true;
        break;
      } else if (_history[i].pole == Pole::NORTH) {
        break;
      }
    }
    
    _detection.type = isSouthFirst ? CarriageType::Garter : CarriageType::NoCarriage;
    _detection.direction = _detectionDirection;
    _detection.beltPhase = _history[0].beltPhase;
    return isSouthFirst; // Only valid if South first
  }

  return false;
}

int16_t DualHallSensor::calculateCarriagePosition() {
  // Use midpoint between first and last detection
  return (_firstDetectionPosition + _lastDetectionPosition) / 2;
}

uint8_t DualHallSensor::validateDetection() {
  uint8_t confidence = 0;

  // Base confidence from history count
  confidence += (_historyCount * 10);

  // Bonus for consistent direction
  bool directionConsistent = true;
  for (uint8_t i = 1; i < _historyCount; i++) {
    if (_history[i].direction != _history[0].direction) {
      directionConsistent = false;
      break;
    }
  }
  if (directionConsistent) {
    confidence += 20;
  }

  // Bonus for expected carriage types
  if (_detection.type == CarriageType::Knit || 
      _detection.type == CarriageType::Lace ||
      _detection.type == CarriageType::Garter) {
    confidence += 20;
  }

  // Cap at 100
  if (confidence > 100) {
    confidence = 100;
  }

  return confidence;
}

void DualHallSensor::reset() {
  _state = State::ST_IDLE;
  _historyCount = 0;
  _needlesRemaining = 0;
  _firstDetectionPosition = 0;
  _lastDetectionPosition = 0;
  _detectionDirection = Direction::Unknown;
}

// Public getters
DualHallSensor::CarriageDetection DualHallSensor::getDetection() {
  return _detection;
}

bool DualHallSensor::isActive() {
  return _state == State::ST_HUNT || 
         _state == State::ST_ANALYZE || 
         _state == State::ST_CONFIRMED;
}

DualHallSensor::Pole DualHallSensor::getLeftPole() {
  Pole pole;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    pole = _leftPole;
  }
  return pole;
}

DualHallSensor::Pole DualHallSensor::getRightPole() {
  Pole pole;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    pole = _rightPole;
  }
  return pole;
}

int16_t DualHallSensor::getDetectedPosition() {
  return _detection.position;
}

CarriageType DualHallSensor::getDetectedCarriage() {
  return _detection.type;
}

bool DualHallSensor::getDetectedBeltPhase() {
  return _detection.beltPhase;
}
