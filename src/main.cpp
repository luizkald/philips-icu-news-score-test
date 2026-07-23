#include "CLI.hpp"
#include "Console.hpp"
#include "Dashboard.hpp"
#include "ICU.hpp"
#include "LED.hpp"

int main() {
    icu::Console::init();

    icu::ICU icu;
    // Observer Pattern: a cada novo score ou alta, a ICU notifica os observers.
    // O Dashboard imprime a tabela (updateDashboard) e o LED imprime o maior
    // risco (updateRiskLevel), sem que a CLI precise conhecê-los.
    icu::Dashboard dashboard;
    icu::LED led;

    icu.attach(&dashboard);
    icu.attach(&led);

    icu::CLI cli(icu, dashboard);
    cli.run();

    return 0;
}
