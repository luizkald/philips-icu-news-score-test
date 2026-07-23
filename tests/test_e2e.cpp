// Testes end-to-end (caixa-preta) do aplicativo completo.
//
// Diferente dos testes unitários (test_news.cpp / test_icu.cpp), que exercitam
// a regra de negócio isoladamente, este arquivo executa o BINÁRIO real
// (icu_news), alimenta a stdin com scripts de entrada — inclusive as demos
// oficiais (demo.txt / demo2.txt) — e valida a saída da CLI, cobrindo todos os
// caminhos do menu: cadastro centrado no leito, validações de faixa, cálculo/
// risco, histórico, listagem, alta, dashboard, LED e entradas inválidas/EOF.
//
// O caminho do binário e o diretório das demos são injetados pelo CMake via
// ICU_NEWS_BINARY e DEMO_DIR.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>

#ifndef ICU_NEWS_BINARY
#error "ICU_NEWS_BINARY não definido (configure via CMake)."
#endif
#ifndef DEMO_DIR
#error "DEMO_DIR não definido (configure via CMake)."
#endif

namespace {

// Remove as sequências ANSI de cor para permitir asserções estáveis no texto.
std::string stripAnsi(const std::string& s) {
    static const std::regex kAnsi("\x1b\\[[0-9;]*m");
    return std::regex_replace(s, kAnsi, "");
}

// Executa o binário do app com `input` na stdin e devolve a stdout já sem ANSI.
// Escreve o input em um arquivo temporário e redireciona (`app < tmp`), o que
// permite alimentar toda a sessão da CLI de uma vez.
//
// Com a variável de ambiente ICU_E2E_VERBOSE=1, imprime a saída COLORIDA do app
// (com o nome do teste em execução), para acompanhar o dashboard atualizando.
std::string runApp(const std::string& input) {
    std::string tmpPath =
        std::string(std::tmpnam(nullptr)) + "_icu_e2e.txt"; // caminho único
    {
        std::ofstream tmp(tmpPath, std::ios::binary);
        tmp << input;
    }

    // Aspas em ambos os caminhos: o diretório do projeto contém espaços.
    std::string cmd = "\"" ICU_NEWS_BINARY "\" < \"" + tmpPath + "\" 2>&1";

    std::string out;
    if (FILE* pipe = popen(cmd.c_str(), "r")) {
        char buf[4096];
        size_t n;
        while ((n = std::fread(buf, 1, sizeof(buf), pipe)) > 0) {
            out.append(buf, n);
        }
        pclose(pipe);
    }

    std::remove(tmpPath.c_str());

    if (const char* v = std::getenv("ICU_E2E_VERBOSE"); v && std::string(v) != "0") {
        const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        std::string name = info ? std::string(info->test_suite_name()) + "." + info->name()
                                : "runApp";
        std::cout << "\n\033[1;36m===== " << name << " =====\033[0m\n" << out << std::flush;
    }

    return stripAnsi(out);
}

// Lê um arquivo de demo do diretório do projeto.
std::string readDemo(const std::string& name) {
    std::ifstream f(std::string(DEMO_DIR) + "/" + name, std::ios::binary);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// Conta ocorrências (não sobrepostas) de `needle` em `hay`.
int countOccurrences(const std::string& hay, const std::string& needle) {
    if (needle.empty()) return 0;
    int count = 0;
    for (size_t pos = hay.find(needle); pos != std::string::npos;
         pos = hay.find(needle, pos + needle.size())) {
        ++count;
    }
    return count;
}

using ::testing::HasSubstr;
using ::testing::Not;

// Sinais vitais na ordem da tabela (RR, SpO2, O2, Temp, SBP, HR, Consciência),
// já com quebras de linha, prontos para concatenar após a admissão.
constexpr const char* kVitalsLow       = "18\n97\n0\n36.8\n120\n80\nA\n"; // total 0
constexpr const char* kVitalsLowMedium = "8\n97\n0\n36.8\n120\n80\nA\n";  // RR=3 -> total 3
constexpr const char* kVitalsMedium    = "18\n90\n1\n36.8\n120\n80\nA\n"; // SpO2 3 + O2 2 = 5
constexpr const char* kVitalsHigh      = "8\n90\n1\n36.8\n120\n80\nA\n";  // total 8
constexpr const char* kVitalsOutOfRange = "999\n97\n0\n36.8\n120\n80\nA\n"; // RR fora da faixa
constexpr const char* kVitalsBadConsc   = "18\n97\n0\n36.8\n120\n80\nZ\n";  // consciência inválida

// Bloco de entrada para admitir um novo paciente em um leito vazio + vitais.
std::string admit(int bed, const std::string& id, const std::string& first,
                  const std::string& last, const std::string& vitals) {
    return "N\n" + std::to_string(bed) + "\n" + id + "\n" + first + "\n" + last + "\n" + vitals;
}

// Bloco de entrada para uma nova medição em um leito JÁ ocupado (sem readmissão).
std::string scoreExistingBed(int bed, const std::string& vitals) {
    return "N\n" + std::to_string(bed) + "\n" + vitals;
}

} // namespace

// ===================== Demos oficiais =====================

TEST(E2E_Demo, DemoTxtCobreFluxoCompleto) {
    std::string out = runApp(readDemo("demo.txt"));

    EXPECT_THAT(out, HasSubstr("Bed 1 - 123 - John Doe"));
    EXPECT_THAT(out, HasSubstr("Bed 2 - 251 - Jane Doe"));
    EXPECT_THAT(out, HasSubstr("NEWS Score: 0, Risk Level: Low"));
    EXPECT_THAT(out, HasSubstr("NEWS Score: 8, Risk Level: High"));

    // LED (Task 2) reflete o maior risco.
    EXPECT_THAT(out, HasSubstr("Highest Risk Level: Green"));
    EXPECT_THAT(out, HasSubstr("Highest Risk Level: Red"));

    // Listagem 'a' renderiza o painel em tabela.
    EXPECT_THAT(out, HasSubstr("ICU Dashboard"));
    EXPECT_THAT(out, HasSubstr("Jane Doe"));

    // Alta com confirmação; após a alta o maior risco volta a Low -> Green.
    EXPECT_THAT(out, HasSubstr("Patient discharged. Bed released."));
    EXPECT_GE(countOccurrences(out, "Highest Risk Level: Green"), 2);

    EXPECT_THAT(out, HasSubstr("Goodbye."));
}

TEST(E2E_Demo, Demo2TxtCobreValidacoes) {
    std::string out = runApp(readDemo("demo2.txt"));

    EXPECT_THAT(out, HasSubstr("Invalid bed. Must be between 1 and 6."));
    EXPECT_THAT(out, HasSubstr("NEWS Score: 5, Risk Level: Medium"));
    EXPECT_THAT(out, HasSubstr("Invalid data. No NEWS score calculated"));
    EXPECT_THAT(out, HasSubstr("Patient not found. Please try again"));
    EXPECT_THAT(out, HasSubstr("Discharge cancelled."));
    // Bob foi admitido mas ficou sem score válido; aparece na tabela final ('A').
    EXPECT_THAT(out, HasSubstr("Bob Jones"));
    EXPECT_THAT(out, HasSubstr("Goodbye."));
}

// ===================== Exemplo do enunciado =====================

TEST(E2E_SpecExample, ExemploDoEnunciadoDa9High) {
    // RR 22, SpO2 94, sem O2, Temp 37.5, SBP 80, HR 80, consciência V -> 9 (High).
    std::string in = "N\n1\n123\nJohn\nDoe\n22\n94\n0\n37.5\n80\n80\nV\nX\n";
    std::string out = runApp(in);
    EXPECT_THAT(out, HasSubstr("Bed 1 - 123 - John Doe"));
    EXPECT_THAT(out, HasSubstr("NEWS Score: 9, Risk Level: High"));
    EXPECT_THAT(out, HasSubstr("Highest Risk Level: Red"));
}

// ===================== Classificação de risco (todos os níveis) =====================

TEST(E2E_Risk, TodosOsNiveisNoScoreENoLED) {
    std::string in;
    in += admit(1, "1", "P", "Low", kVitalsLow);
    in += admit(2, "2", "P", "LowMed", kVitalsLowMedium);
    in += admit(3, "3", "P", "Med", kVitalsMedium);
    in += admit(4, "4", "P", "High", kVitalsHigh);
    in += "X\n";
    std::string out = runApp(in);

    EXPECT_THAT(out, HasSubstr("NEWS Score: 0, Risk Level: Low"));
    EXPECT_THAT(out, HasSubstr("NEWS Score: 3, Risk Level: Low-Medium"));
    EXPECT_THAT(out, HasSubstr("NEWS Score: 5, Risk Level: Medium"));
    EXPECT_THAT(out, HasSubstr("NEWS Score: 8, Risk Level: High"));

    EXPECT_THAT(out, HasSubstr("Highest Risk Level: Green"));
    EXPECT_THAT(out, HasSubstr("Highest Risk Level: Yellow"));
    EXPECT_THAT(out, HasSubstr("Highest Risk Level: Orange"));
    EXPECT_THAT(out, HasSubstr("Highest Risk Level: Red"));
}

// ===================== Menu =====================

TEST(E2E_Menu, OpcaoInvalidaEhRejeitadaSemEncerrar) {
    std::string out = runApp("Z\n9\n?\nX\n");
    EXPECT_EQ(countOccurrences(out, "Invalid option."), 3);
    EXPECT_THAT(out, HasSubstr("Goodbye."));
}

TEST(E2E_Menu, OpcoesAceitamMaiusculasEMinusculas) {
    // 'n' minúsculo admite; 'a' minúsculo mostra a tabela; 'x' encerra.
    std::string out = runApp(admit(1, "1", "Ana", "Souza", kVitalsLow) + "a\nx\n");
    EXPECT_THAT(out, HasSubstr("NEWS Score: 0, Risk Level: Low"));
    EXPECT_THAT(out, HasSubstr("Ana Souza"));
    EXPECT_THAT(out, HasSubstr("Goodbye."));
}

TEST(E2E_Menu, LinhaVaziaNoMenuEhIgnorada) {
    std::string out = runApp("\n\nX\n");
    EXPECT_THAT(out, Not(HasSubstr("Invalid option.")));
    EXPECT_THAT(out, HasSubstr("Goodbye."));
}

TEST(E2E_Menu, EofEncerraSemGoodbye) {
    std::string out = runApp("A\n"); // sem 'X': termina por EOF
    EXPECT_THAT(out, HasSubstr("(no patients admitted)"));
    EXPECT_THAT(out, Not(HasSubstr("Goodbye.")));
}

// ===================== Cadastro (opção 'n') =====================

TEST(E2E_NewScore, LeitoInvalidoEhRejeitado) {
    EXPECT_THAT(runApp("N\n7\nX\n"), HasSubstr("Invalid bed. Must be between 1 and 6."));
    EXPECT_THAT(runApp("N\n0\nX\n"), HasSubstr("Invalid bed. Must be between 1 and 6."));
}

TEST(E2E_NewScore, NovoPacienteImprimeLinhaDoLeito) {
    std::string out = runApp(admit(3, "555", "Ana", "Beatriz", kVitalsMedium) + "X\n");
    EXPECT_THAT(out, HasSubstr("Bed 3 - 555 - Ana Beatriz"));
    EXPECT_THAT(out, HasSubstr("NEWS Score: 5, Risk Level: Medium"));
}

TEST(E2E_NewScore, SegundaMedicaoNoMesmoLeitoNaoReadmite) {
    // Leito ocupado: 'n' vai direto aos vitais, sem pedir ID/nome de novo.
    std::string in = admit(1, "123", "John", "Doe", kVitalsLow);
    in += scoreExistingBed(1, kVitalsHigh);
    in += "H\n123\nX\n";
    std::string out = runApp(in);

    EXPECT_THAT(out, HasSubstr("NEWS Score: 0, Risk Level: Low"));
    EXPECT_THAT(out, HasSubstr("NEWS Score: 8, Risk Level: High"));
    // Histórico com duas medições.
    EXPECT_EQ(countOccurrences(out, ", NEWS Score: 0, Risk Level: Low"), 1);
    EXPECT_THAT(out, HasSubstr(", NEWS Score: 8, Risk Level: High"));
}

TEST(E2E_NewScore, IdDuplicadoEmOutroLeitoEhRejeitado) {
    std::string in = admit(1, "X1", "Ana", "A", kVitalsLow);
    in += "N\n2\nX1\nBob\nB\n"; // mesmo ID em leito vazio -> recusado
    in += "X\n";
    std::string out = runApp(in);
    EXPECT_THAT(out, HasSubstr("Could not admit patient (duplicate ID)."));
}

// ===================== Validação de sinais vitais =====================

TEST(E2E_Validation, ForaDaFaixaNaoCalculaScore) {
    std::string out = runApp(admit(1, "1", "Bob", "J", kVitalsOutOfRange) + "X\n");
    EXPECT_THAT(out, HasSubstr("Invalid data. No NEWS score calculated"));
    EXPECT_THAT(out, Not(HasSubstr("NEWS Score:")));
}

TEST(E2E_Validation, ValorNaoNumericoNaoCalculaScore) {
    // "abc" na frequência respiratória.
    std::string in = "N\n1\n1\nBob\nJ\nabc\n97\n0\n36.8\n120\n80\nA\nX\n";
    std::string out = runApp(in);
    EXPECT_THAT(out, HasSubstr("Invalid data. No NEWS score calculated"));
}

TEST(E2E_Validation, ConscienciaInvalidaNaoCalculaScore) {
    std::string out = runApp(admit(1, "1", "Ana", "L", kVitalsBadConsc) + "X\n");
    EXPECT_THAT(out, HasSubstr("Invalid data. No NEWS score calculated"));
}

TEST(E2E_Validation, FronteirasDasFaixasSaoAceitas) {
    // SpO2 50 (mín), HR 15 (mín), Temp 20 (mín), SBP 0 (mín), RR 0 (mín).
    // RR0->3, SpO2 50->3, O2 0, Temp20->3, SBP0->3, HR15->3, A->0 = 15 (High).
    std::string in = "N\n1\n1\nEdge\nCase\n0\n50\n0\n20\n0\n15\nA\nX\n";
    std::string out = runApp(in);
    EXPECT_THAT(out, Not(HasSubstr("Invalid data")));
    EXPECT_THAT(out, HasSubstr("NEWS Score: 15, Risk Level: High"));
}

// ===================== Histórico (opção 'h') =====================

TEST(E2E_History, PacienteInexistente) {
    EXPECT_THAT(runApp("H\nXXX\nX\n"), HasSubstr("No Patient with ID: XXX"));
}

TEST(E2E_History, CabecalhoELinhasNoFormatoDoEnunciado) {
    std::string in = admit(1, "123", "John", "Doe", kVitalsHigh) + "H\n123\nX\n";
    std::string out = runApp(in);
    EXPECT_THAT(out, HasSubstr("Bed 1 - 123 - John Doe"));
    EXPECT_THAT(out, HasSubstr(", NEWS Score: 8, Risk Level: High"));
}

// ===================== Todos os pacientes (opção 'a') =====================

TEST(E2E_AllPatients, VazioMostraTabelaSemPacientes) {
    std::string out = runApp("A\nX\n");
    EXPECT_THAT(out, HasSubstr("ICU Dashboard"));
    EXPECT_THAT(out, HasSubstr("(no patients admitted)"));
}

TEST(E2E_AllPatients, ExibeTabelaComOPaciente) {
    std::string out = runApp(admit(2, "251", "Jane", "Doe", kVitalsHigh) + "A\nX\n");
    EXPECT_THAT(out, HasSubstr("ICU Dashboard"));
    EXPECT_THAT(out, HasSubstr("Jane Doe"));
    EXPECT_THAT(out, HasSubstr("High"));
    // Painel aparece 2x: pelo observer (após o score) e pela opção 'A'.
    EXPECT_GE(countOccurrences(out, "ICU Dashboard"), 2);
}

TEST(E2E_AllPatients, PacienteAdmitidoSemScoreApareceNaTabela) {
    // Admite com dado inválido (sem score); a admissão não notifica observers,
    // então a tabela só aparece ao pedir 'A', mostrando o paciente.
    std::string out = runApp(admit(1, "9", "No", "Score", kVitalsOutOfRange) + "A\nX\n");
    EXPECT_THAT(out, HasSubstr("ICU Dashboard"));
    EXPECT_THAT(out, HasSubstr("No Score"));
}

// ===================== Alta (opção 'd') =====================

TEST(E2E_Discharge, PacienteInexistente) {
    EXPECT_THAT(runApp("D\nXXX\nX\n"), HasSubstr("Patient not found. Please try again"));
}

TEST(E2E_Discharge, ConfirmadaLiberaLeitoParaReuso) {
    std::string in = admit(1, "123", "John", "Doe", kVitalsLow);
    in += "D\n123\nY\n";                          // dá alta
    in += admit(1, "456", "New", "Guy", kVitalsLow); // reusa leito 1
    in += "X\n";
    std::string out = runApp(in);
    EXPECT_THAT(out, HasSubstr("Discharge patient John Doe? (Y/N)"));
    EXPECT_THAT(out, HasSubstr("Patient discharged. Bed released."));
    EXPECT_THAT(out, HasSubstr("Bed 1 - 456 - New Guy"));
    EXPECT_THAT(out, Not(HasSubstr("Could not admit patient")));
}

TEST(E2E_Discharge, CanceladaComN) {
    std::string in = admit(1, "123", "John", "Doe", kVitalsLow) + "D\n123\nN\nX\n";
    std::string out = runApp(in);
    EXPECT_THAT(out, HasSubstr("Discharge cancelled."));
    EXPECT_THAT(out, Not(HasSubstr("Patient discharged")));
}

TEST(E2E_Discharge, RespostaVaziaCancela) {
    std::string in = admit(1, "123", "John", "Doe", kVitalsLow) + "D\n123\n\nX\n";
    EXPECT_THAT(runApp(in), HasSubstr("Discharge cancelled."));
}

// ===================== Dashboard e LED (Task 2) =====================

TEST(E2E_Dashboard, AtualizaAutomaticamenteAoCalcularScore) {
    std::string out = runApp(admit(1, "123", "John", "Doe", kVitalsMedium) + "X\n");
    EXPECT_THAT(out, HasSubstr("ICU Dashboard"));
    EXPECT_THAT(out, HasSubstr("John Doe"));
    EXPECT_THAT(out, HasSubstr("Medium"));
}

TEST(E2E_LED, AtualizaAposAltaRefletindoMenorRisco) {
    // Dois pacientes (Low e High); ao dar alta no High, o maior risco cai p/ Low.
    std::string in = admit(1, "1", "Low", "Guy", kVitalsLow);
    in += admit(2, "2", "High", "Guy", kVitalsHigh);
    in += "D\n2\nY\n"; // remove o High
    in += "X\n";
    std::string out = runApp(in);
    EXPECT_THAT(out, HasSubstr("Highest Risk Level: Red"));
    // Última linha de LED deve voltar a Green após a alta do paciente High.
    size_t lastGreen = out.rfind("Highest Risk Level: Green");
    size_t lastRed = out.rfind("Highest Risk Level: Red");
    ASSERT_NE(lastGreen, std::string::npos);
    EXPECT_GT(lastGreen, lastRed);
}
