#include "News.hpp"

#include <algorithm>

namespace icu {

int NewsCalculator::scoreRespiratoryRate(int rr) {
    if (rr <= 8)  return 3;
    if (rr <= 11) return 1;
    if (rr <= 20) return 0;
    if (rr <= 24) return 2;
    return 3; // >= 25
}

int NewsCalculator::scoreOxygenSaturation(int spo2) {
    if (spo2 <= 91) return 3;
    if (spo2 <= 93) return 2;
    if (spo2 <= 95) return 1;
    return 0; // >= 96
}

int NewsCalculator::scoreSupplementalOxygen(bool supplementalOxygen) {
    return supplementalOxygen ? 2 : 0;
}

int NewsCalculator::scoreTemperature(double t) {
    if (t <= 35.0) return 3;
    if (t <= 36.0) return 1;
    if (t <= 38.0) return 0;
    if (t <= 39.0) return 1;
    return 2; // >= 39.1
}

int NewsCalculator::scoreSystolicBloodPressure(int bp) {
    if (bp <= 90)  return 3;
    if (bp <= 100) return 2;
    if (bp <= 110) return 1;
    if (bp <= 219) return 0;
    return 3; // >= 220
}

int NewsCalculator::scoreHeartRate(int hr) {
    if (hr <= 40)  return 3;
    if (hr <= 50)  return 1;
    if (hr <= 90)  return 0;
    if (hr <= 110) return 1;
    if (hr <= 130) return 2;
    return 3; // >= 131
}

int NewsCalculator::scoreConsciousness(ConsciousnessLevel c) {
    return c == ConsciousnessLevel::Alert ? 0 : 3;
}

bool NewsCalculator::isValid(const VitalSigns& v) {
    if (v.respiratoryRate < kRespiratoryRateMin || v.respiratoryRate > kRespiratoryRateMax)
        return false;
    if (v.oxygenSaturation < kOxygenSaturationMin || v.oxygenSaturation > kOxygenSaturationMax)
        return false;
    if (v.temperature < kTemperatureMin || v.temperature > kTemperatureMax)
        return false;
    if (v.systolicBloodPressure < kSystolicMin || v.systolicBloodPressure > kSystolicMax)
        return false;
    if (v.heartRate < kHeartRateMin || v.heartRate > kHeartRateMax)
        return false;
    return true;
}

RiskLevel NewsCalculator::classify(int total, int highestSingleParam) {
    if (total >= 7) return RiskLevel::High;
    if (total >= 5) return RiskLevel::Medium;
    // Total entre 0 e 4: se algum parâmetro isolado pontuou 3 (red score),
    // o risco sobe para Low-Medium; caso contrário, Low.
    if (highestSingleParam >= 3) return RiskLevel::LowMedium;
    return RiskLevel::Low;
}

NewsScore NewsCalculator::calculate(const VitalSigns& v) {
    const int scores[] = {
        scoreRespiratoryRate(v.respiratoryRate),
        scoreOxygenSaturation(v.oxygenSaturation),
        scoreSupplementalOxygen(v.supplementalOxygen),
        scoreTemperature(v.temperature),
        scoreSystolicBloodPressure(v.systolicBloodPressure),
        scoreHeartRate(v.heartRate),
        scoreConsciousness(v.consciousness),
    };

    int total = 0;
    int highestSingleParam = 0;
    for (int s : scores) {
        total += s;
        highestSingleParam = std::max(highestSingleParam, s);
    }

    NewsScore result;
    result.total = total;
    result.risk = classify(total, highestSingleParam);
    result.timestamp = std::chrono::system_clock::now();
    result.vitalSigns = v;
    return result;
}

} // namespace icu
