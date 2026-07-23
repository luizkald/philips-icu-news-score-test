#include "CLI.hpp"

#include "Console.hpp"
#include "Models.hpp"
#include "News.hpp"

#include <cctype>
#include <sstream>
#include <string>
#include <vector>

namespace icu {

CLI::CLI(ICU& icu, Dashboard& dashboard, std::istream& in, std::ostream& out)
    : icu_(icu), dashboard_(dashboard), in_(in), out_(out) {}

// ----- Utilitários de leitura -----

bool CLI::readLine(const std::string& prompt, std::string& out) {
    out_ << prompt;
    return static_cast<bool>(std::getline(in_, out));
}

bool CLI::readInt(const std::string& prompt, int& out) {
    while (true) {
        std::string line;
        if (!readLine(prompt, line)) return false; // EOF
        std::istringstream iss(line);
        if (iss >> out) return true;
        out_ << Console::yellow << "Invalid number, please try again.\n" << Console::reset;
    }
}

// Lê os 7 parâmetros na ordem da tabela. Cada campo é lido como uma linha e
// parseado; se algum campo não for numérico/reconhecido, marca `valid=false`,
// mas segue lendo os demais para não desalinhar a entrada. As FAIXAS numéricas
// são validadas depois por NewsCalculator::isValid.
bool CLI::readVitalSigns(VitalSigns& v, bool& valid) {
    valid = true;

    auto readIntField = [&](const std::string& prompt, int& field) -> bool {
        std::string line;
        if (!readLine(prompt, line)) return false; // EOF
        std::istringstream iss(line);
        int x;
        if (iss >> x) field = x;
        else valid = false;
        return true;
    };

    if (!readIntField("Respiratory rate [0-150, Ex: 16]: ", v.respiratoryRate))
        return false;
    if (!readIntField("Oxygen saturation (%) [50-100, Ex: 98]: ", v.oxygenSaturation))
        return false;

    {
        std::string line;
        if (!readLine("Supplemental oxygen? (1 = yes, 0 = no): ", line)) return false;
        std::istringstream iss(line);
        int x;
        if (iss >> x && (x == 0 || x == 1)) v.supplementalOxygen = (x == 1);
        else valid = false;
    }

    {
        std::string line;
        if (!readLine("Temperature (C) [20-50, Ex: 36.6]: ", line)) return false;
        std::istringstream iss(line);
        double x;
        if (iss >> x) v.temperature = x;
        else valid = false;
    }

    if (!readIntField("Systolic BP (mmHg) [0-300, Ex: 120]: ",
                      v.systolicBloodPressure))
        return false;
    if (!readIntField("Heart rate [15-250, Ex: 70]: ", v.heartRate)) return false;

    {
        std::string line;
        const std::string prompt =
            "Consciousness level (AVPU):\n"
            "  A = Alert (awake, responsive)\n"
            "  V = Voice (responds to voice)\n"
            "  P = Pain (responds to pain)\n"
            "  U = Unresponsive\n"
            "Enter A/V/P/U: ";
        if (!readLine(prompt, line)) return false;
        char c = line.empty()
                     ? '\0'
                     : static_cast<char>(std::toupper(static_cast<unsigned char>(line[0])));
        switch (c) {
            case 'A': v.consciousness = ConsciousnessLevel::Alert; break;
            case 'V': v.consciousness = ConsciousnessLevel::Voice; break;
            case 'P': v.consciousness = ConsciousnessLevel::Pain; break;
            case 'U': v.consciousness = ConsciousnessLevel::Unresponsive; break;
            default:  valid = false; break;
        }
    }
    return true;
}

// ----- Menu -----

void CLI::printMenu() {
    out_ << "\n" << Console::bold << Console::cyan
         << "=== ICU NEWS Monitoring ===" << Console::reset << "\n"
         << "N - New NEWS score\n"
         << "H - Patient history\n"
         << "A - All patients\n"
         << "D - Discharge patient\n"
         << "X - Exit\n"
         << "Choose an option: ";
}

// ----- Handlers -----

// Opção 'n': modelo centrado no leito. Lê o leito (1-6); se estiver vazio,
// cadastra o paciente (ID, nome, sobrenome). Depois lê os sinais vitais; se
// algum for inválido, não calcula o score.
void CLI::handleNewScore() {
    int bed = 0;
    if (!readInt("Bed number (1-6): ", bed)) return; // EOF
    if (!icu_.isValidBed(bed)) {
        out_ << Console::yellow << "Invalid bed. Must be between 1 and 6.\n" << Console::reset;
        return;
    }

    std::string id;
    std::string name;

    auto existing = icu_.findPatientByBed(bed);
    if (!existing) {
        // Leito vazio: cadastrar novo paciente.
        if (!readLine("Patient ID: ", id)) return;
        std::string firstName;
        std::string lastname;
        if (!readLine("Name: ", firstName)) return;
        if (!readLine("LastName: ", lastname)) return;
        name = lastname.empty() ? firstName : firstName + " " + lastname;

        if (!icu_.addPatient(id, name, bed)) {
            out_ << Console::red << "Could not admit patient (duplicate ID).\n" << Console::reset;
            return;
        }
    } else {
        id = existing->get().id;
        name = existing->get().name;
    }

    // Identidade do paciente: "Bed 1 - 123 - John Doe".
    out_ << "Bed " << bed << " - " << id << " - " << name << "\n";

    VitalSigns vitals;
    bool valid = false;
    if (!readVitalSigns(vitals, valid)) return; // EOF

    if (!valid || !NewsCalculator::isValid(vitals)) {
        out_ << Console::red << "Invalid data. No NEWS score calculated\n" << Console::reset;
        return;
    }

    if (!icu_.addNewsScore(id, vitals)) {
        out_ << Console::red << "Failed to add score.\n" << Console::reset;
        return;
    }

    auto patient = icu_.findPatientById(id);
    if (patient && !patient->get().history.empty()) {
        const NewsScore& s = patient->get().history.back();
        out_ << Console::bold << "NEWS Score: " << s.total << ", Risk Level: "
             << Console::colorFor(s.risk) << toString(s.risk) << Console::reset << "\n";
    }
}

// Opção 'h': histórico de um paciente pelo ID.
void CLI::handleHistory() {
    std::string id;
    if (!readLine("Patient ID: ", id)) return;

    auto patient = icu_.findPatientById(id);
    if (!patient) {
        out_ << "No Patient with ID: " << id << "\n";
        return;
    }

    const Patient& p = patient->get();
    out_ << "Bed " << p.bedNumber << " - " << p.id << " - " << p.name << "\n";
    if (p.history.empty()) {
        out_ << "  (no scores recorded)\n";
        return;
    }
    for (const NewsScore& s : p.history) {
        out_ << formatTimestamp(s.timestamp) << ", NEWS Score: " << s.total
             << ", Risk Level: " << Console::colorFor(s.risk) << toString(s.risk)
             << Console::reset << "\n";
    }
}

// Opção 'a': exibe o painel (tabela) com o estado atual de todos os leitos.
void CLI::handleAllPatients() {
    dashboard_.render(icu_.getDashboardRows()); // rows já ordenados por leito
}

// Opção 'd': alta de paciente pelo ID, com confirmação.
void CLI::handleDischarge() {
    std::string id;
    if (!readLine("Patient ID: ", id)) return;

    auto patient = icu_.findPatientById(id);
    if (!patient) {
        out_ << "Patient not found. Please try again\n";
        return;
    }

    out_ << "Discharge patient " << patient->get().name << "? (Y/N) ";
    std::string answer;
    if (!std::getline(in_, answer)) return;

    char c = answer.empty() ? '\0'
                            : static_cast<char>(std::tolower(static_cast<unsigned char>(answer[0])));
    if (c == 'y') {
        if (icu_.dischargePatientById(id)) {
            out_ << Console::green << "Patient discharged. Bed released.\n" << Console::reset;
        } else {
            out_ << Console::red << "Failed to discharge patient.\n" << Console::reset;
        }
    } else {
        out_ << "Discharge cancelled.\n";
    }
}

void CLI::run() {
    std::string line;
    while (true) {
        printMenu();
        if (!std::getline(in_, line)) break; // EOF
        if (line.empty()) continue;

        char option = static_cast<char>(std::tolower(static_cast<unsigned char>(line[0])));
        switch (option) {
            case 'n': handleNewScore(); break;
            case 'h': handleHistory(); break;
            case 'a': handleAllPatients(); break;
            case 'd': handleDischarge(); break;
            case 'x':
                out_ << "Goodbye.\n";
                return;
            default:
                out_ << Console::yellow << "Invalid option.\n" << Console::reset;
                break;
        }
    }
}

} // namespace icu
