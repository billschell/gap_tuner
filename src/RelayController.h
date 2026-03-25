#ifndef RELAY_CONTROLLER_H
#define RELAY_CONTROLLER_H

#include <Arduino.h>    // For String, HIGH, LOW, uint8_t
#include <SPI.h>
#include <MAX4820.h>

// --- Pin Definitions for MAX4820 control (from model_code.cpp) ---

// SPI pins (ESP32-S3)
#define PIN_MOSI    11
#define PIN_SCLK    12

// Chip Select pins for each MAX4820
#define PIN_CS_NONLATCH     1    // Non-latching relays K1-K7
#define PIN_CS_LATCH_SET   10    // Latching relay SET coils
#define PIN_CS_LATCH_RESET 13    // Latching relay RESET coils

// Shared RESET pin (active low, all outputs OFF)
#define PIN_RESET   42

// Pulse timing for latching relays (per KEMET EE2 datasheet: >10ms required)
#define LATCH_PULSE_MS 15

// --- Abstract Relay Identifiers ---
// These logical IDs map to physical MAX4820 outputs
typedef enum relay_id {
    // Non-latching relays on Chip 1 (maxChip_NonLatch)
    RELAY_K1 = 0,   // OUT1 on chip 1
    RELAY_K2 = 1,   // OUT2 on chip 1
    RELAY_K3 = 2,   // OUT3 on chip 1
    RELAY_K4 = 3,   // OUT4 on chip 1
    RELAY_K5 = 4,   // OUT5 on chip 1
    RELAY_K6 = 5,   // OUT6 on chip 1
    RELAY_K7 = 6,   // OUT7 on chip 1
    
    // Latching relay identifiers (SET on Chip 2, RESET on Chip 3)
    // These are output positions on the latching chips
    RELAY_LK99_SET   = 100,  // LK99 SET coil: OUT1 on chip 2
    RELAY_LK99_RESET = 101   // LK99 RESET coil: OUT1 on chip 3
} relay_id_t;

// Relay output index on latching chips (0-7 maps to OUT1-OUT8)
#define LK99_RELAY_INDEX 0

typedef struct PinValueStruct {
    relay_id_t  relay;
    uint8_t     value;
} pinValue_t;

class RelayController {
public:
    // Default constructor; chip instances are created in initializePins().
    RelayController();
    // Initialises the SPI bus and instantiates all three MAX4820 chips.
    void initializePins();
    // Drives a batch of relay outputs described by an array of pin-value pairs.
    String applyActions(const pinValue_t actions[], size_t count);
    // Pulses one relay coil for the datasheet-specified duration (15 ms latching, 100 ms non-latching).
    String pulse(const relay_id_t relay);
    // Directly drives one relay output high or low without any timed pulse.
    void setRelay(const relay_id_t relay, bool on);

private:
    // MAX4820 chip instances
    MAX4820* _maxChip_NonLatch;     // K1-K7 non-latching relays
    MAX4820* _maxChip_LatchSet;     // SET coils for latching relays
    MAX4820* _maxChip_LatchReset;   // RESET coils for latching relays
    
    // Returns a printable name for the relay ID for debug output.
    const char* getRelayName(relay_id_t relay_val);

    // Helper to get relay index for latching relays
    uint8_t getLatchingRelayIndex(relay_id_t relay);
};

#endif // RELAY_CONTROLLER_H
