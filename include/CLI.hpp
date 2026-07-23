#pragma once

#include "Dashboard.hpp"
#include "ICU.hpp"

#include <iostream>
#include <string>

namespace icu {

// CLI é a camada de interação (console). Apenas lê/valida entrada, exibe
// resultados e delega toda a regra de negócio à ICU. Não calcula NEWS.
// O Dashboard e o LED são observers da ICU: atualizam-se sozinhos a cada score
// ou alta. A CLI usa o Dashboard também para renderizar a tabela na opção 'A'.
class CLI {
public:
    CLI(ICU& icu, Dashboard& dashboard,
        std::istream& in = std::cin, std::ostream& out = std::cout);

    // Loop principal do menu; retorna quando o usuário escolhe sair (X).
    void run();

private:
    void printMenu();
    void handleNewScore();
    void handleHistory();
    void handleAllPatients();
    void handleDischarge();

    // Leitura resiliente: entrada inválida não encerra a aplicação.
    bool readLine(const std::string& prompt, std::string& out);
    bool readInt(const std::string& prompt, int& out);

    // Lê os sinais vitais na ordem da tabela. Retorna false em EOF; `valid`
    // indica se todos os campos foram numéricos/reconhecidos e nas faixas
    // válidas (as faixas numéricas são checadas por NewsCalculator::isValid).
    bool readVitalSigns(VitalSigns& out, bool& valid);

    ICU& icu_;
    Dashboard& dashboard_;
    std::istream& in_;
    std::ostream& out_;
};

} // namespace icu
