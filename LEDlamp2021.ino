/*
 * STEROWNIK LAMPY LEDOWEJ (2021)
 * autor: Igor Sołdrzyński
 * igor.soldrzynski@gmail.com
 */
#include <RTClib.h>
#include <EEPROM.h>
#include <LiquidCrystal.h>
#include "KanalLED.h"

// Zmierzone maksymalne możliwe moce kanałów:
// White 1.0
// Blue 1.0
// White2(Uv) 0.55
#define sysMaxWhite 1.0
#define sysMaxBlue 1.0
#define sysMaxUv 0.55

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
#elif
#define lcdRs 8
#define lcdEn 9
#define lcdD4 4
#define lcdD5 5
#define lcdD6 6
#define lcdD7 7
#define pinWhite 13
#define pinBlue 11
#define pinUv 2
#define pinWent 12
#define pinBackLight 10
#endif

RTC_Millis rtc;
//RTC_DS3231 rtc;
//RTC_DS1307 rtc;
LiquidCrystal lcd(lcdRs,lcdEn,lcdD4,lcdD5,lcdD6,lcdD7);

//Moce kanałów ustawione przez menu:
float usrMaxWhite = 0.95;
float usrMaxBlue = 0.60;
float usrMaxUv = 0.80;

//Domyślne godziny start/stop (dziesiętnie):
float gStartWhite = 9.5;
float gStartBlue = 9.0;
float gStartUv = 9.5;

//Godziny stop (dziesiętnie):
float gStopWhite = 19.5;
float gStopBlue = 21.5;
float gStopUv = 20.5;

//Czy chłodzenie włączone:
bool cooling = 1;

//Poprzedni stan chłodzenia:
bool prevCooling = 1;

//Wciśnięty przycisk:
byte pButton = 0;
//Aktualna pozycja menu:
byte menuPos = 0;
//Poprzednia pozycja menu:
byte prevMenuPos = 1;
//Pozycje menu:
char* menuTab[] = { 
  "  Ekran glowny  ",
  "    Godzina     ", 
  "  White start   ", 
  "  White stop    ", 
  "   White moc    ", 
  "   Blue start   ", 
  "   Blue stop    ", 
  "    Blue moc    ",
  "  White2 start  ", 
  "  White2 stop   ", 
  "   White2 moc   ",
  "Zastosuj ustaw. ",
  " Zapisz ustaw.  "
};
//Liczba pozycji menu:
byte maxMenuPos = (sizeof(menuTab)/sizeof(menuTab[0]))-1;

//suma godziny i minuty:
float godzinoMinuta = 0.0;
//Poprzednia godzinoMinuta:
float poprzGodzinoMinuta = 0.0;
//ustawiona w menu godzinoMinuta:
float ustawGodzinoMinuta = 0.0;

//powołanie kanałów z wartościami początkowymi
KanalLED white(pinWhite, gStartWhite, gStopWhite, sysMaxWhite*usrMaxWhite);
KanalLED blue(pinBlue, gStartBlue, gStopBlue, sysMaxBlue*usrMaxBlue);
KanalLED uv(pinUv, gStartUv, gStopUv, sysMaxUv*usrMaxUv);

void setup(){
  Serial.begin(57600);
#ifndef ESP8266
    while (!Serial); // wait for serial port to connect. Needed for native USB
#endif
  lcd.begin(16, 2);
  lcd.clear();
  //rtc.begin();
  //poniższy begin dla RTC_Millis:
  rtc.begin(DateTime(F(__DATE__), F(__TIME__)));
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
  digitalWrite(pinBackLight, cooling);
  

  //przywrócenie zapisanych ustawień w EEPROM
//  if(EEPROM.read(0) != 0) {
//    EEPROM.get(0, white);
//    EEPROM.get(sizeof(white), blue);
//    EEPROM.get(sizeof(white)+sizeof(blue), uv);
//    usrMaxWhite = white.getMoc();
//    usrMaxBlue = 0.60;
//    usrMaxUv = 0.80;
//    gStartWhite = 9.5;
//    gStartBlue = 9.0;
//    gStartUv = 9.5;
//    gStopWhite = 19.5;
//    gStopBlue = 21.5;
//    gStopUv = 20.5;
//  }
}


