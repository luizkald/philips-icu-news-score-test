#pragma once

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace icu {

// Nível de consciência (escala AVPU + New Confusion).
// Apenas Alert é considerado normal; qualquer outro estado pontua 3.
enum class ConsciousnessLevel {
    Alert,
    Voice,
    Pain,
    Unresponsive,
    NewConfusion
};

// Classificação de risco clínico derivada do NEWS Score.
enum class RiskLevel {
    Low,
    LowMedium,
    Medium,
    High
};

// Conjunto de sinais vitais de uma medição.
struct VitalSigns {
    int respiratoryRate = 0;
    int oxygenSaturation = 0;
    bool supplementalOxygen = false;
    double temperature = 0.0;
    int systolicBloodPressure = 0;
    int heartRate = 0;
    ConsciousnessLevel consciousness = ConsciousnessLevel::Alert;
};

// Resultado de um cálculo NEWS.
struct NewsScore {
    int total = 0;
    RiskLevel risk = RiskLevel::Low;
    std::chrono::system_clock::time_point timestamp{};
    VitalSigns vitalSigns{};
};

// Paciente internado, associado a um leito, com histórico de medições.
struct Patient {
    std::string id;
    std::string name;
    int bedNumber = 0;
    std::vector<NewsScore> history;
};

// Representação plana para os observers, desacoplada de Patient.
struct DashboardRow {
    int bedNumber = 0;
    std::string patientId;
    std::string patientName;
    RiskLevel latestRisk = RiskLevel::Low;
    int latestScore = 0;
    std::chrono::system_clock::time_point timestamp{};
    bool hasMeasurement = false;
};

// Conversões de enum para texto (independentes de console/cores).
inline std::string toString(RiskLevel level) {
    switch (level) {
        case RiskLevel::Low:       return "Low";
        case RiskLevel::LowMedium: return "Low-Medium";
        case RiskLevel::Medium:    return "Medium";
        case RiskLevel::High:      return "High";
    }
    return "Unknown";
}

inline std::string toUpperLabel(RiskLevel level) {
    switch (level) {
        case RiskLevel::Low:       return "LOW";
        case RiskLevel::LowMedium: return "LOW-MEDIUM";
        case RiskLevel::Medium:    return "MEDIUM";
        case RiskLevel::High:      return "HIGH";
    }
    return "UNKNOWN";
}

// Formata um timestamp como "09 May 2025 12:00" (horário local), no formato do
// enunciado (dia mês-abreviado ano hora:min).
inline std::string formatTimestamp(const std::chrono::system_clock::time_point& tp) {
    std::time_t t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%d %b %Y %H:%M");
    return oss.str();
}

inline std::string toString(ConsciousnessLevel level) {
    switch (level) {
        case ConsciousnessLevel::Alert:        return "Alert";
        case ConsciousnessLevel::Voice:        return "Voice";
        case ConsciousnessLevel::Pain:         return "Pain";
        case ConsciousnessLevel::Unresponsive: return "Unresponsive";
        case ConsciousnessLevel::NewConfusion: return "New Confusion";
    }
    return "Unknown";
}

} // namespace icu
