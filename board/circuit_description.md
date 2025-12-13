Theory of operation of remote-tuned feedpoint with gapped elements
------------------------------------------------------------------

Basic idea: provide a 50 Ohm match **at the feed point** of the antenna, not
the "shack" end. If achieved, this arrangement makes tuning almost insensitive
to cable length, and reduces loss when using cheap coax feedline.
The feedpoint unit will provide Short/Open/Load cal standards for a
VNA, so that -- after calibration -- the antenna impedance can be
measured directly at the feedpoint.

A reasonable goal is 20 W capability and an SWR < 1.5:1
from 3.5 to 30 MHz. 

The proposed design will incorporate a "Collins Tuner" using an
adjustable series inductor and adjustable shunt capacitor. Said
shunt capacitor can be placed on the antenna side of the inductor or
the radio side, depending on the impedance match required.

Rather than continuously variable L and C, the proposed design will
use binary-weighted fixed inductors and capacitors which are
switched in and out of circuit with latching relays. Latching
relays will hold their state so that DC power to the feed point
unit is not required after achieving an impedance match.

If the operator switches to a different frequency, DC power
must be supplied long enough to change the relay settings. The proposed
design will send both RF and DC through the coax feed, using a Bias-Tee
in the feed point unit to separate DC from RF. One could consider
battery operation, perhaps with a solar charger, but that is for a later design.
The proposed design will use WiFi to connect to the uP from a Cell phone app.
The app will allow configuring the calibration network for a VNA sweep and then
allow selecting a particular tuning network.

In a one-time sweep -- done in a laboratory setting -- a computer-controlled
VNA will measure and accumulate S-parameters for all possible
L and C relay settings. These data are stored in a "personality" file
associated with a particular tuner. Possibly (to be determined), with
careful circuit board design and high-tolerance components, a single personality
file would be sufficiently accurate across different fabrications of the
same design, so it would need to be done only once even when making
multiple copies of the design. A fall-back position
is to repeat the sweep for each fabricated unit and somehow key a
personality file to a particular fabricated unit (e.g., with a serial number).
This is to be determined.

Given the personality file and a sweep of the antenna itself (based on
the feedpoint calibration above) it is possible to **compute** a best
matching network, and then select that network before transmitting.
In this sense the tuning is silent or stealth. This scheme
is substantially different from "hunting" tuners which run use
a search algorithm to minimize SWR while transmitting a test tone.
This scheme also avoids the possible trap of a sub-optimal "false root"
because it uses a computer to do a comprehensive ssweep over all possible tuning networks.

The proposed design is a dipole using 450 Ohm "ladder line" as **radiating elements**,
rather than a feed. Specifically, the two ends of an individual radiating
element are shorted together so it acts as one "wide" wire. Experiments
show that this arrangement makes the antenna more broad-band than using
a single wire for each radiating element. In addition, it is possible to
run a DC control signal along the two wires of the ladder to change the
state of a latching relay to adjust the electrical length of each radiating
element -- a "gap" dipole (groan). The proposed design has two elements
about 15 feet long (total 30 feet) with gaps about 8 feet from the
feedpoint, again symmetric on each side. Sweeps with a VNA indicate
that the shorter elements will be easier to match for frequencies
above 20 MHz (e.g., 15 and 10 meter bands).

Important note: in the proposed design, the relays are never "hot switched",
that is asked to change state while transmitting.  At modest power levels, this
can be an advantage for relay life.

Here is a brief overview of the circuit, with reference to the attached schematic diagram.
The design will use both latching and non-latching relay with 3.3 V coils
to be compatible with the digital circuitry. The non-latching relays
are designated Kxx and latching relays as KLBxx or KLMxx, where B stands for
bipolar and M for monopolar. In general, a latching relay requires only
a brief pulse of DC (circa 50 ms) to change state. A bipolar latching relay has only one coil --
the state is switched by changing the _polarity_ of the DC control pulse
applied across its single pair of +/- terminals. A monopolar latching relay
has two coils with two sets of +/- pairs. For monopolar relays both -
terminal are grounded and separate digital control lines are routed to the
two + terminals, designated "set" and "reset". Note the use of shunt diodes to
protect the digital circuity from an inductive "kick back" when a relay switches.
There are only two bipolar relays (polarity sensitive) which are the gap selectors
at the remote end of the 8-foot ladder lines. A non-latching DPDT relay (to be described)
will allow swapping the polarity of the DC control signal to be sent to the gap relays.

