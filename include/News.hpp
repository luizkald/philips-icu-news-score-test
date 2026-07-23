#pragma once

#include "Models.hpp"

namespace icu {

// NewsCalculator concentra a regra de negócio do protocolo NEWS (National
// Early Warning Score). É pura: não lê console, não conhece pacientes nem
// leitos, não armazena histórico e não formata interface.
class NewsCalculator {
public:
    // Faixas válidas de entrada (conforme o enunciado do NEWS).
    static constexpr int    kRespiratoryRateMin = 0,   kRespiratoryRateMax = 150;
    static constexpr int    kOxygenSaturationMin = 50, kOxygenSaturationMax = 100;
    static constexpr double kTemperatureMin = 20.0,    kTemperatureMax = 50.0;
    static constexpr int    kSystolicMin = 0,          kSystolicMax = 300;
    static constexpr int    kHeartRateMin = 15,        kHeartRateMax = 250;

    // Verdadeiro apenas se todos os sinais vitais numéricos estão nas faixas
    // válidas. A consciência é um enum, sempre válida.
    static bool isValid(const VitalSigns& vitalSigns);

    // Produz um NewsScore completo (total, risco, timestamp e sinais vitais).
    static NewsScore calculate(const VitalSigns& vitalSigns);

private:
    static int scoreRespiratoryRate(int respiratoryRate);
    static int scoreOxygenSaturation(int oxygenSaturation);
    static int scoreSupplementalOxygen(bool supplementalOxygen);
    static int scoreTemperature(double temperature);
    static int scoreSystolicBloodPressure(int systolicBloodPressure);
    static int scoreHeartRate(int heartRate);
    static int scoreConsciousness(ConsciousnessLevel consciousness);

    // Classifica o risco a partir do total e da maior pontuação individual.
    static RiskLevel classify(int total, int highestSingleParam);
};

} // namespace icu
