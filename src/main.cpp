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

static const uint16_t BG       = TFT_BLACK;
static const uint16_t FUR      = 0xBEEF;
static const uint16_t FUR_DARK = 0x6DE4;
static const uint16_t MUZZLE   = 0xFE36;
static const uint16_t INNER    = 0xED32;
static const uint16_t INK      = 0x3104;
static const uint16_t ACCENT   = 0xEB69;
static const uint16_t WHITE    = 0xFFFF;

void drawEye(int x, bool closed) {
  if (closed) {
    tft.drawLine(x - 7, 111, x + 7, 111, INK);
    tft.drawLine(x - 5, 113, x + 5, 113, INK);
  } else {
    tft.fillEllipse(x, 111, 7, 10, INK);
    tft.fillCircle(x - 2, 107, 2, WHITE);
  }
}

// Repaint only the two eye patches. The face fur behind them is a solid color.
void drawEyes(bool closed) {
  tft.fillEllipse(135, 111, 10, 13, FUR);
  tft.fillEllipse(179, 111, 10, 13, FUR);
  drawEye(135, closed);
  drawEye(179, closed);
}

// Repaint only the muzzle patch containing the mouth.
void drawMouth(bool open) {
  tft.fillEllipse(157, 143, 14, 12, MUZZLE);
  if (open) {
    tft.fillEllipse(157, 143, 9, 11, INK);
    tft.fillEllipse(157, 148, 5, 3, INNER);
  } else {
    tft.drawArc(157, 140, 10, 8, 20, 160, INK, MUZZLE);
  }
}

void drawLanguageButton() {
  tft.fillRoundRect(244, 8, 68, 30, 8, WHITE);
  tft.drawRoundRect(244, 8, 68, 30, 8, ACCENT);
  tft.setTextColor(INK, WHITE);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(language == DANISH ? "DA  /  EN" : "EN  /  DA", 278, 23, 2);
}

// The bubble sits above the bear, so showing or clearing it doesn't touch the face.
void drawGreeting(bool visible) {
  const int x = 116, y = 4, w = 82, h = 34;
  tft.fillRoundRect(x, y, w, h, 10, visible ? WHITE : BG);
  if (visible) {
    tft.setTextColor(INK, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(language == DANISH ? "Hej!" : "Hello!", x + w / 2, y + h / 2, 4);
  }
}

// Draw the still parts once at startup.
void drawScene() {
  tft.fillScreen(BG);

  // Soft ground shadow and round teddy body.
  tft.fillEllipse(156, 216, 73, 9, 0xE638);
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

  // Nose is static; eyes and mouth are drawn in their own small regions.
  tft.fillEllipse(157, 123, 9, 7, INK);
  drawEyes(false);
  drawMouth(false);

  // A little red bow tie.
  tft.fillCircle(147, 176, 9, ACCENT);
  tft.fillCircle(167, 176, 9, ACCENT);
  tft.fillCircle(157, 176, 5, FUR_DARK);

  drawLanguageButton();
  tft.setTextColor(WHITE, BG);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(language == DANISH ? "Tryk for at sige hej!" : "Tap to say hello!", 157, 226, 2);
  drawGreeting(false);
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

  touchSPI.begin(TOUCH_CLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
  touch.begin(touchSPI);
  touch.setRotation(1);

  const uint32_t now = millis();
  nextBlink = now + 2200;
  nextMouthFrame = now;
  drawScene();
}

void loop() {
  const uint32_t now = millis();
  int x, y;
  if (readTap(x, y)) {
    if (x >= 235 && y <= 45) {
      language = language == DANISH ? ENGLISH : DANISH;
      drawLanguageButton();
      tft.fillRect(0, 214, W, 26, BG);
      tft.setTextColor(WHITE, BG);
      tft.setTextDatum(MC_DATUM);
      tft.drawString(language == DANISH ? "Tryk for at sige hej!" : "Tap to say hello!", 157, 226, 2);
      if (now < greetingUntil) drawGreeting(true);
    } else {
      greetingUntil = now + 1400;
      nextMouthFrame = now;
    }
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

  if (blink != lastBlink) {
    drawEyes(blink);
    lastBlink = blink;
  }
  if (mouthOpen != lastMouth) {
    drawMouth(mouthOpen);
    lastMouth = mouthOpen;
  }
  if (speaking != lastSpeaking) {
    drawGreeting(speaking);
    lastSpeaking = speaking;
  }
  delay(8);
}
