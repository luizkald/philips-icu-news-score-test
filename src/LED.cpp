#include "LED.hpp"

#include "Console.hpp"

#include <iostream>

namespace icu {

std::optional<RiskLevel> LED::computeHighestRisk(const std::vector<DashboardRow>& rows) {
    std::optional<RiskLevel> highest;
    for (const auto& row : rows) {
        if (!row.hasMeasurement) continue;
        if (!highest || row.latestRisk > *highest) {
            highest = row.latestRisk;
        }
    }
    return highest;
}

std::string LED::colorName(std::optional<RiskLevel> risk) {
    if (!risk) return "Off";
    switch (*risk) {
        case RiskLevel::Low:       return "Green";
        case RiskLevel::LowMedium: return "Yellow";
        case RiskLevel::Medium:    return "Orange";
        case RiskLevel::High:      return "Red";
    }
    return "Off";
}

void LED::updateRiskLevel(const std::vector<DashboardRow>& rows) {
    ++updateCount_;
    highestRisk_ = computeHighestRisk(rows);

    const char* color = highestRisk_ ? Console::colorFor(*highestRisk_) : Console::dim;
    std::cout << Console::bold << color
              << "Highest Risk Level: " << colorName(highestRisk_)
              << Console::reset << "\n";
}

} // namespace icu
