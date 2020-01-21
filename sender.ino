#include <RCSwitch.h>
#include <Adafruit_NeoPixel.h>

#define PIXEL_PIN   9    // Digital IO pin connected to the NeoPixels.
#define PIXEL_COUNT 10
#define BRIGHTNESS 62
#define BRIGHTNESS_FLASH 150

RCSwitch mySwitch = RCSwitch();
Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, NEO_GRB + NEO_KHZ800);


/*
  * SN74HC165N_shift_reg
  *
  * MidiFighter mit Funk
  * Mit RGB LEDs
  * Mit Taschenlampe
  *
  * Program to shift in the bit values from a SN74HC165N 8-bit
  * parallel-in/serial-out shift register.
  *
  * This sketch demonstrates reading in 16 digital states from a
  * pair of daisy-chained SN74HC165N shift registers while using
  * only 4 digital pins on the Arduino.
  *
  * You can daisy-chain these chips by connecting the serial-out
  * (Q7 pin) on one shift register to the serial-in (Ds pin) of
  * the other.
  *
  * Of course you can daisy chain as many as you like while still
  * using only 4 Arduino pins (though you would have to process
  * them 4 at a time into separate unsigned long variables).
  *
*/

//SoftwareSerial mySerial(7, 8); // RX, TX

#define POTI0 A0
#define POTI1 A1
boolean bFourBankSwitch;

/* How many shift register chips are daisy-chained.
*/
#define NUMBER_OF_SHIFT_CHIPS   4

/* Width of data (how many ext lines).
*/
#define DATA_WIDTH   NUMBER_OF_SHIFT_CHIPS * 8
boolean bVal[DATA_WIDTH];
boolean bValOld[DATA_WIDTH];

/* Width of pulse to trigger the shift register to read and latch.
*/
#define PULSE_WIDTH_USEC   5

/* Optional delay between shift register reads.
*/
#define POLL_DELAY_MSEC   1

/* You will need to change the "int" to "long" If the
  * NUMBER_OF_SHIFT_CHIPS is higher than 2.
*/
#define BYTES_VAL_T unsigned int

int ploadPin        = 4;  // Connects to Parallel load pin the 165
//int clockEnablePin  = 7;  // Connects to Clock Enable pin the 165
int dataPin         = 2; // Connects to the Q7 pin the 165
int clockPin        = 3; // Connects to the Clock pin the 165

BYTES_VAL_T pinValues;
BYTES_VAL_T oldPinValues;

//int ana0old = 0;
//int ana1old = 0;

int ana0;
int ana1;

uint8_t iBank;
uint8_t ana0_old;
uint8_t ana1_old;


unsigned long pixelsInterval=1;  // the time we need to wait
unsigned long colorWipePreviousMillis=0;
unsigned long theaterChasePreviousMillis=0;
unsigned long theaterChaseRainbowPreviousMillis=0;
unsigned long rainbowPreviousMillis=0;
unsigned long rainbowCyclesPreviousMillis=0;
unsigned long redCyclesPreviousMillis=0;

int theaterChaseQ = 0;
int theaterChaseRainbowQ = 0;
int theaterChaseRainbowCycles = 0;
int rainbowCycles = 0;
int rainbowCycleCycles = 0;
int redCycles=0;

uint16_t currentPixel = 0;// what pixel are we operating on

boolean bNewCommand = false;
uint8_t iRND = 4;
boolean bFlashlight = false;

/* This function is essentially a "shift-in" routine reading the
  * serial Data from the shift register chips and representing
  * the state of those pins in an unsigned integer (or long).
*/
BYTES_VAL_T read_shift_regs()
{
     long bitVal;
     BYTES_VAL_T bytesVal = 0;

     /* Trigger a parallel Load to latch the state of the data lines,
     */
//    digitalWrite(clockEnablePin, HIGH);
     digitalWrite(ploadPin, LOW);
     delayMicroseconds(PULSE_WIDTH_USEC);
     digitalWrite(ploadPin, HIGH);
//    digitalWrite(clockEnablePin, LOW);


     //Einmalig PIN 0 abfragen, um die Position des Schalter an der 
Seite zu erfahren
     bFourBankSwitch = digitalRead(dataPin);
     /* Pulse the Clock (rising edge shifts the next bit).
     */
     digitalWrite(clockPin, HIGH);
     delayMicroseconds(PULSE_WIDTH_USEC);
     digitalWrite(clockPin, LOW);



     /* Loop to read each bit value from the serial out line
      * of the SN74HC165N.
     */
     for(int i = 1; i < DATA_WIDTH-1; i++) //Ein Eingang weniger wegen 
des Schalters
     {
         bitVal = digitalRead(dataPin);

         /* Set the corresponding bit in bytesVal.
         */
         bytesVal |= (bitVal << ((DATA_WIDTH-1) - i));

         /* Pulse the Clock (rising edge shifts the next bit).
         */
         digitalWrite(clockPin, HIGH);
         delayMicroseconds(PULSE_WIDTH_USEC);
         digitalWrite(clockPin, LOW);
     }
     //Serial.print("PinVAL:");Serial.println(bytesVal, DEC);
     return(bytesVal);
}

