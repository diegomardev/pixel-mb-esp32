#pragma once
#include <Adafruit_NeoMatrix.h>
#include "Config.h"

// NeoMatrix tiene DOS constructores: matriz unica y mosaico de paneles.
// Los uint8_t de Config.h elegian el mosaico: 32*8*GPIO4*layout12 = 12288 LED,
// interpretando ademas MATRIX_TYPE (82) como pin. Forzar los tipos correctos.
class PixelMatrix final : public Adafruit_NeoMatrix {
 public:
  PixelMatrix()
      : Adafruit_NeoMatrix(static_cast<int>(MATRIX_WIDTH),
                           static_cast<int>(MATRIX_HEIGHT), MATRIX_PIN,
                           MATRIX_LAYOUT, static_cast<neoPixelType>(MATRIX_TYPE)) {}
  bool configurationValid() const {
    return numPixels() == uint16_t(MATRIX_WIDTH) * MATRIX_HEIGHT &&
           getPin() == MATRIX_PIN && getPixels() != nullptr;
  }
};
