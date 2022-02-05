#include <Arduino.h>
#include "MenuPos.h"

//konstruktor
MenuPos::MenuPos(String text, float value, byte _type) {
  this->text = text;
  this->value = value;
//  0 - moc kanału, 1 - time(float), 2 - informacja:
  this->_type = _type;
}

//zwiększenie wartości:
void MenuPos::increase() {
  if (_type <= 1) {
    float add = 0.05 + (_type * 0.20);  
    //dla mocy max to 1.0 a dla czasu max to 23.75:
    float limit = 1.0 + (_type * 22.75);
    
    value += add;
    if (value > limit) {
      value = limit;
    }
  }
}

//zmniejszenie wartości:
void MenuPos::decrease() {
  if (_type <= 1) {
    float sub = 0.05 + (_type * 0.20);
    //dla mocy i czasu min to 0.0
    
    value -= sub;
    if (value < 0.0) {
      value = 0.0;
    }
  }
}
