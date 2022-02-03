#ifndef KANALLED_H
#define KANALLED_H

#include <Arduino.h>

//----------------------------------------KLASA KanalLED---------------------------
// Klasa, której instancje są poszczególnymi kanałami LED
class KanalLED {
  public:
    KanalLED(int pin, float gStart, float gStop, float moc);
    void ustawKanal(float godzinominuta);
    int getPin();
    int getAktMoc();
    float getMoc();
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

#endif // KANALLED_H
