#include <BluetoothSerial.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

BluetoothSerial SerialBT;  // Para Bluetooth clásico
const char* DEVICE_NAME_BT = "ESP32_BT";  // Nombre Bluetooth clásico
const char* DEVICE_NAME_BLE = "ESP32_BLE";  // Nombre BLE

const int PUMP_PIN = 13;
const int HUMIDITY_SENSOR_PIN = A0;

unsigned long pumpStartTime = 0;
unsigned long pumpDuration = 0;
bool isPumpRunning = false;
bool isConnected = false;

void setup() {
  // Iniciar puerto serie
  Serial.begin(115200);

  // Iniciar Bluetooth clásico
  SerialBT.begin(DEVICE_NAME_BT);
  Serial.println("Bluetooth clásico iniciado");

  // Iniciar BLE
  BLEDevice::init(DEVICE_NAME_BLE);
  BLEServer *pServer = BLEDevice::createServer();
  pServer->getAdvertising()->start();  // Empezar publicidad BLE
  Serial.println("Publicidad BLE iniciada");

  // Configuración de la bomba
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, LOW);
}

void loop() {
  // Control de la bomba con temporizador
  if (isPumpRunning && millis() - pumpStartTime >= pumpDuration) {
    digitalWrite(PUMP_PIN, LOW);
    isPumpRunning = false;
    if (isConnected) {
      SerialBT.println("Bomba detenida");
    }
  }

  // Lectura del sensor de humedad
  int humidity = analogRead(HUMIDITY_SENSOR_PIN);
  int humidityPercent = map(humidity, 0, 4095, 0, 100);

  // Enviar datos por Bluetooth clásico si está conectado
  if (isConnected) {
    static unsigned long lastSensorRead = 0;
    if (millis() - lastSensorRead >= 5000) {
      String sensorData = "SENSOR:" + String(humidityPercent);
      SerialBT.println(sensorData);
      lastSensorRead = millis();
    }
  }

  // Procesar comandos de Bluetooth clásico
  if (SerialBT.available()) {
    String command = SerialBT.readStringUntil('\n');
    command.trim();

    if (command == "CONNECT") {
      isConnected = true;
      SerialBT.println("STATUS:connected");
      Serial.println("Cliente conectado (Bluetooth clásico)");
    } 
    else if (command == "DISCONNECT") {
      isConnected = false;
      digitalWrite(PUMP_PIN, LOW); // Apagar bomba por seguridad
      isPumpRunning = false;
      SerialBT.println("STATUS:disconnected");
      Serial.println("Cliente desconectado (Bluetooth clásico)");
    }
    else if (command.startsWith("PUMP:") && isConnected) {
      int duration = command.substring(5).toInt() * 1000;
      if (duration > 0 && duration <= 300000) {
        digitalWrite(PUMP_PIN, HIGH);
        isPumpRunning = true;
        pumpStartTime = millis();
        pumpDuration = duration;
        SerialBT.println("Bomba activada por " + String(duration / 1000) + " segundos");
      }
    }
  }

  delay(20);  // Pequeña pausa
}
