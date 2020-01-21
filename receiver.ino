#include <RCSwitch.h>
#include <SoftwareSerial.h>

RCSwitch mySwitch = RCSwitch();
SoftwareSerial mySerial(10, 11); // RX, TX

boolean bLED = false;

void setup()
{
   Serial.begin(9600);
   mySwitch.enableReceive(0);  // Empfänger ist an Interrupt-Pin "0" - Das ist am UNO der Pin2
   mySerial.begin(31250);
   pinMode(8, OUTPUT);
}

long lTime1=0;
int iNote;

void loop() {

   //lTime1 = millis();
   if (mySwitch.available()){
     //Serial.print(millis(), DEC);
     if(lTime1+100<millis()){
       //Neuer Anschlag
       bLED = !bLED;
        digitalWrite(8, true);

       lTime1=millis();
       iNote = mySwitch.getReceivedValue()-1;
       if(iNote<99){
         mySerial.write(144);
         mySerial.write(iNote);
         mySerial.write(127);
       }else{
         //----Das Poti fuer die Lautstaerke
         int i=0;
         mySerial.write(176); // MIdiCC auf Channel 1
         mySerial.write(i);
         mySerial.write(iNote-99);
       }

       Serial.println(iNote, DEC);
       digitalWrite(8, false);
     }else{
       //----Wiederholung
     }

     mySwitch.resetAvailable(); // Hier wird der Empfänger "resettet"
   }
}

