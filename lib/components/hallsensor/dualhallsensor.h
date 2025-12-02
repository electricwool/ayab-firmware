/*!
 * \file dualhallsensor.h
 * \brief Dual Hall Effect Sensor Driver for AYAB Firmware
 * 
 * This module implements support for dual-polarity hall effect sensors
 * with hardware comparator (LM393DR) interface. Each sensor (left and right)
 * uses bipolar analog hall effect sensors that output positive or negative
 * voltage depending on magnetic pole detected. The LM393DR comparator 
 * converts these to two digital signals per sensor:
 * 
 * Right Sensor:
 *   - EOL_PIN_R_N: North pole detected (positive voltage input)
 *   - EOL_PIN_R_S: South pole detected (negative voltage input)
 * 
 * Left Sensor:
 *   - EOL_PIN_L_N: North pole detected (positive voltage input)
 *   - EOL_PIN_L_S: South pole detected (negative voltage input)
 * 
 * Carriage Detection:
 * Different carriages have unique magnet arrangements that allow identification:
 *   - Knit (K): Single North pole magnet
 *   - Lace (L): Single South pole magnet
 *   - Garter (G): Two magnets (South then North in travel direction)
 * 
 * The dual sensor system provides:
 *   1. Precise carriage position calibration
 *   2. Carriage type identification
 *   3. Belt phase detection (shifted/regular)
 *   4. Direction detection
 * 
 * \author AYAB Contributors
 * \copyright GPL-3.0 License
 */

#ifndef DUALHALLSENSOR_H
#define DUALHALLSENSOR_H

#include "api.h"
#include "encoder.h"
#include <Arduino.h>
#include "hal.h"

// Forward declarations
class Encoder;

/*!
 * \brief Dual Hall Effect Sensor Driver
 * 
 * Implements asynchronous detection of carriage position and type using
 * dual-polarity hall effect sensors with hardware comparator interface.
 * Uses the same async pattern as the existing AYAB encoder implementation
 * with ISR-driven state updates and doorbell signaling.
 */
class DualHallSensor {
 public:
  /*!
   * \brief Magnetic pole detected by sensor
   */
  enum class Pole {
    NONE = 0,      ///< No magnet detected
    NORTH = 1,     ///< North pole detected (positive voltage)
    SOUTH = 2,     ///< South pole detected (negative voltage)
    BOTH = 3       ///< Both poles detected (error condition or transition)
  };

  /*!
   * \brief Detection event from sensor
   */
  struct DetectionEvent {
    Pole pole;               ///< Detected magnetic pole
    int16_t position;        ///< Encoder position at detection
    bool beltPhase;          ///< Belt phase when detected
    Direction direction;     ///< Carriage direction
    uint32_t timestamp;      ///< Microsecond timestamp
  };

  /*!
   * \brief Configuration for a single dual-polarity sensor
   */
  class SensorConfig {
   public:
    int16_t position;        ///< Sensor position on needle bed
    uint8_t pinNorth;        ///< GPIO pin for North pole detection
    uint8_t pinSouth;        ///< GPIO pin for South pole detection
    bool inverted;           ///< True if sensor signals are inverted
  };

  /*!
   * \brief Carriage detection result
   */
  struct CarriageDetection {
    CarriageType type;       ///< Detected carriage type
    int16_t position;        ///< Calibrated carriage position
    bool beltPhase;          ///< Belt phase (shifted/regular)
    Direction direction;     ///< Travel direction
    bool valid;              ///< True if detection is valid
    uint8_t confidence;      ///< Detection confidence (0-100)
  };

  /*!
   * \brief Constructor for dual hall sensor system
   * 
   * \param hal Hardware abstraction layer interface
   * \param leftConfig Configuration for left sensor
   * \param rightConfig Configuration for right sensor
   */
  DualHallSensor(hardwareAbstraction::HalInterface *hal, 
                 SensorConfig *leftConfig,
                 SensorConfig *rightConfig);
  
  ~DualHallSensor() = default;

  /*!
   * \brief Initialize sensors and attach interrupts
   * 
   * Sets up GPIO pins and attaches interrupt handlers for all four
   * sensor inputs (L_N, L_S, R_N, R_S).
   */
  void init();

  /*!
   * \brief Update detection state machine (call from main loop)
   * 
   * Processes ISR events, runs detection algorithms, and updates
   * carriage state. Must be called regularly from main loop.
   * 
   * \param encoder Encoder instance for position tracking
   * \param direction Current carriage direction
   * \param beltPhase Current belt phase
   */
  void schedule(Encoder *encoder, Direction direction, bool beltPhase);

