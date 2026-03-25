#include "RelayController.h"
#include "DebugUtils.h" // For DEBUG_PRINTF, DEBUG_PRINTLN
#include <stdio.h>      // For snprintf

RelayController::RelayController() 
    : _maxChip_NonLatch(nullptr),
      _maxChip_LatchSet(nullptr),
      _maxChip_LatchReset(nullptr)
{
    // Constructor - chip instances created in initializePins()
}

// Initialises the SPI bus and creates all three MAX4820 chip instances.
void RelayController::initializePins() {
    DEBUG_PRINTLN("RelayController: Initializing SPI and MAX4820 chips...");
    
    // Initialize SPI bus with explicit pin assignment for ESP32-S3
    // SCK, MISO (unused), MOSI, SS (unused)
    SPI.begin(PIN_SCLK, -1, PIN_MOSI, -1);
    DEBUG_PRINTLN("RelayController: SPI bus initialized");
    
    // Create MAX4820 chip instances
    // Chip 1: Non-latching relays K1-K7
    _maxChip_NonLatch = new MAX4820(PIN_CS_NONLATCH, PIN_RESET);
    _maxChip_NonLatch->begin();
    DEBUG_PRINTLN("RelayController: MAX4820 NonLatch chip initialized (K1-K7)");
    
    // Chip 2: Latching relay SET coils
    _maxChip_LatchSet = new MAX4820(PIN_CS_LATCH_SET, PIN_RESET);
    _maxChip_LatchSet->begin();
    DEBUG_PRINTLN("RelayController: MAX4820 LatchSet chip initialized");
    
    // Chip 3: Latching relay RESET coils  
    _maxChip_LatchReset = new MAX4820(PIN_CS_LATCH_RESET, PIN_RESET);
    _maxChip_LatchReset->begin();
    DEBUG_PRINTLN("RelayController: MAX4820 LatchReset chip initialized");
    
    DEBUG_PRINTLN("RelayController: All MAX4820 chips ready, all relays OFF");
}

// Iterates over the action array, routing each relay to the appropriate MAX4820 chip.
String RelayController::applyActions(const pinValue_t actions[], size_t count) {
    String details = ""; 
    char buffer[100]; 
    const size_t bufferSize = sizeof(buffer);
    
    DEBUG_PRINTF("  RelayController: Applying %zu relay actions:\n", count);
    
    for (size_t i = 0; i < count; i++) {
        const char* relayName = getRelayName(actions[i].relay);
        bool turnOn = (actions[i].value == HIGH);
        const char* stateStr = turnOn ? "HIGH" : "LOW";
        
        DEBUG_PRINTF("     - %s -> %s\n", relayName, stateStr);
        snprintf(buffer, bufferSize, " - %s set to %s\n", relayName, stateStr);
        details += buffer;
        
        // Route to appropriate MAX4820 chip based on relay ID
        if (actions[i].relay >= RELAY_K1 && actions[i].relay <= RELAY_K7) {
            // Non-latching relay: use setRelay on chip 1
            uint8_t relayIndex = (uint8_t)actions[i].relay;  // K1=0, K7=6
            _maxChip_NonLatch->setRelay(relayIndex, turnOn);
        } 
        else if (actions[i].relay == RELAY_LK99_SET) {
            // Setting LK99 SET coil directly (unusual - normally use pulse())
            _maxChip_LatchSet->setRelay(LK99_RELAY_INDEX, turnOn);
        }
        else if (actions[i].relay == RELAY_LK99_RESET) {
            // Setting LK99 RESET coil directly (unusual - normally use pulse())
            _maxChip_LatchReset->setRelay(LK99_RELAY_INDEX, turnOn);
        }
        else {
            DEBUG_PRINTF("     - WARNING: Unknown relay ID %d\n", actions[i].relay);
        }
    }
    
    if (details.endsWith("\n")) {
        details.remove(details.length() - 1);
    }
    return details;
}

