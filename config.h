#ifndef CONFIG_H
#define CONFIG_H

// Pines I2C personalizados
#define SDA_PIN 13
#define SCL_PIN 14

// Pines del encoder KY-040
#define ENCODER_CLK 16
#define ENCODER_DT  15
#define ENCODER_SW  3

// Pin del motor
#define MOTOR_PIN 2

// Tamaño de pantalla
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Definición de pantallas
#define MAIN_SCREEN 0
#define HORARIOS_SCREEN 1
#define SELECCIONAR_HORARIO_SCREEN 5
#define CONFIG_HORARIO_SCREEN 6
#define CONFIG_SCREEN 3
#define SUCCESS_SCREEN 4
#define DISPENSANDO_SCREEN 7
#define CONFIG_FECHA_HORA_SCREEN 8

// Estados de edición
#define EDIT_NONE 0
#define EDIT_HORA 1
#define EDIT_MINUTO 2
#define EDIT_PORCION 3
#define EDIT_ANIO 4
#define EDIT_MES 5
#define EDIT_DIA 6
#define EDIT_HORA_RTC 7
#define EDIT_MINUTO_RTC 8
#define EDIT_GUARDAR_SALIR 9  // Nuevo estado para seleccionar GUARDAR o SALIR

// Declaraciones de funciones para preferences_manager
bool obtenerHorario(int numeroHorario, bool &habilitado, int &hora, int &minuto, int &porcion);
void guardarHorario(int numeroHorario, bool habilitado, int hora, int minuto, int porcion);

#endif