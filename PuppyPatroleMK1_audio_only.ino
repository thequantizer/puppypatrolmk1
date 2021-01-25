char charBuffer[100];

//period of one pwm cycle is 50HZ = .02
//Want to have a timeout that is twice as long as any one cycle should be.
//(1/50)*2/10^-6 = 40000. Then added a fudge factor of 10,000 to bring it to 50,000
//(1/50) = Hz to seconds
// *2 = Twice the cycle
// /10^-6 = seconds to microseconds
#define PWM_TIMEOUT_MS 50000

#define AUDIO_ON_THRESHOLD 1250

#define CH6_SWITCH_B_PIN 9
#define AUDIO_PWM_PIN 6

typedef struct {
  int audio;
} Controls;

Controls NewControls() {
  Controls con = {0};
  return con;
}

#define HIGH_FREQ 16000
#define LOW_FREQ 32000

void setup() {
  Serial.begin(9600);
  Serial.println("Starting puppy patrol");
  
  noInterrupts(); // disable all interrupts
  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0
  // set compare match register for 1hz increments
  OCR1A = HIGH_FREQ;
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS10 for no prescaler
  TCCR1B |= (1 << CS10);  
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);

  interrupts(); // enable all interrupts;

  //Recever pins
  pinMode(CH6_SWITCH_B_PIN, INPUT); // Switch B of recever
  pinMode(AUDIO_PWM_PIN, OUTPUT);
}

#define SIN_ARR_LEN 2
#define BITS_PER_MESSAGE 1
#define AUDIO_ROTATE_INTERVAL_LOW 125
#define AUDIO_ROTATE_INTERVAL_HIGH 250
int currentBit = 0;
int currentMsg = 0;
//int sinArr8[] = {0b0000, 0b1000, 0b1100, 0b1110, 0b1111, 0b0111, 0b0011, 0b0001};
//int sinArr6[] = {0b000, 0b100, 0b110, 0b111, 0b011, 0b001};
//int sinArr4[] = {0b00, 0b10, 0b11, 0b01};
int sinArr2[] = {0b0, 0b1};
int audioEnabled = 0;

ISR(TIMER1_COMPA_vect) // timer compare interrupt service routine
{
  if(audioEnabled){
    int outMsg = sinArr2[currentMsg];
    int outBit = ((outMsg >> (currentBit)) & 0x01);
    digitalWrite(AUDIO_PWM_PIN, outBit);
    updateBits();
  
    int static audioDelay = 0;
    int static audioRotateInterval = AUDIO_ROTATE_INTERVAL_LOW;
  
    //  Serial.println(audioDelay);
    if (audioDelay >= audioRotateInterval) {
      //    Serial.println("Filp frequency");
      if (OCR1A == HIGH_FREQ) {
        OCR1A = LOW_FREQ;
        audioRotateInterval = AUDIO_ROTATE_INTERVAL_LOW;
      } else {
        OCR1A = HIGH_FREQ;
        audioRotateInterval = AUDIO_ROTATE_INTERVAL_HIGH;
      }
      audioDelay = 0;
    }
    audioDelay = audioDelay + 1;
  } else {
    //Turn off output otherwise capacitors will charge and cause a delay when audio is turned on.
    digitalWrite(AUDIO_PWM_PIN, 0);
  }
}


void updateBits() {
  if (currentBit > 0) {
    currentBit = currentBit - 1;
  } else {
    //Just read the final bit => reset the count
    currentBit = BITS_PER_MESSAGE-1;
    if (currentMsg < SIN_ARR_LEN - 1) {
      //Read the next byte
      currentMsg = currentMsg + 1;
    } else {
      //Read the final message so restart the buffer
      currentMsg = 0;
    }
  }
}

void loop() {
  Controls con = readControls();
  if(con.audio == 1){
    audioOn();
  } else {
    audioOff();
  }
  
  delay(100);
}

Controls readControls() {
  static int audioChan;
  Controls con = NewControls();

  //This will block while trying to sample a pulse
  audioChan = pulseIn(CH6_SWITCH_B_PIN, HIGH, PWM_TIMEOUT_MS);
  
  if (audioChan < AUDIO_ON_THRESHOLD) {
    con = setAudioOff(con);
  }
  else {
    con = setAudioOn(con);
  }

  return con;
}

Controls setAudioOn(Controls con) {
  con.audio = 1;

  return con;
}

Controls setAudioOff(Controls con) {
  con.audio = 0;

  return con;
}

void audioOn(void) {
  audioEnabled = 1;
}

void audioOff(void) {
  audioEnabled = 0;
}
