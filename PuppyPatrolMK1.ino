char charBuffer[100];

//period of one pwm cycle is 50HZ = .02
//Want to have a timeout that is twice as long as any one cycle should be.
//(1/50)*2/10^-6 = 40000. Then added a fudge factor of 10,000 to bring it to 50,000
//(1/50) = Hz to seconds
// *2 = Twice the cycle
// /10^-6 = seconds to microseconds
#define PWM_TIMEOUT_MS 50000

//A short pulse has a width of 1ms = 1000us
//A nutral pulse has a width of 1.5ms = 1500us
//A long pulse has a widht of 2ms = 2000us

//Up is a short pulse
#define UP_THRESHOLD 1250
//Down is a long pulse
#define DOWN_THRESHOLD 1750

//Left is a short pulse
#define LEFT_THRESHOLD 1250
//Right is a long pulse
#define RIGHT_THRESHOLD 1750

//Left is a short pulse
#define AUDIO_ON_THRESHOLD 1250

#define UP_DOWN_PIN 13
#define LEFT_RIGHT_PIN 12
#define CH1_PWM_PIN 11
#define CH1_DIR_PIN 10
#define CH6_SWITCH_B_PIN 9
#define CH2_PWM_PIN 8
#define CH2_DIR_PIN 7
#define AUDIO_PWM_PIN 6
#define CH3_PWM_PIN 5
#define CH3_DIR_PIN 4
#define CH4_PWM_PIN 3
#define CH4_DIR_PIN 2

typedef struct {
  int up;
  int down;
  int left;
  int right;
  int audio;
} Controls;

Controls NewControls() {
  Controls con = {0, 0, 0, 0, 0};
  return con;
}

#define HIGH_FREQ 16000
#define LOW_FREQ 32000

