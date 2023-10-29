//ARQUIVO PARA RECEBER DADOS SILO BAG

#include <SPI.h> //responsável pela comunicação serial
#include <LoRa.h> //responsável pela comunicação com o WIFI Lora
#include <Wire.h>  //responsável pela comunicação i2c
#include "SSD1306.h" //responsável pela comunicação com o display
#include "logo.h" //contém o logo para usarmos ao iniciar o display

// Definição dos pinos 
#define SCK     5    // GPIO5  -- SX127x's SCK
#define MISO    19   // GPIO19 -- SX127x's MISO
#define MOSI    27   // GPIO27 -- SX127x's MOSI
#define SS      18   // GPIO18 -- SX127x's CS
#define RST     14   // GPIO14 -- SX127x's RESET
#define DI00    26   // GPIO26 -- SX127x's IRQ(Interrupt Request)

#define BAND    915E6  //Frequencia do radio - podemos utilizar ainda : 433E6, 868E6, 915E6

//parametros: address,SDA,SCL 
SSD1306 display(0x3c, 4, 15); //construtor do objeto que controlaremos o display

String rssi = "RSSI --";
String packSize = "--";
String packet ;

const int buzzerpin = 13; //buzzer

void setup() {
  Serial.begin(9600);             //inicia serial com 9600 bits por segundo
   
  //configura os pinos como saida
  pinMode(buzzerpin, OUTPUT);   //configura buzzer como saída
  pinMode(16,OUTPUT);           //RST do oled
  digitalWrite(16, LOW);        // reseta o OLED
  delay(50); 
  digitalWrite(16, HIGH);       // enquanto o OLED estiver ligado, GPIO16 deve estar HIGH
  
  display.init();
  display.flipScreenVertically();  
  display.setFont(ArialMT_Plain_10);
  
  logo();	//imprime o logo na tela
  
  delay(5000);
  display.clear();

  SPI.begin(SCK,MISO,MOSI,SS); //inicia a comunicação serial com o Lora
  LoRa.setPins(SS,RST,DI00); //configura os pinos que serão utlizados pela biblioteca (deve ser chamado antes do LoRa.begin)
  
  //inicializa o Lora com a frequencia específica.
  if (!LoRa.begin(BAND)) {
    display.drawString(0, 0, "Starting LoRa failed!");
    display.display();
    while (1);
  }

  //indica no display que inicilizou corretamente.
  display.drawString(0, 0, "LoRa Initial success!");
  display.drawString(0, 10, "Wait for incomm data...");
  display.display();
  delay(1000);

  //LoRa.onReceive(cbk);
  LoRa.receive(); //habilita o Lora para receber dados
}

void loop() {
  //parsePacket: checa se um pacote foi recebido
  //retorno: tamanho do pacote em bytes. Se retornar 0 (ZERO) nenhum pacote foi recebido
  int packetSize = LoRa.parsePacket();
  //caso tenha recebido pacote chama a função para configurar os dados que serão mostrados em tela
  if (packetSize) { 
    readPacket(packetSize);
  }
  delay(10);
}

//função responsável por recuperar o conteúdo do pacote recebido
//parametro: tamanho do pacote (bytes)
void readPacket(int packetSize) {
  packet ="";
  packSize = String(packetSize,DEC); //transforma o tamanho do pacote em String para imprimirmos
  rssi = "RSSI=  " + String(LoRa.packetRssi(), DEC)+ "dB"; //configura a String de Intensidade de Sinal (RSSI)
  for (int i = 0; i < packetSize; i++) { 
    packet += (char) LoRa.read(); //recupera o dado recebido e concatena na variável "packet"
  }
  Serial.println(packet);

  //se a mensagem recebida contem "alarm", então soa o buzzer
  if(strstr(packet.c_str(),"ALARM")!=NULL)
  {
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_16);
    showDisplay("GAS DETECTED!");   //exibe no display
    alarm();                        //soa o buzzer
  }
  else{
  //mostrar dados em tela
  loraData();
  }
  
}

void showDisplay(String msg)
{
  //limpa o display
  display.clear();
  //configura alinhamento de texto à esquerda
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  //configura fonte do texto
  display.setFont(ArialMT_Plain_10);
  //escreve na pos 0,25 a mensagem
  display.drawString(0 , 25 , msg); 
  //exibe no display
  display.display();  
}

//função responsável por configurar os dadosque serão exibidos em tela.
//RSSI : primeira linha
//RX packSize : segunda linha
//packet : terceira linha
void loraData(){
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, rssi);  
  display.drawString(0 , 18 , "Rx "+ packSize + " bytes");
  display.drawStringMaxWidth(0 , 39 , 128, packet);
  display.display();
}

//esta função ativa o buzzer
void alarm()
{
  digitalWrite(buzzerpin, HIGH);    //ativa buzzer
  delay(1000);                      //aguarda 1 segundo
  digitalWrite(buzzerpin, LOW);     //desativa buzzer
}

//essa função apenas imprime o logo na tela do display
void logo() {
  display.clear();                                            //apaga o conteúdo do display
  display.drawXbm(5, 0, logo_width, logo_height, logo_bits);  //imprime o logo presente na biblioteca "logo.h"
  display.drawString(71, 25, "AgroTech");
  display.drawString(70, 40, "Solutions");
  display.display();
}