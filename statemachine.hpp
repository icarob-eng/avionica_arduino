// statemachine.hpp
// Declara a máquina de estados do voo e seus thresholds.
//
// Transições de estado:
//   ARMADO → ALTA_ENERGIA:
//     aceleração total > THRESHOLD_LANCAMENTO
//   ALTA_ENERGIA → BAIXA_ENERGIA:
//     aceleração total < THRESHOLD_MOTOR_APAGADO
//   BAIXA_ENERGIA → QUEDA:
//     altitude < altitudeMaxima - MARGEM_APOGEU
//   QUEDA → ATERRISSADO:
//     altitude < THRESHOLD_SOLO

#ifndef STATEMACHINE_HPP
#define STATEMACHINE_HPP

#include "globals.hpp"

namespace StateMachine {

    /**
     * Inicializa a máquina de estados.
     * Define o estado inicial como ARMADO e zera
     * a altitude máxima registrada.
     */
    void setup();

    /**
     * Avalia os dados atuais de `dadosVoo` e realiza
     * a transição de estado caso as condições sejam atendidas.
     * Deve ser chamada a cada ciclo de leitura dos sensores.
     */
    void update();

    /**
     * Flag de coleta de dados
     *  Ela deve se manter falsa enquanto não há dados sendo coletados.
     *  Ela deve se manter verdadeira quando os dados são coletados
     */
    extern bool coletaAtiva; 

}

#endif // STATEMACHINE_HPP