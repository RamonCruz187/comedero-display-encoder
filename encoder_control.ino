#include "config.h"

extern int currentScreen;
extern int selectedOption;
extern volatile int encoderPos;
extern bool tempHabilitado;
extern int tempHora;
extern int tempMinuto;
extern int tempPorcion;
extern int editMode;
extern bool dispensandoActivo;

// Variables externas para configuración de fecha/hora
extern int tempAnio;
extern int tempMes;
extern int tempDia;
extern int tempHoraRTC;
extern int tempMinutoRTC;
extern int editModeRTC;
extern bool seleccionGuardar;

void handleEncoderChanges() {
  // Bloquear encoder durante dispensado
  if (dispensandoActivo) {
    return;
  }
  
  static int lastEncoderPos = 0;
  
  if (encoderPos != lastEncoderPos) {
    int change = encoderPos - lastEncoderPos;
    
    // Modo edición de horarios
    if (currentScreen == CONFIG_HORARIO_SCREEN && editMode != EDIT_NONE) {
      int actualChange = (change > 0) ? 1 : -1;
      
      switch (editMode) {
        case EDIT_HORA:
          tempHora += actualChange;
          if (tempHora < 0) tempHora = 23;
          if (tempHora > 23) tempHora = 0;
          break;
        case EDIT_MINUTO:
          tempMinuto += actualChange;
          if (tempMinuto < 0) tempMinuto = 59;
          if (tempMinuto > 59) tempMinuto = 0;
          break;
        case EDIT_PORCION:
          tempPorcion += actualChange;
          if (tempPorcion < 1) tempPorcion = 10;
          if (tempPorcion > 10) tempPorcion = 1;
          break;
      }
      
      lastEncoderPos = encoderPos;
    }
    // Modo edición de fecha/hora
    else if (currentScreen == CONFIG_FECHA_HORA_SCREEN && editModeRTC != EDIT_NONE) {
      int actualChange = (change > 0) ? 1 : -1;
      
      switch (editModeRTC) {
        case EDIT_ANIO:
          tempAnio += actualChange;
          if (tempAnio < 2020) tempAnio = 2030;
          if (tempAnio > 2030) tempAnio = 2020;
          break;
        case EDIT_MES:
          tempMes += actualChange;
          if (tempMes < 1) tempMes = 12;
          if (tempMes > 12) tempMes = 1;
          break;
        case EDIT_DIA:
          tempDia += actualChange;
          if (tempDia < 1) tempDia = 31;
          if (tempDia > 31) tempDia = 1;
          break;
        case EDIT_HORA_RTC:
          tempHoraRTC += actualChange;
          if (tempHoraRTC < 0) tempHoraRTC = 23;
          if (tempHoraRTC > 23) tempHoraRTC = 0;
          break;
        case EDIT_MINUTO_RTC:
          tempMinutoRTC += actualChange;
          if (tempMinutoRTC < 0) tempMinutoRTC = 59;
          if (tempMinutoRTC > 59) tempMinutoRTC = 0;
          break;
        case EDIT_GUARDAR_SALIR:
          // Alternar entre GUARDAR y SALIR
          seleccionGuardar = !seleccionGuardar;
          break;
      }
      
      lastEncoderPos = encoderPos;
    } else {
      // Para navegación normal, mantener el comportamiento actual
      lastEncoderPos = encoderPos;
    }
  }
}