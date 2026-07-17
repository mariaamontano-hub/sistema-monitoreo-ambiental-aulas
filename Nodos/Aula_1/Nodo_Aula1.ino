/******************************************************************************
 *                     FIRMWARE DEL NODO DE MONITOREO AMBIENTAL
 *                Sistema de Monitoreo Ambiental en Aulas
 *                           Smart Campus UNL
 *------------------------------------------------------------------------------
 * Trabajo de Titulación previo a la obtención del título de
 * Ingeniera en Telecomunicaciones
 *
 * Universidad Nacional de Loja
 * Carrera de Ingeniería en Telecomunicaciones
 *
 * Autora  : Maria Andreina Montaño Rojas
 * Año     : 2026
 * Versión : 1.0
 *------------------------------------------------------------------------------
 *
 * DESCRIPCIÓN
 * Este firmware implementa un nodo de monitoreo ambiental basado en la placa
 * Heltec WiFi LoRa 32 V3. El nodo adquiere información de temperatura,
 * humedad relativa, concentración de dióxido de carbono (CO₂), nivel de
 * iluminación, nivel sonoro y detección de movimiento mediante sensores
 * ambientales integrados.
 *
 * Las mediciones obtenidas son encapsuladas en formato JSON y transmitidas
 * mediante tecnología LoRa hacia el gateway del sistema. Posteriormente, el
 * gateway reenvía la información al servidor desarrollado con Django, donde
 * los datos son procesados, almacenados en una base de datos PostgreSQL y
 * visualizados mediante un dashboard web para el monitoreo de las condiciones
 * ambientales de las aulas.
 *
 * VARIABLES MONITOREADAS
 *  • Temperatura y humedad relativa (SHT31)
 *  • Concentración de dióxido de carbono - CO₂ (MH-Z19C)
 *  • Nivel de iluminación (BH1750)
 *  • Nivel sonoro relativo (INMP441)
 *  • Detección de movimiento (HC-SR501)
 *
 * HARDWARE EMPLEADO
 *  • Heltec WiFi LoRa 32 V3
 *  • Sensor SHT31
 *  • Sensor MH-Z19C
 *  • Sensor BH1750
 *  • Micrófono digital INMP441
 *  • Sensor PIR HC-SR501
 *
 * FUNCIONAMIENTO GENERAL
 *  1. Inicializa la comunicación con todos los sensores.
 *  2. Adquiere periódicamente las variables ambientales.
 *  3. Procesa y valida las mediciones obtenidas.
 *  4. Construye un paquete de datos en formato JSON.
 *  5. Transmite la información al gateway mediante LoRa.
 *  6. Actualiza la información mostrada en la pantalla OLED.
 *
 * OBSERVACIONES
 *  • Este firmware es utilizado por los dos nodos implementados en el
 *    proyecto (Aula 1 y Aula 2).
 *  • La única diferencia entre ambos corresponde a las constantes NODO_ID
 *    y AULA_ID, utilizadas para identificar el origen de cada medición.
 *  • La transmisión de datos se realiza cada 15 segundos.
 *
 ******************************************************************************/

#include <heltec_unofficial.h>
#include <Wire.h>
#include <BH1750.h>
#include <Adafruit_SHT31.h>
#include <driver/i2s.h>
#include <math.h>

// ================= CONFIGURACION LORA =================
// Debe coincidir exactamente con la configuracion del gateway.
#define FRECUENCIA_LORA 915.0   // MHz (banda ISM usada en la region)
#define ANCHO_BANDA     125.0   // kHz
#define SPREADING       7       // Spreading Factor (SF7)
#define CODING_RATE     5       // Coding Rate 4/5
#define SYNC_WORD       0x12    // Palabra de sincronizacion de la red privada
#define POTENCIA_TX     14      // dBm

// Identificadores del nodo: UNICO valor que cambia entre Aula 1 y Aula 2.
const char* NODO_ID = "nodo_01";
const char* AULA_ID = "Aula 1";

// Intervalo entre transmisiones LoRa (ms)
#define INTERVALO_ENVIO 15000
unsigned long tUltimoEnvio = 0;

// ================= BUS I2C DE SENSORES (SHT31 + BH1750) =================
// Se usa un segundo bus I2C (TwoWire(1)) distinto al que usa la placa OLED,
// para evitar conflictos de direcciones/pines con la libreria heltec_unofficial.
#define SDA_SENSORES 41
#define SCL_SENSORES 42

