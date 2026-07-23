#include "ICU.hpp"

#include "News.hpp"

#include <algorithm>

namespace icu {

bool ICU::addPatient(const std::string& id, const std::string& name, int bedNumber) {
    if (!isValidBed(bedNumber)) return false;
    if (occupiedBeds() >= kMaxBeds) return false;
    if (containsPatient(id)) return false;
    if (isBedOccupied(bedNumber)) return false;

    Patient patient;
    patient.id = id;
    patient.name = name;
    patient.bedNumber = bedNumber;
    patients_.push_back(std::move(patient));
    return true;
}

bool ICU::containsPatient(const std::string& id) const {
    return std::any_of(patients_.begin(), patients_.end(),
                       [&id](const Patient& p) { return p.id == id; });
}

bool ICU::isBedOccupied(int bedNumber) const {
    return std::any_of(patients_.begin(), patients_.end(),
                       [bedNumber](const Patient& p) { return p.bedNumber == bedNumber; });
}

std::optional<std::reference_wrapper<const Patient>>
ICU::findPatientById(const std::string& id) const {
    for (const auto& p : patients_) {
        if (p.id == id) return std::cref(p);
    }
    return std::nullopt;
}

std::optional<std::reference_wrapper<Patient>>
ICU::findPatientById(const std::string& id) {
    for (auto& p : patients_) {
        if (p.id == id) return std::ref(p);
    }
    return std::nullopt;
}

std::optional<std::reference_wrapper<const Patient>>
ICU::findPatientByBed(int bedNumber) const {
    for (const auto& p : patients_) {
        if (p.bedNumber == bedNumber) return std::cref(p);
    }
    return std::nullopt;
}

bool ICU::addNewsScore(const std::string& patientId, const VitalSigns& vitalSigns) {
    auto patient = findPatientById(patientId);
    if (!patient) return false;

    NewsScore score = NewsCalculator::calculate(vitalSigns);
    patient->get().history.push_back(score);

    notifyObservers();
    return true;
}

bool ICU::dischargePatientById(const std::string& patientId) {
    auto it = std::find_if(patients_.begin(), patients_.end(),
                           [&patientId](const Patient& p) { return p.id == patientId; });
    if (it == patients_.end()) return false;

    patients_.erase(it); // remove da estrutura interna e libera o leito
    notifyObservers();
    return true;
}

std::vector<Patient> ICU::getPatients() const {
    return patients_;
}

std::vector<DashboardRow> ICU::getDashboardRows() const {
    std::vector<DashboardRow> rows;
    rows.reserve(patients_.size());

    for (const auto& p : patients_) {
        DashboardRow row;
        row.bedNumber = p.bedNumber;
        row.patientId = p.id;
        row.patientName = p.name;
        row.hasMeasurement = !p.history.empty();
        if (row.hasMeasurement) {
            const NewsScore& latest = p.history.back();
            row.latestRisk = latest.risk;
            row.latestScore = latest.total;
            row.timestamp = latest.timestamp;
        }
        rows.push_back(std::move(row));
    }

    std::sort(rows.begin(), rows.end(),
              [](const DashboardRow& a, const DashboardRow& b) {
                  return a.bedNumber < b.bedNumber;
              });
    return rows;
}

void ICU::attach(IICUObserver* observer) {
    if (observer == nullptr) return;
    // Evita observers duplicados.
    if (std::find(observers_.begin(), observers_.end(), observer) != observers_.end()) {
        return;
    }
    observers_.push_back(observer);
}

void ICU::detach(IICUObserver* observer) {
    observers_.erase(std::remove(observers_.begin(), observers_.end(), observer),
                     observers_.end());
}

void ICU::notifyObservers() {
    std::vector<DashboardRow> rows = getDashboardRows();
    for (IICUObserver* obs : observers_) {
        obs->update(rows);
    }
}

} // namespace icu
