#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <RTClib.h>
#include <Preferences.h>
#include "config.h"

// Definición de variables globales
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
RTC_DS1307 rtc;
Preferences prefs;

// Variables de control
int currentScreen = MAIN_SCREEN;
int selectedOption = 0;
bool buttonPressed = false;
bool lastButtonState = HIGH;

// Variables del encoder
volatile int encoderPos = 0;
volatile unsigned long lastEncoderTime = 0;
volatile int lastCLK = 0;
volatile int lastDT = 0;

// Variables para configuración de horarios
int horarioSeleccionado = 1;
bool tempHabilitado = false;
int tempHora = 8;
int tempMinuto = 0;
int tempPorcion = 5;

// Variables para modo edición
int editMode = EDIT_NONE;
unsigned long lastBlinkTime = 0;
bool blinkState = false;

void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);

  switch (currentScreen) {
    case MAIN_SCREEN:
      showMainScreen();
      break;
    case HORARIOS_SCREEN:
      showHorariosScreen();
      break;
    case SELECCIONAR_HORARIO_SCREEN:
      showSeleccionarHorarioScreen();
      break;
    case CONFIG_HORARIO_SCREEN:
      showConfigHorarioScreen();
      break;
    case CONFIG_SCREEN:
      showConfigScreen();
      break;
    case SUCCESS_SCREEN:
      showSuccessScreen();
      break;
  }

  display.display();
}

String obtenerProximaComida() {
  DateTime now = rtc.now();
  int horaActual = now.hour();
  int minutoActual = now.minute();
  int tiempoActual = horaActual * 60 + minutoActual; // Convertir a minutos
  
  String proximaComida = "--:--";
  int menorDiferencia = 24 * 60; // Inicializar con un valor grande (24 horas en minutos)
  
  prefs.begin("comedor", true);
  
  for (int i = 1; i <= 4; i++) {
    String base = "h" + String(i) + "_";
    bool habilitado = prefs.getBool((base + "hab").c_str(), false);
    int hora = prefs.getInt((base + "hora").c_str(), -1);
    int minuto = prefs.getInt((base + "minuto").c_str(), -1);
    
    if (habilitado && hora != -1 && minuto != -1) {
      int tiempoHorario = hora * 60 + minuto;
      int diferencia;
      
      if (tiempoHorario >= tiempoActual) {
        // El horario es hoy
        diferencia = tiempoHorario - tiempoActual;
      } else {
        // El horario es mañana
        diferencia = (24 * 60 - tiempoActual) + tiempoHorario;
      }
      
      // Si encontramos un horario más cercano
      if (diferencia < menorDiferencia && diferencia > 0) {
        menorDiferencia = diferencia;
        proximaComida = (hora < 10 ? "0" : "") + String(hora) + ":" + (minuto < 10 ? "0" : "") + String(minuto);
      }
    }
  }
  
  prefs.end();
  return proximaComida;
}

void showMainScreen() {
  DateTime now = rtc.now();
  
  display.setCursor(0, 0);
  
  if (now.day() < 10) display.print("0");
  display.print(now.day());
  display.print("/");
  if (now.month() < 10) display.print("0");
  display.print(now.month());
  display.print(" ");
  
  if (now.hour() < 10) display.print("0");
  display.print(now.hour());
  display.print(":");
  if (now.minute() < 10) display.print("0");
  display.print(now.minute());

  display.drawLine(0, 10, 128, 10, SH110X_WHITE);

  // 🔹 PRÓXIMA COMIDA
  int proxY = 12;
  if (selectedOption == -1) {
    display.fillRect(0, proxY, 128, 9, SH110X_WHITE);
    display.setTextColor(SH110X_BLACK);
  } else {
    display.setTextColor(SH110X_WHITE);
  }
  
  display.setCursor(2, proxY + 1);
  display.print("PROX. COMIDA: ");
  
  // Mostrar la hora de la próxima comida con espacio
  String proximaComida = obtenerProximaComida();
  display.print(proximaComida);

  display.setTextColor(SH110X_WHITE);

  // 🔹 Opciones del menú
  const char* options[] = {"DISPENSAR AHORA", "VER HORARIOS", "CAMBIAR HORARIOS", "CONFIGURACION"};
  int numOptions = 4;

  for (int i = 0; i < numOptions; i++) {
    int y = 22 + i * 11; 
    if (i == selectedOption) {
      display.fillRect(0, y, 128, 9, SH110X_WHITE);
      display.setTextColor(SH110X_BLACK);
    } else {
      display.setTextColor(SH110X_WHITE);
    }
    display.setCursor(2, y + 1);
    display.println(options[i]);
  }
}