TwoWire I2C_Sensores = TwoWire(1);
BH1750 lightMeter;
Adafruit_SHT31 sht31 = Adafruit_SHT31(&I2C_Sensores);

bool bh_ok = false;   // true si el BH1750 respondio correctamente en el setup
bool sht_ok = false;  // true si el SHT31 respondio correctamente en el setup

// ================= SENSOR DE CO2 (MH-Z19C, protocolo UART propietario) =================
#define CO2_RX 6
#define CO2_TX 7

HardwareSerial CO2Serial(1);

// Comando de lectura "Read CO2 concentration" segun el datasheet del MH-Z19C.
// El ultimo byte (0x79) es el checksum fijo para este comando en particular.
byte cmdCO2[9] = {
  0xFF, 0x01, 0x86,
  0x00, 0x00, 0x00,
  0x00, 0x00, 0x79
};

// ================= SENSOR DE PRESENCIA (PIR HC-SR501) =================
#define PIR_PIN 4
#define TIEMPO_RETENCION 8000       // ms que se mantiene "movimiento=true" tras la ultima deteccion
#define TIEMPO_CONFIRMACION 300     // ms que la senal debe mantenerse HIGH para confirmar el evento (evita falsos positivos)

bool estadoMovimiento = false;
unsigned long ultimoMovimiento = 0;
unsigned long inicioDeteccion = 0;
bool detectando = false;

// ================= MICROFONO DIGITAL (INMP441, I2S) =================
#define I2S_SCK_PIN      47
#define I2S_WS_PIN       48
#define I2S_SD_PIN        5
#define I2S_PORT         I2S_NUM_0

#define SAMPLE_RATE      16000
#define BUFFER_SAMPLES   2048

#define BIT_SHIFT        8          // El INMP441 entrega 24 bits utiles alineados a la izquierda en una palabra de 32 bits
#define FULL_SCALE       8388608.0f // 2^23, valor de referencia para normalizar la muestra de 24 bits

// Constantes de calibracion del nivel sonoro (ajustadas de forma empirica
// comparando contra un sonometro de referencia durante las pruebas de campo).
#define DB_OFFSET        82.0f
#define CALIBRACION_DB   0.0f
#define SENSIBILIDAD_DB  14.0f

#define NOISE_FLOOR      30.0f  // Piso de ruido del ambiente/sensor: nunca se reporta un valor menor a este
#define FILTRO_N         4      // Cantidad de muestras para la media movil (suaviza variaciones instantaneas)

// "Retencion tipo peak-hold": el valor mostrado sube de inmediato ante un pico
// de ruido, pero baja lentamente, para que picos breves (ej. una silla) sigan
// siendo visibles unos segundos en vez de desaparecer al instante.
#define TIEMPO_RETENCION_RUIDO 3000
#define CAIDA_RUIDO_DB         0.6f

int32_t sBuffer[BUFFER_SAMPLES];

float historial[FILTRO_N] = {0};
uint8_t filtro_idx = 0;
bool filtro_lleno = false;

float ruidoRetenido = NOISE_FLOOR;
unsigned long tUltimoPicoRuido = 0;

bool i2s_ok = false;
bool lora_ok = false;

// ================= VARIABLES DE LECTURA ACTUAL =================
float temperatura = NAN;
float humedad = NAN;
float lux = -1.0;
float ruidoDB = 30.0;
int co2ppm = -1;
bool movimiento = false;

// ================= CLASIFICACION DE VARIABLES (para el OLED y logs locales) =================
// Los rangos se basan en las normas ASHRAE 55 (confort termico) y ASHRAE 62.1
// (calidad de aire interior) descritas en la tesis. La clasificacion final que
// se muestra en el dashboard se recalcula tambien en el backend Django.

String estadoTemperatura(float t) {
  if (isnan(t)) return "SIN_DATO";
  if (t >= 20 && t <= 26) return "NORMAL";
  if ((t >= 18 && t < 20) || (t > 26 && t <= 28)) return "ADVERTENCIA";
  return "CRITICO";
}

String estadoHumedad(float h) {
  if (isnan(h)) return "SIN_DATO";
  if (h >= 30 && h <= 60) return "NORMAL";
  if (h > 60 && h <= 70) return "ADVERTENCIA";
  return "CRITICO";
}

String estadoRuido(float db) {
  if (db <= 35) return "NORMAL";
  if (db <= 50) return "ADVERTENCIA";
  return "CRITICO";
}

