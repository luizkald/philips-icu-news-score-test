#pragma once

#include "Models.hpp"
#include "Observer.hpp"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace icu {

// ICU é o serviço principal: gerencia pacientes, leitos e histórico, e notifica
// os observers. Não conhece a CLI nem cores/console.
class ICU {
public:
    static constexpr int kMaxBeds = 6;

    ICU() = default;

    // ----- Pacientes / leitos -----

    // Interna um paciente. Retorna false se: leito fora de 1..kMaxBeds, leito
    // ocupado, id já existente ou UTI lotada.
    bool addPatient(const std::string& id, const std::string& name, int bedNumber);

    bool containsPatient(const std::string& id) const;
    bool isBedOccupied(int bedNumber) const;
    bool isValidBed(int bedNumber) const { return bedNumber >= 1 && bedNumber <= kMaxBeds; }

    std::optional<std::reference_wrapper<const Patient>>
    findPatientById(const std::string& id) const;

    std::optional<std::reference_wrapper<Patient>>
    findPatientById(const std::string& id);

    // Localiza o paciente internado em um leito (o modelo é centrado no leito).
    std::optional<std::reference_wrapper<const Patient>>
    findPatientByBed(int bedNumber) const;

    // Calcula e registra uma nova medição NEWS. Retorna false se o id não
    // existir. Em caso de sucesso, notifica os observers.
    bool addNewsScore(const std::string& patientId, const VitalSigns& vitalSigns);

    // Dá alta: remove o paciente, libera o leito e notifica os observers.
    // Retorna false se o id não existir.
    bool dischargePatientById(const std::string& patientId);

    // ----- Consultas -----
    std::vector<Patient> getPatients() const;
    std::vector<DashboardRow> getDashboardRows() const;
    int occupiedBeds() const { return static_cast<int>(patients_.size()); }

    // ----- Observers (Observer Pattern) -----
    // observers_ guarda ponteiros NÃO proprietários: a ICU não é dona do ciclo
    // de vida dos observers, apenas os referencia.
    void attach(IICUObserver* observer);
    void detach(IICUObserver* observer);

private:
    void notifyObservers();

    std::vector<Patient> patients_;
    std::vector<IICUObserver*> observers_;
};

} // namespace icu