void showHorariosScreen() {
  display.setCursor(0, 0);
  display.println("HORARIOS");

  String options[5];
  
  bool prefsOk = prefs.begin("comedor", true);
  
  if (!prefsOk) {
    for (int i = 0; i < 4; i++) {
      options[i] = "H" + String(i+1) + ": --:-- NO P-";
    }
  } else {
    for (int i = 1; i <= 4; i++) {
      String base = "h" + String(i) + "_";
      bool habilitado = prefs.getBool((base + "hab").c_str(), false);
      int hora = prefs.getInt((base + "hora").c_str(), -1);
      int minuto = prefs.getInt((base + "minuto").c_str(), -1);
      int porcion = prefs.getInt((base + "porcion").c_str(), -1);

      if (hora != -1 && minuto != -1 && porcion != -1) {
        String horaStr = (hora < 10 ? "0" : "") + String(hora);
        String minutoStr = (minuto < 10 ? "0" : "") + String(minuto);
        
        String estado = habilitado ? "SI" : "NO";
        options[i-1] = "H" + String(i) + ": " + horaStr + ":" + minutoStr + " " + estado + " P" + String(porcion);
      } else {
        options[i-1] = "H" + String(i) + ": --:-- NO P-";
      }
    }
    prefs.end();
  }
  
  options[4] = "ATRAS";

  for (int i = 0; i < 5; i++) {
    int y = 10 + i * 10;
    if (i == selectedOption) {
      display.fillRect(0, y, 128, 9, SH110X_WHITE);
      display.setTextColor(SH110X_BLACK);
    } else {
      display.setTextColor(SH110X_WHITE);
    }
    display.setCursor(2, y + 1);
    
    String texto = options[i];
    if (texto.length() > 21) {
      texto = texto.substring(0, 21);
    }
    display.println(texto);
  }
}

void showSeleccionarHorarioScreen() {
  display.setCursor(0, 0);
  display.println("SELEC. HORARIO");

  const char* options[] = {"HORARIO 1", "HORARIO 2", "HORARIO 3", "HORARIO 4", "ATRAS"};
  int numOptions = 5;

  for (int i = 0; i < numOptions; i++) {
    int y = 10 + i * 10;
    if (i == selectedOption) {
      display.fillRect(0, y, 128, 9, SH110X_WHITE);
      display.setTextColor(SH110X_BLACK);
    } else {
      display.setTextColor(SH110X_WHITE);
    }
    display.setCursor(2, y + 1);
    display.println(options[i]);
  }
}

