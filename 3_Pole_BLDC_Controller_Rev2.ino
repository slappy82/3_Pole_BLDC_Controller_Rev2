// Campbell Maxwell
// 2025

/*
 * I/O Pin Control
 * x is the port / bank of pins, n is the bit / pin
 * DDRx is the data direction register (0 for input, 1 for output). DDRxn for a specific pin bit
 * PORTx is the data register and holds the state of each pin - PORTxn. If set as an input pin, 1 sets pullup resistor on.
 * PINx is the pin input register and holds the state when affected externally. PINxn for specific pin bit. 'Read only'
 * Digital pins 0-7 = PORTD and PDn, DDRD and DDRDn, and PIND and PINDn
 * Digital pins 8-13 = PORTB and PB(n-8), DDRB and DDB(n-8), and PINB and PINB(n-8)
 */

/*
 * Interrupts
 * TCNT0 - Timer/Counter 0 value register
 * TCCR0x - Timer/Counter Control register (TCCR0A and TCCR0B both have different functions)
 * OCR0x - Output Compare register (OCR0A and OCR0B)
 * OC0x - Output Compare pins (OC0A and OC0B) - can be used to output HI or LO, or toggled to provide PWM
 * TIMSK0 - Timer/Counter Interrupt Mask - used to enable specific interrupts and enable interrupt overflow
 * TIFR0 - Timer/Counter Interrupt Flag - can write to it, but mostly used by hardware to set/clear flags when interrupts occur
 * 
 * Select the clock source - set the CS0[2:0] bits in TCCR0B - TCCR0B[2:0]. 
 * CS02     CS01    CS00    Result
 * 0        0       0       No clock, timer stopped
 * 0        0       1       Clk Freq
 * 0        1       0       Clk Freq/8
 * 0        1       1       Clk Freq/64
 * 1        0       0       Clk Freq/256
 * 1        0       1       Clk Freq/1024
 * 1        1       0       External, T0 pin, Falling edge
 * 1        1       1       External, T0 pin, Rising edge
 * 
 * TCCR0B[3] is WGM02 which is used with TCCR0A[1:0] to provide WGM0[2:0] which determines the Timer/Counter mode: Normal, CTC, PWM etc
 *      Most likely going to use one of: 
 *          WGM0[2:0]=0b101 (phase correct PWM with OCRA set for top value for compare, updating OCR0x at OCRA and setting TOV flag at 0),     
 *          WGM0[2:0]=0b111 (fast PWM, same as above except updating and flag setting reversed)
 *          WGM0[2:0]=0b010 (CTC mode, OCRA for top value, updates OCR0x constantly and flag set at 0bFF)
 * TCNT0 holds the current value of the Timer/Counter
 * OCR0A holds the value set to be constantly compared against the TCNT0 - can use a match to trigger interrupt or output to OC0A pin
 * TIMSK0 bits set here (as well as in the Status Register - SREG[7]=1. Also note that mentions of the I-bit refer to this) can enable/disable specific interrupts
 *      TIMSK0[2]=1 enables Output Compare B interrupts (OCR0B etc)
 *      TIMSK0[1]=1 enables Output Compare A interrupts (OCR0A etc)
 *      TIMSK0[0]=1 enables interrupt overflow, requires TIFR0[0]=1 also set, will trigger the overflow interrupt
 */

 /*
  * PWM
  * Using the IR2110 drivers as they are independant as trying to make it work with the previous ones by using the SD as a state toggle will be a problem with timings etc
  * 
  */

   /*
  * Gate Driver (IR2184)
  * -------------
  * |IN   0   Vb|
  * |           |
  * |SD       HO|
  * |           |
  * |COM      Vs|
  * |           |
  * |LO      Vcc|
  * -------------
  * 
  * IN - Control / toggle states of HO and LO (In as HIGH sets HO as HIGH and LO as LOW, and vica versa)
  * SD (Shutdown, inverted) - Shutdown driver when set as LOW, enable driver when HIGH
  * COM (Common) - GND
  * LO (Low Out) - Output to control the gate of the MOSFET controlling the currently active low side of the H bridge
  * HO (High Out) - Output to control the gate of the MOSFET controlling the currently active high side of the H bridge
  * Vcc - Voltage input to driver
  * Vb - (High side floating supply) - Uses a parallel bootstrap capacitor from Vcc (to Vs) to ensure the Vgs of the high side FET is enough to activate it (eg more than Vcc)
  * Vs - (High side floating supply return) - Return for the high side source / low side drain so the bootstrap capacitor can allow the Vgs to be higher than the mosfet supply
  */

  /*
   * Gate Driver (IR2110 / IR2113)
  * -------------
  * |LO   0   NC|
  * |           |           LIN = Low In (from MCU)
  * |COM     Vss|           HIN = High In (from MCU)
  * |           |           LO = Low Out (to Gate)
  * |Vcc     LIN|           HO = High Out (to Gate)
  * |           |           SD = Disable / Turn Off
  * |NC       SD|           NC = Not Connected (can float)
  * |           |           Vcc = Supply in (10v-20v)
  * |Vs      HIN|           COM = GND for supply
  * |           |           Vdd = Logic supply in (5v)
  * |Vb      Vdd|           Vss = GND for logic supply
  * |           |           Vb = Floating high side voltage (increased by capacitor to Vs)
  * |HO       NC|           Vs = High side load sink
  * -------------
   */

#define console Serial

bool pwmToggleHi;
unsigned char currentStep;
unsigned char motorPhase[6];

void setup() {
    // Initial pin configuration, Digital GPIO 2-7 set as output in LO state 
    DDRD = 252;                             // 1111 1100
    PORTD = 0;                              // 0000 0000

    // Initialise global variables
    currentStep = 0;
    pwmToggleHi = true;
    motorPhase[68, 132, 136, 40, 48, 80];   // More efficient than bit shifting operations for consistent values

    // Serial monitor for debugging
    console.begin(9600);
    console.println("Starting..."); 

    // Initialise PWM interrupts

    TCCR0B |= (0 << WGM02) | (0 << CS02) | (1 << CS01) | (1 << CS00);   // WGM0n to set the mode (CTC), CS0n to set the clock and prescaler (clk/64 prescaler)
    TCCR0A |= (1 << WGM01) | (0 << WGM00);
    OCR0B = 100;                                                        // Value to act as the maximum 
    TIMSK0 |= 0b00000100;
    SREG |= 0b10000000;
}

void loop() {

}

// PWM - will likely want to adjust OCR0B in here
ISR(TIMER0_COMPB) {
    PORTD = motorPhase[currentStep] * pwmToggleHi;
    pwmToggleHi = !pwmToggleHi;    
}

// Rotor Position
ISR(INT0_vect) {                            // Digital pin 
    PORTD = 0;                              // Prevent shoot-through current
    (++currentStep) %= 6;
    PORTD = motorPhase[currentStep];        // Reset to next rotor position in the sequence - could wait for PWM to handle?
}


