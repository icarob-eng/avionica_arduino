// ejection.cpp
// Implementação do namespace Ejection.
//
// Este módulo controla o sistema de ejeção do foguete,
// podendo operar em dois modos:
//
//   1) PYRO  → Acionamento de carga pirotécnica via MOSFET
//   2) SERVO → Acionamento mecânico com servo motor
//
// O modo é definido em tempo de compilação via macro `mode`.
//
// Estratégia:
//   - Não utiliza delay() → sistema totalmente não-bloqueante
//   - Controle temporal via millis()
//   - Uso direto de registradores no modo PYRO para máxima velocidade
//
// Registradores utilizados (modo PYRO):
//
//   DDRB  → Data Direction Register do porto B
//           1 = saída, 0 = entrada
//           PB1 (D9) configurado como saída (Gate do MOSFET)
//
//   PORTB → Escrita nos pinos do porto B
//           Controla nível lógico do Gate do MOSFET
//
// ------------------------------------------------------------

#include "ejection.hpp"
#include "globals.hpp"
#include <Servo.h>

namespace Ejection {

  #if EJECTION_MODE == EJECTION_MODE_PYRO

    // --------------------------------------------------------
    // Variáveis de controle da sequência pirotécnica
    // --------------------------------------------------------
    static unsigned long lastPulseTime = 0; // Timestamp do último evento
    static uint8_t pulseCount = 0;          // Quantidade de pulsos já executados
    static bool isActuating = false;        // Indica se um pulso está ativo
    static bool stateHigh = false;          // Estado atual do pino (HIGH/LOW)

  #elif EJECTION_MODE == EJECTION_MODE_SERVO

    // Servo motor para ejeção mecânica
    Servo ejectionServo;

  #else
    #error "mode must be PYRO or SERVO"
  #endif

  // ------------------------------------------------------------
  // Inicialização do sistema de ejeção
  // ------------------------------------------------------------
  void setup(){

    #if EJECTION_MODE == EJECTION_MODE_PYRO

      // Configura PB1 (D9) como saída
      DDRB |= (1 << 1);

      // Garante nível inicial LOW no pino (MOSFET desligado)
      // (Nota: operação correta seria limpar o bit)
      PORTB &= ~(1 << 1);
    
      Serial.println("[Ejection] Sistema de ejeção iniciado no modo PYRO");

    #elif EJECTION_MODE == EJECTION_MODE_SERVO

      // Inicializa o servo no pino definido
      ejectionServo.attach(SERVO_OUT_PIN);

      // Define posição inicial (0 graus)
      ejectionServo.write(0);

      // Imprime no serial caso funcione
      Serial.println("[Ejection] Sistema de ejeção iniciado no modo SERVO ");

    #else
      Serial.println("[Ejection] Falha de definição de modo");
    #endif
  }

  // ------------------------------------------------------------
  // Executa a sequência de ejeção
  //
  // Retorno:
  //   false → ainda em execução
  //   true  → sequência concluída
  // ------------------------------------------------------------
  bool ejectionEvent() {

    #if EJECTION_MODE == EJECTION_MODE_PYRO
        // ----------------------------------------------------
        // Lógica pirotécnica:
        //   - 3 pulsos
        //   - 500 ms HIGH
        //   - 500 ms intervalo (LOW)
        //   - Total não bloqueante
        // ----------------------------------------------------

        unsigned long now = millis();

        if (pulseCount < 3) {

            // Início de um novo pulso
            if (!isActuating) {
                lastPulseTime = now;
                isActuating = true;
                stateHigh = true;

                // Seta PB1 HIGH → aciona Gate do MOSFET
                PORTB |= (1 << PB1);
            }

            // Controle temporal do pulso
            if (now - lastPulseTime >= 500) {

                if (stateHigh) {
                    // Fim do pulso HIGH → desliga MOSFET
                    PORTB &= ~(1 << PB1);

                    stateHigh = false;
                    lastPulseTime = now;

                } else {
                    // Intervalo LOW concluído
                    isActuating = false;
                    pulseCount++;
                }
            }

            return false; // Sequência ainda em execução
        }
        return true; // Todos os pulsos concluídos

    #elif EJECTION_MODE == EJECTION_MODE_SERVO
        // ----------------------------------------------------
        // Lógica com servo:
        //   Movimento: 0° ↔ 90°
        //   Repetições: 3 ciclos completos
        //
        // Nota:
        //   A biblioteca Servo utiliza Timer1/Timer2 para PWM,
        //   portanto não há necessidade de controle manual.
        // ----------------------------------------------------

        static uint8_t servoRepetitions = 0;   // Número de ciclos concluídos
        static unsigned long lastServoMove = 0;

        if (servoRepetitions < 3) {

            unsigned long now = millis();

            // Intervalo mínimo para movimento mecânico
            if (now - lastServoMove >= 300) {

                int currentPos = ejectionServo.read();

                // Alterna entre 0° e 90°
                ejectionServo.write(currentPos == 0 ? 90 : 0);

                lastServoMove = now;

                // Conta ciclo completo (90 → 0)
                if (currentPos == 90) {
                    servoRepetitions++;
                }
            }

            return false; // Ainda em execução
        }
        return true; // Sequência finalizada

    #else
        #error "mode must be PYRO or SERVO"
    #endif
  }

}