/* Dump the list of zones along with their current status.
*/
void display_pin_values()
{
     Serial.print("Pin States:\r\n");

     for(int i = 0; i < DATA_WIDTH; i++)
     {
         Serial.print("  Pin-");
         Serial.print(i);
         Serial.print(": ");

         if((pinValues >> i) & 1)
             Serial.print("HIGH");
         else
             Serial.print("LOW");

         Serial.print("\r\n");
     }

     Serial.print("\r\n");
}

void setup()
{

     //Init des Arrays fuer die Buttons-States
     for(int i=0; i<DATA_WIDTH; i++){
       bVal[i]=false;
       bValOld[i]=false;
     }

     Serial.begin(31250);
     mySwitch.enableTransmit(8);
     //mySwitch.setPulseLength(360);

     //Optional set pulse length.
     //mySwitch.setPulseLength(320);

   // Optional set protocol (default is 1, will work for most outlets)
       //mySwitch.setProtocol(2);

   // Optional set number of transmission repetitions.
       mySwitch.setRepeatTransmit(6);

     /* Initialize our digital pins...
     */
     pinMode(ploadPin, OUTPUT);
//    pinMode(clockEnablePin, OUTPUT);
     pinMode(clockPin, OUTPUT);
     pinMode(dataPin, INPUT);



     digitalWrite(clockPin, LOW);
     digitalWrite(ploadPin, HIGH);

     /* Read in and display the pin states at startup.
     */
     pinValues = read_shift_regs();
//    display_pin_values();
     oldPinValues = pinValues;

     strip.begin();
     strip.setBrightness(BRIGHTNESS);
     strip.show(); // Initialize all pixels to 'off'

     stripRed();
     randomSeed(analogRead(5));
}


void sendMidiNote(int pButtonIndex, boolean pVal){

   int iCommand;
   int iVelocity;
   int iNote;

   if(pVal){
     //Note ON
     iCommand = 144;// + iBank;  //----Je nach BANK ein anderer CHANNEL
     iVelocity = 127;
   }else{
     //Note OFF
     iCommand = 128 ;//+ iBank;
     iVelocity = 127;
   }

   switch (pButtonIndex) {

     case 27:
       // statements
       iNote = 8;
       iNote += iBank*12;
       break;
     case 26:
       iNote = 9;
       iNote += iBank*12;
       break;
     case 25:
         iNote = 10;
         iNote += iBank*12;
         break;
     case 24:
         iNote = 11;
         iNote += iBank*12;
         break;

     case 31:
         iNote = 4;
         iNote += iBank*12;
         break;
     case 30:
         iNote = 5;
         iNote += iBank*12;
         break;
     case 29:
         iNote = 6;
         iNote += iBank*12;
         break;
     case 28:
         iNote = 7;
         iNote += iBank*12;
         break;

     case 19:
         iNote = 0;
         iNote += iBank*12;
         break;
     case 18:
         iNote = 1;
         iNote += iBank*12;
         break;
     case 17:
         iNote = 2;
         iNote += iBank*12;
         break;
     case 16:
         iNote = 3;
         iNote += iBank*12;
         break;


     default:
       iNote = -1;
       break;
   }



   if(iNote != -1){
     Serial.write(iCommand);
     Serial.write(iNote);
     Serial.write(iVelocity);

     if(iCommand >= 144) {
       if(bFourBankSwitch==true){
         mySwitch.send(iNote+1, 24);
       }
       iRND = random(2,10);
       rainbowCycleCycles=0;
       bNewCommand=true;
     }
   }
}

