#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

// ESP32-2432S028R "Cheap Yellow Display" (landscape, 320 x 240).
// Touch wiring and calibration follow CYD-weather-display.
static constexpr int TOUCH_IRQ = 36;
static constexpr int TOUCH_MOSI = 32;
static constexpr int TOUCH_MISO = 39;
static constexpr int TOUCH_CLK = 25;
static constexpr int TOUCH_CS = 33;
static constexpr int TOUCH_MIN_X = 200;
static constexpr int TOUCH_MAX_X = 3700;
static constexpr int TOUCH_MIN_Y = 240;
static constexpr int TOUCH_MAX_Y = 3800;
static constexpr int W = 320;
static constexpr int H = 240;

TFT_eSPI tft;
SPIClass touchSPI(VSPI);
XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);

enum Language : uint8_t { DANISH, ENGLISH };
Language language = DANISH;
uint32_t nextBlink = 0;
uint32_t blinkUntil = 0;
uint32_t greetingUntil = 0;
bool mouthOpen = false;
uint32_t nextMouthFrame = 0;

static const uint16_t BG       = tft.color565(255, 231, 202);
static const uint16_t FUR      = tft.color565(190, 119, 61);
static const uint16_t FUR_DARK = tft.color565(110,  60, 33);
static const uint16_t MUZZLE   = tft.color565(250, 220, 176);
static const uint16_t INNER    = tft.color565(235, 162, 151);
static const uint16_t INK      = tft.color565(47, 36, 33);
static const uint16_t ACCENT   = tft.color565(237, 101, 75);
static const uint16_t WHITE    = tft.color565(255, 255, 255);

// Named helper avoids depending on a particular TFT_eSPI colour macro set.

void drawEye(int x, bool closed) {
  if (closed) {
    tft.drawLine(x - 7, 111, x + 7, 111, INK);
    tft.drawLine(x - 5, 113, x + 5, 113, INK);
  } else {
    tft.fillEllipse(x, 111, 7, 10, INK);
    tft.fillCircle(x - 2, 107, 2, WHITE);
  }
}

void drawBear(bool blink, bool openMouth) {
  tft.fillScreen(BG);

  // Soft ground shadow and round teddy body.
  tft.fillEllipse(156, 216, 73, 9, tft.color565(224, 196, 161));
  tft.fillEllipse(157, 185, 68, 48, FUR);
  tft.fillEllipse(157, 194, 27, 25, MUZZLE);
  tft.fillCircle(117, 177, 21, FUR);
  tft.fillCircle(197, 177, 21, FUR);
  tft.fillCircle(117, 177, 10, MUZZLE);
  tft.fillCircle(197, 177, 10, MUZZLE);

  // Head and ears.
  tft.fillCircle(157, 112, 67, FUR);
  tft.fillCircle(102, 62, 24, FUR);
  tft.fillCircle(212, 62, 24, FUR);
  tft.fillCircle(102, 62, 12, INNER);
  tft.fillCircle(212, 62, 12, INNER);
  tft.fillEllipse(157, 132, 34, 23, MUZZLE);

  drawEye(135, blink);
  drawEye(179, blink);
  tft.fillEllipse(157, 123, 9, 7, INK);

  if (openMouth) {
    tft.fillEllipse(157, 145, 10, 13, INK);
    tft.fillEllipse(157, 151, 5, 4, INNER);
  } else {
    tft.drawArc(157, 140, 10, 8, 20, 160, INK, BG);
  }

  // A little red bow tie.
  tft.fillCircle(147, 176, 9, ACCENT);
  tft.fillCircle(167, 176, 9, ACCENT);
  tft.fillCircle(157, 176, 5, FUR_DARK);

  // Language picker is always visible and touchable.
  tft.fillRoundRect(244, 8, 68, 30, 8, WHITE);
  tft.drawRoundRect(244, 8, 68, 30, 8, ACCENT);
  tft.setTextColor(INK, WHITE);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(language == DANISH ? "DA  /  EN" : "EN  /  DA", 278, 23, 2);

  tft.setTextColor(FUR_DARK, BG);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(language == DANISH ? "Tryk for at sige hej!" : "Tap to say hello!", 157, 226, 2);

  if (millis() < greetingUntil) {
    const char* words = language == DANISH ? "Hej!" : "Hello!";
    tft.fillRoundRect(202, 51, 88, 37, 12, WHITE);
    tft.fillTriangle(214, 82, 207, 94, 229, 84, WHITE);
    tft.setTextColor(INK, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(words, 246, 70, 4);
  }
}

bool readTap(int& x, int& y) {
  static bool pressed = false;
  const bool down = touch.tirqTouched() && touch.touched();
  if (!down) { pressed = false; return false; }
  if (pressed) return false;
  pressed = true;
  delay(12);
  TS_Point p = touch.getPoint();
  if (p.z < 200) return false;
  x = constrain(map(p.x, TOUCH_MIN_X, TOUCH_MAX_X, 0, W - 1), 0, W - 1);
  y = constrain(map(p.y, TOUCH_MIN_Y, TOUCH_MAX_Y, 0, H - 1), 0, H - 1);
  return true;
}

void setup() {
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(BG);

  touchSPI.begin(TOUCH_CLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
  touch.begin(touchSPI);
  touch.setRotation(1);

  const uint32_t now = millis();
  nextBlink = now + 2200;
  nextMouthFrame = now;
  drawBear(false, false);
}

void loop() {
  const uint32_t now = millis();
  int x, y;
  if (readTap(x, y)) {
    if (x >= 235 && y <= 45) {
      language = language == DANISH ? ENGLISH : DANISH;
    } else {
      greetingUntil = now + 1400;
      nextMouthFrame = now;
      mouthOpen = false;
    }
    drawBear(false, false);
  }

  bool blink = now < blinkUntil;
  if (!blink && now >= nextBlink) {
    blinkUntil = now + 160;
    nextBlink = now + random(1800, 4400);
    blink = true;
  }

  bool speaking = now < greetingUntil;
  if (speaking && now >= nextMouthFrame) {
    mouthOpen = !mouthOpen;
    nextMouthFrame = now + 150;
  } else if (!speaking) {
    mouthOpen = false;
  }

  static bool lastBlink = false;
  static bool lastMouth = false;
  static bool lastSpeaking = false;
  if (blink != lastBlink || mouthOpen != lastMouth || speaking != lastSpeaking) {
    drawBear(blink, mouthOpen);
    lastBlink = blink;
    lastMouth = mouthOpen;
    lastSpeaking = speaking;
  }
  delay(8);
}
