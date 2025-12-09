#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "DHT.h"

const char *bleName = "ESP32-RGB-FAN";
#define SERVICE_UUID "12345678-1234-1234-1234-1234567890ab"
#define CHAR_RGB_UUID "12345678-1234-1234-1234-1234567890ac"
#define CHAR_FAN_UUID "12345678-1234-1234-1234-1234567890ad"

const int PIN_R = 1;
const int PIN_G = 2;
const int PIN_B = 3;
const int MOTOR_PIN = 5;

#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
#define LED_VALUE(x)(x) //CATODO COMUN
#define MOTOR_VALUE(x)(x) //0=apagado 255=max

const float TEMP_INICIO = 23.0;
const float TEMP_MAX = 28.0;

int velocidadPWM_auto = 0;
int velocidadPWM_manual = 0;
bool modoManualFan = false; //false=DHT true=app

//BLE
BLEServer* pServer;
BLECharacteristic* pCharRGB;
BLECharacteristic* pCharFan;

void setRGB(uint8_t r, uint8_t g, uint8_t b){
  analogWrite(PIN_R, LED_VALUE(r));
  analogWrite(PIN_G, LED_VALUE(g));
  analogWrite(PIN_B, LED_VALUE(b));
  Serial.printf("RGB=> R:%d G:%d B:%d\n", r, g, b);
}

void setFanPWM(uint8_t pwm){
  analogWrite(MOTOR_PIN, MOTOR_VALUE(pwm));
  Serial.printf("FanPWM=> %d\n", pwm);
  if (pCharFan != nullptr) {
    String pwmStr = String((int)pwm); //actualizar la characteristic FAN con el PWM actual
    pCharFan->setValue(pwmStr.c_str());
  }
}


//para la parte automatica del motor
uint8_t calcularPWMdesdeTemp(float temp){
  int t = (int)temp;
  int velocidadPWM = 0;
  if (t < TEMP_INICIO){
    return 0; //motor apagado
  } else if (t >= TEMP_MAX){
    return 255; //max velocidad
  } else{
    velocidadPWM = map(t, TEMP_INICIO, TEMP_MAX, 100, 255); //escalar linealmente la velocidad entre TEMP_INICIO y TEMP_MAX
  }
  return (uint8_t)velocidadPWM;
}

//callbacks BLE
class ServerCallbacks: public BLEServerCallbacks{
  void onConnect(BLEServer* pServer){
    Serial.println("Conectado");
  }

  void onDisconnect(BLEServer* pServer){
    Serial.println("Desconectado");
    pServer->startAdvertising();
  }
};

//recibir RGB
class CharRGBCallbacks: public BLECharacteristicCallbacks{
  void onWrite(BLECharacteristic* characteristic){
    String value = characteristic->getValue();
    Serial.print("RGB recibido: ");
    Serial.println(value);
    int r = 0, g = 0, b = 0;
    int c1 = value.indexOf(',');
    int c2 = value.indexOf(',', c1 + 1);
    if (c1 > 0 && c2 > c1){
      r = value.substring(0, c1).toInt();
      g = value.substring(c1 + 1, c2).toInt();
      b = value.substring(c2 + 1).toInt();
      setRGB(r, g, b);
    } else Serial.println("Formato RGB invalido");
  }
};

//recibir PWM
class CharFanCallbacks: public BLECharacteristicCallbacks{
  void onWrite(BLECharacteristic* characteristic){
    String value = characteristic->getValue();
    value.trim();
    Serial.print("FAN recibido: ");
    Serial.println(value);
    if (value == "AUTO"){
      modoManualFan = false;
      Serial.println("DHT");
    } else{
      int pwm = value.toInt(); //si no es numero queda 0
      modoManualFan = true;
      velocidadPWM_manual = pwm;
      Serial.printf("MANUAL PWM=%d\n", pwm);
      setFanPWM(velocidadPWM_manual);
    }
  }
};

void setup(){
  Serial.begin(115200);
  delay(1000);
  Serial.println("Iniciando TODO");
  //pines
  pinMode(PIN_R, OUTPUT);
  pinMode(PIN_G, OUTPUT);
  pinMode(PIN_B, OUTPUT);
  pinMode(MOTOR_PIN, OUTPUT);
  setRGB(0, 0, 0);
  setFanPWM(0);
  //DHT11
  dht.begin();
//BLE
  BLEDevice::init(bleName);
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
//caracteristicas RGB
  pCharRGB = pService->createCharacteristic(CHAR_RGB_UUID, BLECharacteristic::PROPERTY_READ |BLECharacteristic::PROPERTY_WRITE);
  pCharRGB->setCallbacks(new CharRGBCallbacks());
//caracteristicasFAN
  pCharFan = pService->createCharacteristic(CHAR_FAN_UUID, BLECharacteristic::PROPERTY_READ |BLECharacteristic::PROPERTY_WRITE);
  pCharFan->setCallbacks(new CharFanCallbacks());
  pCharFan->setValue("0");

  pService->start();
  pServer->getAdvertising()->addServiceUUID(SERVICE_UUID);
  pServer->getAdvertising()->start();
  Serial.println("ESP32-RGB-FAN listo");
}

unsigned long ultmDhtMillis = 0;
const unsigned long DHT_INTERVALO = 6000;

void loop(){
  unsigned long ahora = millis();
  //solo en modo auto uso DHT
  if (!modoManualFan && (ahora - ultmDhtMillis >= DHT_INTERVALO)){
    ultmDhtMillis = ahora;
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    if (isnan(h) || isnan(t)){
      Serial.println("Error DHT11");
      return;
    }
    Serial.print("DHT=> Humedad: ");
    Serial.print(h);
    Serial.print("%  Temp: ");
    Serial.print(t);
    Serial.println(" °C");
    velocidadPWM_auto = calcularPWMdesdeTemp(t);
    setFanPWM(velocidadPWM_auto);
    Serial.print("Velocidad PWM auto: ");
    Serial.println(velocidadPWM_auto);
  }
}
