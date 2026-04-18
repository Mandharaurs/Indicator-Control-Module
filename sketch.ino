#include <Arduino.h>

// ===================== DEFINITIONS =====================
#define LEFT_BTN   2
#define RIGHT_BTN  3
#define LEFT_LED   10
#define RIGHT_LED  9

typedef enum {
  STATE_OFF,
  STATE_LEFT,
  STATE_RIGHT,
  STATE_HAZARD
} State;

// ===================== BSW =====================
int BSW_GPIO_Read(int pin) { return digitalRead(pin); }
void BSW_GPIO_Init(int pin, int mode) { pinMode(pin, mode); }
void BSW_PWM_Write(int pin, int value) { analogWrite(pin, value); }

void BSW_UART_Init() { Serial.begin(9600); }
void BSW_UART_Log(unsigned long time, const char *msg) {
  Serial.print("["); Serial.print(time);
  Serial.print(" ms] "); Serial.println(msg);
}

// Scheduler (100 ms, robust)
unsigned long currentTime = 0;
unsigned long lastTick = 0;

int BSW_Scheduler_100ms() {
  if (millis() - lastTick >= 100) {
    lastTick += 100;   // no drift
    currentTime += 100;
    return 1;
  }
  return 0;
}

// ===================== ASW =====================
State currentState = STATE_OFF;

// Button handling
int leftStable = 0, rightStable = 0;
int leftPressed = 0, rightPressed = 0;
int prevLeftPressed = 0, prevRightPressed = 0;

int leftHold = 0, rightHold = 0;
int leftEventDone = 0, rightEventDone = 0;

// LED control
int ledState = 0;
unsigned long lastBlinkTime = 0;

// ================= BUTTON TASK =================
void ASW_ReadButtons() {

  int rawLeft = (BSW_GPIO_Read(LEFT_BTN) == LOW);
  int rawRight = (BSW_GPIO_Read(RIGHT_BTN) == LOW);

  // Debounce (200 ms)
  leftStable = rawLeft ? leftStable + 1 : 0;
  rightStable = rawRight ? rightStable + 1 : 0;

  leftPressed = (leftStable >= 2);
  rightPressed = (rightStable >= 2);

  // Edge logging (disabled during hazard for cleaner logs)
  if (currentState != STATE_HAZARD) {
    if (leftPressed && !prevLeftPressed)
      BSW_UART_Log(currentTime, "LEFT_BUTTON_PRESSED");

    if (rightPressed && !prevRightPressed)
      BSW_UART_Log(currentTime, "RIGHT_BUTTON_PRESSED");
  }

  // Hold detection (1 sec)
  leftHold = leftPressed ? leftHold + 100 : 0;
  rightHold = rightPressed ? rightHold + 100 : 0;

  // Reset event lock on release
  if (!leftPressed && prevLeftPressed) leftEventDone = 0;
  if (!rightPressed && prevRightPressed) rightEventDone = 0;

  prevLeftPressed = leftPressed;
  prevRightPressed = rightPressed;
}

// ================= STATE CONTROL =================
void ASW_SetState(State newState) {
  currentState = newState;
  ledState = 0;
  lastBlinkTime = currentTime;

  leftHold = 0;
  rightHold = 0;
}

// ================= STATE MACHINE =================
void ASW_StateMachine() {

  // ===== HAZARD ENTRY =====
  if (leftHold >= 1000 && rightHold >= 1000) {
    if (currentState != STATE_HAZARD) {
      BSW_UART_Log(currentTime, "HAZARD_BUTTON_PRESSED");
      BSW_UART_Log(currentTime, "HAZARD_MODE_ON");
      ASW_SetState(STATE_HAZARD);
    }
    return;
  }

  // ===== HAZARD EXIT =====
  if (currentState == STATE_HAZARD) {
    if ((leftHold >= 1000 && !leftEventDone) ||
        (rightHold >= 1000 && !rightEventDone)) {

      BSW_UART_Log(currentTime, "HAZARD_MODE_OFF");
      ASW_SetState(STATE_OFF);

      leftEventDone = 1;
      rightEventDone = 1;
    }
    return;
  }

  // ===== LEFT INDICATOR =====
  if (leftPressed && !rightPressed) {
    if (leftHold >= 1000 && !leftEventDone) {

      if (currentState == STATE_RIGHT) {
        BSW_UART_Log(currentTime, "RIGHT_INDICATOR_OFF");
        BSW_UART_Log(currentTime, "LEFT_INDICATOR_ON");
        ASW_SetState(STATE_LEFT);
      }
      else if (currentState == STATE_LEFT) {
        BSW_UART_Log(currentTime, "LEFT_INDICATOR_OFF");
        ASW_SetState(STATE_OFF);
      }
      else {
        BSW_UART_Log(currentTime, "LEFT_INDICATOR_ON");
        ASW_SetState(STATE_LEFT);
      }

      leftEventDone = 1;
    }
  }

  // ===== RIGHT INDICATOR =====
  else if (rightPressed && !leftPressed) {
    if (rightHold >= 1000 && !rightEventDone) {

      if (currentState == STATE_LEFT) {
        BSW_UART_Log(currentTime, "LEFT_INDICATOR_OFF");
        BSW_UART_Log(currentTime, "RIGHT_INDICATOR_ON");
        ASW_SetState(STATE_RIGHT);
      }
      else if (currentState == STATE_RIGHT) {
        BSW_UART_Log(currentTime, "RIGHT_INDICATOR_OFF");
        ASW_SetState(STATE_OFF);
      }
      else {
        BSW_UART_Log(currentTime, "RIGHT_INDICATOR_ON");
        ASW_SetState(STATE_RIGHT);
      }

      rightEventDone = 1;
    }
  }
}

// ================= LED TASK =================
void ASW_UpdateLEDs() {

  // Blink every 300 ms
  if (currentState != STATE_OFF) {
    if (currentTime - lastBlinkTime >= 300) {
      lastBlinkTime = currentTime;
      ledState = !ledState;

      if (currentState == STATE_LEFT)
        BSW_UART_Log(currentTime, "LEFT_LED_TOGGLE");
      else if (currentState == STATE_RIGHT)
        BSW_UART_Log(currentTime, "RIGHT_LED_TOGGLE");
      else
        BSW_UART_Log(currentTime, "HAZARD_LED_TOGGLE");
    }
  }

  int leftPWM = 0, rightPWM = 0;

  if (currentState == STATE_LEFT)
    leftPWM = ledState ? 255 : 0;
  else if (currentState == STATE_RIGHT)
    rightPWM = ledState ? 255 : 0;
  else if (currentState == STATE_HAZARD) {
    leftPWM = ledState ? 255 : 0;
    rightPWM = ledState ? 255 : 0;
  }

  BSW_PWM_Write(LEFT_LED, leftPWM);
  BSW_PWM_Write(RIGHT_LED, rightPWM);
}

// ================= MAIN =================
void setup() {
  BSW_GPIO_Init(LEFT_BTN, INPUT_PULLUP);
  BSW_GPIO_Init(RIGHT_BTN, INPUT_PULLUP);
  BSW_GPIO_Init(LEFT_LED, OUTPUT);
  BSW_GPIO_Init(RIGHT_LED, OUTPUT);

  BSW_UART_Init();
}

void loop() {
  if (BSW_Scheduler_100ms()) {
    ASW_ReadButtons();
    ASW_StateMachine();
    ASW_UpdateLEDs();
  }
}