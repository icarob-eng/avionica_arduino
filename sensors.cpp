// sensors.cpp
// Implementação das funções do namespace Sensor.
// Utiliza as bibliotecas Adafruit MPU6050 e Adafruit BMP280.

#include "sensors.hpp"
#include "globals.hpp"
#include <Adafruit_MPU6050.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_Sensor.h>

// ------------------------------------------------------------
// Objetos das bibliotecas — ficam aqui, invisíveis ao resto
// do projeto. Só sensors.cpp sabe que eles existem.
// ------------------------------------------------------------
static Adafruit_MPU6050 mpu;
static Adafruit_BMP280 bmp;

namespace Sensor {

    bool setup() {
        // Tenta inicializar o MPU-6050 no endereço padrão 0x68
        if (!mpu.begin()) {
            Serial.println(F("[SENSOR] MPU-6050 nao encontrado!"));
            return false;
        }

        // Tenta inicializar o BMP280 no endereço 0x76
        if (!bmp.begin(0x76)) {
            Serial.println(F("[SENSOR] BMP280 nao encontrado!"));
            return false;
        }

        // --- Configuração do MPU-6050 ---
        // Escala do acelerômetro: ±8g — bom para capturar
        // a aceleração do lançamento sem saturar o sensor
        mpu.setAccelerometerRange(MPU6050_RANGE_8_G);

        // Escala do giroscópio: ±500°/s — suficiente para
        // rotações moderadas do foguete
        mpu.setGyroRange(MPU6050_RANGE_500_DEG);

        // Filtro passa-baixa: reduz ruído de alta frequência
        // nas leituras sem atrasar demais a resposta
        mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

        // --- Configuração do BMP280 ---
        // Modo normal: sensor faz medições continuamente
        // Oversampling x4 para pressão e temperatura: melhor
        // precisão com custo moderado de tempo
        bmp.setSampling(Adafruit_BMP280::MODE_NORMAL);

        Serial.println(F("[SENSOR] MPU-6050 e BMP280 inicializados."));
        return true;
    }

    void readData() {
        // --- Leitura do MPU-6050 ---
        // A biblioteca retorna os dados em eventos tipados
        sensors_event_t accel, gyro, temp;
        mpu.getEvent(&accel, &gyro, &temp);

        // Escreve na variável global compartilhada
        dadosVoo.acelX = accel.acceleration.x;
        dadosVoo.acelY = accel.acceleration.y;
        dadosVoo.acelZ = accel.acceleration.z;

        dadosVoo.giroX = gyro.gyro.x;
        dadosVoo.giroY = gyro.gyro.y;
        dadosVoo.giroZ = gyro.gyro.z;

        // --- Leitura do BMP280 ---
        dadosVoo.pressao     = bmp.readPressure() / 100.0F; // Pa → hPa
        dadosVoo.temperatura = bmp.readTemperature();
        dadosVoo.altitude    = bmp.readAltitude(PRESSAO_NIVEL_MAR);   // pressão ao nível do mar padrão

        // --- Timestamp ---
        dadosVoo.timestamp = millis();
    }

} 