/* Aviônica de Foguete — Arduino Nano
 *
 * Libs necessárias:
 *   Adafruit MPU6050
 *   Adafruit BMP280
 *   Adafruit Unified Sensor
 *   SPIMemory
 *   EByte LoRa E32
 *
 * Frequências de operação:
 *   Timer1 (CTC, 100Hz) → base de tempo do sistema
 *   A cada 4 ticks  (25Hz) → leitura + gravação
 *   A cada 20 ticks  (5Hz) → transmissão LoRa
 *
 * Custo por ciclo de gravação:
 *   readData()  = ~4.480 clocks (I²C 400kHz, 14 bytes)
 *   saveData()  = ~720 clocks   (SPI 8MHz, 45 bytes)
 *   Total       = ~5.200 clocks (<<  640.000 clocks do período)
 *
 * Custo por ciclo de transmissão:
 *   sendPacket() = ~416.666 clocks (UART 9600bps, 25 bytes)
 *   Total        = ~416.666 clocks (<< 3.200.000 clocks do período)
 *
 * Registradores utilizados:
 *   TCCR1A, TCCR1B → Timer1 em modo CTC
 *   OCR1A           → valor de comparação (100Hz)
 *   TIMSK1          → habilita interrupção do Timer1
 *   EICRA, EIMSK    → interrupção externa INT0 (MPU-6050)
 *   DDRx, PORTx     → direção e controle de pinos (LoRa)
 *   PIND            → polling do AUX do LoRa
 */

#include "globals.hpp"
#include "sensors.hpp"
#include "storage.hpp"
#include "telemetry.hpp"
#include "statemachine.hpp"
#include "ejection.hpp"

// =============================================================
// DEFINIÇÃO DA VARIÁVEL GLOBAL
// Declarada como extern em globals.hpp, definida aqui.
// Todos os módulos apontam para essa única instância.
// =============================================================
DadosVoo dadosVoo;

// =============================================================
// VARIÁVEIS DE CONTROLE DO LOOP
// `volatile` obrigatório: modificadas dentro de ISRs.
// =============================================================

// Contador de ticks do Timer1 (100Hz).
// Controla a cadência de leitura e transmissão.
static volatile uint8_t contadorTicks = 0;

// Flag setada pela ISR do MPU-6050 (INT0).
// Indica que uma nova amostra está disponível para leitura.
static volatile bool leituraPendente = false;

// =============================================================
// ISR — Timer2 Compare Match A (100Hz)
// Chamada a cada 10ms pelo Timer2 em modo CTC.
// Incrementa o contador de ticks que controla o loop.
//
// Registradores configurados em setup():
//   TCCR2A: WGM21=1 (CTC)
//   TCCR2B: CS22=1, CS20=1 (prescaler 1024)
//   OCR2A:  155 → f ≈ 100Hz
//   TIMSK2: OCIE2A=1 (habilita interrupção)
// =============================================================
ISR(TIMER2_COMPA_vect) {
    contadorTicks++;
    if (contadorTicks >= 100) contadorTicks = 0;
}

// =============================================================
// ISR — INT0 (MPU-6050, pino D2)
// Chamada quando o MPU-6050 sinaliza nova amostra disponível.
// Apenas seta a flag — leitura real ocorre no loop principal.
//
// Registradores configurados em setup():
//   EICRA: ISC01=1, ISC00=1 (borda de subida)
//   EIMSK: INT0=1 (habilita INT0)
// =============================================================
ISR(INT0_vect) {
    leituraPendente = true;
}

// =============================================================
// CONFIGURAÇÃO DO TIMER2 EM MODO CTC
//
// Modo CTC (Clear Timer on Compare Match):
//   O contador TCNT2 incrementa a cada clock/prescaler.
//   Quando TCNT2 == OCR2A, dispara a interrupção e zera.
//
// Cálculo do OCR2A para 100Hz:
//   f_alvo    = 100 Hz
//   prescaler = 1024
//   OCR2A     = (16.000.000 / (1024 × 100)) - 1 ≈ 155
//
// TCCR2A = (1 << WGM21) → modo CTC
// TCCR2B = (1 << CS22) | (1 << CS20) → prescaler 1024
// TIMSK2:
//   OCIE2A = 1 → habilita interrupção por comparação
// =============================================================
static void configurarTimer2() {
    TCCR2A = (1 << WGM21);
    TCCR2B = (1 << CS22) | (1 << CS20);
    OCR2A  = 155;
    TIMSK2 = (1 << OCIE2A);
}

// =============================================================
// CONFIGURAÇÃO DA INTERRUPÇÃO EXTERNA INT0 (MPU-6050)
//
// EICRA: ISC01=1, ISC00=1 → dispara na borda de subida.
//   O MPU-6050 mantém INT LOW e vai HIGH quando dado pronto.
// EIMSK: INT0=1 → habilita a interrupção no pino D2.
// =============================================================
static void configurarINT0() {
    EICRA |= (1 << ISC01) | (1 << ISC00);
    EIMSK |= (1 << INT0);
}

// =============================================================
// SETUP
// =============================================================
void setup() {
    Serial.begin(9600);

    configurarTimer2();
    configurarINT0();

    // Inicializa os módulos — aborta se algum falhar
    if (!Storage::setup()) {
        Serial.println("[MAIN] Falha no storage. Sistema abortado.");
        while (true);
    }

    if (!Telemetry::setup()) {
        Serial.println("[MAIN] Falha na telemetria. Sistema abortado.");
        while (true);
    }

    if (!Sensor::setup()) {
        Serial.println("[MAIN] Falha nos sensores. Sistema abortado.");
        while (true);
    }

    // Inicializando sistema de ejeção com configuração escolhida
    Ejection::setup();
    
    // Inicializando maquina de estados finitos
    StateMachine::setup();

    // Apaga a flash antes do voo
    Storage::formatMemory();

    // Habilita interrupções globais
    sei();

    Serial.println("[MAIN] Sistema inicializado. Aguardando lancamento.");
}

// =============================================================
// LOOP PRINCIPAL
//
// Cadência controlada pelo contador de ticks do Timer1 (100Hz):
//   tick % 4  == 0 → 25Hz → leitura + gravação
//   tick % 20 == 0 →  5Hz → transmissão LoRa
//
// No estado ATERRISSADO a coleta frequente é suspensa:
//   apenas transmissão a 5Hz para confirmar posição.
// =============================================================
void loop() {
    // Captura o tick atual atomicamente para evitar que
    // uma interrupção o modifique durante a leitura
    uint8_t tick;
    cli();
    tick = contadorTicks;
    sei();

    // --- Ciclo de leitura e gravação (25Hz) ---
    if (leituraPendente && (tick % 4 == 0)) {
        leituraPendente = false;

        if (dadosVoo.estado != EstadoVoo::ATERRISSADO) {
            Sensor::readData();
            StateMachine::update();
            Storage::saveData();
        }
    }

    // --- Ciclo de transmissão (5Hz) ---
    if (tick % 20 == 0) {
        Telemetry::sendPacket();
    }
}