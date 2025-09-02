/*
  Project: Low-cost H2S Sensor
  Institution: Universidad Nacional Autónoma de México (UNAM)
  Authors: Pech Figueroa, Gómez-Reali M.A., Rosales-Fuerte, Rodríguez-Martínez
  Description:
    This program interfaces a low-cost H2S gas sensor with an Arduino-compatible 
    microcontroller. The system records hydrogen sulfide concentration, sensor 
    temperature, and RTC (DS3231) temperature. Data are stored on an SD card and 
    displayed on an OLED screen. Calibration parameters are read from an external file.

  Hardware:
    - DFRobot MultiGas Sensor (I2C/UART)
    - RTC DS3231
    - OLED SSD1306 (128x32)
    - SD card module
    - Interrupt button on pin 6
*/

#include "DFRobot_MultiGasSensor.h"
#include <RTClib.h>          // RTC DS3231 library
#include <SD.h>              // SD card library
#include <Adafruit_GFX.h>    // OLED graphics library
#include <Adafruit_SSD1306.h>

// Uncomment to use I2C communication (default). Disable for UART.
#define I2C_COMMUNICATION
File myFile;
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire);

// =============================
// Sensor communication options
// =============================
#ifdef I2C_COMMUNICATION
  #define I2C_ADDRESS 0x74
  DFRobot_GAS_I2C gas(&Wire, I2C_ADDRESS);
#else
  #if (!defined ARDUINO_ESP32_DEV) && (!defined __SAMD21G18A__)
    // Arduino UNO SoftwareSerial
    // RX → pin 2, TX → pin 3
    SoftwareSerial mySerial(2, 3);
    DFRobot_GAS_SoftWareUart gas(&mySerial);
  #else
    // ESP32 HardwareSerial
    // RX → IO16, TX → IO17
    DFRobot_GAS_HardWareUart gas(&Serial2);
  #endif
#endif

// =============================
// Global variables
// =============================
RTC_DS3231 rtc;
String Strfecha = "";
String Strhora = "";
String arch;
float H2s = 0;
float temp = 0;
int cuenta = 0;
int buttonState = 0;
boolean estado = true;
String nombre;
float pendiente;
float ordenada;
String fechaCalibracion;

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

// Bitmap logo (128x32px)
static const unsigned char PROGMEM foto[] = {
  // --- Bitmap data (unchanged) ---
};

// For RTC adjustment (example values)
const byte seconds = 0;
const byte minutes = 0;
const byte hours   = 17;
const byte day     = 17;
const byte month   = 11;
const byte year    = 15;

volatile unsigned long lastInterruptTime = 0;
String NameF = "Sensor05.csv";
String sig;
String inf;

// =============================
// Function declarations
// =============================
String ceros(String inf);
void interrup();

