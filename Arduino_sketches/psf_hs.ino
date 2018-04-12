/* Bholanath 2A stepper motor driver controller

 check for push button input and send output pulses accordingly
 psf_hs
 modified from psf_fast
 to use serial communication instead of keyboard emulation
  - Hari Nandakumar 12 Apr 2018
 
 psf_fast
 modified from ask_pi_fast
  -- to be used to capture axial PSF of OCT,
  -- or for autocorrelation experiments
  -- works with c code on capture computer which uses s for capture.
   - Hari Nandakumar  7 Apr 2018 

 ask_pi_fast modified from ask_pi_soft
  - Hari Nandakumar 31 Mar 2018 
  - delayMicroseconds in the loop, testing fastest capture times

 
 ask_pi_soft - 20 Feb 2018
 G R Rohith & Swaroop P
  - include keyboard.h
 

 bhola2 created 2016-06-14
 Hari Nandakumar + K S Adithya

 bhola3 modified from bhola2 on 2017-08-11 using code from
 https://www.arduino.cc/en/Tutorial/ButtonMouseControl
 sending a mouse click whenever a capture is needed.

 Modified from this example code in the public domain.

 http://www.arduino.cc/en/Tutorial/BlinkWithoutDelay
 */

// #include "Mouse.h"
//#include "Keyboard.h"

// constants won't change. Used here to set a pin number :
const int ledPin = 13;      // the number of the LED pin - output PUL
const int stopPin = 06;
const int pushPin = 02;      //  input for single step
const int rotPin = 03;      //  input for rotation
const int tenPin = 04;     //  input for ten operation
const int hunPin = 05;     //  input for hundred operation
const int speedPin = 10;
const int trigPin = 12;   // used in bhola2 for camera trigger
const int camPin = 11;
const int askPin = 07;     //used to control ASK modulation of function generator

// Variables will change :
int ledState = LOW;             // ledState used to set the LED
int rotState = LOW;
int tenState = LOW;
int hunState = LOW;
int pushState = LOW;
int stopState = HIGH;
int speedState = LOW;
int trigState = LOW;
int camState = LOW;
unsigned long i = 0;
unsigned long inneri = 0;
unsigned long  pulseNos = 0;
unsigned long currentMillis = 700;
unsigned long previousMillis = 0;
byte byteRead;


void setup() {
  // set the digital pin as output:
  pinMode(ledPin, OUTPUT);
  pinMode(pushPin, INPUT);
  pinMode(rotPin, INPUT);
  pinMode(tenPin, INPUT);
  pinMode(hunPin, INPUT);
  pinMode(stopPin, INPUT);
  pinMode(speedPin, INPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(camPin, INPUT);
  pinMode(askPin, OUTPUT);
  // initialize mouse control:
  //Mouse.begin();
  //Keyboard.begin();
  Serial.begin(115200);     // opens serial port, sets data rate to 115200 bps
}

void loop()
{
  if (currentMillis - previousMillis >= 500)
  {

    previousMillis = currentMillis;

    pushState = digitalRead(pushPin);
    rotState = digitalRead(rotPin);
    tenState = digitalRead(tenPin);
    hunState = digitalRead(hunPin);
    speedState = digitalRead(speedPin);
    camState = digitalRead(camPin);

    // first set the number of steps to move
    // depending on the DIP switches

    if (rotState == HIGH) {
          if (tenState == HIGH) {
            if (hunState == HIGH) {
              pulseNos = 800000;  // 800 microsteps x 1000 rotations
            }
            else pulseNos = 8000; // 800 microsteps x 10 rotations
          } else if (hunState == HIGH) pulseNos = 80000; // 800 microsteps x 100 rotations
          else pulseNos = 800; // 1 revolution
        } else {                  //rotState is LOW
          if (tenState == HIGH) {
            if (hunState == HIGH) {
              pulseNos = 1000;  // 1000 steps
            }
            else pulseNos = 10; // 10 steps
          } else if (hunState == HIGH) pulseNos = 100; // 100 steps
          else pulseNos = 1; // 1 microstep
        }

       

     if (pushState == HIGH) { //start processing the pulses if push switch pressed
      // if speedState is HIGH, just send the pulses, no clicks 
      if (speedState == HIGH) {
          
          for (i = 0; i < pulseNos; i++) {
            digitalWrite(ledPin, HIGH);
            delay(10);
            digitalWrite(ledPin, LOW);
            delay(40);
           
            stopState = digitalRead(stopPin);
            if (stopState == LOW) { 
              break;
            }
          }
        }// this is if speedState is high
       else { // if speedState is low check camState
       
        if (camState == HIGH) { // then 10 clicks per pulse
          for (i = 0; i < pulseNos; i++) {
            digitalWrite(ledPin, HIGH);
            delay(10);
            digitalWrite(ledPin, LOW);
            delay(10);
            delay(300);
            /*digitalWrite(trigPin, HIGH);
            delay(300);
            digitalWrite(trigPin, LOW);
            delay(1500);
            repeated 10 times;*/
            /* Mouse.click(); changed to keyboard
              */
            
            stopState = digitalRead(stopPin);
            if (stopState == LOW) {
              break;
            }
          } // end for loop
        } // end if camstate high
      
        else { // camstate is low, only a single click per pulse
               
          for (i = 0; i < pulseNos; i++) {
            digitalWrite(askPin, LOW);
            for (inneri=0; inneri<12800; inneri++) {
              digitalWrite(ledPin, HIGH);
              delayMicroseconds(10);
              digitalWrite(ledPin, LOW);
              delayMicroseconds(10); 
            }
            
            //delay(1300);
            /*digitalWrite(trigPin, HIGH);
            delay(300);
            digitalWrite(trigPin, LOW);
            delay(1500);*/
            
           // Mouse.click();
            
           Serial.write('s');

           while(1) {

             if (Serial.available()) {
                /* read the most recent byte */
                byteRead = Serial.read();
             }
             if (byteRead == 'a') {
              break;
             }
           }
           
            //delay(40);
            // 640x480 16 bit goes at 50 fps
            // assuming a factor of 2 for safety, 
            // 40 ms delay for 25 fps
            
            stopState = digitalRead(stopPin);
            if (stopState == LOW) { 
              break;
            }
            // if using single click to capture sequences, add appropriate delay below by uncommenting.
            //delay(1);
          } // end for loop
        } // end of else camstate is low
      } // end of else speedstate is low
    } // end of if statement of push switch
  } // end of millis if statement
  currentMillis = millis();
} // end of loop

