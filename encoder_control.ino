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

void handleEncoderChanges() {
  // Bloquear encoder durante dispensado
  if (dispensandoActivo) {
    return;
  }
  
  static int lastEncoderPos = 0;
  
  if (encoderPos != lastEncoderPos) {
    int change = encoderPos - lastEncoderPos;
    
    if (currentScreen == CONFIG_HORARIO_SCREEN && editMode != EDIT_NONE) {
      // En modo edición, usar cambio directo sin acumulación
      // Esto asegura que el encoder vaya de 1 en 1
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
      
      // Actualizar lastEncoderPos inmediatamente para evitar acumulación
      lastEncoderPos = encoderPos;
    } else {
      // Para navegación normal, mantener el comportamiento actual
      lastEncoderPos = encoderPos;
    }
  }
}