#include <Preferences.h>

Preferences prefs;

bool obtenerHorario(int numeroHorario, bool &habilitado, int &hora, int &minuto, int &porcion) {
  String base = "h" + String(numeroHorario) + "_";
  
  prefs.begin("comedor", true);
  habilitado = prefs.getBool((base + "hab").c_str(), false);
  hora = prefs.getInt((base + "hora").c_str(), -1);
  minuto = prefs.getInt((base + "minuto").c_str(), -1);
  porcion = prefs.getInt((base + "porcion").c_str(), -1);
  prefs.end();
  
  return (hora != -1 && minuto != -1 && porcion != -1);
}

void guardarHorario(int numeroHorario, bool habilitado, int hora, int minuto, int porcion) {
  String base = "h" + String(numeroHorario) + "_";
  
  prefs.begin("comedor", false);
  prefs.putBool((base + "hab").c_str(), habilitado);
  prefs.putInt((base + "hora").c_str(), hora);
  prefs.putInt((base + "minuto").c_str(), minuto);
  prefs.putInt((base + "porcion").c_str(), porcion);
  prefs.end();
  
  Serial.println("Horario " + String(numeroHorario) + " guardado:");
  Serial.println("Habilitado: " + String(habilitado ? "SI" : "NO"));
  Serial.println("Hora: " + String(hora));
  Serial.println("Minuto: " + String(minuto));
  Serial.println("Porcion: " + String(porcion));
}