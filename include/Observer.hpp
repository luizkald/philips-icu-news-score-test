#pragma once

#include "Models.hpp"

#include <vector>

namespace icu {

// Interface do Observer Pattern. Os observers recebem um snapshot plano
// (DashboardRow) e não conhecem a representação interna de Patient nem a CLI.
class IICUObserver {
public:
    virtual ~IICUObserver() = default;

    // Chamado pela ICU após qualquer alteração relevante (novo score ou alta).
    virtual void update(const std::vector<DashboardRow>& rows) = 0;
};

} // namespace icu
