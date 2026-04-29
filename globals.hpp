#ifndef GLOBALS_HPP
#define GLOBALS_HPP

#include <Arduino.h>

// =============================================================
// PINOUT
// Convenção: <COMPONENTE>_<FUNÇÃO>_PIN
// =============================================================
#define LED_BUILTIN_PIN     13  // LED onboard do Arduino Nano

#define MPU_INT_PIN          2  // MPU-6050: pino de interrupção

#define FLASH_CS_PIN        10  // W25Q128: Chip Select (SPI)

#define LORA_RX_PIN          5  // LoRa E32: RX do Arduino
#define LORA_TX_PIN          6 // LoRa E32: TX do Arduino
#define LORA_AUX_PIN         8  // LoRa E32: sinaliza módulo ocupado
#define LORA_M0_PIN          4  // LoRa E32: seleção de modo
#define LORA_M1_PIN          7  // LoRa E32: seleção de modo

#define SERVO_OUT_PIN 3         // Servo-motor: Saída de sinal
#define PYRO_OUTPUT 9           // Canal Pyro: Controle de mosfet

#define EJECTION_MODE_PYRO   0
#define EJECTION_MODE_SERVO  1
#define EJECTION_MODE        EJECTION_MODE_SERVO

// =============================================================
// CONFIGURAÇÕES DE EJEÇÃO DO PARAQUEDAS
// Definir qual tipo de atuação de ejeção será escolhida (Servo
// ou Pyro).
// A escolha do modo é realizada apenas por software
// =============================================================
//#define mode PYRO             // DEFINE MODO PYRO
#define mode SERVO              // DEFINE MODO SERVO
#define FLASH_CS_PIN        10  // W25Q128: Chip Select (SPI)

// LoRa
#define LORA_RX_PIN          5  // LoRa E32: RX do Arduino
#define LORA_TX_PIN          6  // LoRa E32: TX do Arduino

// =============================================================
// THRESHOLDS DA MÁQUINA DE ESTADOS
// Modificar esses valores conforme as características do
// foguete e do motor antes de cada voo.
// =============================================================

// Aceleração total mínima (m/s²) para detectar lançamento.
// 9.81 = 1g (peso do foguete em repouso).
// Valor atual: 2g acima da gravidade — ajustar conforme motor.
#define THRESHOLD_LANCAMENTO    29.43f  // m/s² (~3g)

// Aceleração total máxima (m/s²) abaixo da qual considera-se
// que o motor apagou e o foguete está em voo balístico.
// Valor atual: levemente acima de 1g (gravidade + margem).
#define THRESHOLD_MOTOR_APAGADO 12.0f   // m/s²

// Queda de altitude (metros) abaixo do pico registrado para
// confirmar que o apogeu foi atingido.
// Evita falsos positivos por oscilação do barômetro.
#define MARGEM_APOGEU           5.0f    // metros

// Altitude (metros) abaixo da qual considera-se que o foguete
// aterrissou. Relativa à altitude do BMP280 na inicialização.
#define THRESHOLD_SOLO          5.0f    // metros

// Pressão atmosférica padrão ao nível do mar (hPa)
// Usada como referência para cálculo de altitude pelo BMP280.
#define PRESSAO_NIVEL_MAR       1013.25f

// =============================================================
// ESTADOS DE VOO
// Representa a fase atual da missão.
// `uint8_t` como tipo base para economizar memória (1 byte).
// =============================================================
enum class EstadoVoo : uint8_t {
    ARMADO        = 0,  // Foguete na lança, pronto para ser lançado
    ALTA_ENERGIA  = 1,  // Aceleração maior que threshold
    BAIXA_ENERGIA = 2,  // Trajetória de subida balística
    QUEDA         = 3,  // Após detecção de apogeu
    ATERRISSADO   = 4   // Suspende coleta frequente de dados
};

// =============================================================
// DATA STORAGE STRUCT
// Snapshot completo do estado do foguete num dado instante.
// Esse é o registro que será gravado na memória flash.
// =============================================================
struct DadosVoo {
    // MPU-6050: aceleração linear (m/s²) e angular (°/s)
    float acelX, acelY, acelZ;
    float giroX, giroY, giroZ;
    unsigned long timestamp;     // ms desde o boot (millis())

    // BMP280: condições atmosféricas e altitude
    float pressao;      // hPa
    float temperatura;  // °C
    float altitude;     // metros

    // Contexto da missão
    unsigned long timestamp;  // ms desde o boot (millis())
    EstadoVoo estado;         // fase atual do voo
};

// =============================================================
// TELEMETRY PACKET STRUCT
// Pacote compacto enviado pelo LoRa ao solo.
// Separado do DadosVoo pois a telemetria pode ter frequência
// e conteúdo diferentes do log completo gravado na flash.
// TODO: DEFINIR QUAIS INFO. DEVEM SER TRANSMITIDAS.
// =============================================================
struct PacoteTelemetria {
    float acelX, acelY, acelZ;  // aceleração (m/s²)
    float giroX, giroY, giroZ;

    float altitude;             // altitude (metros)
    unsigned long timestamp;    // ms desde o boot
    EstadoVoo estado;           // fase atual do voo
};

// =============================================================
// VARIÁVEL GLOBAL COMPARTILHADA
// `extern` avisa o compilador que essa variável existe mas é
// definida em outro arquivo (sensors.cpp).
// Todos os módulos enxergam e escrevem/leem daqui.
// =============================================================
extern DadosVoo dadosVoo;

#endif // GLOBALS_HPP