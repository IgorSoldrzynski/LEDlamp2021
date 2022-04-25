/*
 * STEROWNIK LAMPY LEDOWEJ (2021)
 * autor: Igor Sołdrzyński
 * igor.soldrzynski@gmail.com
 */
#include <RTClib.h>
#include <EEPROM.h>

#include "KanalLED.h"
#include "MenuPos.h"

/* Model zegara RTC:
 *  1 - DS3231
 *  2 - DS1307
 *  3 - RTC_Millis (programowy)  
 */
#define RTCtype 1

/* Typ LCD:
 *  1 - LCDshield16x2 normal
 *  2 - LCDshield16x2 resistor (niskie odczyty z A0)
 *  3 - OLED I2C
 */
#define LCDtype 3

/* Ukrycie trzeciego kanału (wersja dwukanałowa):
 *  0 - nie (wersja trójkanałowa)
 *  1 - tak (wersja dwukanałowa)
 */
#define hide3ch 0

/* Zmierzone maksymalne możliwe moce kanałów:
 *  Wersja 3ch: White 1.0, Blue 1.0, White2(Uv) 0.55
 *  Wersja 2ch: White 1.0, Blue 0.75
 */
#define sysMaxWhite 1.0
#if hide3ch == 0
  #define sysMaxBlue 1.0
#else
  #define sysMaxBlue 0.75
#endif
#define sysMaxUv 0.55

#define defGstart 9.5
#define defGstop 21.0
#define defMoc 0.7

#ifdef ARDUINO_ESP8266_WEMOS_D1R1
#define lcdRs D8
#define lcdEn D9
#define lcdD4 D4
#define lcdD5 D5
#define lcdD6 D6
#define lcdD7 D7
#define pinWhite D13
#define pinBlue D11
#define pinUv D2
#define pinWent D12
#define pinBackLight D10
#elif LCDtype == 3
#define pinWhite 3
#define pinBlue 10
#define pinUv 9
#define pinWent 11
#define pinBackLight 8
#else
#define lcdRs 8
#define lcdEn 9
#define lcdD4 4
#define lcdD5 5
#define lcdD6 6
#define lcdD7 7
#define pinWhite 3
#define pinBlue 11
#define pinUv 2
#define pinWent 12
#define pinBackLight 10
#endif

//definicja pozycji w tabeli MenuPos:
//(gdyby zaszła koniecznośc modyfikacji menu)
#define posEkranGlowny 0
#define posUstawGodzine 1
#define posWhiteStart 2
#define posWhiteStop 3
#define posWhiteMoc 4
#define posBlueStart 5
#define posBlueStop 6
#define posBlueMoc 7
#define posUvStart 8
#define posUvStop 9
#define posUvMoc 10
#define posZastosuj 11
#define posZapisz 12

//Pozycje menu:
MenuPos menuTab[] = { 
  MenuPos("  Ekran glowny  ", 0.0, 2),
  MenuPos(" Ustaw godzine  ", 0.0, 1), 
  MenuPos("  White start   ", defGstart, 1), 
  MenuPos("  White stop    ", defGstop, 1), 
  MenuPos("  White moc     ", defMoc, 0), 
  MenuPos("   Blue start   ", defGstart, 1), 
  MenuPos("   Blue stop    ", defGstop, 1), 
  MenuPos("   Blue moc     ", defMoc, 0),
  MenuPos("  White2 start  ", defGstart, 1), 
  MenuPos("  White2 stop   ", defGstop, 1), 
  MenuPos("  White2 moc    ", defMoc, 0),
  MenuPos("Zastosuj ustaw. ", 0.0, 2),
  MenuPos("Zapisz w EEPROM ", 0.0, 2)
};
//Liczba pozycji menu:
byte maxMenuPos = (sizeof(menuTab)/sizeof(menuTab[0]))-1;
//Aktualna pozycja menu:
byte menuPos = 0;
//Poprzednia pozycja menu:
byte prevMenuPos = 1;

//Wciśnięty przycisk:
byte pButton = 0;

//Czy chłodzenie włączone:
bool cooling = 1;

//Czy podświetlenie włączone:
bool backlight = 1;

