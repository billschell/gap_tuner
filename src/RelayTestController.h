#ifndef RELAY_TEST_CONTROLLER_H
#define RELAY_TEST_CONTROLLER_H

#include <Arduino.h>
#include <SPI.h>
#include <MAX4820.h>

// Pin definitions — new 5-chip hardware platform
// SPI bus (shared with RelayController)
#define TEST_PIN_MOSI           11
#define TEST_PIN_SCLK           12

// Chip select pins
#define TEST_PIN_CS_C_SET       10   // maxChip1: KC1-KC8 SET coils
#define TEST_PIN_CS_C_RESET     13   // maxChip2: KC1-KC8 RESET coils
#define TEST_PIN_CS_L_SET       14   // maxChip3: KL1-KL8 SET coils
#define TEST_PIN_CS_L_RESET     21   // maxChip4: KL1-KL8 RESET coils
#define TEST_PIN_CS_KN           1   // maxChip5: KN1-KN7 non-latching relays

// Shared active-low RESET (all outputs OFF)
#define TEST_PIN_RESET          42

// Pulse width for latching relays (KEMET EE2-3TNU datasheet: >10ms)
#define TEST_LATCH_PULSE_MS     15

class RelayTestController {
public:
    // Default constructor; chip instances are created in initializePins().
    RelayTestController();
    // Initialises the SPI bus and all five MAX4820 chips.
    void initializePins();

    // Set or reset a C latching relay. isSet=true pulses SET coil, false pulses RESET coil.
    void setC(int relayNum, bool isSet);

    // Set or reset an L latching relay.
    void setL(int relayNum, bool isSet);

    // Directly set a KN non-latching relay ON or OFF.
    void setN(int relayNum, bool on);

    // Pulse all KC and KL RESET coils, turn off all KN relays.
    void resetAll();

private:
    MAX4820* _maxChip_C_SET;
    MAX4820* _maxChip_C_RESET;
    MAX4820* _maxChip_L_SET;
    MAX4820* _maxChip_L_RESET;
    MAX4820* _maxChip_KN;
};

#endif // RELAY_TEST_CONTROLLER_H
