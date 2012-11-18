/**
 * Exemplo de Uso de LCD com DuinOS gerenciando Recursos com Mutex (Semaphores)
 *
 * O exemplo demonstra as melhores práticas para uso do LCD.
 *
 * Este exemplo foi ajustado para LCD de 16x2
 *
 * O Exemplo demonstra:
 * -- uso de Mutex para evitar concorrencia no posicionamento 
 * do cursor, o que pode causar impressões misturadas, em locais errados.
 * -- uso de areas criticas para se evitar falhas de sincronismos
 * em sinalizações de ativação do LCD
 * 
 * Autor: Carlos Delfino
 * E-mail Autor: consultoria@carlosdelfino.eti.br
 */
 
/*
 * Este parametro informa ao DuinOS que será usado o MUTEX, 
 * deve ser definido antes de carregar qualquer biblioteca.
 */
#define configUSE_MUTEX 1;

#include <LiquidCrystal.h>

#define sensor1_col 10
#define sensor2_col 10
#define sensor1_row 0
#define sensor2_row 1

#define constrast_pin 6
#define background_pin 9

#define sensor1_pin A0
#define sensor2_pin A1

LiquidCrystal lcd(7,8,2,3,4,5);

/*
 * Aqui é carregado o arquivo que define as macros e 
 * recursos que serão usados com Mutex.
 * o nome do arquivo é uma referencia a Semaphores (semaforos)
 * A carga do arqivo deve ser atrasada ao máximo para que os demais
 * cabeçalhos do DuinOS seja carregado corretamente.
 */
#include <DuinOS/semphr.h>
xSemaphoreHandle xMutexLCD = NULL; // defino a variavel que será o semaforo para o LCD
 
declareTaskLoop(readSensor1);
declareTaskLoop(readSensor2);
declareTaskLoop(changeLCDSensor1);
declareTaskLoop(changeLCDSensor2);

/*
 * Aqui guardo os valores lidos pelos sensores
 * alguns sugerem que seja usado Volatile já que é uma
 * área de memoria compartilhada entre threads, porém,
 * como estas variaveis não serão alteradas fora de sua
 * Thread principal, não vejo porque sobrecarrega-las
 */
int sensor1;
int sensor2;

void setup(){

  initMainLoopStackSize(configMINIMAL_STACK_SIZE);

  // em meu exemplo controlo o contraste e brilho do LCD internamente
  pinMode(constrast_pin,OUTPUT);
  pinMode(background_pin,OUTPUT);

  pinMode(13,OUTPUT); // use o led_sinalizador para depuração do código

  analogWrite(constrast_pin, 100); // contraste
  analogWrite(background_pin, 230); // brilho de fundo

  vSemaphoreCreateBinary(xMutexLCD); // Inicializo o Semaphore, veja explicações no tutorial

  lcd.begin(20,2);

  startMsgLcd();

  delay(1000);

  lcd.clear();

  sensorLabelsLcd();

  /* 
   * Abaixo estão as threads de leitura dos sensores, e apresentação no display
   */
  createTaskLoop(readSensor1,LOW_PRIORITY);
  createTaskLoop(readSensor2,LOW_PRIORITY);
  createTaskLoopWithStackSize(changeLCDSensor1,LOW_PRIORITY,200);
  createTaskLoopWithStackSize(changeLCDSensor2,LOW_PRIORITY,200);
}

/**
 * Apresenta a mensagem inicial
 */
void startMsgLcd(){
  lcd.home();
  for(char i = 0; i< 16;i++) lcd.write("*");
  lcd.setCursor(0,1);

  lcd.print(" LCD MultiTask ");
}

/**
 * inicializa os textos estáticos no LCD
 */
void sensorLabelsLcd(){
  lcd.setCursor(0,sensor1_row);
  lcd.print("Sensor 1:");

  lcd.setCursor(0,sensor2_row);
  lcd.print("Sensor 2:");
}

void loop(){ 
  /*
   * A linha abaixo é apenas para depuração
   * experimente coloca-la em pontos diferente de seu código
   * como por exemplo dentro do bloco do semaforo para verificar
   * a alternancia das tarefas. 
   */
  digitalWrite(13,!digitalRead(13));
  delay(800);
}

/**
 * A tarefa abaixo faz a leitura do sensor 1
 * caso seu valor tenha alterado apos a ultima leitura
 * ele acorda a tarefa que atualiza o display LCD 
 * e dá oportunidade a outra tarefa para ser executada.
 */
int oldSensor1;
taskLoop(readSensor1){
  oldSensor1 = sensor1;
  sensor1 = analogRead(sensor1_pin);
  sensor1 = map(sensor1,0,1023,0,10000);
  if(sensor1 != oldSensor1)
    resumeTask(changeLCDSensor1);
  nextTask();
}

/**
 * Como na tarefa do sensor 1, é feita a mesma leitura do sensor2
 * porem aqui é dado um tempo para a proxima leitura.
 * evitando leituras repetidas já que sabemos que o sensor2 
 * leva um maior tempo para oferer um novo resultado
 */
int oldSensor2;
taskLoop(readSensor2){
  oldSensor2 = sensor2;
  sensor2 = analogRead(sensor2_pin);
  sensor2 = map(sensor2,0,1023,-255,255);
  if(sensor2 != oldSensor2)
    resumeTask(changeLCDSensor2);
  nextTask();
  delay(100);
}

/**
 * Esta tarefa atualiza no visor a leitura do sensor1
 * aqui há dois pontos criticos, a concorrencia pelo uso do LCD
 * e o sincronismo de sinais para envio ao dispositivo
 *
 * Inicialmente tento obter o semaforo, aguardando 1000 ticks do DuinOS
 * caso não consiga dou continuidade a tarefa sem atualizar o LCD.
 *
 * Conseguindo o semaforo inicio uma região critica, visto que os comandos
 * a seguir não podem ser interrompidos, uso a macro enterCritical();
 * esta macro desativa as interrupções, parando assim o chaveamento das tarefas
 * neste momento funções que contam o tempo como delay() não funcionarão corretamente,
 * a função delayMicroseconds() é fundamental para uso do LCD, visto que é preciso
 * emitir o pulso de sincronismo de escrita, porém, esta função de tempo não usa 
 * nenhuma informação gerada pelas interrupções de timer.
 * Ao termino da região critica uso a macro exitCritical(); para informar seu termino
 *
 * Então suspendo a tarefa, ela somente será retomada quando a tarefa de leitura do 
 * sensor1 (readSensor1) identificar mudança no valor que foi lido do sensor, assim
 * é chamado a macro resume(changeLCDSensor1).
 * 
 */ 
taskLoop(changeLCDSensor1){
  if(xSemaphoreTake(xMutexLCD, ( portTickType ) 1000 ) == pdTRUE){
    enterCritical();
    {
      lcd.setCursor(sensor1_col,sensor1_row);
      lcd.print("     ");
      lcd.setCursor(sensor1_col,sensor1_row);
      lcd.print(sensor1);
    }
    exitCritical();
    xSemaphoreGive(xMutexLCD);
  }
  suspend(); 
}

/**
 * idem a tarefa changeLCDSensor1
 */
taskLoop(changeLCDSensor2){
  if(xSemaphoreTake(xMutexLCD, ( portTickType ) 1000 ) == pdTRUE){
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
}

