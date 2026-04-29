// statemachine.cpp
// Implementação da máquina de estados do voo.
//
// A altitude máxima é rastreada internamente para detecção
// do apogeu — ponto onde a altitude começa a cair após
// atingir o valor máximo da trajetória balística.

#include "statemachine.hpp"
#include "globals.hpp"
#include <math.h>
#include "ejection.hpp"

// ------------------------------------------------------------
// Altitude máxima registrada durante o voo.
// Atualizada a cada leitura enquanto o foguete sobe.
// Usada para detectar o apogeu na fase BAIXA_ENERGIA.
// ------------------------------------------------------------
static float altitudeMaxima = 0.0;

namespace StateMachine {

    void setup() {
        dadosVoo.estado = EstadoVoo::ARMADO;
        altitudeMaxima  = 0.0;
        Serial.println(F("[STATE] Maquina de estados inicializada: ARMADO"));
    }

    void update() {
        // Calcula a aceleração total (módulo do vetor 3D)
        // Usado para detectar lançamento e apagamento do motor.
        // sqrt(x²+y²+z²) em m/s²
        float acelTotal = sqrt(
            dadosVoo.acelX * dadosVoo.acelX +
            dadosVoo.acelY * dadosVoo.acelY +
            dadosVoo.acelZ * dadosVoo.acelZ
        );

        switch (dadosVoo.estado) {

            case EstadoVoo::ARMADO:
                // Detecta lançamento quando a aceleração total
                // supera o threshold definido em globals.hpp
                if (acelTotal > THRESHOLD_LANCAMENTO) {
                    dadosVoo.estado = EstadoVoo::ALTA_ENERGIA;
                    Serial.println(F("[STATE] ARMADO -> ALTA_ENERGIA"));
                }
                break;

            case EstadoVoo::ALTA_ENERGIA:
                // Atualiza altitude máxima enquanto sobe
                if (dadosVoo.altitude > altitudeMaxima)
                    altitudeMaxima = dadosVoo.altitude;

                // Motor apagou: aceleração cai abaixo do threshold
                if (acelTotal < THRESHOLD_MOTOR_APAGADO) {
                    dadosVoo.estado = EstadoVoo::BAIXA_ENERGIA;
                    Serial.println(F("[STATE] ALTA_ENERGIA -> BAIXA_ENERGIA"));
                }
                break;

            case EstadoVoo::BAIXA_ENERGIA:
                // Continua rastreando altitude máxima
                if (dadosVoo.altitude > altitudeMaxima)
                    altitudeMaxima = dadosVoo.altitude;

                // Apogeu detectado: altitude caiu mais que a
                // margem definida abaixo do pico registrado
                if (dadosVoo.altitude < altitudeMaxima - MARGEM_APOGEU) {
                    dadosVoo.estado = EstadoVoo::QUEDA;
                    Serial.println(F("[STATE] BAIXA_ENERGIA -> QUEDA"));
                    if(Ejection::ejectionEvent()) Serial.println(F("[EVENT] PARAQUEDAS ACIONADO"));
                }
                break;

            case EstadoVoo::QUEDA:
                // Solo detectado: altitude abaixo do threshold
                if (dadosVoo.altitude < THRESHOLD_SOLO) {
                    dadosVoo.estado = EstadoVoo::ATERRISSADO;
                    Serial.println(F("[STATE] QUEDA -> ATERRISSADO"));
                }
                break;

            case EstadoVoo::ATERRISSADO:
                // Estado final — nenhuma transição possível.
                // O loop principal reduz a frequência de coleta.
                break;
        }
    }

}