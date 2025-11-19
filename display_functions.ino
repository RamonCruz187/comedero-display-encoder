#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <RTClib.h>
#include "config.h"

// Definición de variables globales
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
RTC_DS1307 rtc;

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

// Variables externas para dispensado
extern bool dispensandoActivo;
extern bool dispensadoAutomatico;
extern int porcionActual;

// Variables externas para configuración de fecha/hora
extern int tempAnio;
extern int tempMes;
extern int tempDia;
extern int tempHoraRTC;
extern int tempMinutoRTC;
extern int editModeRTC;
extern bool seleccionGuardar; // true = GUARDAR, false = SALIR

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
    case DISPENSANDO_SCREEN:
      showDispensandoScreen();
      break;
    case CONFIG_FECHA_HORA_SCREEN:  // Nueva pantalla
      showConfigFechaHoraScreen();
      break;
  }

  display.display();
}

// Función para pantalla de dispensado
void showDispensandoScreen() {
  display.setCursor(0, 25);
  display.setTextSize(2);
  display.println("SIRVIENDO");
  display.setTextSize(1);
  display.setCursor(0, 45);
  
  if (dispensadoAutomatico) {
    display.println("Horario Programado");
  } else {
    display.println("Dispensado manual");
  }
}

// Función para pantalla de éxito
void showSuccessScreen() {
  display.setCursor(0, 20);
  display.setTextSize(2);
  display.println("COMIDA");
  display.setCursor(0, 40);
  display.println("DISPENSADA");
}