String estadoLux(float l) {
  if (l < 0) return "SIN_DATO";
  if (l >= 300 && l <= 500) return "NORMAL";
  if ((l >= 200 && l < 300) || (l > 500 && l <= 750)) return "ADVERTENCIA";
  return "CRITICO";
}

// ================= LECTURA DE CO2 (MH-Z19C) =================
// Envia el comando de lectura y espera la respuesta de 9 bytes por UART.
// Devuelve -1 si hubo timeout, -2 si la respuesta no tiene el encabezado
// esperado, o el valor en ppm si la lectura fue valida.
int leerCO2() {
  byte response[9];

  // Limpia el buffer de entrada antes de enviar el comando, para no leer
  // bytes sobrantes de una transaccion anterior.
  while (CO2Serial.available()) {
    CO2Serial.read();
  }

  CO2Serial.write(cmdCO2, 9);
  CO2Serial.flush();

  unsigned long start = millis();
  int i = 0;

  while (millis() - start < 1000) {
    if (CO2Serial.available()) {
      response[i++] = CO2Serial.read();
      if (i == 9) break;
    }
  }

  if (i != 9) return -1;
  if (response[0] != 0xFF || response[1] != 0x86) return -2;

  int ppm = response[2] * 256 + response[3];
  return ppm;
}

// ================= LECTURA DE PRESENCIA (PIR) =================
// Aplica un tiempo de confirmacion (evita falsos positivos por ruido electrico)
// y un tiempo de retencion (mantiene "movimiento=true" un rato tras el ultimo
// evento, para no reportar presencia intermitente cuando alguien esta quieto).
bool leerPIR() {
  bool lectura = digitalRead(PIR_PIN);

  if (lectura == HIGH) {
    if (!detectando) {
      detectando = true;
      inicioDeteccion = millis();
    }

    if (millis() - inicioDeteccion > TIEMPO_CONFIRMACION) {
      ultimoMovimiento = millis();

      if (!estadoMovimiento) {
        estadoMovimiento = true;
        Serial.println("Movimiento detectado");
      }
    }
  } else {
    detectando = false;
  }

  if (estadoMovimiento && millis() - ultimoMovimiento > TIEMPO_RETENCION) {
    estadoMovimiento = false;
    Serial.println("Sin movimiento");
  }

  return estadoMovimiento;
}

// ================= CONFIGURACION DEL BUS I2S (INMP441) =================
bool iniciarI2S() {
  const i2s_config_t cfg = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 512,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };

  if (i2s_driver_install(I2S_PORT, &cfg, 0, NULL) != ESP_OK) return false;

  const i2s_pin_config_t pins = {
    .bck_io_num = I2S_SCK_PIN,
    .ws_io_num = I2S_WS_PIN,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD_PIN
  };

  if (i2s_set_pin(I2S_PORT, &pins) != ESP_OK) return false;

  i2s_zero_dma_buffer(I2S_PORT);
  return true;
}

// Calcula el nivel sonoro en dB a partir de un bloque de muestras del INMP441:
// 1) Lee un buffer de audio crudo (24 bits utiles dentro de una palabra de 32).
// 2) Calcula el RMS (valor eficaz) de las muestras.
// 3) Convierte el RMS a decibeles con la formula estandar 20*log10(rms/full_scale)
//    y aplica el offset/calibracion definidos arriba.
float calcularDB() {
  size_t bytes_leidos = 0;

  esp_err_t r = i2s_read(
    I2S_PORT,
    sBuffer,
    sizeof(sBuffer),
    &bytes_leidos,
    pdMS_TO_TICKS(300)
  );

  if (r != ESP_OK || bytes_leidos == 0) return -1.0f;

  int n = bytes_leidos / sizeof(int32_t);
  if (n <= 0) return -1.0f;

  double suma = 0.0;

  for (int i = 0; i < n; i++) {
    int32_t muestra = sBuffer[i] >> BIT_SHIFT;
    suma += muestra;
  }

  double media = suma / n;
  double suma_cuadrados = 0.0;

  for (int i = 0; i < n; i++) {
    int32_t muestra = sBuffer[i] >> BIT_SHIFT;
    double centrada = muestra - media;
    suma_cuadrados += centrada * centrada;
  }

  double rms = sqrt(suma_cuadrados / n);

  if (rms < 1.0f) return NOISE_FLOOR;

  float db = 20.0f * log10f((float)rms / FULL_SCALE)
             + DB_OFFSET
             + CALIBRACION_DB
             + SENSIBILIDAD_DB;

  if (db < NOISE_FLOOR) db = NOISE_FLOOR;
  if (db > 120.0f) db = 120.0f;

  return db;
}

