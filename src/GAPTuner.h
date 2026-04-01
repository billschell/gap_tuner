#ifndef GAP_TUNER_H
#define GAP_TUNER_H

#include <Arduino.h> // For String, size_t (implicitly for array size calculations if needed)
#include "RelayController.h" // For pinValue_t and RELAY_Kx enums (used in static arrays)

class GAPTuner {
public:
    enum class ButtonID : int {
        ANTENNA_SHORT = 2, ANTENNA_LONG  = 1,
        TUNING_NONE   = 3, TUNING_1      = 4, TUNING_2      = 5,
        CAL_OPEN      = 6, CAL_SHORT     = 7, CAL_LOAD      = 8,
        UNKNOWN       = 0
    };
    static constexpr int NUM_ACTIONS = 8;

    // Stores a reference to the relay controller used to drive hardware.
    GAPTuner(RelayController& rc);

    // Sets all relays to their safe power-up defaults (all off).
    void applyDefaultState();
    // Translates a numeric button ID into the corresponding relay sequence; populates outMessage with a status string.
    String processButtonAction(int buttonId_int, String& outMessage);

private:
    // Static relay configurations for each button action
    // Each array contains pin-value pairs to set the relays accordingly

    // Relay configuration for setting antenna length to "Short"
    static constexpr pinValue_t s_antennaLengthShort[] = {{RELAY_K7, LOW}};
    // Relay configuration for setting antenna length to "Long"
    static constexpr pinValue_t s_antennaLengthLong[]  = {{RELAY_K7, HIGH}};

    // Relay configuration for setting Tuning Network to "None" (all tuning relays off)
    static constexpr pinValue_t s_tuningNetNone[]      = {{RELAY_K1, LOW}, {RELAY_K2, LOW}, {RELAY_K3, LOW}, {RELAY_K4, HIGH}, {RELAY_K5, LOW}, {RELAY_K6, LOW}, {RELAY_K7, LOW}};
    // Relay configuration for setting Tuning Network to "1"
    static constexpr pinValue_t s_tuningNet1[]         = {{RELAY_K1, LOW}, {RELAY_K2, LOW}, {RELAY_K3, LOW}, {RELAY_K4, LOW}, {RELAY_K5, LOW}, {RELAY_K6, LOW}, {RELAY_K7, LOW}};
    // Relay configuration for setting Tuning Network to "2"
    static constexpr pinValue_t s_tuningNet2[]         = {{RELAY_K1, LOW}, {RELAY_K2, LOW}, {RELAY_K3, LOW}, {RELAY_K4, LOW}, {RELAY_K5, LOW}, {RELAY_K6, LOW}, {RELAY_K7, LOW}}; // Example correction

    // Relay configuration for "Open" calibration state
    static constexpr pinValue_t s_calOpen[]            = {{RELAY_K1, HIGH},{RELAY_K2, LOW}, {RELAY_K3, LOW}, {RELAY_K4, LOW }};
    // Relay configuration for "Short" calibration state
    static constexpr pinValue_t s_calShort[]           = {{RELAY_K1, HIGH},{RELAY_K2, HIGH},{RELAY_K3, LOW}, {RELAY_K4, LOW}};
    // Relay configuration for "Load" calibration state
    static constexpr pinValue_t s_calLoad[]            = {{RELAY_K1, LOW}, {RELAY_K2, LOW}, {RELAY_K3, HIGH},{RELAY_K4, LOW}};

    // Relay configuration to turn all relays OFF (default power-up state)
    static constexpr pinValue_t s_allOff[]  = {{RELAY_K1, LOW}, {RELAY_K2, LOW}, {RELAY_K3, LOW}, {RELAY_K4, LOW}, {RELAY_K5, LOW}, {RELAY_K6, LOW},
        {RELAY_K7, LOW}, {RELAY_LK99_SET, LOW}, {RELAY_LK99_RESET, LOW}};



    // Applies a static relay configuration array and sets outMsg to the given prefix string.
    template<size_t N>
    String applyRelayActions(String& outMsg, const char* successMsgPrefix, const pinValue_t (&actions)[N]);

    RelayController& _relayController;
    // Returns a human-readable name for the given ButtonID for debug logging.
    const char* getButtonName(ButtonID buttonId);
};

// Template method definition must be in the header or included if not specialized for known types.
template<size_t N>
String GAPTuner::applyRelayActions(String& outMsg, const char* successMsgPrefix, const pinValue_t (&actions)[N]) {
    outMsg = successMsgPrefix;
    return _relayController.applyActions(actions, N); // N is the size deduced by the template
}

#endif // GAP_TUNER_H
