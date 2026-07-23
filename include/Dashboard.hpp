#pragma once

#include "Models.hpp"
#include "Observer.hpp"

#include <vector>

namespace icu {

// Observer que renderiza um painel textual (tabela) com o estado da UTI:
// leito, paciente, nível de risco e horário da última atualização (Task 2).
// É atualizado automaticamente a cada novo score ou alta.
class Dashboard : public IICUObserver {
public:
    Dashboard() = default;

    // Ponto do Observer Pattern: delega para updateDashboard.
    void update(const std::vector<DashboardRow>& rows) override { updateDashboard(rows); }

    // Método exigido pelo enunciado: imprime as informações no console.
    void updateDashboard(const std::vector<DashboardRow>& rows);

    // Renderiza a tabela a partir de um snapshot arbitrário. Reutilizável pela
    // CLI na opção "All patients" (sem alterar o estado do observer).
    void render(const std::vector<DashboardRow>& rows) const;

    const std::vector<DashboardRow>& lastRows() const { return lastRows_; }
    int updateCount() const { return updateCount_; }

private:
    int updateCount_ = 0;
    std::vector<DashboardRow> lastRows_;
};

} // namespace icu
