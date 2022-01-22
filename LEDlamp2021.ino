/*
 * STEROWNIK LAMPY LEDOWEJ (2021)
 * autor: Igor Sołdrzyński
 * igor.soldrzynski@gmail.com
 */
#include <SoftPWM.h>
#include <Wire.h>
#include <RTClib.h>
#include <EEPROM.h>
#include <LiquidCrystal.h>
//#include "KanalLED.h"

#define pinWhite A2
#define pinBlue A1
#define pinUv A4
#define pinWent A3
#define pinBackLight 10
// Zmierzone maksymalne możliwe moce kanałów:
// White 1.0
// Blue 1.0
// White2(Uv) 0.55
#define sysMaxWhite 1.0
#define sysMaxBlue 1.0
#define sysMaxUv 0.55

//RTC_DS3231 rtc;
RTC_DS1307 rtc;
LiquidCrystal lcd(8,9,4,5,6,7);

//Moce kanałów ustawione przez menu:
float usrMaxWhite = 0.95;
float usrMaxBlue = 0.80;
float usrMaxUv = 0.90;

//Domyślne godziny start/stop (dziesiętnie):
const float gStartWhite = 9.5;
const float gStartBlue = 9.0;
const float gStartUv = 8.5;

//Godziny stop (dziesiętnie):
const float gStopWhite = 19.5;
const float gStopBlue = 21.5;
const float gStopUv = 20.5;

//Czy chłodzenie włączone:
bool cooling = 0;

//Wciśnięty przycisk:
byte pButton = 0;
//Aktualna pozycja menu:
byte menuPos = 0;
//Pozycje menu:
char* menuTab[][2] = { 
  {"Ekran główny",""}, 
  {"Godzina","00:00"}, 
  {"Kanał 1 start","00:00"}, {"Kanał 1 stop","00:00"}, {"Kanał 1 moc","100%"}, 
  {"Kanał 2 start","00:00"}, {"Kanał 2 stop","00:00"}, {"Kanał 2 moc","100%"},
  {"Kanał 3 start","00:00"}, {"Kanał 3 stop","00:00"}, {"Kanał 3 moc","100%"},
  {"Zapisz ustawienia","OK"} 
};
//Liczba pozycji menu:
byte maxMenuPos = sizeof(menuTab)/sizeof(menuTab[0]);

//----------------------------------------KLASA KanalLED---------------------------
// Klasa, której instancje są poszczególnymi kanałami LED
class KanalLED {
  public:
    KanalLED(int pin, float gStart, float gStop, float moc);
    void ustawKanal(float godzinominuta);
    int getPin();
    int getAktMoc();
    float getGstart();
    float getGstop();
    void setPin(int newPin);
    void setMoc(float newMoc);
    void setGstart(float newGstart);
    void setGstop(float newGstop);
    
  private:
    int _pin;
    float _gStart;
    float _gStop;
    float _moc;
    int _aktualnaMoc;  
};
//konstruktor
KanalLED::KanalLED(int pin, float gStart, float gStop, float moc) {
  setPin(pin);
  setGstart(gStart);
  setGstop(gStop);
  setMoc(moc);
}
//metoda ustawiająca aktualną moc kanału na podstawie godzinominuty
void KanalLED::ustawKanal(float godzinominuta) {
  //oblicznie aktualnej mocy
  if(godzinominuta>=_gStart && godzinominuta<=_gStop) {
    int czasMap = map((int)(godzinominuta*100), (int)(_gStart*100), (int)(_gStop*100), 900, 2100);
    float czas = czasMap / 100.0;
    //_aktualnaMoc = 0,0831x^3-6,406x^2+133,09x-740,4 ; gdzie x = aktualny czas  
    _aktualnaMoc=(int)((0.0831149*pow(czas,3)-6.406366*pow(czas,2)+133.09292*czas-740.4333)*_moc*255/100);
    if(_aktualnaMoc<0) _aktualnaMoc=0;
  }
  else {
    _aktualnaMoc = 0;
  }

  //korekcja rozbłysków driverów
//  if(aktualnaMoc>0 && aktualnaMoc<26) aktualnaMoc=15;
  //ustawianie pwm
//  analogWrite(_pin, 255 - aktualnaMoc);
  SoftPWMSet(_pin, _aktualnaMoc);
}

//---------------------------------------Setery------------------------------------
//metoda ustawiająca nr pin kanału
void KanalLED::setPin(int newPin){ _pin = newPin; }
//metoda ustawiająca moc (mnożnik) kanału
void KanalLED::setMoc(float newMoc){ _moc = newMoc; }
//metoda ustawiająca godzinę włączenia kanału
void KanalLED::setGstart(float newGstart){ _gStart = newGstart; }
//metoda ustawiająca godzinę wyłączenia kanału
void KanalLED::setGstop(float newGstop){ _gStop = newGstop; }

//---------------------------------------Getery------------------------------------
//metoda zwracająca nr pin kanału
int KanalLED::getPin(){ return _pin; }
//metoda zwracająca moc (mnożnik) kanału
int KanalLED::getAktMoc(){ return _aktualnaMoc; }
//metoda zwracająca godzinę włączenia kanału
float KanalLED::getGstart(){ return _gStart; }
//metoda zwracająca godzinę wyłączenia kanału
float KanalLED::getGstop(){ return _gStop; }

