#include "RelayTestController.h"
#include "DebugUtils.h"

// Default constructor; all chip pointers initialised to null until initializePins() is called.
RelayTestController::RelayTestController()
    : _maxChip_C_SET(nullptr), _maxChip_C_RESET(nullptr),
      _maxChip_L_SET(nullptr), _maxChip_L_RESET(nullptr),
      _maxChip_KN(nullptr)
{
}

// Initialises the SPI bus and instantiates all five MAX4820 chips; all relays start OFF.
void RelayTestController::initializePins() {
    DEBUG_PRINTLN("RelayTestController: Initializing SPI and MAX4820 chips...");

    // SPI bus is shared with RelayController; safe to call begin() again with the same pins.
    SPI.begin(TEST_PIN_SCLK, -1, TEST_PIN_MOSI, -1);

    _maxChip_C_SET   = new MAX4820(TEST_PIN_CS_C_SET,   TEST_PIN_RESET);
    _maxChip_C_RESET = new MAX4820(TEST_PIN_CS_C_RESET, TEST_PIN_RESET);
    _maxChip_L_SET   = new MAX4820(TEST_PIN_CS_L_SET,   TEST_PIN_RESET);
    _maxChip_L_RESET = new MAX4820(TEST_PIN_CS_L_RESET, TEST_PIN_RESET);
    _maxChip_KN      = new MAX4820(TEST_PIN_CS_KN,      TEST_PIN_RESET);

    _maxChip_C_SET->begin();
    _maxChip_C_RESET->begin();
    _maxChip_L_SET->begin();
    _maxChip_L_RESET->begin();
    _maxChip_KN->begin();

    DEBUG_PRINTLN("RelayTestController: All 5 MAX4820 chips initialized, all relays OFF");
}

// Pulses the SET or RESET coil of the specified KC latching relay.
void RelayTestController::setC(int relayNum, bool isSet) {
    int idx = relayNum - 1;
    if (isSet) {
        _maxChip_C_SET->pulseRelay(idx, TEST_LATCH_PULSE_MS);
        DEBUG_PRINTF("RelayTestController: KC%d -> SET\n", relayNum);
    } else {
        _maxChip_C_RESET->pulseRelay(idx, TEST_LATCH_PULSE_MS);
        DEBUG_PRINTF("RelayTestController: KC%d -> RESET\n", relayNum);
    }
}

// Pulses the SET or RESET coil of the specified KL latching relay.
void RelayTestController::setL(int relayNum, bool isSet) {
    int idx = relayNum - 1;
    if (isSet) {
        _maxChip_L_SET->pulseRelay(idx, TEST_LATCH_PULSE_MS);
        DEBUG_PRINTF("RelayTestController: KL%d -> SET\n", relayNum);
    } else {
        _maxChip_L_RESET->pulseRelay(idx, TEST_LATCH_PULSE_MS);
        DEBUG_PRINTF("RelayTestController: KL%d -> RESET\n", relayNum);
    }
}

// Directly drives the specified KN non-latching relay on or off.
void RelayTestController::setN(int relayNum, bool on) {
    DEBUG_PRINTF("RelayTestController: KN%d -> %s\n", relayNum, on ? "ON" : "OFF");
    _maxChip_KN->setRelay(relayNum - 1, on);
}

// Pulses all KC and KL RESET coils and turns off all KN relays.
void RelayTestController::resetAll() {
    DEBUG_PRINTLN("RelayTestController: Resetting all relays...");

    // do one relay at a time to limit power consumption
    for(int relay=0; relay < 8; relay++) {
        _maxChip_L_RESET->pulseRelay(relay, 1);
        delay(10);
        _maxChip_C_RESET->pulseRelay(relay, 1);
        delay(10);
        _maxChip_KN->setRelay(relay, 0);
        delay(10);
    }

    DEBUG_PRINTLN("RelayTestController: All relays OFF");
}
