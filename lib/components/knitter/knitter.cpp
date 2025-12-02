#include "knitter.h"

#include "api.h"
#include "shield.h"
#include "mcp23008.h"
#include "mcp23017.h"
#include "pcf8574.h"

#ifdef USE_DUAL_HALL_SENSOR
#include "dualhallsensor.h"
#endif

//----------------------------------------------------------------------------
// Knitter class
//----------------------------------------------------------------------------

Knitter::Knitter(hardwareAbstraction::HalInterface *hal) : API(hal) {
  // Platform
  _hal = hal;

  // Knitter hardware
  _encoder = new Encoder(_hal, Shield::Encoder::ENC_A_PIN, Shield::Encoder::ENC_B_PIN);
  _hal->pinMode(Shield::Encoder::ENC_C_PIN, INPUT);

#ifdef USE_DUAL_HALL_SENSOR
  // Use dual hall sensor system with all four pins
  static DualHallSensor::SensorConfig leftSensorConfig = {
    .position = -32,  // Left sensor position (adjust as needed)
    .pinNorth = Shield::HallDetectors::EOL_PIN_L_N,
    .pinSouth = Shield::HallDetectors::EOL_PIN_L_S,
    .inverted = false
  };
  
  static DualHallSensor::SensorConfig rightSensorConfig = {
    .position = 32,   // Right sensor position (adjust as needed)
    .pinNorth = Shield::HallDetectors::EOL_PIN_R_N,
    .pinSouth = Shield::HallDetectors::EOL_PIN_R_S,
    .inverted = false
  };
  
  _dualHallSensor = new DualHallSensor(_hal, &leftSensorConfig, &rightSensorConfig);
  _dualHallSensor->init();
  
  // Set traditional hall sensor pointers to nullptr to indicate dual mode
  _hall_left = nullptr;
  _hall_right = nullptr;
#else
  // Use traditional single-input hall sensors
  _hall_left = new HallSensor(_hal, Shield::HallDetectors::EOL_L_PIN);
  
  // Right sensor with optional hardware fix detection pins
  #if defined(EOL_R_L_PIN) && defined(EOL_R_DETECT_PIN)
    // Hardware fix available - enable Lace carriage detection on KH910
    _hall_right = new HallSensor(
                    _hal,
                    Shield::HallDetectors::EOL_R_PIN,
                    Shield::HallDetectors::EOL_R_L_PIN,
                    Shield::HallDetectors::EOL_R_DETECT_PIN
                  );
  #else
    // No hardware fix - basic hall sensor only
    _hall_right = new HallSensor(_hal, Shield::HallDetectors::EOL_R_PIN);
  #endif
  
  _dualHallSensor = nullptr;
#endif

  GpioExpander* gpio_expander[2];
  _detectGpioExpanders(_hal, Shield::GpioExpanders::I2C_ADDRESSES, gpio_expander);
  _solenoids = new Solenoids(_hal, gpio_expander);

  // Knitter objects
  _machine = new Machine();
  _carriage = new Carriage();
  _beltShift = BeltShift::Unknown;
  _direction = Direction::Unknown;

  // Ayab hardware
#ifdef PIEZO_PIN
  _beeper = new Beeper(_hal, Shield::Piezo::PIEZO_PIN);
#else
  _beeper = new Beeper(_hal); // No-op beeper when pin not available
#endif

#ifdef LED_A_PIN
  _led_a = new Led(_hal, Shield::Leds::LED_A_PIN, HIGH, LOW);
#else
  _led_a = nullptr;
#endif

#ifdef LED_B_PIN
  _led_b = new Led(_hal, Shield::Leds::LED_B_PIN, HIGH, LOW);
#else
  _led_b = nullptr;
#endif

  _resetFromOperate = false;
  reset();
}

void Knitter::reset() { _state = KnitterState::Reset; }

