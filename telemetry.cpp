// telemetry.cpp
// Implementação do namespace Telemetry.
//
// Ausência de ISR para recepção:
//   Nessa aplicação o Arduino opera exclusivamente como
//   transmissor de telemetria. O pino AUX do E32 é monitorado
//   por polling direto no registrador PINB — custo de 3 clocks
//   por iteração, negligenciável quando o módulo está livre.
//   A decisão de não usar ISR no AUX preserva INT0 (D2) para
//   o MPU-6050, onde a aquisição determinística tem prioridade.
//
// Registradores utilizados:
//
//   DDRD  → Data Direction Register porto D.
//           D4 (M0) e D7 (M1) → saída
//
//   DDRB  → Data Direction Register porto B.
//           D8 (AUX) → entrada
//
//   PORTD → Escrita nos pinos de saída do porto D.
//           Usado para setar M0=0, M1=0 (modo normal).
//
//   PORTB → Pull-up interno no AUX via bit 0.
//
//   PINB  → Leitura do estado atual dos pinos do porto B.
//           PINB & (1 << 0): lê AUX (D8).
//           3 clocks por leitura (IN + ANDI + BREQ).

#include "telemetry.hpp"
#include "globals.hpp"
#include <SoftwareSerial.h>

// ------------------------------------------------------------
// SoftwareSerial para comunicação com o E32.
// Custo de transmissão: 1 bit a cada 1/9600s
// Para sizeof(PacoteTelemetria) ≈ 25 bytes:
//   25 × 10 bits (8 dados + start + stop) = 250 bits
//   250 / 9600 = 26.04ms = ~416.666 clocks
// ------------------------------------------------------------
static SoftwareSerial loraSerial(LORA_RX_PIN, LORA_TX_PIN);

namespace Telemetry {

    bool setup() {
        loraSerial.begin(9600);

        // --- Configuração de pinos via registradores ---

        // DDRD: M0 (D4) e M1 (D7) como saída
        DDRD |=  (1 << 4);  // M0 → saída
        DDRD |=  (1 << 7);  // M1 → saída

        // DDRB: AUX (D8) como entrada
        DDRB &= ~(1 << 0);  // AUX → entrada

        // PORTB: habilita pull-up interno no AUX (D8)
        PORTB |= (1 << 0);

        // PORTD: M0=0, M1=0 → modo normal de operação
        PORTD &= ~(1 << 4); // M0 = 0
        PORTD &= ~(1 << 7); // M1 = 0

        // Aguarda AUX HIGH via polling — módulo pronto
        unsigned long inicio = millis();
        while (!(PINB & (1 << 0))) {
            if (millis() - inicio > 1000) {
                Serial.println(F("[TELEMETRY] Timeout: modulo nao respondeu."));
                return false;
            }
        }

        Serial.println(F("[TELEMETRY] LoRa E32 inicializado."));
        return true;
    }

    void sendPacket() {
        // --- Verificação não-bloqueante do AUX ---
        if (digitalRead(LORA_AUX_PIN) != HIGH) {
            return; // Módulo ocupado, tenta novamente no próximo ciclo
        }

        // Monta o pacote a partir da variável global dadosVoo
        PacoteTelemetria pacote;
        pacote.acelX     = dadosVoo.acelX;
        pacote.acelY     = dadosVoo.acelY;
        pacote.acelZ     = dadosVoo.acelZ;
        pacote.altitude  = dadosVoo.altitude;
        pacote.pressao   = dadosVoo.pressao;
        pacote.timestamp = dadosVoo.timestamp;
        pacote.estado    = dadosVoo.estado;

        // Serializa o struct como array de bytes e transmite.
        // Custo: ~416.666 clocks para 25 bytes a 9600bps.
        // O receptor deve usar PacoteTelemetria para desserializar.
        loraSerial.write((uint8_t*)&pacote, sizeof(PacoteTelemetria));
    }

    void rcvPacket() {
        // Recepção não implementada nessa versão.
        // Justificativa: ver cabeçalho telemetry.hpp.
        // Função mantida no namespace para compatibilidade
        // com o esqueleto definido pelo gerente do projeto.
    }

}