#ifndef MENUPOS_H
#define MENUPOS_H

#include <Arduino.h>

//----------------------------------------KLASA MenuPos---------------------------
// Klasa, której instancje są poszczególnymi pozycjami w menu
class MenuPos {
  public:
    MenuPos(String text, float value, byte _type);
    void increase();
    void decrease();
    String text;
    float value;
//  0 - moc kanału, 1 - time(float), 2 - informacja
    byte _type;
};

#endif // MENUPOS_H
