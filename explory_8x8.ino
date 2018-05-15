/*
 Connections
 Display             Arduino UNO
 VCC                      +5V
 DIN                      MOSI (Pin 11)
 LOAD(CS)                      (Pin 10)
 CLK                      SCK  (Pin 13)
 GND                      Gnd
*/
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Max72xxPanel.h>
#define pinCS 10
#define spacer 1
int numberOfHorizontalDisplays = 3;// number of 8x8 displays
int numberOfVerticalDisplays = 1;
 
Max72xxPanel matrix = Max72xxPanel(pinCS, numberOfHorizontalDisplays, numberOfVerticalDisplays);
int scrollspeed = 60; //Domyślna prędkość wyświetlania - czym mniejsza, tym szybciej tekst się przewija
int width = 5 + spacer; // szerokość czcionki
boolean inChar = false, NewData = true, pause = false, dataAvailable = true;//zmienne potrzebne do funkcjonowania programu
char inputString[1024]="E(x)plory 2018 Bydgoszcz";//napis, do którego wpisujemy wszystkie wyświetlane znaki
int count = strlen(inputString), BTvalue = 6;//definiujemy wskaźnik indeksu i jasność
char zmienna = 0;//zmienna potrzebna do prawidłowego wyświetlania polskich znaków diakrytycznych
void(* resetFunc) (void) = 0; //funkcja resetu programowego
 
void setup() {
 
  matrix.setIntensity(BTvalue);//domyślna jasność
  
  //tą sekcję ustawiamy w zależności od ilości posiadanych wyświetlaczy: (nrWyswietlacza, liczba wyświetlaczy - 1 - aktualnie definiowany wyświetlacz, 0)
  for(int i = 0;i<numberOfHorizontalDisplays;i++)
  {
  matrix.setPosition(i, numberOfHorizontalDisplays-i-1, 0);
  }
  matrix.fillScreen(0);//wygaszenie ekranu
  matrix.write();
  Serial.begin(9600);//inicjalizacja pracy bluetooth
  //program używa przerwania na pinie 2, który powinien być podłączony do pinu 0(RX) przez opornik 10K
  pinMode(2, INPUT);
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
  digitalWrite(2, LOW);
  attachInterrupt(0, serialInterrupt, CHANGE);
}
 
void loop(){
 if(dataAvailable){
  display_data();
 } 
}
 
void display_data(){
  for ( int i = 0 ; i < width * count + matrix.width() - 1 - spacer; i++ ) {
 
    matrix.fillScreen(0);
 
    int letter = i / width;
    int x = (matrix.width() - 1) - i % width;
    int y = (matrix.height() - 8) / 2; // wyśrodkowanie tekstu
 
    while ( x + width - spacer >= 0 && letter >= 0 ) {
      if ( letter < count ) {
        matrix.drawChar(x, y, inputString[letter], HIGH, LOW, 1);
      }
 
      letter--;
      x -= width;
    }
    matrix.write(); // Wyślij do wyświetlenia
    if(!dataAvailable){
      i = width * count + matrix.width() - 1 - spacer; 
      matrix.fillScreen(LOW);
      matrix.write();
    }
    if(NewData){
      i = 0; 
      matrix.fillScreen(LOW);
      matrix.write();
      NewData = false;
    }
    while(pause == true){
      delay(0);
    } //Zatrzymaj program jeżeli pauza jest włączona
    
    delay(scrollspeed);
  }
}
volatile boolean inService = false;
//przerwanie
void serialInterrupt(){
  
  if (inService) return;
  inService = true;
  
  // Włącz przerwania, aby komunikacja szeregowa mogła działać.
  interrupts();
  while(!Serial.available());
  while (Serial.available()) {
     char ch = Serial.read();
     if(ch =='('){
       count = 0;
       inChar = true;
       
       while(inChar){
         if (Serial.available()) {
           ch = Serial.read();
           if(ch == '$'){
             inChar = false;
             dataAvailable = true;
            } else{
              
              if(ch<0)
              {
                if(zmienna<0)
                {
                  //tutaj znajduje się część kodu eliminująca konflikt występujący przy odbieraniu polskich znaków diakrytycznych
                  if(zmienna == -60 && ch == -123) inputString[count] = 182;//ą
                  else if(zmienna == -59 && ch == -125) inputString[count] = 187;//Ń
                  else if(zmienna == -60 && ch == -122) inputString[count] = 180;//Ć
                  else{
                  int dod = abs(zmienna + ch);
                  inputString[count] = dod;
                  }
                   count++;
                   zmienna = 0;
                }
                else zmienna = ch;
              }
              else
              {
             inputString[count] = ch;
             count++;
              }
           }
           if(count > 0) {
             NewData = true;
           }
         }
       }
     }  
     
     if(ch =='/'){   //Tryb komend
       inChar = true;
       while(inChar){
         if (Serial.available()) {
           ch = Serial.read();
           if(ch == 'r'){
             resetFunc();
             break;
           }
           //jasność ++
           if(ch == '+'){
             if(BTvalue < 15) {
               BTvalue ++;
               matrix.setIntensity(BTvalue);
              }
             break;
           }  
           //jasność --           
           if(ch == '-'){
            if(BTvalue > 0){
             BTvalue --;
             matrix.setIntensity(BTvalue);
            }
            break;
           }    
 
           //Zwalnianie
           if(ch == '>'){
             if(scrollspeed < 500) {
               scrollspeed = scrollspeed + 10;
             }
             break;
           }  
           //Przyspieszanie          
           if(ch == '<'){
            if(scrollspeed > 20){
             scrollspeed=scrollspeed-10;
            }
            break;
           }   
 
           //Czyszczenie ekranu          
           if(ch == 'e'){
            dataAvailable = false;
             break; 
           }
           
           //Pauza         
           if(ch == 'p'){
             if(pause == false){
               pause = true;
             } 
             else {
               pause = false;
             } 
             break; 
           }
           
           else {
            break;  //Nierozpoznana komenda 
           }
           
           
         }
       }
     }
 
    
  }
  inService = false;
}