void Knitter::schedule() {
  API::schedule();
  _beeper->schedule();
  _encoder->schedule();
  if (_led_a) _led_a->schedule();
  if (_led_b) _led_b->schedule();

  _runMachine();

  // Finite State Machine of this knitting machine
  bool isStateChange = _state != _lastState;
  _lastState = _state;
  switch (_state) {
    case KnitterState::Reset:
      // Skip machine reset when initiated from reqInit
      if (_resetFromOperate) {
        _resetFromOperate = false;
      } else {
        _machine->reset();
      }
      _carriage->reset();
      _carriage->setPosition(_encoder->getPosition());

      _solenoids->reset();

      if (_led_a) _led_a->on();
      if (_led_b) _led_b->on();

      _config.valid = false;
      _config.continuousReporting = false;
      _state = KnitterState::Init;

      break;

    case KnitterState::Init:
      if (isStateChange) {
        if (_led_a) _led_a->blink(LED_SLOW_ON, LED_SLOW_OFF);
        _beeper->beep(BEEPER_INIT);
      }
      if (_machine->isDefined() && _carriage->isDefined()) {
        _state = KnitterState::Ready;
        _config.valid = false;
      }
      break;

    case KnitterState::Ready:
      if (isStateChange) {
        if (_led_a) _led_a->blink(LED_FAST_ON, LED_FAST_OFF);
      }
      if (_config.valid) {
        _state = KnitterState::Operate;
        _currentLine.reset();
      }
      break;

    case KnitterState::Operate:
      if (isStateChange) {
        if (_led_a) _led_a->off();  // turn off, used for API Rx indication
        if (_led_b) _led_b->off();  // turn off, used for API Tx indication
      }
      if (_currentLine.finished) {
        if (_currentLine.isLastLine()) {
          _state = KnitterState::Reset;
        } else {
          if (!_currentLine.requested) {
            // TODO: Implement a timeout/retry mechanism ?
            _apiRequestLine(_currentLine.getNextLineNumber(), ErrorCode::Success);
            _currentLine.requested = true;
          }
        }
      }
      break;

    default:
      _state = KnitterState::Reset;
      break;
  }
}

void Knitter::_detectGpioExpanders(hardwareAbstraction::HalInterface *hal, const uint8_t i2cAddress[][2], GpioExpander* gpio_expander[2]) {
  // FIXME: First one is selected when none are detected -> should raise an error towards desktop app instead
  int i2c_address_set = 0;
  for (int id = 0; (i2cAddress[id][0] != 0) || (i2cAddress[id][1] != 0); id++) {
    if (hal->i2c->detect(i2cAddress[id][0]) && hal->i2c->detect(i2cAddress[id][1])) {
      i2c_address_set = id;
      break;
    };
  }

  for (int i = 0; i < 2; i++) {
    // Detect GPIO expander type
    // MCP23008 IOCON.0 always reads as 0 while PCF8574 will latch the last written value
    hal->i2c->write(i2cAddress[i2c_address_set][i], Mcp23008::IOCON, 0x01);
    if ((hal->i2c->read(i2cAddress[i2c_address_set][i], Mcp23008::IOCON) & 0x01) == 0x00) {
      Mcp23008 *mcp23008 = new Mcp23008(hal, i2cAddress[i2c_address_set][i]);
      mcp23008->write(Mcp23008::IODIR, 0);  // Configure as output
      gpio_expander[i] = mcp23008;
    } else {
      // Not MCP23008 — could be MCP23017 or PCF8574. Try a benign OLATB write/read
      uint8_t addr = i2cAddress[i2c_address_set][i];
      // Read current OLATB (may be undefined for non-MCP23017 devices)
      uint8_t orig_olatb = hal->i2c->read(addr, Mcp23017::OLATB);
      uint8_t test = orig_olatb ^ 0x55; // flip some bits for test
      // Try writing to OLATB and read back
      hal->i2c->write(addr, Mcp23017::OLATB, test);
      uint8_t readback = hal->i2c->read(addr, Mcp23017::OLATB);
      if (readback == test) {
        // Likely an MCP23017 (supports OLATB). Instantiate and configure both banks as outputs
        Mcp23017 *mcp23017 = new Mcp23017(hal, addr);
        mcp23017->write(Mcp23017::IODIRA, 0);
        mcp23017->write(Mcp23017::IODIRB, 0);
        // Restore previous OLATB value
        mcp23017->write(Mcp23017::OLATB, orig_olatb);
        gpio_expander[i] = mcp23017;
      } else {
        // Fallback to PCF8574
        gpio_expander[i] = new Pcf8574(hal, addr);
      }
    }
  }
}

void Knitter::_apiRxTrafficIndication() { if (_led_a) _led_a->flash(LED_FLASH_DURATION); }

void Knitter::_apiTxTrafficIndication() { if (_led_b) _led_b->flash(LED_FLASH_DURATION); }

void Knitter::_apiRequestReset() { reset(); }

