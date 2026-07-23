#pragma once

#include "Models.hpp"
#include "Observer.hpp"

#include <optional>
#include <string>
#include <vector>

namespace icu {

// Observer que reflete o maior risco atual da UTI através de uma cor:
//   Baixo -> Green, Baixo-médio -> Yellow, Médio -> Orange, Alto -> Red.
// Atualizado a cada novo score ou alta (Task 2).
class LED : public IICUObserver {
public:
    LED() = default;

    // Ponto do Observer Pattern: delega para updateRiskLevel.
    void update(const std::vector<DashboardRow>& rows) override { updateRiskLevel(rows); }

    // Método exigido pelo enunciado: imprime a cor atual do LED, por exemplo:
    //   Highest Risk Level: Red
    void updateRiskLevel(const std::vector<DashboardRow>& rows);

    // Estado observável (para testes / inspeção).
    std::optional<RiskLevel> highestRisk() const { return highestRisk_; }
    std::string colorName() const { return colorName(highestRisk_); }
    int updateCount() const { return updateCount_; }

private:
    static std::optional<RiskLevel> computeHighestRisk(const std::vector<DashboardRow>& rows);
    static std::string colorName(std::optional<RiskLevel> risk);

    std::optional<RiskLevel> highestRisk_;
    int updateCount_ = 0;
};

} // namespace icu