void setup() {
  Serial.begin(9600);
  Serial.println("Starting puppy patrol");


  noInterrupts(); // disable all interrupts
  //set timer1 interrupt at 1Hz
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
  pinMode(LEFT_RIGHT_PIN, INPUT); // Ch 1 of recever
  pinMode(UP_DOWN_PIN, INPUT); // Ch 2 of recever
  pinMode(CH6_SWITCH_B_PIN, INPUT); // Switch B of recever

  //Motor controller pins
  pinMode(CH1_PWM_PIN, OUTPUT);
  pinMode(CH1_DIR_PIN, OUTPUT);
  pinMode(CH2_PWM_PIN, OUTPUT);
  pinMode(CH2_DIR_PIN, OUTPUT);
  pinMode(CH3_PWM_PIN, OUTPUT);
  pinMode(CH3_DIR_PIN, OUTPUT);
  pinMode(CH4_PWM_PIN, OUTPUT);
  pinMode(CH4_DIR_PIN, OUTPUT);
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


void loop() {
  Controls con = readControls();

  //Neutral
  if (con.up == 0 && con.down == 0 && con.left == 0 && con.right == 0) {
//    Serial.println("Neutral");
    leftTreadsOff();
    rightTreadsOff();
  }
  //Forward
  else if (con.up == 1 && con.down == 0 && con.left == 0 && con.right == 0) {
//    Serial.println("Forward!");
    leftTreadsForward();
    leftTreadsFullSpeed();

    rightTreadsForward();
    rightTreadsFullSpeed();
  }
  //Forward Right
  else if (con.up == 1 && con.down == 0 && con.left == 0 && con.right == 1) {
//    Serial.println("Forward Right!");
  }
  //Right
  else if (con.up == 0 && con.down == 0 && con.left == 0 && con.right == 1) {
//    Serial.println("Right!");

    leftTreadsForward();
    leftTreadsFullSpeed();

    rightTreadsReverse();
    rightTreadsFullSpeed();
  }
  //Back Right
  else if (con.up == 0 && con.down == 1 && con.left == 0 && con.right == 1) {
//    Serial.println("Back Right!");
  }
  //Back
  else if (con.up == 0 && con.down == 1 && con.left == 0 && con.right == 0) {
//    Serial.println("Back!");

    leftTreadsReverse();
    leftTreadsFullSpeed();

    rightTreadsReverse();
    rightTreadsFullSpeed();
  }
  //Back Left
  else if (con.up == 0 && con.down == 1 && con.left == 1 && con.right == 0) {
//    Serial.println("Back Left!");
  }
  //Left
  else if (con.up == 0 && con.down == 0 && con.left == 1 && con.right == 0) {
//    Serial.println("Left!");

    leftTreadsReverse();
    leftTreadsFullSpeed();

    rightTreadsForward();
    rightTreadsFullSpeed();
  }
  //Forward Left
  else if (con.up == 1 && con.down == 0 && con.left == 1 && con.right == 0) {
//    Serial.println("Forward Left!");
  }
  //Invalid combo
  else {
    sprintf(charBuffer, "Error combo not possible, up:%i\tdown:%i\tleft:%i\tright:%i", con.up, con.down, con.left, con.right);
//    Serial.println(charBuffer);
  }

  if(con.audio == 1){
    audioOn();
  } else {
    audioOff();
  }
  
  delay(100);
}

Controls readControls() {
  static int upDownChan;
  static int leftRightChan;
  static int audioChan;
  Controls con = NewControls();

  //This will block while trying to sample a pulse
  leftRightChan = pulseIn(LEFT_RIGHT_PIN, HIGH, PWM_TIMEOUT_MS);
  //This will block while trying to sample a pulse
  upDownChan = pulseIn(UP_DOWN_PIN, HIGH, PWM_TIMEOUT_MS);
  //This will block while trying to sample a pulse
  audioChan = pulseIn(CH6_SWITCH_B_PIN, HIGH, PWM_TIMEOUT_MS);
  if (upDownChan == 0) {
    con = killUpDown(con);
  }
  else if (upDownChan < UP_THRESHOLD) {
    con =  setUp(con);
  }
  else if ( upDownChan >= UP_THRESHOLD && upDownChan <= DOWN_THRESHOLD) {
    con = killUpDown(con);
  }
  else if (upDownChan > DOWN_THRESHOLD) {
    con = setDown(con);
  }
  else {
    con = killUpDown(con);
  }

  if (leftRightChan == 0) {
    con = killLeftRight(con);
  }
  else if (leftRightChan < LEFT_THRESHOLD) {
    con = setLeft(con);
  }
  else if (leftRightChan >= LEFT_THRESHOLD && leftRightChan <= RIGHT_THRESHOLD) {
    con = killLeftRight(con);
  }
  else if (leftRightChan > RIGHT_THRESHOLD) {
    con = setRight(con);
  }
  else {
    con = killLeftRight(con);
  }

  if (audioChan < AUDIO_ON_THRESHOLD) {
    con = setAudioOff(con);
  }
  else {
    con = setAudioOn(con);
  }

  return con;
}

Controls setUp(Controls con) {
  con.up = 1;
  con.down = 0;

  return con;
}

Controls setDown(Controls con) {
  con.up = 0;
  con.down = 1;

  return con;
}

Controls killUpDown(Controls con) {
  con.up = 0;
  con.down = 0;

  return con;
}

Controls setLeft(Controls con) {
  con.left = 1;
  con.right = 0;

  return con;
}

Controls setRight(Controls con) {
  con.left = 0;
  con.right = 1;

  return con;
}

Controls killLeftRight(Controls con) {
  con.left = 0;
  con.right = 0;

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

void leftTreadsForward(void) {
  digitalWrite(CH2_DIR_PIN, LOW);
  digitalWrite(CH4_DIR_PIN, HIGH);
}

void leftTreadsReverse(void) {
  digitalWrite(CH2_DIR_PIN, HIGH);
  digitalWrite(CH4_DIR_PIN, LOW);
}

void leftTreadsFullSpeed(void) {
  digitalWrite(CH2_PWM_PIN, HIGH);
  digitalWrite(CH4_PWM_PIN, HIGH);
}

void leftTreadsOff(void) {
  digitalWrite(CH2_PWM_PIN, LOW);
  digitalWrite(CH4_PWM_PIN, LOW);
}

void rightTreadsForward(void) {
  digitalWrite(CH1_DIR_PIN, LOW);
  digitalWrite(CH3_DIR_PIN, HIGH);
}

void rightTreadsReverse(void) {
  digitalWrite(CH1_DIR_PIN, HIGH);
  digitalWrite(CH3_DIR_PIN, LOW);
}

void rightTreadsFullSpeed(void) {
  digitalWrite(CH1_PWM_PIN, HIGH);
  digitalWrite(CH3_PWM_PIN, HIGH);
}

void rightTreadsOff(void) {
  digitalWrite(CH1_PWM_PIN, LOW);
  digitalWrite(CH3_PWM_PIN, LOW);
}

void audioOn(void) {
  audioEnabled = 1;
}

void audioOff(void) {
  audioEnabled = 0;
}