// =============================
// Setup function
// =============================
void setup() {
  pinMode(6, INPUT_PULLDOWN);     
  attachInterrupt(6, interrup, FALLING);
  delay(500);

  Serial.begin(9600);

  // Initialize OLED display
  Serial.println("Starting OLED...");
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); 
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Starting...");
  display.println("SAMMO_V2");
  display.display();
  delay(1000);

  // Show bitmap logo
  display.clearDisplay();
  display.drawBitmap(0, 0, foto, SCREEN_WIDTH, SCREEN_HEIGHT, 1);
  display.display();
  delay(1000);

  Wire.begin();
  
  // Initialize RTC
  if (!rtc.begin()) {
    Serial.println("RTC module not found!");
    while (1);
  }
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // Set RTC with compile time

  // Initialize SD card
  Serial.println("Initializing SD...");
  if (!SD.begin(SDCARD_SS_PIN)) {
    Serial.println("SD initialization failed!");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("SD failed");
    display.println("Restart...");
    display.display();
    while (1);
  }
  Serial.println("SD OK");

  // Initialize gas sensor
  while (!gas.begin()) {
    Serial.println("No sensor detected!");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Connect");
    display.println("Sensor");
    display.display();
    delay(2000);
  }
  Serial.println("Gas sensor connected");

  gas.changeAcquireMode(gas.PASSIVITY);
  gas.setTempCompensation(gas.OFF);

  // Display init status
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("H2S OK");
  display.println("SD OK");
  display.display();
  delay(400);

  // Get calibration data from SD
  File archivo = SD.open("Cal.txt");
  if (!archivo) {
    Serial.println("Could not open Cal.txt");
    return;
  }

  Serial.println("Reading calibration file...");
  while (archivo.available()) {
    String linea = archivo.readStringUntil('\n');
    linea.trim();

    if (linea.startsWith("Nombre,")) {
      nombre = linea.substring(linea.indexOf(',') + 1);
    } else if (linea.startsWith("Pendiente,")) {
      pendiente = linea.substring(linea.indexOf(',') + 1).toFloat();
    } else if (linea.startsWith("Ordenada,")) {
      ordenada = linea.substring(linea.indexOf(',') + 1).toFloat();
    } else if (linea.startsWith("Fecha de calibración:")) {
      fechaCalibracion = linea.substring(linea.indexOf(':') + 1);
    }
  }
  archivo.close();

  // Print calibration data
  Serial.println("Calibration data:");
  Serial.println("Name: " + nombre);
  Serial.print("Slope: "); Serial.println(pendiente, 4);
  Serial.print("Intercept: "); Serial.println(ordenada, 4);
  Serial.println("Calibration date: " + fechaCalibracion);

  NameF = nombre + ".csv";

  // Create CSV file if it does not exist
  if (!SD.exists(NameF)) {
    myFile = SD.open(NameF, FILE_WRITE);
    if (myFile) {
      myFile.println("Fecha,Hora,var,H2S,Temp Sen,Temp RTC");
      myFile.println("YYYY-MM-DD,HH:MM:SS,GAS,ppm,°C,°C");
      myFile.close();
    }    
  }

  // Display calibration date
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Cal Date:");
  display.println(fechaCalibracion);
  display.display();
  delay(500);

  // Wait until Cal.txt exists
  while (!SD.exists("Cal.txt")) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Missing file");
    display.println("Cal.txt");
    display.display();
    delay(400);
  }
}

// =============================
// Main loop
// =============================
void loop() {
  delay(200);

  if (estado) {
    DateTime fecha = rtc.now();

    String segundo = ceros(String(fecha.second()));
    String minuto  = ceros(String(fecha.minute()));
    String hora    = ceros(String(fecha.hour()));
    String mes     = ceros(String(fecha.month()));
    String dia     = ceros(String(fecha.day()));

    if (segundo != sig) {
      sig = segundo;
      Strfecha = String(fecha.year()) + "-" + mes + "-" + dia;
      Strhora  = hora + ":" + minuto + ":" + segundo;

      float calib = gas.readGasConcentrationPPM() * pendiente + ordenada;
      if (calib > ordenada) {
        String Dato = Strfecha + "," + Strhora + "," + gas.queryGasType() + "," + calib + "," + gas.readTempC() + "," + rtc.getTemperature();
        
        Serial.println(Dato);
        myFile = SD.open(NameF, FILE_WRITE);
        if (myFile) {
          myFile.println(Dato);
          myFile.close();
        }
      } else {
        calib = 0;
      }

      // Display values on OLED
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println(Strhora);
      display.println(nombre + ":" + String(calib));
      display.display();
    }
  } else {
    // Standby mode (LED off)
    digitalWrite(LED_BUILTIN, LOW);
    delay(250);
  }
}

// =============================
// Helper functions
// =============================

// Add leading zero to values < 10
String ceros(String inf) {
  if (inf.toInt() <= 9) {
    inf = "0" + inf;
  }
  return inf;
}

// Interrupt handler (button on pin 6)
void interrup() {
  unsigned long interruptTime = millis();
  if (interruptTime - lastInterruptTime > 300) { // debounce 300 ms
    buttonState = digitalRead(6);
    estado = !estado; // toggle state

    if (estado) {
      digitalWrite(LED_BUILTIN, HIGH);
      Serial.println(">> Activated");
    } else {
      digitalWrite(LED_BUILTIN, LOW);
      Serial.println(">> Standby requested");
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("OFF...");
      display.display();
      delay(500);
      display.clearDisplay();
    }
    lastInterruptTime = interruptTime;
  }
}
