// ARQUIVOS PARA ENVIAR DADOS SILO BAG

#include <SPI.h>        //responsável pela comunicação serial
#include <LoRa.h>       //responsável pela comunicação com o WIFI Lora
#include <Wire.h>       //responsável pela comunicação i2c
#include "SSD1306.h"    //responsável pela comunicação com o display
#include <SimpleDHT.h>  //lib para comunicação com o dht
#include "logo.h"       //contém o logo para usarmos ao iniciar o display - CRIAR UMA NOSSA

// Definição dos pinos
#define SCK   5       // GPIO5  -- SX127x's SCK
#define MISO  19      // GPIO19 -- SX127x's MISO
#define MOSI  27      // GPIO27 -- SX127x's MOSI
#define SS    18      // GPIO18 -- SX127x's CS
#define RST   14      // GPIO14 -- SX127x's RESET
#define DI00  26      // GPIO26 -- SX127x's IRQ(Interrupt Request)

#define BAND 915E6        //Frequencia do radio - podemos utilizar ainda : 433E6, 868E6, 915E6

//Sensor de temperatura e umidade
#define DHTTYPE DHT11  // DHT 11

#define MQ_analog   12    //pino analógico do sensor de gás
#define MQ_dig      13    //pino digital do sensor de gás

//parametros: address,SDA,SCL
SSD1306 display(0x3c, 4, 15);  //construtor do objeto que controlaremos o display
SimpleDHT11 dht11;

String rssi = "RSSI --";
String packSize;         //variável usada para receber tamanho do pacote convertido em String
String packet = "OK";    //variável usada para armazenar a string enviada, ao final ela é concatenada com a variável "values" abaixo
String values = "|-|-";  //valores de humidade e temperatura são separados por pipe

// DHT Sensor
const int DHTPin = 23;

int analog_value;    //variável usada para receber o sinal analógico do sensor (de 100 a 10000)
int dig_value;       //variável usada para receber o sinal digital do sensor
int gas_limit = 0;  //usado para indicar o valor mínimo de acionamento do sensor de gás (este valor varia conforme for ajustado o parafuso do sensor)
float h, c;			//variável usada para receber o valor h = humidade; c = temperatura em graus celsius


void setup() {
  h = c = 0;           //inicializa valores de humidade e temperatuda com zero
  Serial.begin(9600);  //inicia serial com 9600 bits por segundo

  pinMode(MQ_analog, INPUT);  //configura pino analog. do sensor como entrada
  pinMode(MQ_dig, INPUT);     //configura pino digital do sensor como entrada

  //configura os pinos como saida
  pinMode(16, OUTPUT);  //RST do oled
  pinMode(25, OUTPUT);  // Led Indicativo

  digitalWrite(16, LOW);   // reseta o OLED
  delay(50);               //aguarda 50ms
  digitalWrite(16, HIGH);  // enquanto o OLED estiver ligado, GPIO16 deve estar HIGH

  display.init();                     //inicializa o display
  display.flipScreenVertically();     //inverte verticalmente o display (de ponta cabeça)
  display.setFont(ArialMT_Plain_10);  //configura a fonte para um tamanho maior

  //imprime o logo na tela
  logo();

  delay(5000);      //aguarda 5000ms
  display.clear();  //apaga todo o conteúdo da tela do display

  SPI.begin(SCK, MISO, MOSI, SS);  //inicia a comunicação serial com o Lora
  LoRa.setPins(SS, RST, DI00);     //configura os pinos que serão utlizados pela biblioteca (deve ser chamado antes do LoRa.begin)

  //inicializa o Lora com a frequencia específica.
  if (!LoRa.begin(BAND)) {
    display.drawString(0, 0, "Starting LoRa failed!");
    display.display();  //mostra o conteúdo na tela
    while (1)
      ;
  }

  //indica no display que inicilizou corretamente.
  display.drawString(0, 0, "LoRa Initial success!");
  display.display();  //mostra o conteúdo na tela
  delay(2000);        //aguarda 2000ms
}

void loop() {

  readDhtSensor();                             //lê valores do sensor de temperatura
  values = "H: " + String(h) + "% T: " + String(c) + "°";  //insere na string os valores de humidade e temperatura separados por pipe

  //se no pino digital do sensor receber sinal baixo, significa que foi detectado gás
  if(gasDetected())
  {
    packet = "ALARM";     //atribui à variável o texto ALARM  
    packet+=values;       //concatena com o pacote os valores
    sendPacket();         //envia pacote por LoRa
    showDisplay(true);    //chama função para exibir no display com flag true, indicando gás detectadox
  }
  else
  { 
    packet = "";        
    packet+=values;       //concatena com o pacote os valores
    sendPacket();         //envia pacote por LoRa
    showDisplay(false);   //chama função para exibir no display, com flag false, indicando gás não detectado
  }
  Serial.println(packet);

  digitalWrite(25, HIGH);  // liga o LED indicativo
  delay(500);              // aguarda 500ms
  digitalWrite(25, LOW);   // desliga o LED indicativo
  delay(500);              // aguarda 500ms
}