//suma godziny i minuty:
float godzinoMinuta = 0.0;
//Poprzednia godzinoMinuta:
float poprzGodzinoMinuta = 0.0;

//powołanie kanałów z wartościami początkowymi
KanalLED white(pinWhite, defGstart, defGstop, sysMaxWhite*defMoc);
KanalLED blue(pinBlue, defGstart, defGstop, sysMaxBlue*defMoc);
KanalLED uv(pinUv, defGstart, defGstop, sysMaxUv*defMoc);

//W zależności od zastosowanego modułu RTC:
#if RTCtype == 1
  RTC_DS3231 rtc;
#elif RTCtype == 2
  RTC_DS1307 rtc;
#else 
  RTC_Millis rtc;
#endif

//W zależności od typu LCD:
#if LCDtype == 2
  #define buttonReadFactor 0.7
#else
  #define buttonReadFactor 1
#endif
#if LCDtype == 3
  #include <Wire.h>
  #include <Adafruit_GFX.h>
  #include <Adafruit_SSD1306.h>
  Adafruit_SSD1306 lcd(128, 32, &Wire, -1);
  #define pinButOk 5
  #define pinButDown 7
  #define pinButPlus 6
  #define pinButMin 4
#else
  #include <LiquidCrystal.h>
  LiquidCrystal lcd(lcdRs,lcdEn,lcdD4,lcdD5,lcdD6,lcdD7);
#endif



void setup(){
  Serial.begin(57600);
  #ifndef ESP8266
      while (!Serial); // wait for serial port to connect. Needed for native USB
  #endif
  #if LCDtype == 3
    lcd.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    delay(2000);
    lcd.clearDisplay();
    lcd.setTextSize(1);
    lcd.setTextColor(WHITE);
    pinMode(pinButOk, INPUT_PULLUP);
    pinMode(pinButDown, INPUT_PULLUP);
    pinMode(pinButPlus, INPUT_PULLUP);
    pinMode(pinButMin, INPUT_PULLUP);
  #else
    lcd.begin(16, 2);
    lcd.clear();
  #endif

  #if RTCtype == 1 || RTCtype == 2
    rtc.begin();
  #else 
    rtc.begin(DateTime(F(__DATE__), F(__TIME__)));
  #endif
  // Ustawienie czasu w zegarze. Po ustawieniu zakomentować poniższe linie kodu i wgrać program jeszcze raz!
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); //systemowy
  //rtc.adjust(DateTime(2022, 01, 01, 20, 58, 0)); //lub ręcznie ustawiony

  // Ustawienie pinów wyjścia:
  pinMode(pinWhite, OUTPUT);
  pinMode(pinBlue, OUTPUT);
  pinMode(pinUv, OUTPUT);
  pinMode(pinWent, OUTPUT);
  pinMode(pinBackLight, OUTPUT);
  digitalWrite(pinWent, cooling);
  digitalWrite(pinBackLight, backlight);
  
  //przywrócenie zapisanych ustawień w EEPROM
  #ifdef ESP8266
    EEPROM.begin(sizeof(white)+sizeof(blue)+sizeof(uv));
  #endif
  if(EEPROM.read(0) != 0) {
    EEPROM.get(0, white);
    EEPROM.get(sizeof(white), blue);
    EEPROM.get(sizeof(white)+sizeof(blue), uv);

    menuTab[posWhiteStart].value = white.getGstart();
    menuTab[posWhiteStop].value = white.getGstop();
    menuTab[posWhiteMoc].value = white.getMoc()/sysMaxWhite;

    menuTab[posBlueStart].value = blue.getGstart();
    menuTab[posBlueStop].value = blue.getGstop();
    menuTab[posBlueMoc].value = blue.getMoc()/sysMaxBlue;

    menuTab[posUvStart].value = uv.getGstart();
    menuTab[posUvStop].value = uv.getGstop();
    menuTab[posUvMoc].value = uv.getMoc()/sysMaxUv;
  }
  #ifdef ESP8266
    EEPROM.end();
  #endif
}


