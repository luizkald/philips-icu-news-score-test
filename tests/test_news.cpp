#include "Models.hpp"
#include "News.hpp"

#include <gtest/gtest.h>

using namespace icu;

namespace {

// Sinais vitais normais -> total 0 -> Low.
VitalSigns normalVitals() {
    VitalSigns v;
    v.respiratoryRate = 18;      // 0
    v.oxygenSaturation = 97;     // 0
    v.supplementalOxygen = false; // 0
    v.temperature = 36.8;        // 0
    v.systolicBloodPressure = 120; // 0
    v.heartRate = 80;            // 0
    v.consciousness = ConsciousnessLevel::Alert; // 0
    return v;
}

// Calcula o total NEWS variando apenas a frequência respiratória.
int totalWithRR(int rr) {
    VitalSigns v = normalVitals();
    v.respiratoryRate = rr;
    return NewsCalculator::calculate(v).total;
}
int totalWithSpo2(int spo2) {
    VitalSigns v = normalVitals();
    v.oxygenSaturation = spo2;
    return NewsCalculator::calculate(v).total;
}
int totalWithTemp(double t) {
    VitalSigns v = normalVitals();
    v.temperature = t;
    return NewsCalculator::calculate(v).total;
}
int totalWithSBP(int bp) {
    VitalSigns v = normalVitals();
    v.systolicBloodPressure = bp;
    return NewsCalculator::calculate(v).total;
}
int totalWithHR(int hr) {
    VitalSigns v = normalVitals();
    v.heartRate = hr;
    return NewsCalculator::calculate(v).total;
}

} // namespace

// ---------- Classificação ----------

TEST(NewsCalculator, Low) {
    NewsScore s = NewsCalculator::calculate(normalVitals());
    EXPECT_EQ(s.total, 0);
    EXPECT_EQ(s.risk, RiskLevel::Low);
}

TEST(NewsCalculator, LowMedium) {
    VitalSigns v = normalVitals();
    v.respiratoryRate = 8; // pontua 3 (parâmetro individual)
    NewsScore s = NewsCalculator::calculate(v);
    EXPECT_EQ(s.total, 3);
    EXPECT_EQ(s.risk, RiskLevel::LowMedium);
}

TEST(NewsCalculator, Medium) {
    VitalSigns v = normalVitals();
    v.respiratoryRate = 21;   // 2
    v.oxygenSaturation = 92;  // 2
    v.heartRate = 95;         // 1
    NewsScore s = NewsCalculator::calculate(v);
    EXPECT_EQ(s.total, 5);
    EXPECT_EQ(s.risk, RiskLevel::Medium);
}

TEST(NewsCalculator, High) {
    VitalSigns v = normalVitals();
    v.respiratoryRate = 8;        // 3
    v.oxygenSaturation = 90;      // 3
    v.supplementalOxygen = true;  // 2
    NewsScore s = NewsCalculator::calculate(v);
    EXPECT_EQ(s.total, 8);
    EXPECT_EQ(s.risk, RiskLevel::High);
}

// ---------- Consciência ----------

TEST(NewsCalculator, ConsciousnessAlertIsZeroOthersAreThree) {
    VitalSigns v = normalVitals();

    v.consciousness = ConsciousnessLevel::Alert;
    EXPECT_EQ(NewsCalculator::calculate(v).total, 0);

    for (ConsciousnessLevel level : {ConsciousnessLevel::Voice, ConsciousnessLevel::Pain,
                                     ConsciousnessLevel::Unresponsive,
                                     ConsciousnessLevel::NewConfusion}) {
        v.consciousness = level;
        NewsScore s = NewsCalculator::calculate(v);
        EXPECT_EQ(s.total, 3);
        EXPECT_EQ(s.risk, RiskLevel::LowMedium);
    }
}

TEST(NewsCalculator, SupplementalOxygenScoresTwo) {
    VitalSigns v = normalVitals();
    v.supplementalOxygen = true;
    EXPECT_EQ(NewsCalculator::calculate(v).total, 2);
    EXPECT_EQ(NewsCalculator::calculate(v).risk, RiskLevel::Low);
}

// ---------- Fronteiras: frequência respiratória ----------
TEST(NewsBoundaries, RespiratoryRate) {
    EXPECT_EQ(totalWithRR(8), 3);
    EXPECT_EQ(totalWithRR(9), 1);
    EXPECT_EQ(totalWithRR(11), 1);
    EXPECT_EQ(totalWithRR(12), 0);
    EXPECT_EQ(totalWithRR(20), 0);
    EXPECT_EQ(totalWithRR(21), 2);
    EXPECT_EQ(totalWithRR(24), 2);
    EXPECT_EQ(totalWithRR(25), 3);
}