//essa função prepara pacote de dados
void sendPacket() {
  LoRa.beginPacket();  //beginPacket : abre um pacote para adicionarmos os dados para envio
  LoRa.print(packet);  //print: adiciona os dados no pacote
  LoRa.endPacket();    //endPacket : fecha o pacote e envia - retorno= 1:sucesso | 0: falha
}

//essa função apenas imprime o logo na tela do display
void logo() {
  display.clear();                                            //apaga o conteúdo do display
  display.drawXbm(5, 0, logo_width, logo_height, logo_bits);  //imprime o logo presente na biblioteca "logo.h"
  display.drawString(71, 25, "AgroTech");
  display.drawString(70, 40, "Solutions");
  display.display();
}

//Essa função lê o sensor DHT11
void readDhtSensor() {
  float novoC, novoH;                                   //declara variaveis que receberão a nova temperatura e humidade
  delay(250);                                           //aguarda 250ms
  int err = dht11.read2(DHTPin, &novoC, &novoH, NULL);  //lê valores do sensor dht11 para as variaveis &c e &h

  if (err != SimpleDHTErrSuccess)  //verifica se ocorreu algum erro
  {
    //informa o erro pela serial
    Serial.print("Read DHT11 failed, err=");
    Serial.println(err);
    return;
  }

  //se foi possível ler a temperatura, então atribui para as variáveis
  c = novoC;
  h = novoH;
  //exibe valores pela serial
  Serial.print((float)c);
  Serial.println(" *C ");
  Serial.print((float)h);
  Serial.println(" H");

  delay(250);  //aguarda 250ms
}

void showDisplay(bool gasDetected) {
  display.clear();                            //apaga o conteúdo do display
  display.setTextAlignment(TEXT_ALIGN_LEFT);  //alinha texto do display à esquerda
  display.setFont(ArialMT_Plain_10);          //configura fonte do texto

  display.drawString(0, 0, "Sending packet... ");  //exibe na pos 0,0 a mensagem

  //se o gás foi detectado
  if (gasDetected) {
    display.drawString(0, 20, "Status: ALARM");  //escreve na pos 0,20 a mensagem
    display.drawString(0, 40, "GAS DETECTED!");  //escreve na pos 0,40 a mensagem
    display.display();                           //exibe display
  } else {
    display.drawString(0, 20, "Status: OK");                                  //escreve na pos 0,20 a mensagem
    display.drawString(0, 40, "H: " + String(h) + "% T: " + String(c) + "°");  //escreve na pos 0,40 a mensagem
    display.display();                                                        //exibe no display
  }
}

//função responsável por disparar mensagem caso o sensor detectar gás
bool gasDetected()
{  
  analog_value = analogRead(MQ_analog);   //lê o valor analógico do sensor (servirá apenas como informativo pela serial)
  dig_value = digitalRead(MQ_dig);        //lê o valor digital do sensor, caso sinal baixo o gás foi detectado
 
  //obs: as exibições pela serial neste código não influenciam o funcionamento do protótipo
  //exibe valor pela serial
  Serial.print(analog_value);
  //exibe separador "||" pela serial
  Serial.print(" || ");

  //lógica inversa, se "0" então foi detectado gás
  if(dig_value == 0)
  {
    //define o valor analógico mínimo (servirá apenas como informativo pela serial)
    if(gas_limit == 0 || gas_limit > analog_value)
      gas_limit = analog_value;

    //exibe na serial "Gas detectado"
    Serial.println("GAS DETECTED !!!");
    //exibe o limite mínimo de gás captado para o evento ocorrer
    Serial.println("Gas limit: "+String(gas_limit));

    //retorna verdadeiro
    return true;
  }
  else 
  {
    //exibe na serial "Gas ausente" 
    Serial.println("No gas detected...");

    //exibe o limite mínimo default "X" caso não foi acionado pela primeira vez
    if(gas_limit == 0)
      Serial.println("Gas limit: X");
    else //exibe o limite mínimo de gás captado para o evento ocorrer
      Serial.println("Gas limit: "+String(gas_limit));

    //retorna falso
    return false;
  }
}