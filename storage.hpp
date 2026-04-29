// storage.hpp
// Declara as funções de gerenciamento da memória flash W25Q128
// via protocolo SPI, utilizando a biblioteca SPIMemory.
//
// Capacidade de armazenamento:
//   - W25Q128: 16MB = 16.777.216 bytes
//   - sizeof(DadosVoo) ≈ 45 bytes
//   - Registros máximos: ~372.827 registros
//
// Tempo de voo máximo estimado:
//   - Frequência de amostragem: 25 Hz (limitada pelo BMP280)
//   - Tempo máximo: 372.827 / 25 ≈ 14.913s ≈ 4,1 horas
//   - Para um voo de foguete típico (~120s), a flash é mais
//     que suficiente.

#ifndef STORAGE_HPP
#define STORAGE_HPP

#include "globals.hpp"

namespace Storage {
  /**
  * Inicializa a memória flash W25Q128.
  * Deve ser chamada uma vez no setup() do .ino.
  * @return true se a memória respondeu corretamente.
  */
  bool setup();

  /**
  * Apaga completamente a memória flash, resetando todos os
  * blocos para o estado padrão (todos os bits em 1).
  * Operação lenta (~20s). Usar apenas em solo, antes do voo.
  * Necessário pois a flash só grava bits de 1 → 0,
  * sendo o apagamento a única forma de restaurar bits para 1.
  */
  void formatMemory();
  
  /**
  * Adiciona um novo dado ao buffer interno.
  * Quando o buffer enche (10 amostras), grava automaticamente na flash.
  */
  void push(const DadosVoo& dado);

  /**
  * Adiciona o dado de voo atual ao buffer e grava quando necessário.
  * Mantém compatibilidade com chamadas existentes em avionica_arduino.ino.
  */
  void saveData();

  /**
  * Força a gravação do buffer mesmo que não esteja cheio.
  * Deve ser chamado ao final do voo.
  */
  void flush();
  
  /**
  * Lê e imprime via Serial todos os registros gravados
  * na memória flash, do primeiro ao último.
  * Usar em solo após o voo para recuperação completa dos dados.
  * Formato: CSV com cabeçalho, adequado para importar em
  * planilhas ou ferramentas de análise.
  */
  void readData();
}

#endif // STORAGE_HPP