// ---------- Fronteiras: saturação de oxigênio ----------
TEST(NewsBoundaries, OxygenSaturation) {
    EXPECT_EQ(totalWithSpo2(91), 3);
    EXPECT_EQ(totalWithSpo2(92), 2);
    EXPECT_EQ(totalWithSpo2(93), 2);
    EXPECT_EQ(totalWithSpo2(94), 1);
    EXPECT_EQ(totalWithSpo2(95), 1);
    EXPECT_EQ(totalWithSpo2(96), 0);
}

// ---------- Fronteiras: temperatura ----------
TEST(NewsBoundaries, Temperature) {
    EXPECT_EQ(totalWithTemp(35.0), 3);
    EXPECT_EQ(totalWithTemp(35.1), 1);
    EXPECT_EQ(totalWithTemp(36.0), 1);
    EXPECT_EQ(totalWithTemp(36.1), 0);
    EXPECT_EQ(totalWithTemp(38.0), 0);
    EXPECT_EQ(totalWithTemp(38.1), 1);
    EXPECT_EQ(totalWithTemp(39.0), 1);
    EXPECT_EQ(totalWithTemp(39.1), 2);
}

// ---------- Fronteiras: pressão sistólica ----------
TEST(NewsBoundaries, SystolicBloodPressure) {
    EXPECT_EQ(totalWithSBP(90), 3);
    EXPECT_EQ(totalWithSBP(91), 2);
    EXPECT_EQ(totalWithSBP(100), 2);
    EXPECT_EQ(totalWithSBP(101), 1);
    EXPECT_EQ(totalWithSBP(110), 1);
    EXPECT_EQ(totalWithSBP(111), 0);
    EXPECT_EQ(totalWithSBP(219), 0);
    EXPECT_EQ(totalWithSBP(220), 3);
}

// ---------- Validação de faixas de entrada ----------

TEST(NewsValidation, AcceptsValuesInRange) {
    EXPECT_TRUE(NewsCalculator::isValid(normalVitals()));
}

TEST(NewsValidation, AcceptsRangeBoundaries) {
    VitalSigns v = normalVitals();
    v.respiratoryRate = 0;   v.oxygenSaturation = 50; v.temperature = 20.0;
    v.systolicBloodPressure = 0; v.heartRate = 15;
    EXPECT_TRUE(NewsCalculator::isValid(v));

    v.respiratoryRate = 150; v.oxygenSaturation = 100; v.temperature = 50.0;
    v.systolicBloodPressure = 300; v.heartRate = 250;
    EXPECT_TRUE(NewsCalculator::isValid(v));
}

TEST(NewsValidation, RejectsRespiratoryRateOutOfRange) {
    VitalSigns v = normalVitals();
    v.respiratoryRate = -1;  EXPECT_FALSE(NewsCalculator::isValid(v));
    v.respiratoryRate = 151; EXPECT_FALSE(NewsCalculator::isValid(v));
}

TEST(NewsValidation, RejectsOxygenSaturationOutOfRange) {
    VitalSigns v = normalVitals();
    v.oxygenSaturation = 49;  EXPECT_FALSE(NewsCalculator::isValid(v));
    v.oxygenSaturation = 101; EXPECT_FALSE(NewsCalculator::isValid(v));
}

TEST(NewsValidation, RejectsTemperatureOutOfRange) {
    VitalSigns v = normalVitals();
    v.temperature = 19.9; EXPECT_FALSE(NewsCalculator::isValid(v));
    v.temperature = 50.1; EXPECT_FALSE(NewsCalculator::isValid(v));
}

TEST(NewsValidation, RejectsSystolicOutOfRange) {
    VitalSigns v = normalVitals();
    v.systolicBloodPressure = -1;  EXPECT_FALSE(NewsCalculator::isValid(v));
    v.systolicBloodPressure = 301; EXPECT_FALSE(NewsCalculator::isValid(v));
}

TEST(NewsValidation, RejectsHeartRateOutOfRange) {
    VitalSigns v = normalVitals();
    v.heartRate = 14;  EXPECT_FALSE(NewsCalculator::isValid(v));
    v.heartRate = 251; EXPECT_FALSE(NewsCalculator::isValid(v));
}

// ---------- Fronteiras: frequência cardíaca ----------
TEST(NewsBoundaries, HeartRate) {
    EXPECT_EQ(totalWithHR(40), 3);
    EXPECT_EQ(totalWithHR(41), 1);
    EXPECT_EQ(totalWithHR(50), 1);
    EXPECT_EQ(totalWithHR(51), 0);
    EXPECT_EQ(totalWithHR(90), 0);
    EXPECT_EQ(totalWithHR(91), 1);
    EXPECT_EQ(totalWithHR(110), 1);
    EXPECT_EQ(totalWithHR(111), 2);
    EXPECT_EQ(totalWithHR(130), 2);
    EXPECT_EQ(totalWithHR(131), 3);
}
