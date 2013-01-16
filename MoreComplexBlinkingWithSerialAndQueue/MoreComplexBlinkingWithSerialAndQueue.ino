/*
 DuinOS MoreComplexBlinkingWithSerialAndQueue
 
 Este exemplo pisca dois LEDs, sendo que um LED mostra uma combinacao de ondas, 
 Tambem e exibido na serial uma mensagem que demonstra a alternancia das Task, 
 porem para se evitar o competicao pela porta serial corrompendo sua saida e usado
 entao um queue para demonstrar uma forma de controlar recursos que precisam ser 
 sincronizados, este exemplo nao resolve o problema em protocolos como I2C
 
 Exemplo Baseado no MoreComplexBlinking
 
 Original criado 2009.10.26 (yyyy.mm.dd)
 by Julián da Silva Gillig
 
 Alterado para uso da serial em (2013.01.16)
 por Carlos Delfino
 
 http://www.carlosdelfino.eti.br
 
 */

#if ! defined(__AVR_ATmega2560__)
#error "Este codigo apenas roda no ATMega2560"
#endif

#include "DuinOS/FreeRTOS.h"
#include "DuinOS/queue.h"

#define INITIAL_GREEN_DELAY  200
#define NEW_GREEN_DELAY  20

int ledPinRed =  8;
int ledPinGreen =  9;

unsigned int greenDelay = INITIAL_GREEN_DELAY;
boolean redLED_isOn = false;
boolean greenLED_isOn = false;

//Forward declaration, to let redLED call to resumeTask(greenLED). It's not necessary if we put the greenLED 
//itself here, but declareLoopTask may be usefull in more complex situations:
declareTaskLoop(greenLED);


/* Declare a variable of type xQueueHandle. This is used to send messages from the print tasks and the tick interrupt to the gatekeeper task. */
xQueueHandle xPrintQueue;

char *pcMessageToPrint;


taskLoop(serialGateKeeper){
    /* Wait for a message to arrive. An indefinite block time is specified so there is no need to check the return value – the function will only
     return when a message has been successfully received. */
    xQueueReceive( xPrintQueue, &pcMessageToPrint, portMAX_DELAY );
    Serial.println(pcMessageToPrint);
}

taskLoop(serialT1){
    /* Print out the string, not directly but instead by passing a pointer to
     the string to the gatekeeper task via a queue. The queue is created before the scheduler is started so will already exist by the time this task executes for the first time. A block time is not specified because there should always be space in the queue. */
    char *msg = "SerialT1c";
    xQueueSendToBack( xPrintQueue, &msg, 0 );

    //Serial.print("SerialT1a ");
    //Serial.println(micros());
    delay(500);
    msg = "SerialT1d";
    xQueueSendToBack( xPrintQueue, &msg, 0 );
    //Serial.print("SerialT2a ");
    //  Serial.println(micros());
}

taskLoop(serialT2){ 
    char *msg = "SerialT2c";
    xQueueSendToBack( xPrintQueue, &msg, 0 );
    // Serial.print("SerialT2a ");
    //  Serial.println(micros());
    for(int i = 0;i<=10;i++) delayMicroseconds(1000);
    msg = "SerialT2d";
    xQueueSendToBack( xPrintQueue, &msg, 0 );
    //    Serial.print("SerialT2b ");
    // Serial.println(micros());
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


// The setup() method runs once, when the sketch starts

void setup()   
{              
    /* Before a queue is used it must be explicitly created. The queue is created to hold a maximum of 5 character pointers. */
    xPrintQueue = xQueueCreate( 100, sizeof( char * ) );  

    // Initialize the digital pins as outputs:
    pinMode(ledPinRed, OUTPUT);
    pinMode(ledPinGreen, OUTPUT);
    Serial.begin(9600);

    createTaskLoopWithStackSize(serialGateKeeper, LOW_PRIORITY,200);
    createTaskLoopWithStackSize(serialT2, LOW_PRIORITY,200);
    createTaskLoopWithStackSize(serialT1, LOW_PRIORITY,200);
    createTaskLoop(redLED, NORMAL_PRIORITY);
    createTaskLoop(greenLED, LOW_PRIORITY);

    //This initializes the main loop's with a different priority (default is LOW_PRIORITY):
    //initMainLoopPriority(NORMAL_PRIORITY);

    //Try this and see what happends:
    //suspendTask(redLED);
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