void showConfigHorarioScreen() {
  display.setCursor(0, 0);
  display.println("HORARIO " + String(horarioSeleccionado));

  // Actualizar el efecto de titileo
  if (millis() - lastBlinkTime > 300) {
    blinkState = !blinkState;
    lastBlinkTime = millis();
  }

  // Definir las posiciones de los valores
  int valorHoraX = 40;
  int valorMinutoX = 55;
  int valorPorcionX = 60;

  // Opciones sin valores (solo texto)
  String labels[5];
  labels[0] = "HABILITAR: " + String(tempHabilitado ? "SI" : "NO");
  labels[1] = "HORA: ";
  labels[2] = "MINUTO: ";
  labels[3] = "PORCION: ";
  labels[4] = "GUARDAR Y SALIR";

  for (int i = 0; i < 5; i++) {
    int y = 10 + i * 10;
    
    bool isSelected = (i == selectedOption);
    bool isEditing = false;
    
    if (editMode != EDIT_NONE) {
      if (i == 1 && editMode == EDIT_HORA) isEditing = true;
      if (i == 2 && editMode == EDIT_MINUTO) isEditing = true;
      if (i == 3 && editMode == EDIT_PORCION) isEditing = true;
    }

    // Dibujar fondo de selección (solo si no está en modo edición)
    if (isSelected && !isEditing) {
      display.fillRect(0, y, 128, 9, SH110X_WHITE);
      display.setTextColor(SH110X_BLACK);
    } else {
      display.setTextColor(SH110X_WHITE);
    }
    
    // Dibujar la etiqueta
    display.setCursor(2, y + 1);
    display.print(labels[i]);
    
    // Dibujar los valores con titileo si están en edición
    switch (i) {
      case 1: // HORA
        if (isEditing) {
          if (blinkState) {
            display.fillRect(valorHoraX, y, 15, 9, SH110X_WHITE);
            display.setTextColor(SH110X_BLACK);
          }
        }
        display.setCursor(valorHoraX, y + 1);
        display.print(tempHora < 10 ? "0" : "");
        display.print(tempHora);
        break;
        
      case 2: // MINUTO
        if (isEditing) {
          if (blinkState) {
            display.fillRect(valorMinutoX, y, 15, 9, SH110X_WHITE);
            display.setTextColor(SH110X_BLACK);
          }
        }
        display.setCursor(valorMinutoX, y + 1);
        display.print(tempMinuto < 10 ? "0" : "");
        display.print(tempMinuto);
        break;
        
      case 3: // PORCION
        if (isEditing) {
          if (blinkState) {
            display.fillRect(valorPorcionX, y, 15, 9, SH110X_WHITE);
            display.setTextColor(SH110X_BLACK);
          }
        }
        display.setCursor(valorPorcionX, y + 1);
        display.print(tempPorcion);
        break;
    }
    
    // Restaurar color para la siguiente iteración
    if (isSelected && !isEditing) {
      display.setTextColor(SH110X_BLACK);
    } else {
      display.setTextColor(SH110X_WHITE);
    }
  }
}

void showConfigScreen() {
  display.setCursor(0, 0);
  display.println("CONFIGURACION");

  const char* options[] = {"CONF. FECHA/HORA", "VER PLATO", "ATRAS"};
  int numOptions = 3;

  for (int i = 0; i < numOptions; i++) {
    int y = 10 + i * 10;
    if (i == selectedOption) {
      display.fillRect(0, y, 128, 9, SH110X_WHITE);
      display.setTextColor(SH110X_BLACK);
    } else {
      display.setTextColor(SH110X_WHITE);
    }
    display.setCursor(2, y + 1);
    display.println(options[i]);
  }
}

void showSuccessScreen() {
  display.setCursor(0, 20);
  display.setTextSize(2);
  display.println("COMIDA");
  display.setCursor(0, 40);
  display.println("DISPENSADA");
}

void cargarHorarioDesdePreferences() {
  prefs.begin("comedor", true);
  
  String base = "h" + String(horarioSeleccionado) + "_";
  tempHabilitado = prefs.getBool((base + "hab").c_str(), false);
  tempHora = prefs.getInt((base + "hora").c_str(), 8);
  tempMinuto = prefs.getInt((base + "minuto").c_str(), 0);
  tempPorcion = prefs.getInt((base + "porcion").c_str(), 5);
  
  prefs.end();
}

void guardarHorarioEnPreferences() {
  prefs.begin("comedor", false);
  
  String base = "h" + String(horarioSeleccionado) + "_";
  prefs.putBool((base + "hab").c_str(), tempHabilitado);
  prefs.putInt((base + "hora").c_str(), tempHora);
  prefs.putInt((base + "minuto").c_str(), tempMinuto);
  prefs.putInt((base + "porcion").c_str(), tempPorcion);
  
  prefs.end();
  
  Serial.println("Horario " + String(horarioSeleccionado) + " guardado:");
  Serial.println("Habilitado: " + String(tempHabilitado ? "SI" : "NO"));
  Serial.println("Hora: " + String(tempHora));
  Serial.println("Minuto: " + String(tempMinuto));
  Serial.println("Porcion: " + String(tempPorcion));
}