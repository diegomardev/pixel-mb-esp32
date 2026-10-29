#pragma once
#include <time.h>
#include <stdio.h>
#include <stdint.h>

inline void formatClockText(char (&text)[9], const tm *local, bool seconds) {
  if (local) strftime(text, sizeof(text), seconds ? "%H:%M:%S" : "%H:%M", local);
  else snprintf(text, sizeof(text), "%s", seconds ? "--:--:--" : "--:--");
}

// Digitos de 3x5: siempre una celda de 3 columnas, incluso para el 1.
// Cada celda lleva un pixel de separacion; ':' ocupa una columna.
constexpr uint8_t CLOCK_DIGITS[11][5] = {
  {7,5,5,5,7}, {2,6,2,2,7}, {7,1,7,4,7}, {7,1,7,1,7}, {5,5,7,1,1},
  {7,4,7,1,7}, {7,4,7,5,7}, {7,1,1,1,1}, {7,5,7,5,7}, {7,5,7,1,7},
  {0,0,7,0,0} // sin sincronizar: guion
};
constexpr int16_t clockTextWidth(bool seconds) { return seconds ? 27 : 17; }

template<class Display>
void drawFixedClock(Display &display, const char *text, bool seconds, uint16_t color) {
  int16_t x = (display.width() - clockTextWidth(seconds)) / 2;
  const int16_t y = (display.height() - 5) / 2;
  const uint8_t length = seconds ? 8 : 5;
  for (uint8_t i = 0; i < length && text[i]; ++i) {
    if (text[i] == ':') {
      display.drawPixel(x, y + 1, color); display.drawPixel(x, y + 3, color);
      x += 2;
    } else {
      const uint8_t digit = (text[i] >= '0' && text[i] <= '9') ? text[i] - '0' : 10;
      for (uint8_t row = 0; row < 5; ++row)
        for (uint8_t column = 0; column < 3; ++column)
          if (CLOCK_DIGITS[digit][row] & (4 >> column)) display.drawPixel(x + column, y + row, color);
      x += 4;
    }
  }
}

// Siete marcas de 3 pixeles, de lunes a domingo, en la ultima fila.
// tm_wday usa domingo=0; sin hora sincronizada no se destaca ningun dia.
inline int8_t weekdayFromMonday(const tm *local) {
  return local && local->tm_wday>=0 && local->tm_wday<=6 ? (local->tm_wday+6)%7 : -1;
}
template<class Display>
void drawWeekdayIndicators(Display &display,const tm *local,uint16_t inactiveColor,uint16_t activeColor) {
  constexpr int16_t width=27;
  if(display.width()<width || display.height()<1)return;
  const int16_t left=(display.width()-width)/2,y=display.height()-1;
  const int8_t current=weekdayFromMonday(local);
  for(uint8_t day=0;day<7;++day)
    for(uint8_t pixel=0;pixel<3;++pixel)
      display.drawPixel(left+day*4+pixel,y,day==current?activeColor:inactiveColor);
}