// Nueva función para pantalla de configuración de fecha/hora (CORREGIDA)
void showConfigFechaHoraScreen() {
  display.setCursor(0, 0);
  display.println("CONFIG. FECHA/HORA");

  // Actualizar el efecto de titileo
  if (millis() - lastBlinkTime > 300) {
    blinkState = !blinkState;
    lastBlinkTime = millis();
  }

  // Definir las posiciones de los valores (CORREGIDAS)
  int valorAnioX = 40;
  int valorMesX = 35;
  int valorDiaX = 35;
  int valorHoraX = 40;
  int valorMinutoX = 35;  // Corregido - mismo que los demás

  // Opciones (CORREGIDAS - un solo espacio después de los dos puntos)
  String labels[6];
  labels[0] = "ANIO: ";
  labels[1] = "MES: ";
  labels[2] = "DIA: ";
  labels[3] = "HORA: ";
  labels[4] = "MIN: ";  // Corregido - mismo formato
  labels[5] = "GUARDAR | SALIR";

  for (int i = 0; i < 6; i++) {
    int y = 10 + i * 9;
    
    bool isSelected = (i == selectedOption);
    bool isEditing = false;
    
    if (editModeRTC != EDIT_NONE) {
      if (i == 0 && editModeRTC == EDIT_ANIO) isEditing = true;
      if (i == 1 && editModeRTC == EDIT_MES) isEditing = true;
      if (i == 2 && editModeRTC == EDIT_DIA) isEditing = true;
      if (i == 3 && editModeRTC == EDIT_HORA_RTC) isEditing = true;
      if (i == 4 && editModeRTC == EDIT_MINUTO_RTC) isEditing = true;
      if (i == 5 && editModeRTC == EDIT_GUARDAR_SALIR) isEditing = true;
    }

    // Dibujar fondo de selección (solo si no está en modo edición)
    if (isSelected && !isEditing) {
      display.fillRect(0, y, 128, 8, SH110X_WHITE);
      display.setTextColor(SH110X_BLACK);
    } else {
      display.setTextColor(SH110X_WHITE);
    }
    
    // Dibujar la etiqueta
    display.setCursor(2, y + 1);
    
    // Caso especial para GUARDAR | SALIR
    if (i == 5 && editModeRTC == EDIT_GUARDAR_SALIR) {
      // Mostrar GUARDAR parpadeante o SALIR parpadeante según selección
      if (blinkState) {
        if (seleccionGuardar) {
          display.print("GUARDAR | SALIR");
        } else {
          display.print("GUARDAR | SALIR");
        }
      } else {
        if (seleccionGuardar) {
          display.print("[GUARDAR] | SALIR");
        } else {
          display.print("GUARDAR | [SALIR]");
        }
      }
    } else {
      display.print(labels[i]);
    }
    
    // Dibujar los valores con titileo si están en edición
    switch (i) {
      case 0: // AÑO
        if (isEditing) {
          if (blinkState) {
            display.fillRect(valorAnioX, y, 25, 8, SH110X_WHITE);
            display.setTextColor(SH110X_BLACK);
          }
        }
        display.setCursor(valorAnioX, y + 1);
        display.print(tempAnio);
        break;
        
      case 1: // MES
        if (isEditing) {
          if (blinkState) {
            display.fillRect(valorMesX, y, 15, 8, SH110X_WHITE);
            display.setTextColor(SH110X_BLACK);
          }
        }
        display.setCursor(valorMesX, y + 1);
        display.print(tempMes < 10 ? "0" : "");
        display.print(tempMes);
        break;
        
      case 2: // DIA
        if (isEditing) {
          if (blinkState) {
            display.fillRect(valorDiaX, y, 15, 8, SH110X_WHITE);
            display.setTextColor(SH110X_BLACK);
          }
        }
        display.setCursor(valorDiaX, y + 1);
        display.print(tempDia < 10 ? "0" : "");
        display.print(tempDia);
        break;
        
      case 3: // HORA
        if (isEditing) {
          if (blinkState) {
            display.fillRect(valorHoraX, y, 15, 8, SH110X_WHITE);
            display.setTextColor(SH110X_BLACK);
          }
        }
        display.setCursor(valorHoraX, y + 1);
        display.print(tempHoraRTC < 10 ? "0" : "");
        display.print(tempHoraRTC);
        break;
        
      case 4: // MINUTO
        if (isEditing) {
          if (blinkState) {
            display.fillRect(valorMinutoX, y, 15, 8, SH110X_WHITE);
            display.setTextColor(SH110X_BLACK);
          }
        }
        display.setCursor(valorMinutoX, y + 1);
        display.print(tempMinutoRTC < 10 ? "0" : "");
        display.print(tempMinutoRTC);
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

// El resto de las funciones se mantienen igual...
String obtenerProximaComida() {
  DateTime now = rtc.now();
  int horaActual = now.hour();
  int minutoActual = now.minute();
  int tiempoActual = horaActual * 60 + minutoActual; // Convertir a minutos
  
  String proximaComida = "--:--";
  int menorDiferencia = 24 * 60; // Inicializar con un valor grande (24 horas en minutos)
  
  for (int i = 1; i <= 4; i++) {
    bool habilitado;
    int hora, minuto, porcion;
    
    if (obtenerHorario(i, habilitado, hora, minuto, porcion)) {
      if (habilitado) {
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
  }
  
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
  
  for (int i = 1; i <= 4; i++) {
    bool habilitado;
    int hora, minuto, porcion;
    
    if (obtenerHorario(i, habilitado, hora, minuto, porcion)) {
      String horaStr = (hora < 10 ? "0" : "") + String(hora);
      String minutoStr = (minuto < 10 ? "0" : "") + String(minuto);
      
      String estado = habilitado ? "SI" : "NO";
      options[i-1] = "H" + String(i) + ": " + horaStr + ":" + minutoStr + " " + estado + " P" + String(porcion);
    } else {
      options[i-1] = "H" + String(i) + ": --:-- NO P-";
    }
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

void cargarHorarioDesdePreferences() {
  bool habilitado;
  int hora, minuto, porcion;
  
  if (obtenerHorario(horarioSeleccionado, habilitado, hora, minuto, porcion)) {
    tempHabilitado = habilitado;
    tempHora = hora;
    tempMinuto = minuto;
    tempPorcion = porcion;
  }
}

void guardarHorarioEnPreferences() {
  guardarHorario(horarioSeleccionado, tempHabilitado, tempHora, tempMinuto, tempPorcion);
}