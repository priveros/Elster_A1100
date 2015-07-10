/*
  This code is been tested in an Arduino Uno clone wiring a comercial infrared
  sensor in pin 2. it will print to to the serial just when it detects a change 
  in Total Imports, Total exports or a change in direction (0=Importing , 1=Exporting)
  
  I have tried some IR sensors so far the only one working at the moment is RPM7138-R
  
  Based on Dave's code to read an elter a100c for more info on that vist: 
  http://www.rotwang.co.uk/projects/meter.html
  Thanks Dave.
*/
const uint8_t intPin = 2;
#define BIT_PERIOD 860 // us
#define BUFF_SIZE 64
volatile long data[BUFF_SIZE];
volatile uint8_t in;
volatile uint8_t out;
volatile unsigned long last_us;
uint8_t dbug = 0;

ISR(INT0_vect) {
   unsigned long us = micros();
   unsigned long diff = us - last_us;
   if (diff > 20 ) {
      last_us = us;
      int next = in + 1;
      if (next >= BUFF_SIZE) next = 0;
      data[in] = diff;
      in = next;
   }
}

void setup() {
    //pinMode(intPin, INPUT);
    in = out = 0; 
    Serial.begin(115200);
    EICRA |= 3;    //RISING interrupt
    EIMSK |= 1;    
    if (dbug) Serial.print("Start ........\r\n");
    last_us = micros();
}

unsigned int statusFlag;
float imports;
float exports;

void loop() {
//    decode_buff();
  int rd = decode_buff();
  if (!rd) return;
  if (rd==3) {
   Serial.println("");
   Serial.print(imports);    Serial.print("\t");
   Serial.print(exports);    Serial.print("\t");
   Serial.print(statusFlag); Serial.println("");    
  }
//   unsigned long end_time = millis() + 6000;
//   while (end_time >= millis()) ;
}

float last_data;
uint8_t sFlag;
float imps;
float exps;
uint16_t idx=0;
uint8_t byt_msg = 0;
uint8_t bit_left = 0;
uint8_t bit_shft = 0;
uint8_t pSum = 0;
uint16_t BCC = 0;
uint8_t eom = 1;

static int decode_buff(void) {
   if (in == out) return 0;
   int next = out + 1;
   if (next >= BUFF_SIZE) next = 0;
   int p = (((data[out]) + (BIT_PERIOD/2)) / BIT_PERIOD);
   if (dbug) { Serial.print(data[out]); Serial.print(" "); if (p>500) Serial.println("<-"); }   
   if (p>500) {
     idx = BCC = eom = imps = exps = sFlag = 0;   
     out = next;
     return 0;
   }
   bit_left = (4 - (pSum % 5));
   bit_shft = (bit_left<p)? bit_left : p;
   pSum = (pSum==10)? p : ((pSum+p>10)? 10: pSum + p);
   if (eom==2 && pSum>=7) {
      pSum=pSum==7?11:10;
      eom=0;   
   }

   if (bit_shft>0) {
      byt_msg >>= bit_shft;
      if (p==2) byt_msg += 0x40<<(p-bit_shft);
      if (p==3) byt_msg += 0x60<<(p-bit_shft);
      if (p==4) byt_msg += 0x70<<(p-bit_shft);   
      if (p>=5) byt_msg += 0xF0;
    }
//    Serial.print(p); Serial.print(" ");Serial.print(pSum);Serial.print(" ");    
//    Serial.print(bit_left);Serial.print(" ");Serial.print(bit_shft);Serial.print(" ");    
//    Serial.println(byt_msg,BIN);
    if (pSum >= 10) {
       idx++;
       if (idx!=328) BCC=(BCC+byt_msg)&255;
//       if (dbug){Serial.print("[");Serial.print(idx);Serial.print(":");Serial.print(byt_msg,HEX); Serial.print("]");}
       if (idx>=95 && idx<=101)  
          imps += ((float)byt_msg-48) * pow(10 , (101 - idx));
       if (idx==103) 
          imps += ((float)byt_msg-48) / 10;
       if (idx>=114 && idx<=120) 
          exps += ((float)byt_msg-48) * pow(10 , (120-idx));
       if (idx==122) 
          exps += ((float)byt_msg-48) / 10;
       if (idx==210) 
          sFlag = (byt_msg-48)>>3; //1=Exporting ; 0=Importing
       if (byt_msg == 3) eom=2; 
       if (idx==328) {
          if ((byt_msg>>(pSum==10?1:2))==((~BCC)&0x7F)) {
             if (last_data != (imps + exps + sFlag)) {
                imports=imps;
                exports=exps;
                statusFlag=sFlag;
                last_data = imps + exps + sFlag;
                out = next;
                return 3;
             }
          }
          if (dbug) {
             Serial.println(""); Serial.print("---->>>>");
             Serial.print(imps); Serial.print("\t");
             Serial.print(exps); Serial.print("\t");
             Serial.print(sFlag); Serial.print("\t"); 
             Serial.print(pSum); Serial.print("\t");              
             Serial.print(byt_msg>>(pSum==10?1:2),BIN); Serial.print("\t"); //BCC
             Serial.print((~BCC)&0x7F,BIN); Serial.print("\t"); //BCC             

          }
       }  
    }
    out = next;
    return 0;
}
