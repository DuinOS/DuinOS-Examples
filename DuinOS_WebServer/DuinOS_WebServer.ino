/*
 DuinOS DuinOS_WebServer
 
 This example is based on morecomplexblinking and ethernet server examples
 
 by
 Nicolás S.
 
 */

#if ! defined(__AVR_ATmega2560__)
#error "Este codigo apenas roda no ATMega2560"
#endif

#include "DuinOS/FreeRTOS.h"
#include "DuinOS/queue.h"

#define INITIAL_GREEN_DELAY  200
#define NEW_GREEN_DELAY  20

int ledPinRed =  11;
int ledPinGreen =  12;

unsigned int greenDelay = INITIAL_GREEN_DELAY;
boolean redLED_isOn = false;
boolean greenLED_isOn = false;

//Forward declaration, to let redLED call to resumeTask(greenLED). It's not necessary if we put the greenLED 
//itself here, but declareLoopTask may be usefull in more complex situations:
declareTaskLoop(greenLED);


/* Declare a variable of type xQueueHandle. This is used to send messages from the print tasks and the tick interrupt to the gatekeeper task. */
xQueueHandle xPrintQueue;

char *pcMessageToPrint;

taskLoop(webserv)
{
  web_loop();
}

taskLoop(redLED)
{
  static unsigned char counter = 0;

  if (!greenLED_isOn)
  {
    if (counter >2)
      resumeTask(greenLED);
    counter++;
  }

  redLED_isOn = false;
  delay(1000);
  redLED_isOn = true; 
  delay(1000);
}


taskLoop(greenLED)
{
  static unsigned char counter = 1;

  digitalWrite(ledPinGreen, HIGH);  // set the LED on
  delay(greenDelay);
  digitalWrite(ledPinGreen, LOW);  // set the LED off
  delay(greenDelay);

  if ( (counter >= 9) && (greenDelay != NEW_GREEN_DELAY) )
    greenDelay = NEW_GREEN_DELAY; //now, after 10 blinks, accelerates
  if (counter >= 99)
  {
    //Reset vars, so next time, if the task is resumed, it executes all again:
    counter = 0;
    greenDelay = INITIAL_GREEN_DELAY;
    greenLED_isOn = false;
    suspend();      //After a while, the tasks suspends itself (forever)
  }
  counter++;
}


void setup()   
{              
  /* Before a queue is used it must be explicitly created. The queue is created to hold a maximum of 5 character pointers. */
  xPrintQueue = xQueueCreate( 100, sizeof( char * ) );  

  // Initialize the digital pins as outputs:
  pinMode(ledPinRed, OUTPUT);
  pinMode(ledPinGreen, OUTPUT);
  Serial.begin(9600);

  web_setup();
  
  createTaskLoopWithStackSize(webserv, LOW_PRIORITY,2000);
  createTaskLoop(redLED, NORMAL_PRIORITY);
  createTaskLoop(greenLED, LOW_PRIORITY);

}


// This is the main loop() method, wich runs over and over again,
// as long as the Arduino has power. Is a LOW_PRIORITY taskLoop:

void loop()                     
{
  if (redLED_isOn)
  {
    digitalWrite(ledPinRed, LOW);  // set the LED off
    delay(25);                      // The OS can be tested reducing these delays, and seeing how both LEDs work together...
    digitalWrite(ledPinRed, HIGH);   // set the LED on
    delay(25);    
  }
  else
  {
    digitalWrite(ledPinRed, LOW);  //  LED is off
    //If nextTask is not called, the application will not hang, because the OS is preemptive. BUT, the current task
    //will consume a lot of computational resources (due to it's lack of a delay() in this branch), the application will 
    //turn slower, and the other tasks may be affected by this, loossing precision in their timing:
    nextTask();
  }
}










