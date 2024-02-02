#include <Arduino.h>
#include <OneButton.h>
#include <Adafruit_NeoPixel.h>
#include <TM1637Display.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// DEFINES
////////////////////////////////////////////////////////////////////////////////////////////////////

#define PIN_BTN_R 2 // RED
#define PIN_BTN_G 3 // GREEN
#define PIN_BTN_B 4 // BLUE

#define PIN_ALARM 5

#define PIN_DISPLAY_DIO 6
#define PIN_DISPLAY_CLK 7
#define SHOW_DOTS 0b01000000

#define LED_DIN 8
#define LED_QTD 30

#define PIN_BUZZER 9

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

const long TIME_SECOND = 1000;
const long TIME_MINUTE = 60 * TIME_SECOND;
const long TIME_HOUR = 60 * TIME_MINUTE;

const long CAPTURE_TIME_STEP = 5 * TIME_SECOND;
const long CAPTURE_TIME_MIN_VALUE = 5 * TIME_SECOND;
const long CAPTURE_TIME_MAX_VALUE = TIME_MINUTE;

const long DEFENSE_TIME_STEP = 10 * TIME_MINUTE;
const long DEFENSE_TIME_MIN_VALUE = 1 * TIME_MINUTE;
const long DEFENSE_TIME_MAX_VALUE = 5 * TIME_HOUR;

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
long GAME_CAPTURE_TIME = CAPTURE_TIME_MIN_VALUE;
long GAME_DEFENSE_TIME = DEFENSE_TIME_MIN_VALUE;

int GAME_TEAM = TEAM_NULL;
bool GAME_ALARM = false;
long GAME_STARTED_AT = 0;
long GAME_TIMER_STARTED_AT = 0;
long GAME_TEAM_TIME[3] = { 0, 0, 0 };

////////////////////////////////////////////////////////////////////////////////////////////////////
// INITIALIZERS
////////////////////////////////////////////////////////////////////////////////////////////////////

Adafruit_NeoPixel leds(LED_QTD, LED_DIN, NEO_GRB + NEO_KHZ800);
TM1637Display display(PIN_DISPLAY_CLK, PIN_DISPLAY_DIO);

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
void handleDuringLongPressRestartGame(int pressedTime);

void buzzerFeedback(int feedbackCount);
void changeStateWithFeedback(int newState, int feedbackCount);

int getDisplayValueFromMs(long ms);

void turnOnLed(int index, int color[3]);
void turnOnLedProgress(long progress, long total, int color[3]);
void turnOnLedIddle(int color[3]);
void tuenOnLedFlashing(int color[3]);

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

  pinMode(PIN_ALARM, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  display.setBrightness(0x0f);

  leds.begin();

  buzzerFeedback(1);
}