  /*!
   * \brief Get last detected carriage information
   * 
   * \return CarriageDetection structure with latest detection results
   */
  CarriageDetection getDetection();

  /*!
   * \brief Check if carriage is currently detected
   * 
   * \return True if a carriage is actively detected by either sensor
   */
  bool isActive();

  /*!
   * \brief Get current sensor state for left sensor
   * 
   * \return Current magnetic pole detected by left sensor
   */
  Pole getLeftPole();

  /*!
   * \brief Get current sensor state for right sensor
   * 
   * \return Current magnetic pole detected by right sensor
   */
  Pole getRightPole();

  /*!
   * \brief Get position where carriage was detected
   * 
   * \return Encoder position at carriage detection
   */
  int16_t getDetectedPosition();

  /*!
   * \brief Get detected carriage type
   * 
   * \return Type of carriage detected (Knit, Lace, Garter, etc.)
   */
  CarriageType getDetectedCarriage();

  /*!
   * \brief Get belt phase at detection
   * 
   * \return True if belt was in shifted phase
   */
  bool getDetectedBeltPhase();

  /*!
   * \brief Reset detection state machine
   * 
   * Clears all detection state and prepares for new detection cycle.
   */
  void reset();

  // ISR handlers (static wrappers call instance methods)
  static void staticLeftNorthISR();
  static void staticLeftSouthISR();
  static void staticRightNorthISR();
  static void staticRightSouthISR();

 private:
  /*!
   * \brief Detection state machine states
   */
  enum class State {
    ST_IDLE,       ///< Waiting for initial magnet detection
    ST_HUNT,       ///< Tracking magnet passage, collecting data
    ST_ANALYZE,    ///< Analyzing collected data to identify carriage
    ST_CONFIRMED,  ///< Carriage identified and confirmed
    ST_ESCAPE      ///< Waiting for carriage to leave sensor range
  };

  /*!
   * \brief Event buffer for async ISR communication
   */
  static const uint8_t EVENT_BUFFER_SIZE = 16;
  
  struct EventBuffer {
    DetectionEvent events[EVENT_BUFFER_SIZE];
    volatile uint8_t writeIndex;
    volatile uint8_t readIndex;
    volatile bool overflow;
  };

  // ISR instance methods
  void leftNorthISR();
  void leftSouthISR();
  void rightNorthISR();
  void rightSouthISR();

  /*!
   * \brief Read current state of sensor pins (ISR-safe)
   * 
   * \param isLeft True for left sensor, false for right
   * \return Detected pole
   */
  Pole readSensor(bool isLeft);

  /*!
   * \brief Add event to buffer from ISR
   * 
   * \param pole Detected pole
   * \param isLeft True for left sensor
   */
  void pushEvent(Pole pole, bool isLeft);

  /*!
   * \brief Process next event from buffer in main loop
   * 
   * \return True if event was processed
   */
  bool processNextEvent(Encoder *encoder, Direction direction, bool beltPhase);

  /*!
   * \brief Analyze event pattern to identify carriage
   * 
   * \return True if carriage was successfully identified
   */
  bool analyzeCarriagePattern();

  /*!
   * \brief Calculate carriage position from detection points
   * 
   * \return Calibrated position
   */
  int16_t calculateCarriagePosition();

  /*!
   * \brief Validate detection for confidence scoring
   * 
   * \return Confidence score 0-100
   */
  uint8_t validateDetection();

  // Hardware interface
  hardwareAbstraction::HalInterface *_hal;
  
  // Sensor configuration
  SensorConfig *_leftConfig;
  SensorConfig *_rightConfig;

  // Current sensor states (updated by ISR)
  volatile Pole _leftPole;
  volatile Pole _rightPole;
  volatile bool _isrDoorbell;

  // Event buffer for ISR communication
  EventBuffer _eventBuffer;

  // Detection state
  State _state;
  CarriageDetection _detection;
  
  // Detection history for pattern analysis
  static const uint8_t HISTORY_SIZE = 8;
  DetectionEvent _history[HISTORY_SIZE];
  uint8_t _historyCount;

  // Timing for debounce and validation
  uint32_t _lastEventTime;
  static const uint32_t DEBOUNCE_MICROS = 100; // 100μs debounce
  
  // Position tracking
  int16_t _firstDetectionPosition;
  int16_t _lastDetectionPosition;
  Direction _detectionDirection;
  
  // Detection window (needles to track after first detection)
  static const uint8_t DETECTION_WINDOW = 5;
  uint8_t _needlesRemaining;
};

// Global instance for ISR access
extern DualHallSensor *dualHallSensorInstance;

#endif // DUALHALLSENSOR_H
