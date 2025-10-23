#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <RTClib.h>
#include "config.h"

// Declaración de variables externas
extern Adafruit_SH1106G display;
extern RTC_DS1307 rtc;
extern int currentScreen;
extern int selectedOption;
extern bool buttonPressed;
extern bool lastButtonState;
extern volatile int encoderPos;
extern volatile unsigned long lastEncoderTime;
extern volatile int lastCLK;
extern volatile int lastDT;
extern int horarioSeleccionado;
extern bool tempHabilitado;
extern int tempHora;
extern int tempMinuto;
extern int tempPorcion;
extern int editMode;
extern unsigned long lastBlinkTime;
extern bool blinkState;

// Declaraciones de funciones
void updateEncoder();
void handleButtonPress();
void updateDisplay();
void guardarHorarioEnPreferences();
void cargarHorarioDesdePreferences();
void handleEncoderChanges();

void setup() {
  Serial.begin(115200);
  
  Wire.begin(SDA_PIN, SCL_PIN);

  if (!display.begin(0x3C, true)) {
    Serial.println("No se encontró SH1106");
    while (1);
  }

  if (!rtc.begin()) {
    Serial.println("No se encontró RTC");
  }

  if (!rtc.isrunning()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  pinMode(ENCODER_CLK, INPUT_PULLUP);
  pinMode(ENCODER_DT, INPUT_PULLUP);
  pinMode(ENCODER_SW, INPUT_PULLUP);
  
  lastCLK = digitalRead(ENCODER_CLK);
  lastDT = digitalRead(ENCODER_DT);
  
  attachInterrupt(digitalPinToInterrupt(ENCODER_CLK), updateEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_DT), updateEncoder, CHANGE);

  selectedOption = -1;
  encoderPos = -1;
  currentScreen = MAIN_SCREEN;

  display.clearDisplay();
  display.display();
}

void loop() {
  bool currentButtonState = digitalRead(ENCODER_SW);
  static unsigned long lastButtonTime = 0;
  
  if (currentButtonState == LOW && lastButtonState == HIGH) {
    if (millis() - lastButtonTime > 300) {
      buttonPressed = true;
      lastButtonTime = millis();
    }
  }
  lastButtonState = currentButtonState;

  static int lastEncoderPos = 0;
  static int accumulatedChange = 0;
  static unsigned long lastChangeTime = 0;
  
  if (encoderPos != lastEncoderPos && editMode == EDIT_NONE) {
    int totalChange = encoderPos - lastEncoderPos;
    accumulatedChange += totalChange;
    
    unsigned long currentTime = millis();
    bool timeForChange = (currentTime - lastChangeTime > 200);
    
    if (abs(accumulatedChange) >= 2 || (abs(accumulatedChange) >= 1 && timeForChange)) {
      int changeToApply = accumulatedChange > 0 ? 1 : -1;
      
      int oldSelection = selectedOption;
      switch (currentScreen) {
        case MAIN_SCREEN:
          selectedOption = constrain(selectedOption + changeToApply, -1, 3);
          break;
        case HORARIOS_SCREEN:
          selectedOption = constrain(selectedOption + changeToApply, 0, 4);
          break;
        case SELECCIONAR_HORARIO_SCREEN:
          selectedOption = constrain(selectedOption + changeToApply, 0, 4);
          break;
        case CONFIG_HORARIO_SCREEN:
          selectedOption = constrain(selectedOption + changeToApply, 0, 4);
          break;
        case CONFIG_SCREEN:
          selectedOption = constrain(selectedOption + changeToApply, 0, 2);
          break;
      }
      
      if (selectedOption != oldSelection) {
        lastChangeTime = currentTime;
      }
      
      if (abs(accumulatedChange) >= 2) {
        accumulatedChange = 0;
      } else {
        accumulatedChange -= changeToApply;
      }
      
      encoderPos = selectedOption;
    }
    
    lastEncoderPos = encoderPos;
  }

  if (buttonPressed) {
    handleButtonPress();
    buttonPressed = false;
  }

  handleEncoderChanges();
  updateDisplay();
  delay(50);
}