//główna pętla
void loop() {
  //--------------------------------Godzinominuta obliczanie-------------------------
  DateTime zegar = rtc.now();
  //suma godziny i minuty:
  godzinoMinuta = (zegar.hour() * 100 + zegar.minute() * 100 / 60)/100.0; 
  //--------------------------------Godzinominuta koniec-----------------------------

  //jeśli zmieniła się godzinoMinuta:
  if (menuPos == 0 && (prevMenuPos != menuPos || poprzGodzinoMinuta != godzinoMinuta)) {
    //ustawianie kanałów
    blue.ustawKanal(godzinoMinuta);
    uv.ustawKanal(godzinoMinuta);
    white.ustawKanal(godzinoMinuta);

    //włącznie/wyłączenie chłodzenia i podświetlenia ekranu:
    if ((blue.getAktMoc()+uv.getAktMoc()+white.getAktMoc()) > 0) {
      cooling = 1;
    }
    else {
      cooling = 0;
    }
    if (cooling != prevCooling) {     
      digitalWrite(pinWent, cooling);
      digitalWrite(pinBackLight, cooling);
      prevCooling = cooling;
    }
    prevMenuPos = menuPos;
  } 

  menu();
  
  delay(170);
}


//funkcja zastosowująca zmiany:
void zastosujUstawienia() {
  if (ustawGodzinoMinuta != 0.0) {
    rtc.adjust(DateTime(2022, 01, 01, int(ustawGodzinoMinuta), int(60*fmod(ustawGodzinoMinuta,1)), 0));
  }
  godzinoMinuta = ustawGodzinoMinuta;
  ustawGodzinoMinuta = 0.0;
  white.setMoc(sysMaxWhite*usrMaxWhite);
  blue.setMoc(sysMaxBlue*usrMaxBlue);
  uv.setMoc(sysMaxUv*usrMaxUv);
  white.setGstart(gStartWhite);
  blue.setGstart(gStartBlue);
  uv.setGstart(gStartUv);
  white.setGstop(gStopWhite);
  blue.setGstop(gStopBlue);
  uv.setGstop(gStopUv);
  blue.ustawKanal(godzinoMinuta);
  uv.ustawKanal(godzinoMinuta);
  white.ustawKanal(godzinoMinuta);
  delay(250);
}