void loop() {
  leds.clear();

  int showTime = 0;
  switch (GAME_STATE)
  {
    case STATE_CONFIG_CAPTURE_TIME:
      showTime = getDisplayValueFromMs(GAME_CAPTURE_TIME);
      break;
    
    case STATE_CONFIG_DEFENSE_TIME:
      showTime = getDisplayValueFromMs(GAME_DEFENSE_TIME);
      break;
    
    case STATE_GAME_STARTED:
      turnOnLedIddle(TEAM_COLOR[GAME_TEAM]);
      if (GAME_TEAM != TEAM_NULL) {
        long currentTime = millis() - GAME_TIMER_STARTED_AT;
        showTime = getDisplayValueFromMs(GAME_DEFENSE_TIME - (GAME_TEAM_TIME[GAME_TEAM] + currentTime));

        if (GAME_TEAM_TIME[GAME_TEAM] + currentTime >= GAME_DEFENSE_TIME) {
          GAME_TEAM_TIME[GAME_TEAM] = GAME_DEFENSE_TIME;
          GAME_STATE = STATE_GAME_FINISHED;
          GAME_ALARM = true;
        }
      }
      break;

    case STATE_GAME_FINISHED:
      tuenOnLedFlashing(TEAM_COLOR[GAME_TEAM]);
      digitalWrite(PIN_ALARM, GAME_ALARM ? HIGH : LOW);
      showTime = getDisplayValueFromMs(GAME_DEFENSE_TIME - GAME_TEAM_TIME[GAME_TEAM]);
      break;
  }

  BtnR.tick();
  BtnG.tick();
  BtnB.tick();
  
  leds.show();
  display.showNumberDecEx(showTime, SHOW_DOTS, true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// ACTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

void handleDuringLongPressCaptureTeam(int pressedTime, int team) {
  if (GAME_STATE == STATE_GAME_STARTED && GAME_TEAM != team) {
    if (pressedTime <= GAME_CAPTURE_TIME) {
      turnOnLedProgress(pressedTime, GAME_CAPTURE_TIME, TEAM_COLOR[team]);
    } else {      
      if (GAME_STARTED_AT == 0) {
        GAME_STARTED_AT = millis();
      }

      if (GAME_TEAM != TEAM_NULL) {
        GAME_TEAM_TIME[GAME_TEAM] += millis() - GAME_TIMER_STARTED_AT;
      }

      GAME_TEAM = team;
      GAME_TIMER_STARTED_AT = millis();

      buzzerFeedback(3);
    }
  }
}

void handleDuringLongPressRestartGame(int pressedTime) {
  if (GAME_STATE != STATE_GAME_FINISHED) return;

  if (GAME_ALARM || pressedTime >= (1 * TIME_SECOND)) {
    buzzerFeedback(2);
    GAME_ALARM = false;
  }

  if (BtnR.state() == 6 && BtnG.state() == 6 && BtnB.state() == 6) {
    GAME_TEAM = TEAM_NULL;
    GAME_ALARM = false;
    GAME_STARTED_AT = 0;
    GAME_TIMER_STARTED_AT = 0;
    GAME_TEAM_TIME[0] = 0;
    GAME_TEAM_TIME[1] = 0;
    GAME_TEAM_TIME[2] = 0;

    // GAME_CAPTURE_TIME = CAPTURE_TIME_MIN_VALUE;
    // GAME_DEFENSE_TIME = DEFENSE_TIME_MIN_VALUE;
    GAME_STATE = STATE_CONFIG_CAPTURE_TIME;
  }
}

//--------------------------------------------------------------------------------------------------
// BTN RED
//--------------------------------------------------------------------------------------------------

void handleClickBtnR() {
  if (GAME_STATE == STATE_GAME_FINISHED) {
    buzzerFeedback(1);
    GAME_TEAM = TEAM_RED;
  }
}

void handleDuringLongPressBtnR() {
  long pressedTime = getBtnPressedMs(BtnR);
  handleDuringLongPressCaptureTeam(pressedTime, TEAM_RED);
  handleDuringLongPressRestartGame(pressedTime);
}

void handleLongPressStopBtnR() {
}

//--------------------------------------------------------------------------------------------------
// BTN GREEN
//--------------------------------------------------------------------------------------------------

void handleClickBtnG() {
  if (GAME_STATE == STATE_CONFIG_CAPTURE_TIME) {
    buzzerFeedback(1);
    GAME_CAPTURE_TIME = min(GAME_CAPTURE_TIME + CAPTURE_TIME_STEP, CAPTURE_TIME_MAX_VALUE);
  }
  if (GAME_STATE == STATE_CONFIG_DEFENSE_TIME) {
    buzzerFeedback(1);
    GAME_DEFENSE_TIME = min(GAME_DEFENSE_TIME + DEFENSE_TIME_STEP, DEFENSE_TIME_MAX_VALUE);
  }
  if (GAME_STATE == STATE_GAME_FINISHED) {
    buzzerFeedback(1);
    GAME_TEAM = TEAM_GREEN;
  }
}

void handleDuringLongPressBtnG() {
  long pressedTime = getBtnPressedMs(BtnG);

  if (GAME_STATE == STATE_CONFIG_CAPTURE_TIME || GAME_STATE == STATE_CONFIG_DEFENSE_TIME) {
    if (pressedTime > TIME_SECOND) return;
    turnOnLedProgress(pressedTime, TIME_SECOND, COLOR_WHITE);
  }
  handleDuringLongPressCaptureTeam(pressedTime, TEAM_GREEN);
  handleDuringLongPressRestartGame(pressedTime);
}

void handleLongPressStopBtnG() {
  if (GAME_STATE == STATE_CONFIG_CAPTURE_TIME && getBtnPressedMs(BtnG) >= TIME_SECOND) {
    changeStateWithFeedback(STATE_CONFIG_DEFENSE_TIME, 2);
    return;
  }
  if (GAME_STATE == STATE_CONFIG_DEFENSE_TIME && getBtnPressedMs(BtnG) >= TIME_SECOND) {
    changeStateWithFeedback(STATE_GAME_STARTED, 3);
    return;
  }
}

//--------------------------------------------------------------------------------------------------
// BTN BLUE
//--------------------------------------------------------------------------------------------------

void handleClickBtnB() {
  if (GAME_STATE == STATE_CONFIG_CAPTURE_TIME) {
    buzzerFeedback(1);
    GAME_CAPTURE_TIME = max(GAME_CAPTURE_TIME - CAPTURE_TIME_STEP, CAPTURE_TIME_MIN_VALUE);
  }
  if (GAME_STATE == STATE_CONFIG_DEFENSE_TIME) {
    buzzerFeedback(1);
    GAME_DEFENSE_TIME = max(GAME_DEFENSE_TIME - DEFENSE_TIME_STEP, DEFENSE_TIME_MIN_VALUE);
  }
  if (GAME_STATE == STATE_GAME_FINISHED) {
    buzzerFeedback(1);
    GAME_TEAM = TEAM_BLUE;
  }
}

void handleDuringLongPressBtnB() {
  long pressedTime = getBtnPressedMs(BtnB);

  if (GAME_STATE == STATE_CONFIG_DEFENSE_TIME) {
    if (pressedTime > TIME_SECOND) return;
    turnOnLedProgress(pressedTime, TIME_SECOND, COLOR_WHITE);
  }
  handleDuringLongPressCaptureTeam(pressedTime, TEAM_BLUE);
  handleDuringLongPressRestartGame(pressedTime);
}

void handleLongPressStopBtnB() {
  if (GAME_STATE == STATE_CONFIG_DEFENSE_TIME && getBtnPressedMs(BtnB) >= TIME_SECOND) {
    changeStateWithFeedback(STATE_CONFIG_CAPTURE_TIME, 2);
    return;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

long getBtnPressedMs(OneButton btn) {
  return btn.getPressedMs() - BTN_HOLD_DURATION_TO_TRIGGER_LONG_PRESS;
}

void buzzerFeedback(int feedbackCount) {
  while (feedbackCount > 0) {
    feedbackCount -= 1;
    digitalWrite(PIN_BUZZER, HIGH);
    delay(64);
    digitalWrite(PIN_BUZZER, LOW);
    delay(64);
  }
}

void changeStateWithFeedback(int newState, int feedbackCount) {
  buzzerFeedback(feedbackCount);
  GAME_STATE = newState;
}

int getDisplayValueFromMs(long ms) {
  int hours = ms / TIME_HOUR;
  ms %= TIME_HOUR;
  int minutes = ms / TIME_MINUTE;
  ms %= TIME_MINUTE;
  int seconds = ms / TIME_SECOND;

  return hours > 0 ? (hours * 100) + minutes : (minutes * 100) + seconds;
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
  int ms = millis() % TIME_SECOND;
  int index = (LED_QTD * ms) / TIME_SECOND;
  turnOnLed(index, color);

  int nextIndex = index + (LED_QTD / 2);
  if (nextIndex > LED_QTD) {
    nextIndex -= LED_QTD;
  }
  turnOnLed(nextIndex, color);
}

void tuenOnLedFlashing(int color[3]) {
  int ms = millis() % TIME_SECOND;
  if (ms < (TIME_SECOND / 2)) return;
  for (int i = 0; i < LED_QTD; i += 1) {
    turnOnLed(i, color);
  }
}