void loop(){

     queryPins();

if(bVal[16]&&bVal[19]&&bVal[24]&&bVal[27]){
       //----Die vier Buttons an der Ecke wurden zusammen geklickt
       strip.setBrightness(BRIGHTNESS_FLASH);
       bFlashlight=true;
       delay(500);
     }else{
       for(int i=0; i<DATA_WIDTH; i++){
         if(bVal[i]!=bValOld[i]){
           bValOld[i]=bVal[i];
           //Serial.print("Button ");Serial.print(i, DEC); 
Serial.print(" Wert:"); Serial.println(bVal[i]);

           //bFlashlight = false;
           if(bVal[i]==true){
             strip.setBrightness(BRIGHTNESS);
             bFlashlight = false;
           }
           sendMidiNote(i, bVal[i]);
         }
       }
     }

//    if(bFourBankSwitch==true){
       //----ana0 ist der BANK Switch, wenn der Switch an der Seite 
aktiv ist
           ana0 = analogRead(POTI0);

         if(ana0<256){
           iBank = 3;
         }else if((ana0>255)&&(ana0<512)){
           iBank = 2;
         }else if((ana0>511)&&(ana0<768)){
           iBank = 1;
         }else if(ana0>767){
           iBank = 0;
         }
//    }else{
//        iBank = 0;
//        uint8_t ana0_new = map(analogRead(POTI0),0, 1023, 127, 0);
//        if(ana0_new != ana0_old){
//          ana0_old = ana0_new;
//          Serial.write(176); // MIdiCC auf Channel 1
//          Serial.write(1);
//          Serial.write(ana0_new);
//        }
//    }

     uint8_t ana1_new = map(analogRead(POTI1),0, 1023, 127, 0);
     if(ana1_new != ana1_old){
       if( abs(ana1_new-ana1_old)>3){
         ana1_old = ana1_new;
         Serial.write(176); // MIdiCC auf Channel 1
         Serial.write(0);
         Serial.write(ana1_new);
         if(bFourBankSwitch==true){
           mySwitch.send(ana1_new+100, 24);
         }
       }
     }


     pinValues = read_shift_regs();
//    Serial.print("PinVAL:");Serial.println(pinValues, DEC);


     //Serial.print("Analog:");Serial.println(ana0, DEC);

     /* If there was a chage in state, display which ones changed.
     */
     /*
     if(pinValues != oldPinValues)
     {
         Serial.print("*Pin value change detected*\r\n");
         display_pin_values();
         oldPinValues = pinValues;
     }
     */
     delay(POLL_DELAY_MSEC);
/*
         if ((unsigned long)(millis() - colorWipePreviousMillis) >= 
pixelsInterval) {
           colorWipePreviousMillis = millis();
           colorWipe(strip.Color(0,255,125));
         }

         if ((unsigned long)(millis() - theaterChasePreviousMillis) >= 
pixelsInterval) {
           theaterChasePreviousMillis = millis();
           theaterChase(strip.Color(127, 127, 127)); // White
         }

         if ((unsigned long)(millis() - 
theaterChaseRainbowPreviousMillis) >= pixelsInterval) {
           theaterChaseRainbowPreviousMillis = millis();
           theaterChaseRainbow();
         }

         if ((unsigned long)(millis() - rainbowPreviousMillis) >= 
pixelsInterval) {
           rainbowPreviousMillis = millis();
           rainbow();
         }
*/
         if(!bFlashlight){
           if(bNewCommand){
             if ((unsigned long)(millis() - rainbowCyclesPreviousMillis) 
 >= pixelsInterval) {
                 rainbowCyclesPreviousMillis = millis();
                 rainbowCycle();
             }
           }else{
             if ((unsigned long)(millis() - redCyclesPreviousMillis) >= 
pixelsInterval*30) {
                 redCyclesPreviousMillis = millis();
                 redCycle();
             }
           }
         }else{
           flashOn();
         }
}

/**
  * Speichert den IST-Zustand der Buttons in den Array bVal;
  */
void queryPins(){
    /* Trigger a parallel Load to latch the state of the data lines,
     */
//    digitalWrite(clockEnablePin, HIGH);
     digitalWrite(ploadPin, LOW);
     delayMicroseconds(PULSE_WIDTH_USEC);
     digitalWrite(ploadPin, HIGH);
//    digitalWrite(clockEnablePin, LOW);

     /* Loop to read each bit value from the serial out line
      * of the SN74HC165N.
     */
     for(int i = 0; i < DATA_WIDTH; i++)
     {

         bVal[i] = !digitalRead(dataPin);


         /* Pulse the Clock (rising edge shifts the next bit).
         */
         digitalWrite(clockPin, HIGH);
         delayMicroseconds(PULSE_WIDTH_USEC);
         digitalWrite(clockPin, LOW);
     }
}