//funkcja zapisująca aktualne ustawienia w EEPROM
void zapiszUstawienia() {
  zastosujUstawienia();
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

void menu() {
//wczytanie wciśniętego przycisku:
  switch(button()){
    case 1:
      if (menuPos == 11) {
        zastosujUstawienia();
      }    
      menuPos = 0;
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
      if (menuPos == 4) {
        increaseFloat(&usrMaxWhite);
      } else if (menuPos == 7) {
        increaseFloat(&usrMaxBlue);
      } else if (menuPos == 10) {
        increaseFloat(&usrMaxUv);
      } else if (menuPos == 1) {
        increaseTimeFloat(&ustawGodzinoMinuta);
      } else if (menuPos == 2) {
        increaseTimeFloat(&gStartWhite);
      } else if (menuPos == 3) {
        increaseTimeFloat(&gStopWhite);
      } else if (menuPos == 5) {
        increaseTimeFloat(&gStartBlue);
      } else if (menuPos == 6) {
        increaseTimeFloat(&gStopBlue);
      } else if (menuPos == 8) {
        increaseTimeFloat(&gStartUv);
      } else if (menuPos == 9) {
        increaseTimeFloat(&gStopUv);
      }
      prevMenuPos -= 1;
      break;
    case 5:
      if (menuPos == 4) {
        decreaseFloat(&usrMaxWhite);
      } else if (menuPos == 7) {
        decreaseFloat(&usrMaxBlue);
      } else if (menuPos == 10) {
        decreaseFloat(&usrMaxUv);
      } else if (menuPos == 1) {
        decreaseTimeFloat(&ustawGodzinoMinuta);
      } else if (menuPos == 2) {
        decreaseTimeFloat(&gStartWhite);
      } else if (menuPos == 3) {
        decreaseTimeFloat(&gStopWhite);
      } else if (menuPos == 5) {
        decreaseTimeFloat(&gStartBlue);
      } else if (menuPos == 6) {
        decreaseTimeFloat(&gStopBlue);
      } else if (menuPos == 8) {
        decreaseTimeFloat(&gStartUv);
      } else if (menuPos == 9) {
        decreaseTimeFloat(&gStopUv);
      }
      prevMenuPos -= 1;
      break;
    default:
      break;
  }
  lcd.setCursor(0,0);
  if (menuPos == 0 && (prevMenuPos != menuPos || poprzGodzinoMinuta != godzinoMinuta)) {
    lcd.print(FloatToStrTime(godzinoMinuta));
    lcd.print("   REF:");
    lcd.print(int(uv.getAktMoc()/sysMaxUv/255*100));
    lcd.print("%  ");
    lcd.setCursor(0,1);
    lcd.print("WH:");
    lcd.print(int(white.getAktMoc()/sysMaxWhite/255*100));
    lcd.print("%  ");
    lcd.setCursor(8,1);
    lcd.print("BL:");
    lcd.print(int(blue.getAktMoc()/sysMaxBlue/255*100));
    lcd.print("%  ");
    poprzGodzinoMinuta = godzinoMinuta;
  } else if (prevMenuPos != menuPos) {
    lcd.clear();
    lcd.print(menuTab[menuPos]); 
    lcd.setCursor(5,1);
    if (menuPos == 4) {
      lcd.print(usrMaxWhite);
    } else if (menuPos == 7) {
      lcd.print(usrMaxBlue);
    } else if (menuPos == 10) {
      lcd.print(usrMaxUv);
    } else if (menuPos == 1) {
      lcd.print(FloatToStrTime(ustawGodzinoMinuta));
    } else if (menuPos == 2) {
      lcd.print(FloatToStrTime(gStartWhite));
    } else if (menuPos == 3) {
      lcd.print(FloatToStrTime(gStopWhite));
    } else if (menuPos == 5) {
      lcd.print(FloatToStrTime(gStartBlue));
    } else if (menuPos == 6) {
      lcd.print(FloatToStrTime(gStopBlue));
    } else if (menuPos == 8) {
      lcd.print(FloatToStrTime(gStartUv));
    } else if (menuPos == 9) {
      lcd.print(FloatToStrTime(gStopUv));
    } else {
      lcd.print("  OK");
    }
    prevMenuPos = menuPos;   
  }
}


//zwiększa zmienną z godziną:
void increaseTimeFloat(float *timeIn) {
  *timeIn+=0.25;
  if (*timeIn > 23.75) {
    *timeIn = 23.75;
  }
}


//zmniejsza zmienną z godziną:
void decreaseTimeFloat(float *timeIn) {
  *timeIn-=0.25;
  if (*timeIn < 0.0) {
    *timeIn = 0.0;
  }
}


//zwiększa zmienną z mocą:
void increaseFloat(float *mocIn) {
  *mocIn+=0.05;
  if (*mocIn > 1.0) {
    *mocIn = 1.0;
  }
}


//zmniejsza zmienną z mocą:
void decreaseFloat(float *mocIn) {
  *mocIn-=0.05;
  if (*mocIn < 0.0) {
    *mocIn = 0.0;
  }
}


//funkcja przekształca float do String - reprezentacja godziny (np. 12:50)
String FloatToStrTime(float _float) {
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
