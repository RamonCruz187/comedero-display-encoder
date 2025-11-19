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

// Variables para control de dispensado
unsigned long dispensandoStartTime = 0;
bool dispensandoActivo = false;
int porcionActual = 0;
bool dispensadoAutomatico = false;

// Variables para control de horarios automáticos
int ultimoHorarioActivado = -1;
int ultimoMinutoActivado = -1;

// Variables para configuración de fecha/hora
int tempAnio = 2024;
int tempMes = 1;
int tempDia = 1;
int tempHoraRTC = 0;
int tempMinutoRTC = 0;
int editModeRTC = EDIT_NONE;
bool seleccionGuardar = true;  // true = GUARDAR, false = SALIR

// Declaraciones de funciones
void updateEncoder();
void handleButtonPress();
void updateDisplay();
void guardarHorarioEnPreferences();
void cargarHorarioDesdePreferences();
void handleEncoderChanges();
void iniciarDispensado();
void iniciarDispensadoAutomatico(int porcion);
void actualizarDispensado();
void revisarHorariosAutomaticos();
void cargarFechaHoraActual();
void guardarFechaHoraRTC();

void setup() {
  Serial.begin(115200);
  
  Wire.begin(SDA_PIN, SCL_PIN);

  // Configurar pin del motor
  pinMode(MOTOR_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, LOW);

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
  
  // Solo procesar botón si no está dispensando
  if (!dispensandoActivo) {
    if (currentButtonState == LOW && lastButtonState == HIGH) {
      if (millis() - lastButtonTime > 300) {
        buttonPressed = true;
        lastButtonTime = millis();
      }
    }
  }
  lastButtonState = currentButtonState;

  // Solo procesar encoder si no está dispensando
  static int lastEncoderPos = 0;
  static int accumulatedChange = 0;
  static unsigned long lastChangeTime = 0;
  
  if (!dispensandoActivo && encoderPos != lastEncoderPos && editMode == EDIT_NONE && editModeRTC == EDIT_NONE) {
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
        case CONFIG_FECHA_HORA_SCREEN:
          selectedOption = constrain(selectedOption + changeToApply, 0, 5);
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

  // Solo procesar botón si no está dispensando
  if (buttonPressed && !dispensandoActivo) {
    handleButtonPress();
    buttonPressed = false;
  }

  // Revisar horarios automáticos (siempre, incluso durante dispensado)
  revisarHorariosAutomaticos();

  // Control del dispensado
  if (dispensandoActivo) {
    actualizarDispensado();
  }

  handleEncoderChanges();
  updateDisplay();
  delay(50);
}

void revisarHorariosAutomaticos() {
  DateTime now = rtc.now();
  int horaActual = now.hour();
  int minutoActual = now.minute();

  // Solo revisar si no estamos ya dispensando
  if (!dispensandoActivo) {
    for (int i = 1; i <= 4; i++) {
      bool habilitado;
      int hora, minuto, porcion;
      
      if (obtenerHorario(i, habilitado, hora, minuto, porcion)) {
        if (habilitado && hora == horaActual && minuto == minutoActual) {
          // Evitar múltiples activaciones en el mismo minuto
          if (ultimoHorarioActivado != i || ultimoMinutoActivado != minutoActual) {
            Serial.println("Horario automático activado: H" + String(i));
            iniciarDispensadoAutomatico(porcion);
            ultimoHorarioActivado = i;
            ultimoMinutoActivado = minutoActual;
            break; // Solo un horario a la vez
          }
        }
      }
    }
  }
}

void handleButtonPress() {
  switch (currentScreen) {
    case MAIN_SCREEN:
      if (selectedOption >= 0 && selectedOption <= 3) {
        switch (selectedOption) {
          case 0: // DISPENSAR AHORA
            iniciarDispensado();
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
      switch (selectedOption) {
        case 0: // CONF. FECHA/HORA
          cargarFechaHoraActual();
          currentScreen = CONFIG_FECHA_HORA_SCREEN;
          selectedOption = 0;
          encoderPos = 0;
          editModeRTC = EDIT_NONE;
          seleccionGuardar = true; // Por defecto seleccionar GUARDAR
          break;
        case 1: // VER PLATO
          // Aquí iría la funcionalidad de ver plato (para futuro)
          break;
        case 2: // ATRAS
          currentScreen = MAIN_SCREEN;
          selectedOption = -1;
          encoderPos = -1;
          break;
      }
      break;

    case CONFIG_FECHA_HORA_SCREEN:
      switch (selectedOption) {
        case 0: // AÑO
          if (editModeRTC == EDIT_NONE) {
            editModeRTC = EDIT_ANIO;
          } else if (editModeRTC == EDIT_ANIO) {
            editModeRTC = EDIT_NONE;
          }
          break;
        case 1: // MES
          if (editModeRTC == EDIT_NONE) {
            editModeRTC = EDIT_MES;
          } else if (editModeRTC == EDIT_MES) {
            editModeRTC = EDIT_NONE;
          }
          break;
        case 2: // DIA
          if (editModeRTC == EDIT_NONE) {
            editModeRTC = EDIT_DIA;
          } else if (editModeRTC == EDIT_DIA) {
            editModeRTC = EDIT_NONE;
          }
          break;
        case 3: // HORA
          if (editModeRTC == EDIT_NONE) {
            editModeRTC = EDIT_HORA_RTC;
          } else if (editModeRTC == EDIT_HORA_RTC) {
            editModeRTC = EDIT_NONE;
          }
          break;
        case 4: // MINUTO
          if (editModeRTC == EDIT_NONE) {
            editModeRTC = EDIT_MINUTO_RTC;
          } else if (editModeRTC == EDIT_MINUTO_RTC) {
            editModeRTC = EDIT_NONE;
          }
          break;
        case 5: // GUARDAR | SALIR
          if (editModeRTC == EDIT_NONE) {
            // Entrar en modo selección GUARDAR/SALIR
            editModeRTC = EDIT_GUARDAR_SALIR;
          } else if (editModeRTC == EDIT_GUARDAR_SALIR) {
            // Ejecutar la acción seleccionada
            if (seleccionGuardar) {
              guardarFechaHoraRTC();
            }
            // Salir en ambos casos
            currentScreen = CONFIG_SCREEN;
            selectedOption = 0;
            encoderPos = 0;
            editModeRTC = EDIT_NONE;
          }
          break;
      }
      break;

    case SUCCESS_SCREEN:
      // Después de mostrar "comida dispensada", volver al menú principal
      currentScreen = MAIN_SCREEN;
      selectedOption = -1;
      encoderPos = -1;
      dispensandoActivo = false;
      dispensadoAutomatico = false;
      break;
  }
}

void iniciarDispensado() {
  dispensandoActivo = true;
  dispensandoStartTime = millis();
  porcionActual = 6; // Porción fija para dispensado manual (3 segundos)
  dispensadoAutomatico = false;
  currentScreen = DISPENSANDO_SCREEN;
  
  // Activar el motor
  digitalWrite(MOTOR_PIN, HIGH);
  Serial.println("Dispensado manual iniciado - Motor ACTIVADO");
}

void iniciarDispensadoAutomatico(int porcion) {
  dispensandoActivo = true;
  dispensandoStartTime = millis();
  porcionActual = porcion;
  dispensadoAutomatico = true;
  currentScreen = DISPENSANDO_SCREEN;
  
  // Activar el motor
  digitalWrite(MOTOR_PIN, HIGH);
  Serial.println("Dispensado automático iniciado - Motor ACTIVADO");
  Serial.println("Porción: " + String(porcion) + " -> Tiempo: " + String(porcion * 500) + " ms");
}

void actualizarDispensado() {
  unsigned long tiempoActual = millis();
  unsigned long tiempoTranscurrido = tiempoActual - dispensandoStartTime;
  unsigned long tiempoDispensado = porcionActual * 500; // porcion/2 segundos (en milisegundos)

  if (tiempoTranscurrido >= tiempoDispensado && currentScreen == DISPENSANDO_SCREEN) {
    // Terminar dispensado
    digitalWrite(MOTOR_PIN, LOW);
    Serial.println("Dispensado completado - Motor DESACTIVADO");
    
    // Cambiar a pantalla de éxito
    currentScreen = SUCCESS_SCREEN;
    dispensandoStartTime = tiempoActual; // Reiniciar contador para pantalla de éxito
  }
  else if (tiempoTranscurrido >= 3000 && currentScreen == SUCCESS_SCREEN) {
    // Después de 3 segundos en pantalla de éxito, volver al menú principal
    currentScreen = MAIN_SCREEN;
    selectedOption = -1;
    encoderPos = -1;
    dispensandoActivo = false;
    dispensadoAutomatico = false;
  }
}

void cargarFechaHoraActual() {
  DateTime now = rtc.now();
  tempAnio = now.year();
  tempMes = now.month();
  tempDia = now.day();
  tempHoraRTC = now.hour();
  tempMinutoRTC = now.minute();
}

void guardarFechaHoraRTC() {
  rtc.adjust(DateTime(tempAnio, tempMes, tempDia, tempHoraRTC, tempMinutoRTC, 0));
  Serial.println("Fecha y hora actualizadas en RTC:");
  Serial.println(String(tempDia) + "/" + String(tempMes) + "/" + String(tempAnio) + " " + 
                 String(tempHoraRTC) + ":" + String(tempMinutoRTC));
}

void IRAM_ATTR updateEncoder() {
  // Bloquear encoder durante dispensado
  if (dispensandoActivo) {
    return;
  }
  
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