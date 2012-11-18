/**
 * Exemplo de Uso de LCD com DuinOS
 * O exemplo demonstra as melhores práticas para uso do LCD.
 * Este exemplo está sendo construindo e no momento seu código foi ajustado para
 * LCD de 16x2 e 20x4
 * O Exemplo também demonstra o uso de Mutex para evitar concorrencia no posicionamento 
 * do cursor, o que pode causar impressões misturadas, em locais errados.
 *
 * Autor: Carlos Delfino
 * E-mail Autor: consultoria@carlosdelfino.eti.br
 */
#define configUSE_MUTEX 1;

#include <LiquidCrystal.h>

#define LCD_C16_R2 1
#define LCD_C20_R4 2
#define LCD_TYPE  LCD_C20_R4

#if (LCD_TYPE == LCD_C16_R2)
#define COL 16
#define ROW 2
#define LCD_BACKGROUND_LIMIT_MAX 255
#define LCD_BACKGROUND_LIMIT_MIN 100
#define sensor1_col 10
#define sensor2_col 10
#define sensor1_row 0
#define sensor2_row 1
#elif (LCD_TYPE == LCD_C20_R4)
#define COL 20
#define ROW 4
#define LCD_BACKGROUND_LIMIT_MAX 180
#define LCD_BACKGROUND_LIMIT_MIN 50
#define sensor1_col 10
#define sensor2_col 10
#define sensor1_row 2
#define sensor2_row 3
#else
#define COL 16
#define ROW 2
#define LCD_BACKGROUND_LIMIT_MAX 180
#define LCD_BACKGROUND_LIMIT_MIN 50
#define sensor1_col 10
#define sensor2_col 10
#define sensor1_row 0
#define sensor2_row 1
#endif



#define constrast_pin 6
#define background_pin 9

#define sensor1_pin A0
#define sensor2_pin A1


LiquidCrystal lcd(7,8,2,3,4,5);
//LiquidCrystal2 lcd(10,11,22,24,26,28);


#include <DuinOS/semphr.h>
xSemaphoreHandle xMutexLCD = NULL;

declareTaskLoop(changeLCD);
declareTaskLoop(readSensor1);
declareTaskLoop(readSensor2);
declareTaskLoop(changeLCDSensor1);
declareTaskLoop(changeLCDSensor2);
declareTaskLoop(consultaRT);
declareTaskLoop(changeLCDRT);

int sensor1;
int sensor2;

void setup(){

  initMainLoopStackSize(configMINIMAL_STACK_SIZE);

  pinMode(constrast_pin,OUTPUT);
  pinMode(background_pin,OUTPUT);

  pinMode(13,OUTPUT);

  analogWrite(constrast_pin, 100); // contraste
  analogWrite(background_pin, LCD_BACKGROUND_LIMIT_MIN); // brilho de fundo

  vSemaphoreCreateBinary(xMutexLCD);

  lcd.begin(COL,ROW);

  startMsgLcd();

  delay(1000);

  lcd.clear();

  sensorLabelsLcd();

  /* 
   * Abaixo estão as threads de leitura dos sensores, e apresentação no display
   * Apesar de serem 5 threads + o Loop, apenas 4 + o Loop estão sendo executadas.
   *
   * Já tentei a execução da ultima thread da lista definindo diversas prioridades e também
   * com Stack de diferentes tamanhos porem ainda não tive sucesso.
   */
  createTaskLoop(readSensor1,LOW_PRIORITY);
  createTaskLoop(readSensor2,LOW_PRIORITY);
  createTaskLoopWithStackSize(changeLCDSensor1,LOW_PRIORITY,200);
  createTaskLoopWithStackSize(changeLCDSensor2,LOW_PRIORITY,200);
  if(ROW > 2)  createTaskLoopWithStackSize(changeLCD,NORMAL_PRIORITY,50);
  //createTaskLoop(consultaRT,LOW_PRIORITY);
  //createTaskLoop(changeLCDRT,LOW_PRIORITY);

}

void sensorLabelsLcd(){
  if(ROW > 2){
    lcd.setCursor(0,0);
    lcd.print("Tempo");
    lcd.setCursor(8,0);
    lcd.print(":");
    lcd.setCursor(11,0);
    lcd.print(":");
  }
  lcd.setCursor(0,sensor1_row);
  lcd.print("Sensor 1:");

  lcd.setCursor(0,sensor2_row);
  lcd.print("Sensor 2:");
}

void startMsgLcd(){
  lcd.home();
  for(char i = 0; i< COL;i++) lcd.write("*");
  lcd.setCursor(0,1);

  int fator = (COL - 15)/2;
  for(char i = 0; i<fator;i++) lcd.write("*");
  lcd.print(" LCD MultiTask ");
  fator += (COL-15)%2; 
  for(char i = 0; i< fator;i++) lcd.write("*");
}

volatile int x = 40;
void loop(){ 
  delay(800);

  unsigned char i=LCD_BACKGROUND_LIMIT_MAX;
  for(;i>LCD_BACKGROUND_LIMIT_MIN;i--) {
    analogWrite(background_pin,i);
    delay(15);
  }
  for(;i<=LCD_BACKGROUND_LIMIT_MAX;i++) {
    analogWrite(background_pin,i);
    delay(15);
  }

}

taskLoop(changeLCD){ 
  char x=0;
  for(char i=0;i<255;i++){
    if(xSemaphoreTake(xMutexLCD, ( portTickType ) 100 ) == pdTRUE){
      enterCritical();
      lcd.setCursor(x++,1);
      lcd.write(i);
      exitCritical();
      xSemaphoreGive(xMutexLCD);

      nextTask();

      if(x>=COL)x=0;
    }
    delay(200);
  }   
}

int oldSensor1;
taskLoop(readSensor1){
  oldSensor1 = sensor1;
  sensor1 = analogRead(sensor1_pin);
  sensor1 = map(sensor1,0,1023,0,10000);
  if(sensor1 != oldSensor1)
    resumeTask(changeLCDSensor1);
  nextTask();
}

int oldSensor2;
taskLoop(readSensor2){
  oldSensor2 = sensor2;
  sensor2 = analogRead(sensor2_pin);
  sensor2 = map(sensor2,0,1023,-255,255);
  if(sensor2 != oldSensor2)
    resumeTask(changeLCDSensor2);
  nextTask();
}

taskLoop(changeLCDSensor1){
  if(xSemaphoreTake(xMutexLCD, ( portTickType ) 1000 ) == pdTRUE){
    portENTER_CRITICAL();
    {
      lcd.setCursor(sensor1_col,sensor1_row);
      lcd.print("     ");
      lcd.setCursor(sensor1_col,sensor1_row);
      lcd.print(sensor1);
    }
    portEXIT_CRITICAL();
    xSemaphoreGive(xMutexLCD);
  }
  suspend();
  nextTask();
  delay(200);
}

taskLoop(changeLCDSensor2){

  if(xSemaphoreTake(xMutexLCD, ( portTickType ) 1000 ) == pdTRUE){
    digitalWrite(13,!digitalRead(13));
    portENTER_CRITICAL();
    {
      lcd.setCursor(sensor2_col,sensor2_row);
      lcd.print("     ");
      lcd.setCursor(sensor2_col,sensor2_row);
      lcd.print(sensor2);
    }
    portEXIT_CRITICAL();
    xSemaphoreGive(xMutexLCD);
  }
  suspend();
  nextTask();
  delay(200);
}