//główna pętla
void loop() {
  //--------------------------------Godzinominuta obliczanie-------------------------
  DateTime zegar = rtc.now();
  //suma godziny i minuty:
  godzinoMinuta = (zegar.hour() * 100 + zegar.minute() * 100 / 60)/100.0; 
  //--------------------------------Godzinominuta koniec-----------------------------

  //jeśli zmieniła się godzinoMinuta:
  if (poprzGodzinoMinuta != godzinoMinuta) {
    loopUpdate();
    lcdUpdate();
    poprzGodzinoMinuta = godzinoMinuta;
  } 

  menu();
  
  delay(170);
}


//funkcja realizująca cykliczne czynności:
void loopUpdate() {
//ustawianie kanałów
    blue.ustawKanal(godzinoMinuta);
    uv.ustawKanal(godzinoMinuta);
    white.ustawKanal(godzinoMinuta);

    //włącznie/wyłączenie chłodzenia i podświetlenia ekranu:
    if ((blue.getAktMoc()+uv.getAktMoc()+white.getAktMoc()) > 0) {
      cooling = 1;
      backlight = 1;
    }
    else {
      cooling = 0;
      if (menuPos == posEkranGlowny)  {
        backlight = 0;
      } else {
        backlight = 1;
      }
    }
    digitalWrite(pinWent, cooling);
    digitalWrite(pinBackLight, backlight);    
}


void menu() {
//wczytanie wciśniętego przycisku:
  switch(button()){
    case 1:
      if (menuPos == posUstawGodzine) {
        zastosujGodzina();
      }
      else if (menuPos == posZastosuj) {
        zastosujUstawienia();
      } else if (menuPos == posZapisz) {
        zapiszUstawienia();
      }
      menuPos = posEkranGlowny;
      break;
    case 2:
      menuPos-=1;
      if (menuPos < 1 || menuPos > maxMenuPos) {
        menuPos = maxMenuPos;
      }
      break;
    case 3:
      menuPos+=1;
      if (menuPos > maxMenuPos) {
        menuPos = 1;
      }
      break;
    case 4:
      menuTab[menuPos].increase();
      prevMenuPos -= 1;
      break;
    case 5:
      menuTab[menuPos].decrease();
      prevMenuPos -= 1;
      break;
    default:
      break;
  }
  if (prevMenuPos != menuPos) {
    lcdUpdate();
    prevMenuPos = menuPos;
    //loopUpdate(); //?
  }
}


void lcdUpdate() {
  Serial.println("lcdUpdate()");
  #if LCDtype == 3
    lcd.clearDisplay();
    if (menuPos == posEkranGlowny) {
      lcd.setCursor(90,0);
      lcd.println(FloatToStrNice(godzinoMinuta, 1));
      lcd.print(F("Biale  "));
      lcd.print(String(int(white.getAktMoc()/sysMaxWhite/255*100)));
      lcd.println(F("%"));
      lcd.print(F("Nieb.  "));
      lcd.print(String(int(blue.getAktMoc()/sysMaxBlue/255*100)));
      lcd.println(F("%"));
      #if hide3ch == 0
        lcd.print(F("Boczne "));
        lcd.print(String(int(uv.getAktMoc()/sysMaxUv/255*100)));
        lcd.print(F("%"));
      #endif 
    } else {
      lcd.setCursor(0,0);
      String menuText = menuTab[menuPos].text;
      menuText.trim();
      lcd.println(menuText);
      lcd.println();
      lcd.setTextSize(2);
      lcd.println(FloatToStrNice(menuTab[menuPos].value, menuTab[menuPos]._type));
      lcd.setTextSize(1);
    }
    lcd.display();
  #else
    lcd.clear(); 
    lcd.setCursor(0,0);
    if (menuPos == posEkranGlowny) {
      lcd.print(FloatToStrNice(godzinoMinuta, 1));
      #if hide3ch == 0
        lcd.print("   REF:");
        lcd.print(int(uv.getAktMoc()/sysMaxUv/255*100));
        lcd.print("%  ");
      #endif
      lcd.setCursor(0,1);
      lcd.print("WH:");
      lcd.print(int(white.getAktMoc()/sysMaxWhite/255*100));
      lcd.print("%  ");
      lcd.setCursor(8,1);
      lcd.print("BL:");
      lcd.print(int(blue.getAktMoc()/sysMaxBlue/255*100));
      lcd.print("%  ");   
    } else {
      digitalWrite(pinBackLight, HIGH);
      lcd.print(menuTab[menuPos].text); 
      lcd.setCursor(5,1);
      lcd.print(FloatToStrNice(menuTab[menuPos].value, menuTab[menuPos]._type));
    }
  #endif
}


