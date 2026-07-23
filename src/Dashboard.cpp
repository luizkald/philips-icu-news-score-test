#include "Dashboard.hpp"

#include "Console.hpp"

#include <iostream>
#include <string>

namespace icu {

namespace {

// Trunca/preenche uma string em uma largura fixa (para colunas alinhadas).
std::string fit(const std::string& s, std::size_t width) {
    if (s.size() > width) return s.substr(0, width);
    return s + std::string(width - s.size(), ' ');
}

} // namespace

void Dashboard::updateDashboard(const std::vector<DashboardRow>& rows) {
    ++updateCount_;
    lastRows_ = rows;
    render(rows);
}

void Dashboard::render(const std::vector<DashboardRow>& rows) const {
    const char* sep =
        "+------+------------+----------------------+-------+-------------+---------------------+";

    std::cout << "\n" << Console::bold << "ICU Dashboard" << Console::reset << "\n";
    std::cout << sep << "\n";
    std::cout << "| " << fit("Bed", 4)
              << " | " << fit("ID", 10)
              << " | " << fit("Patient", 20)
              << " | " << fit("Score", 5)
              << " | " << fit("Risk", 11)
              << " | " << fit("Timestamp", 19) << " |\n";
    std::cout << sep << "\n";

    if (rows.empty()) {
        std::cout << "| " << fit("(no patients admitted)", 78) << " |\n";
        std::cout << sep << "\n";
        return;
    }

    for (const auto& row : rows) {
        std::string score = row.hasMeasurement ? std::to_string(row.latestScore) : "-";
        std::string risk = row.hasMeasurement ? toString(row.latestRisk) : "-";
        std::string ts = row.hasMeasurement ? formatTimestamp(row.timestamp) : "-";
        const char* riskColor = row.hasMeasurement ? Console::colorFor(row.latestRisk)
                                                    : Console::reset;

        std::cout << "| " << fit(std::to_string(row.bedNumber), 4)
                  << " | " << fit(row.patientId, 10)
                  << " | " << fit(row.patientName, 20)
                  << " | " << fit(score, 5)
                  << " | " << riskColor << fit(risk, 11) << Console::reset
                  << " | " << fit(ts, 19) << " |\n";
    }
    std::cout << sep << "\n";
}

} // namespace icu
