/*
  =============================================================================
  NODO RECEPTOR / GATEWAY LORA-WIFI - Smart Campus UNL
  =============================================================================
  Placa: Heltec WiFi LoRa 32 V3 (ESP32-S3)

  Funcion:
  Recibe por LoRa el paquete JSON enviado por los nodos sensores, lo muestra
  en su pantalla OLED local (con RSSI/SNR para verificar la calidad del
  enlace) y lo reenvia por WiFi mediante una peticion HTTP POST al backend
  Django, que se encarga de guardarlo en la base de datos.

  NOTA: este dispositivo se utilizo unicamente durante la etapa de pruebas
  del proyecto, para validar la comunicacion nodo -> gateway -> backend.

  IMPORTANTE - CREDENCIALES:
  Las constantes WIFI_SSID, WIFI_PASS y DJANGO_URL de mas abajo son valores
  de EJEMPLO. Antes de compilar, reemplazalas con los datos reales de tu red
  y de tu servidor Django, y NUNCA subas esos valores reales a un repositorio
  publico (ni siquiera en el historial de commits).
  =============================================================================
*/

#include <heltec_unofficial.h>
#include <WiFi.h>
#include <HTTPClient.h>

// ================= WIFI + BACKEND DJANGO =================
// Reemplazar con los datos reales de la red y del servidor antes de compilar.
const char* WIFI_SSID  = "NOMBRE_DE_TU_RED";
const char* WIFI_PASS  = "CONTRASENA_DE_TU_RED";
const char* DJANGO_URL = "http://IP_DEL_SERVIDOR:8000/api/mediciones/";

// ================= CONFIGURACION LORA =================
// Debe coincidir exactamente con la configuracion usada en los nodos sensores.
#define FRECUENCIA_LORA 915.0
#define ANCHO_BANDA     125.0
#define SPREADING       7
#define CODING_RATE     5
#define SYNC_WORD       0x12
#define POTENCIA_TX     14

// ================= VARIABLES DE ESTADO =================
String mensajeRecibido = "";
int rssi = 0;    // Intensidad de la senal recibida (dBm)
float snr = 0;   // Relacion senal/ruido del ultimo paquete LoRa recibido
bool wifi_ok = false;
bool lora_ok = false;

// ================= EXTRACCION DE CAMPOS DEL JSON =================
String extraerValor(String json, String clave) {
  String buscar = "\"" + clave + "\":";
  int inicio = json.indexOf(buscar);

  if (inicio == -1) return "--";

  inicio += buscar.length();

  if (json.charAt(inicio) == '\"') {
    inicio++;
    int fin = json.indexOf("\"", inicio);
    return json.substring(inicio, fin);
  } else {
    int fin = json.indexOf(",", inicio);
    if (fin == -1) fin = json.indexOf("}", inicio);
    return json.substring(inicio, fin);
  }
}

// ================= CONEXION WIFI =================
void conectarWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("Conectando WiFi");

  int intentos = 0;
  while (WiFi.status() != WL_CONNECTED && intentos < 40) {
    delay(500);
    Serial.print(".");
    intentos++;
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    wifi_ok = true;
    Serial.print("WiFi OK - IP: ");
    Serial.println(WiFi.localIP());
  } else {
    wifi_ok = false;
    Serial.println("WiFi FAIL");
  }
}

// ================= INICIALIZACION DEL MODULO LORA =================
void iniciarLoRa() {
  int state = radio.begin();

  if (state == RADIOLIB_ERR_NONE) {
    lora_ok = true;
    Serial.println("LoRa iniciado correctamente");
  } else {
    lora_ok = false;
    Serial.print("Error iniciando LoRa: ");
    Serial.println(state);
    return;
  }

  radio.setFrequency(FRECUENCIA_LORA);
  radio.setBandwidth(ANCHO_BANDA);
  radio.setSpreadingFactor(SPREADING);
  radio.setCodingRate(CODING_RATE);
  radio.setSyncWord(SYNC_WORD);
  radio.setOutputPower(POTENCIA_TX);

  Serial.println("Frecuencia LoRa: 915 MHz");
}

// ================= ENVIO AL BACKEND DJANGO =================
// Reintenta la conexion WiFi si se habia perdido, y solo entonces envia el
// JSON recibido por LoRa como POST al endpoint de la API.
void enviarDjango(String json) {
  if (WiFi.status() != WL_CONNECTED) {
    wifi_ok = false;
    conectarWiFi();
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] No conectado. No se envio a Django.");
    return;
  }

  HTTPClient http;
  http.begin(DJANGO_URL);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(5000);

  int codigo = http.POST(json);
  String respuesta = http.getString();

  Serial.println("[Django] HTTP " + String(codigo));
  Serial.println("[Respuesta] " + respuesta);

  http.end();
}

// ================= PANTALLA OLED LOCAL =================
// Muestra los valores del ultimo paquete recibido, junto con RSSI/SNR, para
// verificar en campo la calidad del enlace LoRa entre el nodo y el gateway.
void mostrarDatosOLED(String json) {
  String nodo = extraerValor(json, "nodo");
  String aula = extraerValor(json, "aula");
  String temp = extraerValor(json, "temperatura");
  String hum  = extraerValor(json, "humedad");
  String co2  = extraerValor(json, "co2");
  String lux  = extraerValor(json, "iluminacion");
  String ruido = extraerValor(json, "ruido");
  String mov = extraerValor(json, "movimiento");

  if (mov == "true") mov = "SI";
  if (mov == "false") mov = "NO";

  display.clear();
  display.setFont(ArialMT_Plain_10);

  display.drawString(0, 0, "CO2:" + co2 + "ppm  Mov:" + mov);
  display.drawString(0, 12, "T:" + temp + "C  H:" + hum + "%");
  display.drawString(0, 24, "Lux:" + lux + " lx");
  display.drawString(0, 36, "Ruido:" + ruido + " dB");
  display.drawString(0, 48, "RSSI:" + String(rssi) + " SNR:" + String(snr, 1));

  display.display();
}

// ================= SETUP =================
void setup() {
  heltec_setup();
  heltec_ve(true);

  Serial.begin(115200);
  delay(1000);

  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, "Gateway LoRa");
  display.drawString(0, 15, "Iniciando...");
  display.display();

  Serial.println("================================");
  Serial.println(" HELTEC 2 - GATEWAY LORA WIFI");
  Serial.println(" LoRa -> WiFi -> Django");
  Serial.println("================================");

  conectarWiFi();
  iniciarLoRa();

  display.clear();
  display.drawString(0, 0, "Gateway LoRa");
  display.drawString(0, 15, wifi_ok ? "WiFi OK" : "WiFi FAIL");
  display.drawString(0, 30, lora_ok ? "LoRa OK" : "LoRa FAIL");
  display.drawString(0, 45, "Esperando datos...");
  display.display();
}

// ================= LOOP =================
void loop() {
  heltec_loop();

  if (!lora_ok) {
    delay(1000);
    return;
  }

  String mensaje = "";
  int state = radio.receive(mensaje);

  if (state == RADIOLIB_ERR_NONE) {
    mensajeRecibido = mensaje;
    rssi = radio.getRSSI();
    snr = radio.getSNR();

    Serial.println("-------------");
    Serial.println("Paquete LoRa recibido:");
    Serial.println(mensajeRecibido);
    Serial.print("RSSI: ");
    Serial.print(rssi);
    Serial.println(" dBm");
    Serial.print("SNR: ");
    Serial.print(snr);
    Serial.println(" dB");

    mostrarDatosOLED(mensajeRecibido);
    enviarDjango(mensajeRecibido);
  }

  delay(100);
}
