#include <Arduino.h>
#include <OneButton.h>
#include <Adafruit_NeoPixel.h>
// #include <TM1637Display.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// DEFINES
////////////////////////////////////////////////////////////////////////////////////////////////////

#define PIN_BTN_R 2 // RED
#define PIN_BTN_G 3 // GREEN
#define PIN_BTN_B 4 // BLUE

// #define DISPLAY_DIO 6
// #define DISPLAY_CLK 7
// #define DISPLAY_PIN_POT A0
// #define SHOW_DOTS 0b01000000

#define LED_DIN 8
#define LED_QTD 30

////////////////////////////////////////////////////////////////////////////////////////////////////
// CONSTANTS
////////////////////////////////////////////////////////////////////////////////////////////////////

OneButton BtnR = OneButton(PIN_BTN_R, true);
OneButton BtnG = OneButton(PIN_BTN_G, true);
OneButton BtnB = OneButton(PIN_BTN_B, true);

const int BTN_HOLD_DURATION_TO_TRIGGER_LONG_PRESS = 400;

int COLOR_WHITE[3] = { 255, 255, 255 };
int COLOR_RED[3] = { 255, 0, 0 };
int COLOR_GREEN[3] = { 0, 255, 0 };
int COLOR_BLUE[3] = { 0, 0, 255 };

const int STATE_CONFIG_CAPTURE_TIME = 0;
const int STATE_CONFIG_DEFENSE_TIME = 1;
const int STATE_GAME_STARTED = 2;
const int STATE_GAME_FINISHED = 3;

const int CAPTURE_TIME_STEP = 5;
const int CAPTURE_TIME_MIN_VALUE = 5;
const int CAPTURE_TIME_MAX_VALUE = 20;

const int DEFENSE_TIME_STEP = 10;
const int DEFENSE_TIME_MIN_VALUE = 10;
const int DEFENSE_TIME_MAX_VALUE = 600;

const int TEAM_RED = 0;
const int TEAM_GREEN = 1;
const int TEAM_BLUE = 2;
const int TEAM_NULL = 3;