// Media movil simple sobre las ultimas FILTRO_N lecturas de dB, para suavizar
// variaciones bruscas entre muestras consecutivas.
float mediaMovil(float valor) {
  historial[filtro_idx] = valor;
  filtro_idx = (filtro_idx + 1) % FILTRO_N;

  if (filtro_idx == 0) filtro_lleno = true;

  int n = filtro_lleno ? FILTRO_N : filtro_idx;
  if (n <= 0) return valor;

  float suma = 0.0f;

  for (int i = 0; i < n; i++) {
    suma += historial[i];
  }

  return suma / n;
}

// Logica de "peak-hold": mantiene el pico mas alto reciente y lo deja caer
// gradualmente, para que el valor reportado no oculte picos breves de ruido.
float retenerRuido(float db_filtrado) {
  if (db_filtrado > ruidoRetenido) {
    ruidoRetenido = db_filtrado;
    tUltimoPicoRuido = millis();
  }

  if (millis() - tUltimoPicoRuido > TIEMPO_RETENCION_RUIDO) {
    ruidoRetenido -= CAIDA_RUIDO_DB;

    if (ruidoRetenido < db_filtrado) {
      ruidoRetenido = db_filtrado;
    }

    if (ruidoRetenido < NOISE_FLOOR) {
      ruidoRetenido = NOISE_FLOOR;
    }
  }

  return ruidoRetenido;
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

  Serial.println("Configuracion LoRa:");
  Serial.println("Frecuencia: 915 MHz");
  Serial.println("BW: 125 kHz");
  Serial.println("SF: 7");
  Serial.println("CR: 4/5");
  Serial.println("SyncWord: 0x12");
}

// Arma el paquete JSON que se transmite por LoRa. Los valores invalidos/no
// disponibles se codifican como -99 (temperatura/humedad) o -1 (iluminacion),
// para que el backend pueda distinguir "sensor sin dato" de una lectura real.
String crearJSON() {
  float tEnv = isnan(temperatura) ? -99.0f : temperatura;
  float hEnv = isnan(humedad) ? -99.0f : humedad;
  float lEnv = lux < 0 ? -1.0f : lux;

  String json = "{";
  json += "\"nodo\":\"" + String(NODO_ID) + "\",";
  json += "\"aula\":\"" + String(AULA_ID) + "\",";
  json += "\"temperatura\":" + String(tEnv, 2) + ",";
  json += "\"humedad\":" + String(hEnv, 2) + ",";
  json += "\"co2\":" + String(co2ppm) + ",";
  json += "\"iluminacion\":" + String(lEnv, 1) + ",";
  json += "\"ruido\":" + String(ruidoDB, 1) + ",";
  json += "\"movimiento\":" + String(movimiento ? "true" : "false");
  json += "}";

  return json;
}

void enviarLoRa() {
  if (!lora_ok) {
    Serial.println("[LoRa] No iniciado. No se envio paquete.");
    return;
  }

  String json = crearJSON();

  int state = radio.transmit(json);

  Serial.println("[JSON LoRa] " + json);

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("[LoRa] Paquete enviado correctamente");
  } else {
    Serial.print("[LoRa] Error enviando paquete: ");
    Serial.println(state);
  }
}

// ================= PANTALLA OLED LOCAL =================
// Muestra las ultimas lecturas directamente en el nodo, util para verificar
// en campo que los sensores estan funcionando sin depender del dashboard.
void actualizarOLED() {
  display.clear();
  display.setFont(ArialMT_Plain_10);

  display.drawString(0, 0, "CO2: " + String(co2ppm) + " ppm");

  String strT = isnan(temperatura) ? "T:--" : "T:" + String(temperatura, 1) + "C";
  String strH = isnan(humedad) ? "H:--" : "H:" + String(humedad, 0) + "%";

  display.drawString(0, 12, strT + "  " + strH);
  display.drawString(0, 24, "Lux: " + String(lux, 0) + " lx");
  display.drawString(0, 36, "Ruido: " + String(ruidoDB, 1) + " dB");
  display.drawString(0, 48, "Mov: " + String(movimiento ? "SI" : "NO"));

  display.display();
}

