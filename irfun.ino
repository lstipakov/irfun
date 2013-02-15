#include "ir_constants.h"

#define IRpin_PIN      PIND
#define IRpin          2
#define IRledPin       3

// the maximum pulse we'll listen for - 65 milliseconds is a long time
#define MAXPULSE 65000

// what our timing resolution should be, larger is better
// as its more 'precise' - but too large and you wont get
// accurate timing
#define RESOLUTION 25

#define MAXPULSES 100
#define FUZZINESS 0.25

// we will store up to MAXPULSES pulse pairs (this is -a lot-)
uint16_t pulses[MAXPULSES][2];  // pair is high and low pulse 
uint8_t currentpulse = 0; // index for pulses we're storing

void setup(void) {
  Serial.begin(9600);
  Serial.println("Ready");

  // initialize the IR digital pin as an output:
  pinMode(IRledPin, OUTPUT);
  pinMode(IRpin, INPUT);

  attachInterrupt(0, interrupt_handler, LOW);
}

unsigned long last = millis();
boolean handling = false;

boolean got_power = false;
boolean got_lamp = false;

void interrupt_handler() {
  if (millis() - last < 200) return; 

  int num = listenForIR();
  Serial.print("Pulse ");
  Serial.println(num);

  if (IRCompare(LAMP_LAMP, sizeof(LAMP_LAMP) / 4, num)) {
    got_lamp = true;
  } 
  else
    if (IRCompare(TV_POWER, sizeof(TV_POWER) / 4, num)) {
      got_power = true;
    }

  last = millis();
}

int listenForIR(void) {
  currentpulse = 0;

  while (1) {
    uint16_t highpulse, lowpulse;  // temporary storage timing
    highpulse = lowpulse = 0; // start out with no pulse length

    // pin HIGHT  
    while (IRpin_PIN & _BV(IRpin)) {
      highpulse++;
      delayMicroseconds(RESOLUTION);

      if (((highpulse >= MAXPULSE) && (currentpulse != 0))|| currentpulse == MAXPULSES) {
        return currentpulse;
      }
    }
    // we didn't time out so lets stash the reading
    pulses[currentpulse][0] = highpulse;

    // pin LOW
    while (! (IRpin_PIN & _BV(IRpin))) {
      lowpulse++;
      delayMicroseconds(RESOLUTION);
      if (((lowpulse >= MAXPULSE)  && (currentpulse != 0))|| currentpulse == MAXPULSES) {
        return currentpulse;
      }
    }
    pulses[currentpulse][1] = lowpulse;

    // we read one high-low pulse successfully, continue!
    currentpulse++;
  }
}

boolean IRCompare(int ref[], int refSize, int readedSize) {
  int len = min(refSize, readedSize);

  // filter garbage
  if (len < 6) return false;

  //if (len < refSize) return false;

  for (int i = 1; i < len; ++ i) {
    int refVal = ref[i * 2];
    int pulseVal = RESOLUTION * pulses[i][0] / 10;

    int d = abs(refVal - pulseVal);

    int t = refVal * FUZZINESS;
    if (d > t) {
      Serial.println(d);
      Serial.println(refVal, DEC);
      Serial.println(pulseVal, DEC);
      return false;  
    }

    refVal = ref[(i * 2) + 1];
    pulseVal = RESOLUTION * pulses[i][1] / 10;

    d = abs(refVal - pulseVal);

    t = refVal * FUZZINESS;
    if (d > t) {
      Serial.println(d);
      Serial.println(refVal, DEC);
      Serial.println(pulseVal, DEC);
      return false;  
    }
  }

  return true;
}

void loop(void) {
  if (got_lamp) {
    Serial.println("LAMP_LAMP");
    sendCommand(TV_POWER, sizeof(TV_POWER) / sizeof(TV_POWER[0]));
    got_lamp = false;
  }
  if (got_power) {
    Serial.println("TV_POWER");
    sendCommand(LAMP_LAMP, sizeof(LAMP_LAMP) / sizeof(LAMP_LAMP[0]));
    got_power = false;
  }
  //Serial.println("*");
  //delay(500);
}

// This procedure sends a 38KHz pulse to the IRledPin 
// for a certain # of microseconds. We'll use this whenever we need to send codes
void pulseIR(long microsecs) {
  // we'll count down from the number of microseconds we are told to wait

  //Serial.println(microsecs, DEC);

  cli();  // this turns off any background interrupts

  while (microsecs > 0) {
    // 38 kHz is about 13 microseconds high and 13 microseconds low
    digitalWrite(IRledPin, HIGH);  // this takes about 3 microseconds to happen
    delayMicroseconds(10);         // hang out for 10 microseconds, you can also change this to 9 if its not working
    digitalWrite(IRledPin, LOW);   // this also takes about 3 microseconds
    delayMicroseconds(10);         // hang out for 10 microseconds, you can also change this to 9 if its not working

    // so 26 microseconds altogether
    microsecs -= 26;
  }
  sei();  // this turns them back on
}

void sendCommand(int data[], int len) {
  for (int i = 0; i < len; ++ i) {
    if (i & 1) {
      pulseIR(data[i] * 10);
    } 
    else {
      delayMicroseconds(data[i] * 10);
    }
  }
}

void printpulses(void) {
  Serial.println("\n\r\n\rReceived: \n\rOFF \tON");
  for (uint8_t i = 0; i < currentpulse; i++) {
    Serial.print(pulses[i][0] * RESOLUTION, DEC);
    Serial.print(" usec, ");
    Serial.print(pulses[i][1] * RESOLUTION, DEC);
    Serial.println(" usec");
  }
}





