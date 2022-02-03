#include <Arduino.h>
#include "KanalLED.h"

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

  analogWrite(_pin, _aktualnaMoc);
//  SoftPWMSet(_pin, _aktualnaMoc);
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
//metoda ustawioną moc maksymalną kanału
float KanalLED::getMoc(){ return _moc; }
//metoda zwracająca godzinę włączenia kanału
float KanalLED::getGstart(){ return _gStart; }
//metoda zwracająca godzinę wyłączenia kanału
float KanalLED::getGstop(){ return _gStop; }