// ================= SETUP =================
void setup() {
  heltec_setup();
  heltec_ve(true);

  Serial.begin(115200);
  delay(1000);

  Serial.println("================================");
  Serial.println(" NODO AMBIENTAL LORA TX");
  Serial.println(" Sensores -> LoRa -> Receptor");
  Serial.println("================================");

  iniciarLoRa();

  pinMode(PIR_PIN, INPUT);
  Serial.println("PIR HC-SR501: GPIO4");

  CO2Serial.begin(9600, SERIAL_8N1, CO2_RX, CO2_TX);
  Serial.println("MH-Z19C: RX GPIO6 / TX GPIO7");

  I2C_Sensores.begin(SDA_SENSORES, SCL_SENSORES, 100000);

  sht_ok = sht31.begin(0x44);

  // El BH1750 puede responder en 0x23 o 0x5C segun el estado del pin ADDR;
  // se intenta primero la direccion por defecto y, si falla, la alternativa.
  bh_ok = lightMeter.begin(
    BH1750::CONTINUOUS_HIGH_RES_MODE,
    0x23,
    &I2C_Sensores
  );

  if (!bh_ok) {
    bh_ok = lightMeter.begin(
      BH1750::CONTINUOUS_HIGH_RES_MODE,
      0x5C,
      &I2C_Sensores
    );
  }

  Serial.println(sht_ok ? "SHT31 detectado" : "ERROR SHT31");
  Serial.println(bh_ok ? "BH1750 detectado" : "ERROR BH1750");

  i2s_ok = iniciarI2S();

  if (i2s_ok) {
    i2s_start(I2S_PORT);
    Serial.println("INMP441 iniciado correctamente.");
    Serial.println("Pines: SCK=47, WS=48, SD=5, L/R=GND");
  } else {
    Serial.println("ERROR: No se pudo inicializar INMP441.");
  }

  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, "Nodo LoRa TX");
  display.drawString(0, 15, lora_ok ? "LoRa OK" : "ERROR LoRa");
  display.drawString(0, 30, "Iniciando sensores");
  display.display();

  delay(2000);
}

// ================= LOOP =================
void loop() {
  heltec_loop();

  // El MH-Z19C ocasionalmente entrega valores fuera de rango fisico posible;
  // se descartan y se conserva la ultima lectura valida.
  int lecturaCO2 = leerCO2();
  if (lecturaCO2 >= 350 && lecturaCO2 <= 5000) {
    co2ppm = lecturaCO2;
  }

  movimiento = leerPIR();

  if (sht_ok) {
    float t = sht31.readTemperature();
    float h = sht31.readHumidity();

    if (!isnan(t)) temperatura = t;
    if (!isnan(h)) humedad = h;
  }

  if (bh_ok) {
    float lecturaLux = lightMeter.readLightLevel();
    if (lecturaLux >= 0) {
      lux = lecturaLux;
    }
  }

  if (i2s_ok) {
    float db_raw = calcularDB();

    if (db_raw >= 0) {
      float db_filtrado = mediaMovil(db_raw);
      ruidoDB = retenerRuido(db_filtrado);
    }
  }

  // Muestra un resumen por consola cada 5 segundos (solo para depuracion via USB).
  static unsigned long tSerial = 0;

  if (millis() - tSerial >= 5000) {
    tSerial = millis();

    Serial.println("-------------");
    Serial.print("Temperatura: ");
    Serial.print(temperatura);
    Serial.print(" C | ");
    Serial.println(estadoTemperatura(temperatura));

    Serial.print("Humedad: ");
    Serial.print(humedad);
    Serial.print(" % | ");
    Serial.println(estadoHumedad(humedad));

    Serial.print("CO2: ");
    Serial.print(co2ppm);
    Serial.println(" ppm");

    Serial.print("Lux: ");
    Serial.print(lux);
    Serial.print(" lx | ");
    Serial.println(estadoLux(lux));

    Serial.print("Ruido: ");
    Serial.print(ruidoDB);
    Serial.print(" dB | ");
    Serial.println(estadoRuido(ruidoDB));

    Serial.print("Movimiento: ");
    Serial.println(movimiento ? "SI" : "NO");
  }

  actualizarOLED();

  // Envio por LoRa solo cada INTERVALO_ENVIO ms, para no saturar el canal
  // y ahorrar energia (el resto del tiempo se sigue muestreando localmente).
  if (millis() - tUltimoEnvio >= INTERVALO_ENVIO) {
    tUltimoEnvio = millis();
    enviarLoRa();
  }

  delay(50);
}