int TEAM_COLOR[4][3] = {
  { COLOR_RED[0], COLOR_RED[1], COLOR_RED[2] },
  { COLOR_GREEN[0], COLOR_GREEN[1], COLOR_GREEN[2] },
  { COLOR_BLUE[0], COLOR_BLUE[1], COLOR_BLUE[2] },
  { COLOR_WHITE[0], COLOR_WHITE[1], COLOR_WHITE[2] },
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// GAME VARIABLES
////////////////////////////////////////////////////////////////////////////////////////////////////

int GAME_STATE = STATE_CONFIG_CAPTURE_TIME;
int GAME_CAPTURE_TIME = CAPTURE_TIME_MIN_VALUE;
int GAME_DEFENSE_TIME = DEFENSE_TIME_MIN_VALUE;

int GAME_TEAM = TEAM_NULL;
long GAME_STARTED_AT = 0;
long GAME_TEAM_TIME[3] = { 0, 0, 0 };

////////////////////////////////////////////////////////////////////////////////////////////////////
// INITIALIZERS
////////////////////////////////////////////////////////////////////////////////////////////////////

Adafruit_NeoPixel leds(LED_QTD, LED_DIN, NEO_GRB + NEO_KHZ800);
// TM1637Display display(DISPLAY_CLK, DISPLAY_DIO);

////////////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTION DECLARATIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

long getBtnPressedMs(OneButton btn);

void handleClickBtnR();
void handleDuringLongPressBtnR();
void handleLongPressStopBtnR();

void handleClickBtnG();
void handleDuringLongPressBtnG();
void handleLongPressStopBtnG();

void handleClickBtnB();
void handleDuringLongPressBtnB();
void handleLongPressStopBtnB();

void handleDuringLongPressCaptureTeam(int pressedTime, int team);

int transformToDisplayValue(int secondsOrMinutes);

void turnOnLed(int index, int color[3]);
void turnOnLedProgress(long progress, long total, int color[3]);
void turnOnLedIddle(int color[3]);

////////////////////////////////////////////////////////////////////////////////////////////////////
// ARDUINO
////////////////////////////////////////////////////////////////////////////////////////////////////

void setup() {
  Serial.begin(9600);

  BtnR.setPressMs(BTN_HOLD_DURATION_TO_TRIGGER_LONG_PRESS);
  BtnR.attachClick(handleClickBtnR);
  BtnR.attachDuringLongPress(handleDuringLongPressBtnR);
  BtnR.attachLongPressStop(handleLongPressStopBtnR);

  BtnG.setPressMs(BTN_HOLD_DURATION_TO_TRIGGER_LONG_PRESS);
  BtnG.attachClick(handleClickBtnG);
  BtnG.attachDuringLongPress(handleDuringLongPressBtnG);
  BtnG.attachLongPressStop(handleLongPressStopBtnG);

  BtnB.setPressMs(BTN_HOLD_DURATION_TO_TRIGGER_LONG_PRESS);
  BtnB.attachClick(handleClickBtnB);
  BtnB.attachDuringLongPress(handleDuringLongPressBtnB);
  BtnB.attachLongPressStop(handleLongPressStopBtnB);

  leds.begin();
  // display.setBrightness(0x0f);
}

void loop() {
  leds.clear();

  if (GAME_STATE == STATE_GAME_STARTED) {
    turnOnLedIddle(TEAM_COLOR[GAME_TEAM]);

    Serial.print("R: ");
    Serial.print(GAME_TEAM_TIME[TEAM_RED]);
    Serial.print(", G: ");
    Serial.print(GAME_TEAM_TIME[TEAM_GREEN]);
    Serial.print(", B: ");
    Serial.println(GAME_TEAM_TIME[TEAM_BLUE]);
  }

  BtnR.tick();
  BtnG.tick();
  BtnB.tick();
  
  leds.show();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// ACTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

void handleDuringLongPressCaptureTeam(int pressedTime, int team) {
  if (GAME_STATE == STATE_GAME_STARTED && GAME_TEAM != team) {
    if (pressedTime <= GAME_CAPTURE_TIME * 1000) {
      turnOnLedProgress(pressedTime, GAME_CAPTURE_TIME * 1000, TEAM_COLOR[team]);
    } else {      
      GAME_TEAM = team;
      if (GAME_STARTED_AT == 0) {
        GAME_STARTED_AT = millis();
        GAME_TEAM_TIME[TEAM_RED] = 0;
        GAME_TEAM_TIME[TEAM_GREEN] = 0;
        GAME_TEAM_TIME[TEAM_BLUE] = 0;
      }
    }
  }
}

//--------------------------------------------------------------------------------------------------
// BTN RED
//--------------------------------------------------------------------------------------------------

void handleClickBtnR() {}

void handleDuringLongPressBtnR() {
  long pressedTime = getBtnPressedMs(BtnR);
  handleDuringLongPressCaptureTeam(pressedTime, TEAM_RED);
}

void handleLongPressStopBtnR() {}

//--------------------------------------------------------------------------------------------------
// BTN GREEN
//--------------------------------------------------------------------------------------------------

void handleClickBtnG() {
  if (GAME_STATE == STATE_CONFIG_CAPTURE_TIME) {
    GAME_CAPTURE_TIME = min(GAME_CAPTURE_TIME + CAPTURE_TIME_STEP, CAPTURE_TIME_MAX_VALUE);
  }
  if (GAME_STATE == STATE_CONFIG_DEFENSE_TIME) {
    GAME_DEFENSE_TIME = min(GAME_DEFENSE_TIME + DEFENSE_TIME_STEP, DEFENSE_TIME_MAX_VALUE);
  }
}

void handleDuringLongPressBtnG() {
  long pressedTime = getBtnPressedMs(BtnG);

  if (GAME_STATE == STATE_CONFIG_CAPTURE_TIME || GAME_STATE == STATE_CONFIG_DEFENSE_TIME) {
    if (pressedTime > 1000) return;
    turnOnLedProgress(pressedTime, 1000, COLOR_WHITE);
  }
  handleDuringLongPressCaptureTeam(pressedTime, TEAM_GREEN);
}

void handleLongPressStopBtnG() {
  if (GAME_STATE == STATE_CONFIG_CAPTURE_TIME && getBtnPressedMs(BtnG) >= 1000) {
    GAME_STATE = STATE_CONFIG_DEFENSE_TIME;
    return;
  }
  if (GAME_STATE == STATE_CONFIG_DEFENSE_TIME && getBtnPressedMs(BtnG) >= 1000) {
    GAME_STATE = STATE_GAME_STARTED;
    return;
  }
}

//--------------------------------------------------------------------------------------------------
// BTN BLUE
//--------------------------------------------------------------------------------------------------

void handleClickBtnB() {
  if (GAME_STATE == STATE_CONFIG_CAPTURE_TIME) {
    GAME_CAPTURE_TIME = max(GAME_CAPTURE_TIME - CAPTURE_TIME_STEP, CAPTURE_TIME_MIN_VALUE);
  }
  if (GAME_STATE == STATE_CONFIG_DEFENSE_TIME) {
    GAME_DEFENSE_TIME = max(GAME_DEFENSE_TIME - DEFENSE_TIME_STEP, DEFENSE_TIME_MIN_VALUE);
  }
}

void handleDuringLongPressBtnB() {
  long pressedTime = getBtnPressedMs(BtnB);

  if (GAME_STATE == STATE_CONFIG_DEFENSE_TIME) {
    if (pressedTime > 1000) return;
    turnOnLedProgress(pressedTime, 1000, COLOR_WHITE);
  }
  handleDuringLongPressCaptureTeam(pressedTime, TEAM_BLUE);
}

void handleLongPressStopBtnB() {
  if (GAME_STATE == STATE_CONFIG_DEFENSE_TIME && getBtnPressedMs(BtnG) >= 1000) {
    GAME_STATE = STATE_CONFIG_CAPTURE_TIME;
    return;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

long getBtnPressedMs(OneButton btn) {
  return btn.getPressedMs() - BTN_HOLD_DURATION_TO_TRIGGER_LONG_PRESS;
}

int transformToDisplayValue(int secondsOrMinutes) {
  return (secondsOrMinutes / 60 * 100) + (secondsOrMinutes % 60);
}

void turnOnLed(int index, int color[3]) {
  leds.setPixelColor(index, leds.Color(color[0], color[1], color[2]));
}

void turnOnLedProgress(long progress, long total, int color[3]) {
  leds.clear();
  
  long qtdLEDs = (LED_QTD * progress) / total;
  for (int i = 0; i < qtdLEDs; i += 1) {
    turnOnLed(i, color);
  }
}

void turnOnLedIddle(int color[3]) {
  int ms = millis() % 1000;
  int index = (LED_QTD * ms) / 1000;
  turnOnLed(index, color);

  int nextIndex = index + (LED_QTD / 2);
  if (nextIndex > LED_QTD) {
    nextIndex -= LED_QTD;
  }
  turnOnLed(nextIndex, color);
}