//zapisuje ustawioną godzinę
void zastosujGodzina() {
  float ustawGodzinoMinuta = menuTab[posUstawGodzine].value;
  if (ustawGodzinoMinuta != 0.0) {
    rtc.adjust(DateTime(2022, 01, 01, int(ustawGodzinoMinuta), int(60*fmod(ustawGodzinoMinuta,1)), 0));
    godzinoMinuta = ustawGodzinoMinuta;
  }
  menuTab[posUstawGodzine].value = 0.0;
  loopUpdate();
  lcdUpdate();
}


//funkcja zastosowująca zmiany:
void zastosujUstawienia() {
  white.setGstart(menuTab[posWhiteStart].value);
  white.setGstop(menuTab[posWhiteStop].value);  
  white.setMoc(sysMaxWhite*menuTab[posWhiteMoc].value);

  blue.setGstart(menuTab[posBlueStart].value);
  blue.setGstop(menuTab[posBlueStop].value);
  blue.setMoc(sysMaxBlue*menuTab[posBlueMoc].value);

  uv.setGstart(menuTab[posUvStart].value);
  uv.setGstop(menuTab[posUvStop].value);
  uv.setMoc(sysMaxUv*menuTab[posUvMoc].value);
  
  loopUpdate();
  lcdUpdate();
  delay(250);
}


//funkcja zapisująca aktualne ustawienia w EEPROM
void zapiszUstawienia() {
  zastosujUstawienia();
  //opis z dokumentacji EEPROM.put():
  //This function uses EEPROM.update() to perform the write, so does not rewrites the value if it didn't change.
  #ifdef ESP8266
    EEPROM.begin(sizeof(white)+sizeof(blue)+sizeof(uv));
  #endif  
  EEPROM.put(0, white);
  EEPROM.put(sizeof(white), blue);
  EEPROM.put(sizeof(white)+sizeof(blue), uv);
  #ifdef ESP8266
    EEPROM.end();
  #endif
}


// zwraca stan przycisku:
// 0 - brak wciśnięcia
// 1 - select
// 2 - góra
// 3 - dół
// 4 - prawo
// 5 - lewo
byte button() {
  #if LCDtype == 3
    if (digitalRead(pinButPlus) == LOW) {
      return 4;
    }
    else if (digitalRead(pinButDown) == LOW) {
      return 3;
    }
    else if (digitalRead(pinButMin) == LOW) {
      return 5;
    }
    else if (digitalRead(pinButOk) == LOW) {
      return 1;
    }
    else {
      return 0;
    }
     
  #else
    int x = analogRead(A0);
    if (x >= 800*buttonReadFactor) {
      return 0;
    }
    else if (x < 60*buttonReadFactor) {
      return 4;
    }
    else if (x < 200*buttonReadFactor) {
      return 2;
    }
    else if (x < 400*buttonReadFactor) {
      return 3;
    }
    else if (x < 600*buttonReadFactor) {
      return 5;
    }
    else if (x < 800*buttonReadFactor) {
      return 1;
    }
  #endif
}


//funkcja przekształca float do String - reprezentacja godziny (np. 12:50), reprezentacja mocy (np. 95%)
String FloatToStrNice(float _float, byte _type) {
  if (_type == 0 ) {
    return String(int(round(_float*100)))+"%";
  }
  else if (_type == 1) {
    String _hours = String(int(_float));
    String _minutes = String(int(60*fmod(_float,1)));
    if (_hours.length()<2) {
      _hours = "0"+_hours;
    }
    if (_minutes.length()<2) {
      _minutes = "0"+_minutes;
    }
    else if (_minutes.length()>2) {
      _minutes.remove(1);
    }
    return _hours+":"+_minutes;
  }
  else {
    return "  OK";
  }
}
