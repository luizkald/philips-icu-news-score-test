#include "ICU.hpp"
#include "Models.hpp"
#include "Observer.hpp"

#include <gtest/gtest.h>

using namespace icu;

namespace {

// Observer de teste que conta notificações e guarda o último snapshot.
class FakeObserver : public IICUObserver {
public:
    void update(const std::vector<DashboardRow>& rows) override {
        ++updateCount;
        lastRows = rows;
    }

    int updateCount = 0;
    std::vector<DashboardRow> lastRows;
};

// Sinais vitais High (RR 8, SpO2 90, O2 sim -> total 8).
VitalSigns highVitals() {
    VitalSigns v;
    v.respiratoryRate = 8;
    v.oxygenSaturation = 90;
    v.supplementalOxygen = true;
    v.temperature = 36.8;
    v.systolicBloodPressure = 120;
    v.heartRate = 80;
    v.consciousness = ConsciousnessLevel::Alert;
    return v;
}

} // namespace

// ---------- Cadastro / leitos ----------

TEST(ICU, AddPatientSucceeds) {
    ICU icu;
    EXPECT_TRUE(icu.addPatient("P001", "Maria", 1));
    EXPECT_TRUE(icu.containsPatient("P001"));
    EXPECT_TRUE(icu.isBedOccupied(1));
}

TEST(ICU, RejectsDuplicateId) {
    ICU icu;
    ASSERT_TRUE(icu.addPatient("P001", "Maria", 1));
    EXPECT_FALSE(icu.addPatient("P001", "Outro", 2));
}

TEST(ICU, RejectsOccupiedBed) {
    ICU icu;
    ASSERT_TRUE(icu.addPatient("P001", "Maria", 1));
    EXPECT_FALSE(icu.addPatient("P002", "Joao", 1));
}

TEST(ICU, RejectsInvalidBed) {
    ICU icu;
    EXPECT_FALSE(icu.addPatient("P001", "Maria", 0));
    EXPECT_FALSE(icu.addPatient("P002", "Joao", 7));
    EXPECT_FALSE(icu.addPatient("P003", "Ana", -1));
}

TEST(ICU, RejectsWhenFull) {
    ICU icu;
    for (int bed = 1; bed <= ICU::kMaxBeds; ++bed) {
        ASSERT_TRUE(icu.addPatient("P" + std::to_string(bed), "Name", bed));
    }
    // Não há um 7º leito válido, mas mesmo um leito válido já estaria ocupado.
    EXPECT_FALSE(icu.addPatient("P999", "Extra", 3));
    EXPECT_EQ(icu.occupiedBeds(), ICU::kMaxBeds);
}

// ---------- Localização ----------

TEST(ICU, FindPatientById) {
    ICU icu;
    ASSERT_TRUE(icu.addPatient("P001", "Maria", 1));

    auto found = icu.findPatientById("P001");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->get().name, "Maria");

    EXPECT_FALSE(icu.findPatientById("XXX").has_value());
}

// ---------- Medições / histórico ----------

TEST(ICU, AddNewsScoreStoresHistory) {
    ICU icu;
    ASSERT_TRUE(icu.addPatient("P001", "Maria", 1));

    EXPECT_TRUE(icu.addNewsScore("P001", highVitals()));
    EXPECT_TRUE(icu.addNewsScore("P001", highVitals()));

    auto p = icu.findPatientById("P001");
    ASSERT_TRUE(p.has_value());
    EXPECT_EQ(p->get().history.size(), 2u);
    EXPECT_EQ(p->get().history.back().total, 8);
    EXPECT_EQ(p->get().history.back().risk, RiskLevel::High);
}

TEST(ICU, AddNewsScoreOnUnknownPatientFails) {
    ICU icu;
    EXPECT_FALSE(icu.addNewsScore("XXX", highVitals()));
}

// ---------- Alta ----------

TEST(ICU, DischargeExistingRemovesPatientAndFreesBed) {
    ICU icu;
    ASSERT_TRUE(icu.addPatient("P001", "Maria", 3));
    ASSERT_TRUE(icu.isBedOccupied(3));

    EXPECT_TRUE(icu.dischargePatientById("P001"));
    EXPECT_FALSE(icu.containsPatient("P001"));
    EXPECT_FALSE(icu.isBedOccupied(3));

    // O leito pode ser reutilizado.
    EXPECT_TRUE(icu.addPatient("P002", "Joao", 3));
}

TEST(ICU, DischargeNonExistingFails) {
    ICU icu;
    ASSERT_TRUE(icu.addPatient("P001", "Maria", 1));
    EXPECT_FALSE(icu.dischargePatientById("XXX"));
    EXPECT_EQ(icu.occupiedBeds(), 1);
}

// ---------- Observers ----------

TEST(ICU, ObserverNotifiedAfterScore) {
    ICU icu;
    FakeObserver obs;
    icu.attach(&obs);
    ASSERT_TRUE(icu.addPatient("P001", "Maria", 1));

    ASSERT_TRUE(icu.addNewsScore("P001", highVitals()));
    EXPECT_EQ(obs.updateCount, 1);
    ASSERT_EQ(obs.lastRows.size(), 1u);
    EXPECT_TRUE(obs.lastRows[0].hasMeasurement);
    EXPECT_EQ(obs.lastRows[0].latestRisk, RiskLevel::High);
}

TEST(ICU, ObserverNotifiedAfterDischarge) {
    ICU icu;
    FakeObserver obs;
    icu.attach(&obs);
    ASSERT_TRUE(icu.addPatient("P001", "Maria", 1));
    ASSERT_TRUE(icu.addNewsScore("P001", highVitals())); // updateCount = 1

    ASSERT_TRUE(icu.dischargePatientById("P001"));        // updateCount = 2
    EXPECT_EQ(obs.updateCount, 2);
    EXPECT_TRUE(obs.lastRows.empty());
}

TEST(ICU, DuplicateObserverIsNotNotifiedTwice) {
    ICU icu;
    FakeObserver obs;
    icu.attach(&obs);
    icu.attach(&obs); // duplicado deve ser ignorado
    ASSERT_TRUE(icu.addPatient("P001", "Maria", 1));

    ASSERT_TRUE(icu.addNewsScore("P001", highVitals()));
    EXPECT_EQ(obs.updateCount, 1);
}

TEST(ICU, DetachStopsNotifications) {
    ICU icu;
    FakeObserver obs;
    icu.attach(&obs);
    ASSERT_TRUE(icu.addPatient("P001", "Maria", 1));
    ASSERT_TRUE(icu.addNewsScore("P001", highVitals())); // updateCount = 1

    icu.detach(&obs);
    ASSERT_TRUE(icu.addNewsScore("P001", highVitals())); // não deve notificar
    EXPECT_EQ(obs.updateCount, 1);
}