ErrorCode Knitter::_apiRequestInit(MachineType machine) {
  if (_state == KnitterState::Init || _state == KnitterState::Operate) {
    _machine->setType(machine);
    _hall_left->config(_machine->getSensorConfig(MachineSide::Left));
    _hall_right->config(_machine->getSensorConfig(MachineSide::Right));
    // Reset machine upon reception of a new reqInit while in Operate state
    // because there is no reqReset API call from ayab-desktop as of today
    // and there is no hardware reset when the serial is open on all platforms
    // e.g. UNO R4
    if (_state == KnitterState::Operate) {
      _resetFromOperate = true;
      reset();
    }
    return ErrorCode::Success;
  }
  return ErrorCode::MachineInvalidState;
}

ErrorCode Knitter::_apiRxSetConfig(uint8_t startNeedle, uint8_t stopNeedle,
                                   bool continuousReporting,
                                   bool beeperEnabled) {
  _config.valid = false;
  if (_state == KnitterState::Ready) {
    if ((startNeedle >= 0) && (stopNeedle < _machine->getNumberofNeedles()) &&
        (startNeedle < stopNeedle)) {
      _config = {.startNeedle = startNeedle,
                 .stopNeedle = stopNeedle,
                 .continuousReporting = continuousReporting};
      _beeper->config(beeperEnabled);
      _config.valid = true;
      return ErrorCode::Success;
    }
    return ErrorCode::MessageInvalidArguments;
  }
  return ErrorCode::MachineInvalidState;
}

ErrorCode Knitter::_apiRxSetLine(uint8_t lineNumber, const uint8_t *pattern,
                                 uint8_t size, bool isLastLine) {
  bool success = false;
  if (_state == KnitterState::Operate) {
    if (size != _machine->getNumberofNeedles() >> 3) {
      return ErrorCode::MessageIncorrectLenght;
    }
    success = _currentLine.setPattern(lineNumber, pattern, isLastLine);
    if (success) {
      _beeper->beep(BEEPER_NEXT_LINE);
      return ErrorCode::Success;
    } else {  // Request line again, TODO: Use a different error code
      _apiRequestLine(_currentLine.getNextLineNumber(), ErrorCode::Success);
      return ErrorCode::MessageInvalidArguments;
    }
  }
  return ErrorCode::MachineInvalidState;
}

void Knitter::_apiRxIndicateState() {
  MachineSide hallActive = _hall_left->isActive()    ? MachineSide::Left
                           : _hall_right->isActive() ? MachineSide::Right
                                                     : MachineSide::None;
  CarriageType carriage = _carriage->getType();
  if (carriage == CarriageType::Knit270) {
    // FIXME: APIv6 doesn't know about Knit270
    carriage = CarriageType::Knit;
  }
  _apiIndicateState(_state, _hall_left->getSensorValue(),
                    _hall_right->getSensorValue(), carriage,
                    _carriage->getPosition(), _direction, hallActive,
                    _beltShift);
}

