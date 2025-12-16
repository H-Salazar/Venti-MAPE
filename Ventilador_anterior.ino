#include "DHT.h"
#define DHTPIN 3
#define DHTTYPE DHT11
#define MOTOR_PIN 5 //el que controla al mosfet

DHT dht(DHTPIN, DHTTYPE);

int velocidadPWM = 0; //valor PWM entre 0 y 255

const float TEMP_INICIO = 23.0; //se enciende a partir de 23
const float TEMP_MAX = 28.0; //a 35 es max

void setup() {
  Serial.begin(9600);
  pinMode(MOTOR_PIN, OUTPUT); //se configura como salida
  dht.begin();
}

void loop() {
  delay(6000);
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (isnan(h) || isnan(t)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
  Serial.print("DHT=> Humedad: ");
  Serial.print(h);
  Serial.print("% Temp: ");
  Serial.print(t);
  Serial.println(" °C");
  if(t < TEMP_INICIO){
    velocidadPWM = 0; //motor apagado
  }
  else if(t >= TEMP_MAX){
    velocidadPWM = 255; //max velocidad
  }
  else{
    //escalar linealmente la velocidad entre t inicio y max
    velocidadPWM = map(t, TEMP_INICIO, TEMP_MAX,100, 255);
  }
  analogWrite(MOTOR_PIN, velocidadPWM);
  Serial.print("Velocidad PWM: ");
  Serial.println(velocidadPWM);
}