//-----------------------------------KONIEC klasy KanalLED-------------------------


//powołanie kanałów z wartościami początkowymi
KanalLED white(pinWhite, gStartWhite, gStopWhite, sysMaxWhite*usrMaxWhite);
KanalLED blue(pinBlue, gStartBlue, gStopBlue, sysMaxBlue*usrMaxBlue);
KanalLED uv(pinUv, gStartUv, gStopUv, sysMaxUv*usrMaxUv);


void setup(){
  Serial.begin(9600);
  SoftPWMBegin();
  lcd.begin(16, 2);
  rtc.begin();
  // Ustawienie czasu w zegarze. Po ustawieniu zakomentować poniższe linie kodu i wgrać program jeszcze raz!
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); //systemowy
  //rtc.adjust(DateTime(2022, 01, 01, 19, 22, 0)); //lub ręcznie ustawiony


  // Ustawienie pinów wyjścia:
  pinMode(pinWent, OUTPUT);
  pinMode(pinBackLight, OUTPUT);


  //przywrócenie zapisanych ustawień w EEPROM
//  if(EEPROM.read(0) != 0) {
//    EEPROM.get(0, white);
//    EEPROM.get(sizeof(white), blue);
//    EEPROM.get(sizeof(white)+sizeof(blue), uv);
//  }
}


//główna pętla
void loop() {
  //--------------------------------Godzinominuta obliczanie-------------------------
  DateTime zegar = rtc.now();
  //suma godziny i minuty:
  float godzinoMinuta = (zegar.hour() * 100 + zegar.minute() * 100 / 60)/100.0; 
  //--------------------------------Godzinominuta koniec-----------------------------

  //ustawianie kanałów
  blue.ustawKanal(godzinoMinuta);
  uv.ustawKanal(godzinoMinuta);
  white.ustawKanal(godzinoMinuta);

  if ((blue.getAktMoc()+uv.getAktMoc()+white.getAktMoc()) > 0) {
    cooling = 1;
  }
  else {
    cooling = 0;
  }
  digitalWrite(pinWent, cooling);
  digitalWrite(pinBackLight, cooling);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(FloatToStrTime(godzinoMinuta));
  lcd.setCursor(0,1);
  lcd.print("W:");
  lcd.print(white.getAktMoc());
  lcd.print("B:");
  lcd.print(blue.getAktMoc());
  lcd.print("@:");
  lcd.print(uv.getAktMoc());

  //wczytanie wciśniętego przycisku:
  // 0 - brak wciśnięcia
  // 1 - select
  // 2 - góra
  // 3 - dół
  // 4 - prawo
  // 5 - lewo
//  switch(button()){
//    case 1:
//      if (menuPos > 0) {
//        menuPos = 0;
//      }
//      else if (menuPos = 0) {
//        menuPos = 1;
//      }
//      break;
//    case 2:
//      if (menuPos > 0) {
//        menuPos+=1;
//        if (menuPos > maxMenuPos) {
//          menuPos = 1;
//        }
//      }
//      break;
//    case 3:
//      if (menuPos > 0) {
//        menuPos-=1;
//        if (menuPos < 1) {
//          menuPos = maxMenuPos;
//        }
//      }
//      break;
//    default:
//      break;
//  }
  
  delay(50);
}


//funkcja zapisująca aktualne ustawienia w EEPROM
void zapiszUstawienia() {
  //opis z dokumentacji EEPROM.put():
  //This function uses EEPROM.update() to perform the write, so does not rewrites the value if it didn't change.
  EEPROM.put(0, white);
  EEPROM.put(sizeof(white), blue);
  EEPROM.put(sizeof(white)+sizeof(blue), uv);
}


//funkcja czyscząca EEPROM
void clearEEPROM() {
  for (int i = 0; i < EEPROM.length(); i++) {
    EEPROM.write(i, 0);
  }
}


// zwraca stan przycisku:
// 0 - brak wciśnięcia
// 1 - select
// 2 - góra
// 3 - dół
// 4 - prawo
// 5 - lewo
byte button() {
  int x = analogRead(A0);
  if (x >=800) {
    return 0;
  }
  else if (x < 60) {
    return 4;
  }
  else if (x < 200) {
    return 2;
  }
  else if (x < 400) {
    return 3;
  }
  else if (x < 600) {
    return 5;
  }
  else if (x < 800) {
    return 1;
  }
}


//funkcja przekształca float do String - reprezentacja godziny (np. 12:50)
String FloatToStrTime(float _float) {
  String _hours = String(int(_float));
  String _minutes = String(int(60*fmod(_float,1)));
  if(_hours.length()<2) {
    _hours = "0"+_hours;
  }
  if(_minutes.length()<2) {
    _minutes = "0"+_minutes;
  }
  else if(_minutes.length()>2) {
    _minutes.remove(1);
  }
  return _hours+":"+_minutes;
}