// Fill the dots one after the other with a color
void colorWipe(uint32_t c){
   strip.setPixelColor(currentPixel,c);
   strip.show();
   currentPixel++;
   if(currentPixel == PIXEL_COUNT){
     currentPixel = 0;
   }
}

void stripOff(){
   for(uint16_t i=0; i<strip.numPixels(); i++) {
     strip.setPixelColor(i, 0);
   }
   strip.show();
}

void stripRed(){
   for(uint16_t i=0; i<strip.numPixels(); i++) {
     strip.setPixelColor(i, 255,0 ,0);
   }
   strip.show();
}


void rainbow() {
   for(uint16_t i=0; i<strip.numPixels(); i++) {
     strip.setPixelColor(i, Wheel((i+rainbowCycles) & 255));
   }
   strip.show();
   rainbowCycles++;
   if(rainbowCycles >= 256) rainbowCycles = 0;
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle() {
   uint16_t i;

    for(i=0; i< strip.numPixels(); i++) {
       strip.setPixelColor(i, Wheel(((i * 255 / strip.numPixels()) + 
rainbowCycleCycles) & 255));
     }
     strip.show();

   rainbowCycleCycles++;
   if(rainbowCycleCycles >= 255*iRND){
     rainbowCycleCycles = 0;
     //stripOff();
//    stripRed();
     bNewCommand=false;
   }
}


void redCycle(){
   uint16_t i;

    for(i=0; i< strip.numPixels(); i++) {
       strip.setPixelColor(i, redWheel(((i * 255 / strip.numPixels()) + 
redCycles) & 255));
     }
     strip.show();

   redCycles++;
   if(redCycles >= 255*5){
     redCycles = 0;
   }

}

//Theatre-style crawling lights.
void theaterChase(uint32_t c) {
   for (int i=0; i < strip.numPixels(); i=i+3) {
       strip.setPixelColor(i+theaterChaseQ, c);    //turn every third 
pixel on
     }
     strip.show();
     for (int i=0; i < strip.numPixels(); i=i+3) {
       strip.setPixelColor(i+theaterChaseQ, 0);        //turn every 
third pixel off
     }
     theaterChaseQ++;
     if(theaterChaseQ >= 3) theaterChaseQ = 0;
}


//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow() {
   for (int i=0; i < strip.numPixels(); i=i+3) {
     strip.setPixelColor(i+theaterChaseRainbowQ, Wheel( 
(i+theaterChaseRainbowCycles) % 255));    //turn every third pixel on
   }

   strip.show();
   for (int i=0; i < strip.numPixels(); i=i+3) {
     strip.setPixelColor(i+theaterChaseRainbowQ, 0);        //turn every 
third pixel off
   }
   theaterChaseRainbowQ++;
   theaterChaseRainbowCycles++;
   if(theaterChaseRainbowQ >= 3) theaterChaseRainbowQ = 0;
   if(theaterChaseRainbowCycles >= 256) theaterChaseRainbowCycles = 0;
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
   WheelPos = 255 - WheelPos;
   if(WheelPos < 85) {
     return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
   }
   if(WheelPos < 170) {
     WheelPos -= 85;
     return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
   }
   WheelPos -= 170;
   return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

uint32_t redWheel(byte WheelPos) {
   //WheelPos = 255 - WheelPos/2;

   //if(WheelPos < 127) {
     //return strip.Color(abs(sin(0.02431*WheelPos))*255, 
abs(sin(0.02431*WheelPos+100))*100, 0);
     return strip.Color(255, abs(sin(0.02431*WheelPos))*50, 0);
   //}
   //return strip.Color(abs(sin(0.02431*WheelPos))*255, , 0);
}

void flashOn(){
   for (int i=0; i < strip.numPixels(); i++) {
     strip.setPixelColor(i, 0);        //turn everypixel off
   }

   strip.setPixelColor(7, 255,255,255);
   strip.setPixelColor(8, 255,255,255);
   strip.setPixelColor(9, 255,255,255);
   strip.show();
}

