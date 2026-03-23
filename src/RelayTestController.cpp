#include "RelayTestController.h"
#include "DebugUtils.h"

RelayTestController::RelayTestController()
    : _maxChip_C_SET(nullptr), _maxChip_C_RESET(nullptr),
      _maxChip_L_SET(nullptr), _maxChip_L_RESET(nullptr),
      _maxChip_KN(nullptr)
{
}

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

void RelayTestController::setN(int relayNum, bool on) {
    DEBUG_PRINTF("RelayTestController: KN%d -> %s\n", relayNum, on ? "ON" : "OFF");
    _maxChip_KN->setRelay(relayNum - 1, on);
}

void RelayTestController::resetAll() {
    DEBUG_PRINTLN("RelayTestController: Resetting all relays...");

    _maxChip_C_RESET->pulseRelays(0xFF, TEST_LATCH_PULSE_MS);
    DEBUG_PRINTLN("RelayTestController: KC1-KC8 RESET");

    delay(10);

    _maxChip_L_RESET->pulseRelays(0xFF, TEST_LATCH_PULSE_MS);
    DEBUG_PRINTLN("RelayTestController: KL1-KL8 RESET");

    delay(10);

    _maxChip_KN->setRelays(0x00);
    DEBUG_PRINTLN("RelayTestController: KN1-KN7 OFF");
}
