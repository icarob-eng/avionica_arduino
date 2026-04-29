// storage.cpp
// Implementação do namespace Storage para gerenciamento da
// memória flash W25Q128 via SPI, usando a biblioteca SPIMemory.
//
// Estratégia de endereçamento sequencial:
//   endereço = contadorRegistros * sizeof(DadosVoo)
// Um contador em RAM rastreia a posição de escrita atual.
// Atenção: o contador é zerado ao resetar o Arduino.
// Sempre executar formatMemory() antes de um novo voo.

#include "storage.hpp"
#include "globals.hpp"
#include <SPIMemory.h>

// ------------------------------------------------------------
// Objeto da biblioteca — privado a este arquivo.
// ------------------------------------------------------------
static SPIFlash flash(FLASH_CS_PIN);

// ------------------------------------------------------------
// Contador de registros gravados — mantido em RAM.
// Zerado ao ligar; não sobrevive a um reset.
// ------------------------------------------------------------
static uint32_t contadorRegistros = 0;

// Buffer interno (3 amostras) - reduzido ainda mais para economizar RAM
#define BUFFER_SIZE 3
static DadosVoo buffer[BUFFER_SIZE];
static uint8_t bufferIndex = 0;

namespace Storage {

    bool setup() {
        if (!flash.begin()) {
            Serial.println("[STORAGE] Flash W25Q128 nao encontrada!");
            return false;
        }

        Serial.print("[STORAGE] Flash inicializada. JEDEC ID: ");
        Serial.println(flash.getJEDECID(), HEX);
        return true;
    }

    void formatMemory() {
        Serial.println("[STORAGE] Apagando flash... (pode demorar ~20s)");

        if (flash.eraseChip()) {
            contadorRegistros = 0;
            Serial.println("[STORAGE] Flash apagada com sucesso.");
        } else {
            Serial.println("[STORAGE] Falha ao apagar flash!");
        }
    }
    
    void push(const DadosVoo& dado) {
        buffer[bufferIndex++] = dado;

        if (bufferIndex >= BUFFER_SIZE) {
            flush();
        }
    }

    void saveData() {
        push(dadosVoo);
    }

    void flush() {
        if (bufferIndex == 0) return;

        uint32_t bytes = bufferIndex * sizeof(DadosVoo);
        uint32_t endereco = contadorRegistros * sizeof(DadosVoo);

        if (endereco + bytes > flash.getCapacity()) {
            Serial.println("[STORAGE] Memoria cheia!");
            return;
        }

        bool sucesso = flash.writeByteArray(
            endereco,
            (uint8_t*)buffer,
            bytes
        );

        if (sucesso) {
            contadorRegistros += bufferIndex;
            bufferIndex = 0;
        } else {
            Serial.println("[STORAGE] Falha na gravacao!");
        }
    }

    void readData() {
        if (contadorRegistros == 0) {
            Serial.println("[STORAGE] Nenhum registro gravado.");
            return;
        }

        Serial.print("[STORAGE] Iniciando dump de ");
        Serial.print(contadorRegistros);
        Serial.println(" registros...");

        // Cabeçalho CSV — adequado para importar direto em
        // Excel, LibreOffice Calc ou pandas (Python)
        Serial.println(
            "indice,timeStamp,estado,"
            "acelX,acelY,acelZ,"
            "giroX,giroY,giroZ,"
            "pressao,temperatura,altitude"
        );

        DadosVoo temp;

        for (uint32_t i = 0; i < contadorRegistros; i++) {
            uint32_t endereco = i * sizeof(DadosVoo);

            if (!flash.readByteArray(endereco, (uint8_t*)&temp, sizeof(DadosVoo))) {
                Serial.print("[STORAGE] Falha ao ler registro #");
                Serial.println(i);
                continue; // pula esse registro e tenta o próximo
            }

            // Imprime uma linha CSV por registro
            Serial.print(i);                          Serial.print(",");
            Serial.print(temp.timestamp);             Serial.print(",");
            Serial.print((uint8_t)temp.estado);       Serial.print(",");
            Serial.print(temp.acelX, 4);              Serial.print(",");
            Serial.print(temp.acelY, 4);              Serial.print(",");
            Serial.print(temp.acelZ, 4);              Serial.print(",");
            Serial.print(temp.giroX, 4);              Serial.print(",");
            Serial.print(temp.giroY, 4);              Serial.print(",");
            Serial.print(temp.giroZ, 4);              Serial.print(",");
            Serial.print(temp.pressao, 2);            Serial.print(",");
            Serial.print(temp.temperatura, 2);        Serial.print(",");
            Serial.println(temp.altitude, 2);
        }

        Serial.println("[STORAGE] Dump concluido.");
    }

}