/*!
 * \file kh970_sensors.h
 * \brief Optical Line Break Sensor Driver for Brother KH970
 * 
 * This module implements support for the Brother KH970's optical line break
 * sensor system. Unlike hall effect sensors, the KH970 uses mechanical sliders
 * that are pushed by the carriage as it moves across the needle bed. These
 * sliders block optical line break sensors (typically infrared LED/phototransistor
 * pairs) to detect carriage presence and position.
 * 
 * Sensor Operation:
 * - Optical sensors are normally unblocked (light passes through)
 * - When carriage moves across sensor position, it pushes a mechanical slider
 * - Slider blocks the optical path, triggering the sensor
 * - Sensor state changes from HIGH (unblocked) to LOW (blocked) or vice versa
 * 
 * Detection Strategy:
 * Unlike magnetic sensors that can identify carriage type by magnet polarity,
 * optical sensors only detect presence/absence. Carriage type identification
 * requires analyzing:
 *   1. Timing patterns of slider actuation
 *   2. Width of blocked period (different carriages have different widths)
 *   3. Number of sliders engaged (some carriages may engage multiple sliders)
 *   4. Belt phase correlation
 * 
 * The KH970 typically has sensors at specific positions along the needle bed
 * to detect when the carriage passes calibration points.
 * 
 * \author AYAB Contributors
 * \copyright GPL-3.0 License
 */

#ifndef KH970_SENSORS_H
#define KH970_SENSORS_H

#include "api.h"
#include "encoder.h"
#include <Arduino.h>
#include "hal.h"

// Forward declarations
class Encoder;

/*!
 * \brief Optical Line Break Sensor Driver for KH970
 * 
 * Implements asynchronous detection of carriage position using optical
 * line break sensors with mechanical slider actuation. Uses the same
 * async pattern as the AYAB encoder implementation with ISR-driven
 * state updates and doorbell signaling.
 */
class KH970Sensors {
 public:
  /*!
   * \brief Optical sensor state
   */
  enum class SensorState {
    UNBLOCKED = 0,   ///< Light path clear, no carriage present
    BLOCKED = 1,     ///< Light path blocked by slider
    TRANSITION = 2,  ///< Transitioning between states (debouncing)
    ERROR = 3        ///< Sensor error or invalid state
  };

  /*!
   * \brief Sensor event from optical detector
   */
  struct SensorEvent {
    SensorState state;       ///< Sensor state at event
    int16_t position;        ///< Encoder position at event
    bool beltPhase;          ///< Belt phase when event occurred
    Direction direction;     ///< Carriage direction
    uint32_t timestamp;      ///< Microsecond timestamp
    bool rising;             ///< True for blocked→unblocked, false for unblocked→blocked
  };

  /*!
   * \brief Configuration for a single optical sensor
   */
  class SensorConfig {
   public:
    int16_t position;        ///< Sensor position on needle bed
    uint8_t pin;             ///< GPIO pin for optical sensor
    bool activeLow;          ///< True if sensor is active-low (LOW = blocked)
    uint16_t debounceUs;     ///< Debounce time in microseconds (default 500μs)
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
    uint16_t blockDuration;  ///< Duration sensor was blocked (microseconds)
    uint8_t sensorsMask;     ///< Bitmask of which sensors detected carriage
  };

  /*!
   * \brief Carriage signature for pattern matching
   * 
   * Each carriage type has a characteristic "signature" based on:
   * - Width of the blocking period
   * - Pattern of multiple sensor activations
   * - Timing between sensor activations
   */
  struct CarriageSignature {
    CarriageType type;           ///< Carriage type this signature identifies
    uint16_t minBlockDuration;   ///< Minimum block duration in microseconds
    uint16_t maxBlockDuration;   ///< Maximum block duration in microseconds
    uint8_t expectedSensors;     ///< Bitmask of expected sensor pattern
    uint16_t maxTimingJitter;    ///< Maximum allowed timing variation
  };

  /*!
   * \brief Constructor for KH970 optical sensor system
   * 
   * \param hal Hardware abstraction layer interface
   * \param leftConfig Configuration for left sensor
   * \param rightConfig Configuration for right sensor (can be nullptr if single sensor)
   */
  KH970Sensors(hardwareAbstraction::HalInterface *hal, 
               SensorConfig *leftConfig,
               SensorConfig *rightConfig = nullptr);
  
  ~KH970Sensors() = default;

  /*!
   * \brief Initialize sensors and attach interrupts
   * 
   * Sets up GPIO pins as inputs with pull-ups and attaches interrupt
   * handlers for both edge detection (rising and falling).
   */
  void init();

  /*!
   * \brief Register a carriage signature for pattern matching
   * 
   * \param signature Carriage signature to register
   */
  void registerSignature(const CarriageSignature &signature);