void handleButtonPress() {
  switch (currentScreen) {
    case MAIN_SCREEN:
      if (selectedOption >= 0 && selectedOption <= 3) {
        switch (selectedOption) {
          case 0:
            currentScreen = SUCCESS_SCREEN;
            break;
          case 1:
            currentScreen = HORARIOS_SCREEN;
            selectedOption = 0;
            encoderPos = 0;
            break;
          case 2:
            currentScreen = SELECCIONAR_HORARIO_SCREEN;
            selectedOption = 0;
            encoderPos = 0;
            break;
          case 3:
            currentScreen = CONFIG_SCREEN;
            selectedOption = 0;
            encoderPos = 0;
            break;
        }
      }
      break;

    case HORARIOS_SCREEN:
      if (selectedOption == 4) {
        currentScreen = MAIN_SCREEN;
        selectedOption = -1;
        encoderPos = -1;
      }
      break;

    case SELECCIONAR_HORARIO_SCREEN:
      if (selectedOption >= 0 && selectedOption < 4) {
        horarioSeleccionado = selectedOption + 1;
        cargarHorarioDesdePreferences();
        currentScreen = CONFIG_HORARIO_SCREEN;
        selectedOption = 0;
        encoderPos = 0;
        editMode = EDIT_NONE;
      } else if (selectedOption == 4) {
        currentScreen = MAIN_SCREEN;
        selectedOption = -1;
        encoderPos = -1;
      }
      break;

    case CONFIG_HORARIO_SCREEN:
      switch (selectedOption) {
        case 0: // HABILITAR
          if (editMode == EDIT_NONE) {
            tempHabilitado = !tempHabilitado;
          }
          break;
        case 1: // HORA
          if (editMode == EDIT_NONE) {
            editMode = EDIT_HORA;
          } else if (editMode == EDIT_HORA) {
            editMode = EDIT_NONE;
          }
          break;
        case 2: // MINUTO
          if (editMode == EDIT_NONE) {
            editMode = EDIT_MINUTO;
          } else if (editMode == EDIT_MINUTO) {
            editMode = EDIT_NONE;
          }
          break;
        case 3: // PORCION
          if (editMode == EDIT_NONE) {
            editMode = EDIT_PORCION;
          } else if (editMode == EDIT_PORCION) {
            editMode = EDIT_NONE;
          }
          break;
        case 4: // GUARDAR Y SALIR
          if (editMode == EDIT_NONE) {
            guardarHorarioEnPreferences();
            currentScreen = SELECCIONAR_HORARIO_SCREEN;
            selectedOption = 0;
            encoderPos = 0;
            editMode = EDIT_NONE;
          }
          break;
      }
      break;

    case CONFIG_SCREEN:
      if (selectedOption == 2) {
        currentScreen = MAIN_SCREEN;
        selectedOption = -1;
        encoderPos = -1;
      }
      break;

    case SUCCESS_SCREEN:
      currentScreen = MAIN_SCREEN;
      selectedOption = -1;
      encoderPos = -1;
      break;
  }
}

void IRAM_ATTR updateEncoder() {
  int clkValue = digitalRead(ENCODER_CLK);
  int dtValue = digitalRead(ENCODER_DT);
  
  if (micros() - lastEncoderTime < 1500) {
    return;
  }
  
  if (clkValue != lastCLK || dtValue != lastDT) {
    lastEncoderTime = micros();
    
    if (lastCLK == LOW && clkValue == HIGH) {
      if (dtValue == LOW) {
        encoderPos++;
      } else {
        encoderPos--;
      }
    }
    else if (lastDT == LOW && dtValue == HIGH) {
      if (clkValue == LOW) {
        encoderPos--;
      } else {
        encoderPos++;
      }
    }
    
    lastCLK = clkValue;
    lastDT = dtValue;
  }
}