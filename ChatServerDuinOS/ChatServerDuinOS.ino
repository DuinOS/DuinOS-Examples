/*
 Chat  Server
 
 A simple server that distributes any incoming messages to all
 connected clients.  To use telnet to  your device's IP address and type.
 You can see the client's input in the serial monitor as well.
 Using an Arduino Wiznet Ethernet shield. 
 
 Circuit:
 * Ethernet shield attached to pins 10, 11, 12, 13
 * Analog inputs attached to pins A0 through A5 (optional)
 
 created 18 Dec 2009
 by David A. Mellis
 modified 9 Apr 2012
 by Tom Igoe
 
 */
//#if !( defined(__AVR_ATmega2560__) ||  defined(__AVR_ATmega644P__))
//#error "Este codigo apenas roda no ATMega2560" 
//#endif

#include "DuinOS/FreeRTOS.h"
#include "DuinOS/queue.h"

#include <SPI.h>
#include <Ethernet.h>

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network.
// gateway and subnet are optional:
byte mac[] = { 
    0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
//IPAddress ip(192,168,1, 177);
//IPAddress gateway(192,168,1, 1);
//IPAddress gateway(192,168,1, 1);
//IPAddress subnet(255, 255, 0, 0);
IPAddress ip(177,189,135,172);
IPAddress gateway(177,189,135,129);
IPAddress dnserver(177,189,128,4);
IPAddress subnet(255,255,255,192);

// telnet defaults to port 23
EthernetServer server(23);
boolean alreadyConnected = false; // whether or not the client was connected previously

/* Declare a variable of type xQueueHandle. This is used to send messages from the print tasks and the tick interrupt to the gatekeeper task. */
xQueueHandle xClientsQueue;


// Cria o cliente para Ethernet que sera usado na task.

EthernetClient client;
taskLoop(clientAvailableTask){

    xQueueReceive( xClientsQueue, &client, portMAX_DELAY );

    for(byte i = 0;i<4;i++){
        digitalWrite(40,LOW);
        delay(300);
        digitalWrite(40,HIGH);
        delay(300);
    }
    // when the client sends the first byte, say hello:
    if (client.available() > 0) {
        // read the bytes incoming from the client:
        char thisChar = client.read();
        // echo the bytes back to the client:
        server.write(thisChar);
        // echo the bytes to the server as well:
        Serial.write(thisChar);
    }

    for(byte i = 0;i<4;i++){
        digitalWrite(40,LOW);
        delay(300);
        digitalWrite(40,HIGH);
        delay(300);
    }
}



void setup() {
    pinMode(48,OUTPUT);
    pinMode(46,OUTPUT);
    pinMode(44,OUTPUT);
    pinMode(42,OUTPUT);
    pinMode(40,OUTPUT);
    digitalWrite(48,LOW);
    digitalWrite(46,LOW);
    digitalWrite(44,LOW);
    digitalWrite(42,LOW);
    digitalWrite(40,LOW);

    // initialize the ethernet device
    Ethernet.begin(mac, ip, dnserver, gateway, subnet);
    // start listening for clients
    server.begin();
    // Open serial communications and wait for port to open:
    Serial.begin(9600);
    while (!Serial) {
        ; // wait for serial port to connect. Needed for Leonardo only
    }


    Serial.print("Chat server address:");
    Serial.println(Ethernet.localIP());
    for(byte i = 0;i<4;i++){
        digitalWrite(42,LOW);
        delay(300);
        digitalWrite(42,HIGH);
        delay(300);
    }


    initMainLoopStackSize(200);

    xClientsQueue = xQueueCreate( 5, sizeof( EthernetClient * ) );
    createTaskLoopWithStackSize(clientAvailableTask, LOW_PRIORITY, 600);
}

void loop() {
    digitalWrite(44,LOW);
    delay(300);
    digitalWrite(44,HIGH);
    delay(300);

    EthernetClient clientAvailable = server.available();
    digitalWrite(46,LOW);
    delay(300);
    digitalWrite(46,HIGH);
    delay(300);
    // put new client on queue
    if(clientAvailable) {   

        if (!alreadyConnected) {
            // clead out the input buffer:
            clientAvailable.flush();    
            Serial.println("We have a new client");
            clientAvailable.println("Hello, client!"); 
            alreadyConnected = true;
            xQueueSendToBack( xClientsQueue, &clientAvailable, 0 );
            digitalWrite(48,LOW);
            delay(300);
            digitalWrite(48,HIGH);
            delay(300);
        } 


        digitalWrite(48,LOW);
        delay(300);
        digitalWrite(48,HIGH);
        delay(300);
    }
}