// (Re)set carriage type/position and beltshift when crossing one sensor
void Knitter::_checkHallSensors() {
  // When crossing left/right sensors towards the center, update carriage
  // (via isCrossing), encoder and beltshift states
  bool beltPhase = _hal->digitalRead(Shield::Encoder::ENC_C_PIN) != 0;
  CarriageType lastCarriageType = _carriage->getType();

#ifdef USE_DUAL_HALL_SENSOR
  // Use dual hall sensor system
  if (_dualHallSensor) {
    // Schedule the dual sensor to process ISR events
    _dualHallSensor->schedule(_encoder, _direction, beltPhase);
    
    // Check if carriage detected
    if (_dualHallSensor->isActive()) {
      DualHallSensor::CarriageDetection detection = _dualHallSensor->getDetection();
      
      if (detection.valid) {
        // Update carriage type from detection
        _carriage->setType(detection.type);
        _carriage->setPosition(detection.position);
        
        // Update encoder position
        _encoder->setPosition(detection.position);
        
        // Determine belt shift based on carriage type and detected phase
        if (detection.type == CarriageType::Knit || detection.type == CarriageType::Knit270) {
          _beltShift = detection.beltPhase ? BeltShift::Shifted : BeltShift::Regular;
        } else if (detection.type == CarriageType::Lace || detection.type == CarriageType::Garter) {
          _beltShift = detection.beltPhase ? BeltShift::Regular : BeltShift::Shifted;
        }
        
        _apiRxIndicateState();
        if (_carriage->getType() != lastCarriageType) {
          _beeper->beep(BEEPER_CARRIAGE);
        }
      }
    }
  }
#else
  // Use traditional single-input hall sensors
  if (_hall_left->isDetected(_encoder, _direction, beltPhase)) {
    if (_carriage->isCrossing(_hall_left, Direction::Right)) {
      _encoder->setPosition(_carriage->getPosition());
      if (_carriage->getType() == CarriageType::Knit) {
        _beltShift = _hall_left->getDetectedBeltPhase() ? BeltShift::Shifted
                                                        : BeltShift::Regular;
      } else if (_carriage->getType() == CarriageType::Knit270) {
        _beltShift = BeltShift::Regular;
      } else {  // CarriageType::Lace and CarriageType::Garter
        _beltShift = _hall_left->getDetectedBeltPhase() ? BeltShift::Regular
                                                        : BeltShift::Shifted;
      }
      _apiRxIndicateState();
      if (_carriage->getType() != lastCarriageType ) {
        _beeper->beep(BEEPER_CARRIAGE);
      }
    }
  } else if (_hall_right->isDetected(_encoder, _direction, beltPhase)) {
    if (_carriage->isCrossing(_hall_right, Direction::Left)) {
      _encoder->setPosition(_carriage->getPosition());
      if (_carriage->getType() == CarriageType::Lace) {
        _beltShift = _hall_right->getDetectedBeltPhase() ? BeltShift::Shifted
                                                         : BeltShift::Regular;
      } else if (_carriage->getType() == CarriageType::Knit270) {
        _beltShift = BeltShift::Regular;
      } else {  // CarriageType::Knit and CarriageType::Garter
        _beltShift = _hall_right->getDetectedBeltPhase() ? BeltShift::Regular
                                                         : BeltShift::Shifted;
      }
      _apiRxIndicateState();
      if (_carriage->getType() != lastCarriageType ) {
        _beeper->beep(BEEPER_CARRIAGE);
      }
    }
  }
#endif  // USE_DUAL_HALL_SENSOR
}

void Knitter::_runMachine() {
  if (_machine->isDefined()) {
    int16_t newPosition = _encoder->getPosition();
    if (newPosition != _carriage->getPosition()) {
      // Infer current direction and update carriage position
      _direction = ((newPosition - _carriage->getPosition()) > 0)
                       ? Direction::Right
                       : Direction::Left;
      _carriage->setPosition(newPosition);

      _checkHallSensors();

      if (_carriage->isDefined()) {
        // Get needle to set given current carriage position/type/direction
        int16_t selectPosition = _carriage->getSelectPosition(_direction);
        // Map needle to set to solenoid
        uint8_t solenoidToSet = _machine->solenoidToSet(selectPosition);
        // Belt shift handling
        if (_beltShift == BeltShift::Shifted) {
          _machine->solenoidShift(solenoidToSet);
        }      
        // For the Lace and K270 carriages, the solenoid to set when moving
        // to the left is offset by 8 and 6 needles (half the number of solenoids)
        // compared to a movement to the right.
        if (((_carriage->getType() == CarriageType::Lace) ||
              (_carriage->getType() == CarriageType::Knit270)) &&
              (_direction == Direction::Left)) {
          _machine->solenoidShift(solenoidToSet);
        }
        _machine->solenoidMap(solenoidToSet);

        // Set solenoid according to current machine state
        if (!_currentLine.finished) {
          MachineSide machineSide = MachineSide::None;
          // Set solenoid
          if ((selectPosition >= _config.startNeedle) &&
              (selectPosition <= _config.stopNeedle)) {
            _solenoids->set(solenoidToSet,
                            _currentLine.getNeedleValue(selectPosition));
          } else {
            _solenoids->reset(solenoidToSet);
            // Delay _currentLine.finished until safe 
            if (selectPosition < _config.startNeedle) {
              machineSide = MachineSide::Left;    
            } else { // equivalent to > _config.stopNeedle
              machineSide = MachineSide::Right;       
            }
          }
          _currentLine.finished = _carriage->workFinished(machineSide, _direction);
        } else {
          _solenoids->reset(solenoidToSet);
        }
      }
      // Update host SW
      if (_config.continuousReporting) {
        _apiRxIndicateState();
      }
    }
  }
}
