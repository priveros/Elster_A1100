/*
  This code is been tested in an Arduino Uno clone wiring a comercial infrared
  sensor in pin 2. it will print to to the serial just when it detects a change 
  in Total Imports, Total exports or a change in direction (0=Importing , 1=Exporting)
*/
const uint8_t intPin = 2;  //arduino pin number that supports interrupts
#define BIT_PERIOD 860 // us
#define BUFF_SIZE 64
volatile long data[BUFF_SIZE];
volatile uint8_t in;
volatile uint8_t out;
volatile unsigned long last_us;
uint8_t waiting;
long lp;
#define BITS(t) (((t) + (BIT_PERIOD/2)) / BIT_PERIOD)

void on_change(void) { 
  unsigned long us = micros();
  unsigned long diff = us - last_us;
  const int PinS = digitalRead(intPin);
  if (PinS != 0 && diff > 20 ) {
    last_us = us;
    int next = in + 1;
    if (next >= BUFF_SIZE)
      next = 0;
    if (diff < 21 * BIT_PERIOD) {
      data[in] = diff;
      in = next;
    }
  }
  waiting = 0;
}

uint8_t reading = 0;

void setup() {
  pinMode(intPin, INPUT);
  in = out = waiting = lp = 0;
  last_us = micros();
  Serial.begin(115200);
  attachInterrupt(0, on_change, RISING); //CHANGE RISING FALLING LOW
}

uint16_t nibbles;
uint8_t n = 0;
uint16_t byte_word = 0;
uint8_t isLowNib = 0;

void loop() {
  const unsigned long us_lp = micros();
  if (!decode_buff())
    return;
  const long d = us_lp - last_us;
  if (d > 1000000 & !waiting) {
    lp++;
    nibbles = n = 0;
    waiting = 1;
    isLowNib = 0;
    reading = 0;
    //Serial.print("Waiting ........\r\n");
  };
}


unsigned char last_4[4];
unsigned int statusFlags;
float imports;
float exports;
float last_data;
const unsigned char match[] = { 0x01, 0x4F, 0x42, 0x02 };
uint16_t idx;

static int decode_buff(void) {
  if (in == out)
    return -1;
  int next = out + 1;
  if (next >= BUFF_SIZE)
    next = 0;

  int p = BITS(data[out]) * 2;
  //  Serial.print(p);
  //  Serial.print("\t");
  for (int i = 1;  i < p; i++) {
    if ((n & 1)) //
      nibbles += 1 << (n / 2);
    if (n + 1 <= 10)
      n++;
  }
  if (n + 1 <= 10)
    n++;
  nibbles = nibbles & 15;
  //  Serial.print(n);
  //  Serial.print("\t");
  //  Serial.println(nibbles);
  if (n >= 10) {
    //    Serial.println(nibbles,BIN);
    if (!isLowNib) {
      isLowNib = 1;
      byte_word = 0;
      byte_word += nibbles;
    } else {
      isLowNib = 0;
      byte_word += nibbles << 4;
      //      Serial.println(lp);
      //      Serial.print(byte_word,HEX);
      
      if (!reading) {
        last_4[0] = last_4[1];
        last_4[1] = last_4[2];
        last_4[2] = last_4[3];
        last_4[3] = byte_word;
        if (memcmp(match, last_4, sizeof(match)) == 0) {
          idx = 0;
          reading = 1;
        }
      } else {
        idx++;
        if (idx>=91 && idx<=97)  
          imports += ((float)byte_word-48) * pow(10 , (97 - idx));
        if (idx==99) 
          imports += ((float)byte_word-48) / 10;
        if (idx>=110 && idx<=116) 
          exports += ((char)byte_word-48) * pow(10 , (116-idx));
        if (idx==118) 
          exports += ((float)byte_word-48) / 10;
        if (idx==206) {
          if (last_data != imports + exports + ((byte_word-48)>>3)) {
            Serial.print(imports);
            Serial.print("\t");
            Serial.print(exports);
            Serial.print("\t");
            Serial.print((byte_word-48)>>3);  //1=Exporting ; 0=Importing
            Serial.print("\n");
          }
          last_data = imports + exports + ((byte_word-48)>>3);
          imports = exports = 0;
        }
      }

//      if (byte_word == 3)
//        Serial.print("\n");
        
    }
    nibbles = n = 0;
  }
  out = next;
  return 0;
}

static int buff_print(void) {
  if (in == out)
    return -1;
  int next = out + 1;
  if (next >= BUFF_SIZE)
    next = 0;
  Serial.print(lp);
  Serial.print("\t");
  Serial.print(data[out]);
  Serial.print("\t");
  Serial.print(BITS(data[out]));
  Serial.print("\n");
  out = next;
  return 0;
}