  /*!
   * \brief Update detection state machine (call from main loop)
   * 
   * Processes ISR events, runs detection algorithms, analyzes patterns,
   * and updates carriage state. Must be called regularly from main loop.
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
   * \return True if a carriage is actively detected by any sensor
   */
  bool isActive();

  /*!
   * \brief Get current state of left sensor
   * 
   * \return Current sensor state
   */
  SensorState getLeftState();

  /*!
   * \brief Get current state of right sensor (if configured)
   * 
   * \return Current sensor state, or ERROR if no right sensor
   */
  SensorState getRightState();

  /*!
   * \brief Get position where carriage was detected
   * 
   * \return Encoder position at carriage detection
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
   * \brief Get duration sensor was blocked during last detection
   * 
   * \return Block duration in microseconds
   */
  uint16_t getBlockDuration();

  /*!
   * \brief Reset detection state machine
   * 
   * Clears all detection state and prepares for new detection cycle.
   */
  void reset();

  /*!
   * \brief Enable or disable automatic carriage type detection
   * 
   * When disabled, only position is tracked without type identification.
   * 
   * \param enabled True to enable automatic detection
   */
  void setAutoDetect(bool enabled);

  // ISR handlers (static wrappers call instance methods)
  static void staticLeftSensorISR();
  static void staticRightSensorISR();

 private:
  /*!
   * \brief Detection state machine states
   */
  enum class State {
    ST_IDLE,       ///< Waiting for sensor activation
    ST_BLOCKING,   ///< Sensor blocked, carriage passing
    ST_ANALYZE,    ///< Analyzing block pattern for carriage ID
    ST_CONFIRMED,  ///< Carriage identified and confirmed
    ST_UNBLOCKING, ///< Sensor unblocking, carriage leaving
    ST_ESCAPE      ///< Waiting for complete sensor clearance
  };

  /*!
   * \brief Event buffer for async ISR communication
   */
  static const uint8_t EVENT_BUFFER_SIZE = 32;
  
  struct EventBuffer {
    SensorEvent events[EVENT_BUFFER_SIZE];
    volatile uint8_t writeIndex;
    volatile uint8_t readIndex;
    volatile bool overflow;
  };

  // ISR instance methods
  void leftSensorISR();
  void rightSensorISR();

  /*!
   * \brief Read current state of sensor pin (ISR-safe)
   * 
   * \param isLeft True for left sensor, false for right
   * \return Current sensor state
   */
  SensorState readSensor(bool isLeft);

  /*!
   * \brief Add event to buffer from ISR
   * 
   * \param state Sensor state
   * \param isLeft True for left sensor
   * \param rising True if rising edge (unblocking)
   */
  void pushEvent(SensorState state, bool isLeft, bool rising);

  /*!
   * \brief Process next event from buffer in main loop
   * 
   * \return True if event was processed
   */
  bool processNextEvent(Encoder *encoder, Direction direction, bool beltPhase);

  /*!
   * \brief Match event pattern against registered signatures
   * 
   * \return Matched carriage type, or NoCarriage if no match
   */
  CarriageType matchSignature();

  /*!
   * \brief Calculate carriage position from sensor events
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

  /*!
   * \brief Check if debounce period has elapsed
   * 
   * \param lastTime Last event timestamp
   * \param debounceUs Debounce period in microseconds
   * \return True if debounce period elapsed
   */
  bool debounceElapsed(uint32_t lastTime, uint16_t debounceUs);

  // Hardware interface
  hardwareAbstraction::HalInterface *_hal;
  
  // Sensor configuration
  SensorConfig *_leftConfig;
  SensorConfig *_rightConfig;
  bool _hasRightSensor;

  // Current sensor states (updated by ISR)
  volatile SensorState _leftState;
  volatile SensorState _rightState;
  volatile bool _isrDoorbell;

  // Event buffer for ISR communication
  EventBuffer _eventBuffer;

  // Detection state
  State _state;
  CarriageDetection _detection;
  bool _autoDetect;

  // Registered carriage signatures
  static const uint8_t MAX_SIGNATURES = 8;
  CarriageSignature _signatures[MAX_SIGNATURES];
  uint8_t _signatureCount;

  // Detection timing
  uint32_t _blockStartTime;
  uint32_t _blockEndTime;
  uint32_t _lastLeftEvent;
  uint32_t _lastRightEvent;

  // Position tracking
  int16_t _blockStartPosition;
  int16_t _blockEndPosition;
  Direction _detectionDirection;

  // Sensor activation tracking
  uint8_t _activeSensorsMask;  // Bitmask: bit 0=left, bit 1=right
  
  // Detection window (needles to track after first activation)
  static const uint8_t DETECTION_WINDOW = 8;
  uint8_t _needlesRemaining;
};

// Global instance for ISR access
extern KH970Sensors *kh970SensorsInstance;

#endif // KH970_SENSORS_H