// Pulses the specified relay coil for its required duration and returns a log string.
String RelayController::pulse(const relay_id_t relay) {
    char buffer[100]; 
    const size_t bufferSize = sizeof(buffer);
    const char* relayName = getRelayName(relay);
    
    DEBUG_PRINTF("RelayController: Pulsing relay %s for %dms\n", relayName, LATCH_PULSE_MS);
    
    if (relay == RELAY_LK99_SET) {
        // Pulse SET coil on chip 2
        _maxChip_LatchSet->pulseRelay(LK99_RELAY_INDEX, LATCH_PULSE_MS);
    }
    else if (relay == RELAY_LK99_RESET) {
        // Pulse RESET coil on chip 3
        _maxChip_LatchReset->pulseRelay(LK99_RELAY_INDEX, LATCH_PULSE_MS);
    }
    else if (relay >= RELAY_K1 && relay <= RELAY_K7) {
        // Non-latching relay pulse (turn on, wait, turn off)
        uint8_t relayIndex = (uint8_t)relay;
        _maxChip_NonLatch->setRelay(relayIndex, true);
        delay(100);  // 100ms pulse for non-latching
        _maxChip_NonLatch->setRelay(relayIndex, false);
    }
    else {
        DEBUG_PRINTF("RelayController: WARNING - Unknown relay %d for pulse\n", relay);
        snprintf(buffer, bufferSize, "\n - Unknown relay pulsed");
        return String(buffer);
    }
    
    snprintf(buffer, bufferSize, "\n - %s Pulsed", relayName);
    return String(buffer);
}

// Directly sets the relay output high or low on the appropriate MAX4820 chip.
void RelayController::setRelay(const relay_id_t relay, bool on) {
    const char* relayName = getRelayName(relay);
    const char* stateStr = on ? "ON" : "OFF";
    
    DEBUG_PRINTF("RelayController: Setting %s to %s\n", relayName, stateStr);
    
    if (relay >= RELAY_K1 && relay <= RELAY_K7) {
        // Non-latching relay: use setRelay on chip 1
        uint8_t relayIndex = (uint8_t)relay;  // K1=0, K7=6
        _maxChip_NonLatch->setRelay(relayIndex, on);
    } 
    else if (relay == RELAY_LK99_SET) {
        _maxChip_LatchSet->setRelay(LK99_RELAY_INDEX, on);
    }
    else if (relay == RELAY_LK99_RESET) {
        _maxChip_LatchReset->setRelay(LK99_RELAY_INDEX, on);
    }
    else {
        DEBUG_PRINTF("RelayController: WARNING - Unknown relay %d\n", relay);
    }
}

// Maps a latching relay ID to its zero-based output index on the MAX4820 chip.
uint8_t RelayController::getLatchingRelayIndex(relay_id_t relay) {
    // Map latching relay IDs to their chip output index (0-7)
    switch (relay) {
        case RELAY_LK99_SET:
        case RELAY_LK99_RESET:
            return LK99_RELAY_INDEX;  // LK99 is on OUT1 (index 0)
        default:
            return 0;
    }
}

// Maps relay ID enum values to printable name strings for debug output.
const char* RelayController::getRelayName(relay_id_t relay_val) {
    switch (relay_val) {
        case RELAY_K1: return "RELAY_K1"; 
        case RELAY_K2: return "RELAY_K2";
        case RELAY_K3: return "RELAY_K3"; 
        case RELAY_K4: return "RELAY_K4";
        case RELAY_K5: return "RELAY_K5"; 
        case RELAY_K6: return "RELAY_K6";
        case RELAY_K7: return "RELAY_K7";
        case RELAY_LK99_SET: return "RELAY_LK99_SET"; 
        case RELAY_LK99_RESET: return "RELAY_LK99_RESET";
        default: return "UNKNOWN_RELAY";
    }
}