Recall two important points:

1. Never switch relays with RF across them
2. After selecting an impedance match at a specific frequency,
no DC power is needed to use the antenna. It might be advantageous, also,
to power down the uP when not needed to avoid injecting digital noise; TBD.

Point 2 requires that the unenergized or default state of the non-latching relays
(Kxx) route RF through the matching network to the balun.

The circuit has several blocks:

1. Input Bias-Tee to separate RF from DC used to run the uP and relays and
associated 3.3 V voltage regulator
2. Calibration network (K1,K2,K3,K4) for Short/Open/Load/Thru
3. LC match network, using all monopolar latching relays
4. Balun to convert to single-ended RF
5. Gap relays (two bipolar latching) and non-latching relays (KB1,KB2,K5,K6,K7)
to select the element length and to select polarity of the control signal to be sent to the gap relays
6. uP and relay drivers


RF+DC comes into coax connector J1, then to a bias-tee formed by L1/C1. Diode D1 is an "idiot" diode to protect
the regulator from reverse polarity of the DC feed. The RF then goes to the K1/K2/K3 "tree" which selects Open/Short/Load
cal with equal signal path lengths. The schematic shows both armatures of each DPDT relay strapped together
in parallel. A SPDT relay would work here, but the design can use the same DPDT relay everywhere. DPDT capability
is needed for K5,K6 and K7.

These non-latching relays are shown in the unenrgized
state. To select, e.g., a Short cal, energize K1 and K2. In the default state, the RF
gets a thru to K4. K4 by default (unergenized) routes the RF through the matching network, to be
described later. However, the user may briefly energize K4 to get an antenna sweep or "through" after
first doing an O/S/L cal. There will be a small parasitic impedance due to K4 -- hopefully
this will be equal for the "to balun" or "to matching network" connection and will cancel out.

The Balun converts the floating differential dipole elements to single-ended RF w.r.t. system
ground. KB1 and KB2 (the only bipolar relays) open or close the gap elements with a dual-polarity control
voltage applied across the ladder line. Relays K5,K6,K7 direct DC or RF to the elements and
select the polarity for KB1 and KB2. Suppose we want to apply positive control to KB1 and KB2
(presumably to switch their state). In the default state, K7 applies positive drive to the upper
ladder line wire and ground to the lower ladder line wire. If K7 is energized, the polarity is
reversed, with positive drive applied to the lower ladder line wire and the upper ladder line wire grounded.
However, this only happens when K5 and K6 are both energized. Hence, to change the
state of the gap relays, make a polarity choice with K7, then briefly energize K5 and K6 together
to "pulse" the gap relays. If K7 is energized, it can be unenergized after the pulse of K5 and K6.

The tuning network, shown as a separate box, must provide an adjustable series inductor,
an adjustable shunt capacitor and a way to chose if the cap is on the antenna side or the
radio side of the inductor. This is the function of KM1 (monopolar latching relay).

There should be provision on the PCB to access these components
in isolation for the one-time personality sweep. This indicated as TP1,2,3 on the schematic.
Somehow, completely disconnect the inductor from the shunt cap and the input/output ports (A,B)
then extract 2-port S-params for all settings. Possibly small RF connectors soldered close to the
PCB with short jumpers to disconnect from the main circuitry. Same idea for the capacitors
network and TP3.

The inductor chain is a series connection of KM relays which each select a fixed
inductor in series or do a pass through. The capacitor chain would be parallel
relays. Some thought should be given to minimizing parasitic capacitance and
parasitic inductance. To be discussed.

So, here is a complete sequence of operation.

1. Connect DC supply and VNA in shack through a shack-side Bias tee.

2. Do O/S/L of the VNA
   O: energize K1
   S: energize K1, K2
   L: energize K3

3. Sweep the antenna through the balun by energizing K4.
(gap selection to be discussed later)

4. Compute desired matching network at an anticipated frequency.

5. Set tuner: set or reset KM1 to put shunt cap on one side
or the other of the inductor. Select various KMxxx in the inductor chain
and cap chain for desired value.

6. If desired, re-sweep vna using previous cal data, at least in the
vicinity of the chosen frequency to verify the match (now, going through
the programed tuning network, *not* the default setting of K4)

7. Gap selection (either for initial sweep or subsequent QSY)
   Select polarity by energizing or not K7;  briefly energize ("pulse")
   K5, K6; unenrgize K7 if energized











