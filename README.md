# ICU NEWS Monitoring

Aplicação de console em **C++17** para monitoramento de pacientes de UTI com
base no protocolo **NEWS — National Early Warning Score**. Cadastra pacientes em
até 6 leitos, calcula o NEWS a partir dos sinais vitais, classifica o risco
clínico, mantém histórico e reflete o estado da UTI em observadores (Dashboard e
LED) através do **Observer Pattern**.

Projeto com **CMake + Ninja** e testes em **GoogleTest**, escrito com separação
de responsabilidades e regra de negócio totalmente independente da CLI.

---

## Features

- Fluxo **centrado no leito** (1–6): ao calcular um novo score, informa-se o
  leito; se estiver vazio, cadastra-se o paciente (ID, nome, sobrenome).
- Cálculo do NEWS Score a partir de 7 parâmetros clínicos.
- **Validação das faixas de entrada**; se algum parâmetro for inválido, imprime
  `Invalid data. No NEWS score calculated` e não calcula o score.
- Classificação de risco: **Low**, **Low-Medium**, **Medium**, **High**.
- Histórico cronológico de medições por paciente.
- Observer Pattern: Dashboard (`updateDashboard`) e LED (`updateRiskLevel`)
  atualizados automaticamente a cada novo score e a cada alta.
- Alta de paciente com confirmação, liberando o leito.
- CLI resiliente: opções aceitam maiúsculas e minúsculas; leito inválido não
  encerra a aplicação.

---

## Arquitetura

O projeto separa **regra de negócio**, **estado**, **apresentação** e
**interação**, com baixo acoplamento:

```
                 +------------------+
   entrada  -->  |       CLI        |  (interação; só I/O e validação)
                 +--------+---------+
                          | delega
                          v
                 +------------------+        +----------------------+
                 |       ICU        |------->|  NewsCalculator      |
                 | (pacientes,      | usa    |  (News.hpp/.cpp)     |
                 |  leitos,         |        |  regra de negócio    |
                 |  histórico,      |        +----------------------+
                 |  observers)      |
                 +--------+---------+
                          | notifica (DashboardRow[])
              +-----------+-----------+
              v                       v
      +---------------+       +---------------+
      |  Dashboard    |       |     LED       |   (IICUObserver)
      +---------------+       +---------------+
```

### Responsabilidades das classes

| Componente        | Arquivo                     | Responsabilidade                                                                 |
|-------------------|-----------------------------|----------------------------------------------------------------------------------|
| `VitalSigns`, `NewsScore`, `Patient`, `DashboardRow`, enums | `Models.hpp` | Modelos de domínio + conversões de enum para texto. Não dependem de console.     |
| `NewsCalculator`  | `News.hpp` / `News.cpp`     | Regra pura do NEWS: pontua parâmetros, soma total, classifica risco. Sem estado. |
| `IICUObserver`    | `Observer.hpp`              | Interface do Observer Pattern (`update(rows)`), destrutor virtual.               |
| `ICU`             | `ICU.hpp` / `ICU.cpp`       | Serviço principal: pacientes, leitos, histórico, snapshot e notificação.         |
| `Dashboard`       | `Dashboard.hpp` / `.cpp`    | Observer que guarda o último snapshot e renderiza a tabela; a CLI o redesenha a cada ação. |
| `LED`             | `LED.hpp` / `.cpp`          | Observer que reflete o maior risco; não repete estado sem mudança.               |
| `Console`         | `Console.hpp`               | Sequências ANSI de cor. Não conhece regras de risco.                             |
| `CLI`             | `CLI.hpp` / `CLI.cpp`       | Camada de interação; delega tudo à `ICU`. Não calcula NEWS.                       |
| `main`            | `main.cpp`                  | Composição: instancia, registra observers e roda a CLI.                          |

**Decisões de design**

- **Observers desacoplados via `DashboardRow`**: a `ICU` publica um snapshot
  plano; os observers não conhecem a estrutura interna `Patient` nem a CLI.
- **Sem ownership por ponteiro cru**: `patients_` é `std::vector<Patient>`
  (valor). `observers_` guarda ponteiros **não proprietários** — a ICU apenas
  referencia observers cujo ciclo de vida é externo (documentado no header).
- **`std::optional<std::reference_wrapper<...>>`** para busca de paciente que
  pode não existir, sem cópia e sem ponteiros nulos.
- **Domínio não depende de cores**: `Models.hpp` não inclui `Console.hpp`,
  evitando dependência circular e acoplamento de domínio com apresentação.

---

## Preparação do ambiente (Ubuntu / WSL)

Pré-requisitos: **GCC** (C++17), **CMake ≥ 3.20**, **Ninja** e **Git**. O
GoogleTest é baixado automaticamente via `FetchContent` (requer internet na
primeira configuração).

Instale tudo de uma vez:

```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build git
```

> **Use o `apt`, não o `snap`.** Se rodar `cmake` e aparecer
> `Command 'cmake' not found`, é só a ferramenta ainda não instalada — rode o
> comando acima. A versão do `apt` atende ao requisito (CMake ≥ 3.20).

Verifique se ficou tudo disponível:

```bash
g++ --version && cmake --version && ninja --version
```

> **Terminal correto:** no VS Code, use um terminal **WSL/Ubuntu** (o prompt
> começa com `usuario@maquina:/mnt/c/...$`). Se aparecer `PS C:\...>`, é o
> PowerShell do Windows — troque para o perfil **Ubuntu (WSL)**.

## Build

Configuração e compilação:

```bash
cmake -S . -B build -G Ninja
cmake --build build
```

## Execução

```bash
./build/icu_news
```

Menu:

```
N - New NEWS score
H - Patient history
A - All patients
D - Discharge patient
X - Exit
```

As opções aceitam maiúsculas ou minúsculas. Em `N`, informa-se primeiro o
**leito (1–6)**; se estiver vazio, o sistema pede ID, nome e sobrenome e imprime
a identidade do paciente (`Bed 1 - 123 - John Doe`) antes de coletar os sinais
vitais na ordem da tabela. Se o leito já estiver ocupado, a medição é adicionada
ao paciente daquele leito, sem readmissão.

O **Dashboard** (tabela) e o **LED** (`Highest Risk Level: <cor>`) são
**observers** e se atualizam sozinhos a cada novo score e a cada alta. A opção
`A` renderiza o painel em tabela com o estado atual de todos os leitos:

```
ICU Dashboard
+------+------------+----------------------+-------+-------------+---------------------+
| Bed  | ID         | Patient              | Score | Risk        | Timestamp           |
+------+------------+----------------------+-------+-------------+---------------------+
| 1    | 123        | John Doe             | 8     | High        | 09 May 2025 14:00   |
+------+------------+----------------------+-------+-------------+---------------------+
```

### Faixas válidas de entrada

| Parâmetro          | Faixa   |
|--------------------|---------|
| Freq. respiratória | 0–150   |
| Saturação de O₂    | 50–100  |
| Temperatura (°C)   | 20–50   |
| Pressão sistólica  | 0–300   |
| Freq. cardíaca     | 15–250  |
| Consciência        | A, V, P, U |

Cada pergunta exibe a faixa e um exemplo (ex.: `Respiratory rate (bpm) [0-150,
Ex: 16]:`). Legenda do nível de consciência:

- **A** = Alert (alerta, desperto)
- **V** = Voice (responde à voz)
- **P** = Pain (responde à dor)
- **U** = Unresponsive (não responde)

Se qualquer parâmetro estiver fora da faixa (ou não for reconhecido), o programa
imprime `Invalid data. No NEWS score calculated` e não registra o score.

## Testes

O projeto tem duas camadas de teste, ambas em **GoogleTest** e descobertas pelo
`ctest`:

- **Unitários** (`tests/test_news.cpp`, `tests/test_icu.cpp`): exercitam a regra
  de negócio isolada (`NewsCalculator`, `ICU`, observers), sem console.
- **End-to-end** (`tests/test_e2e.cpp`): executam o **binário real** `icu_news`,
  alimentam a `stdin` com scripts — incluindo as demos `demo.txt`/`demo2.txt` —
  e validam a saída da CLI, cobrindo todos os caminhos do menu (cadastro,
  validações, risco, histórico, alta, dashboard, LED, entradas inválidas/EOF).

### Rodar todos os testes

```bash
# Configura, compila e roda toda a suíte (unitários + E2E):
./run_tests.sh

# Só rodar (build já feito):
ctest --test-dir build --output-on-failure
```

### Selecionar por nome/etiqueta

```bash
# Só os testes end-to-end (pelo nome, via regex do ctest):
ctest --test-dir build -R 'E2E_' --output-on-failure

# Só os unitários (exclui os E2E):
ctest --test-dir build -E 'E2E_' --output-on-failure

# Listar todos os testes registrados sem executar:
ctest --test-dir build -N
```

### Rodar os binários de teste diretamente (filtros do GoogleTest)

```bash
# Unitários:
./build/tests/unit_tests
./build/tests/unit_tests --gtest_list_tests           # lista as suítes/casos
./build/tests/unit_tests --gtest_filter='News*'       # cálculo, fronteiras e validação
./build/tests/unit_tests --gtest_filter='ICU.*'       # só a suíte ICU

# End-to-end:
./build/tests/e2e_tests
./build/tests/e2e_tests --gtest_list_tests            # lista os cenários
./build/tests/e2e_tests --gtest_filter='E2E_Demo.*'   # só as demos
./build/tests/e2e_tests --gtest_filter='E2E_Risk.*'   # só a classificação de risco
```

### Ver o app trabalhando durante os testes E2E

Com `ICU_E2E_VERBOSE=1`, cada cenário imprime a saída **colorida** do app
(dashboard e LED atualizando), precedida por `===== Suite.Teste =====`:

```bash
ICU_E2E_VERBOSE=1 ./build/tests/e2e_tests --gtest_color=yes
ICU_E2E_VERBOSE=1 ./build/tests/e2e_tests --gtest_filter='E2E_Risk.*' --gtest_color=yes
```

### Reproduzir uma sessão "ao vivo" (fora dos testes)

`watch_demo.sh` alimenta o app linha a linha com uma pausa entre cada uma, como
se alguém digitasse, para acompanhar o dashboard/LED sendo redesenhados:

```bash
./watch_demo.sh                 # demo.txt, 0.6s por linha (padrão)
./watch_demo.sh demo2.txt       # a outra demo (validações/entrada inválida)
./watch_demo.sh demo.txt 1.2    # mais devagar
```

Ou simplesmente rodar o binário com um script na entrada (rápido, sem pausas):

```bash
./build/icu_news < demo.txt
./build/icu_news < demo2.txt
```

---

## Protocolo NEWS

Cada parâmetro contribui de 0 a 3 pontos:

| Parâmetro              | 3        | 2        | 1          | 0         | 1         | 2      | 3     |
|------------------------|----------|----------|------------|-----------|-----------|--------|-------|
| Freq. respiratória     | ≤8       | –        | 9–11       | 12–20     | –         | 21–24  | ≥25   |
| SpO₂ (%)               | ≤91      | 92–93    | 94–95      | ≥96       | –         | –      | –     |
| O₂ suplementar         | –        | Sim      | –          | Não       | –         | –      | –     |
| Temperatura (°C)       | ≤35.0    | –        | 35.1–36.0  | 36.1–38.0 | 38.1–39.0 | ≥39.1  | –     |
| Pressão sistólica      | ≤90      | 91–100   | 101–110    | 111–219   | –         | –      | ≥220  |
| Freq. cardíaca         | ≤40      | –        | 41–50      | 51–90     | 91–110    | 111–130| ≥131  |
| Consciência (A/V/P/U)  | V, P, U  | –        | –          | A (Alert) | –         | –      | –     |

**Classificação de risco:**

- **High** — total ≥ 7
- **Medium** — total 5–6
- **Low-Medium** — total 0–4 **com** algum parâmetro individual pontuando 3
- **Low** — total 0–4 sem nenhum parâmetro individual em 3

**LED (maior risco atual):** o LED imprime `Highest Risk Level: <cor>`, onde
Low → `Green`; Low-Medium → `Yellow`; Medium → `Orange`; High → `Red`; sem
medição → `Off`.

---

## Tasks implementadas

- **Task 1 — Base.** Fluxo `N` centrado no leito (1–6), cadastro com ID/nome/
  sobrenome, `NewsCalculator` isolado, **validação das faixas de entrada**
  (`Invalid data. No NEWS score calculated`), classificação Low/Low-Medium/
  Medium/High, histórico de scores e menu `N/H/A/X`. `ICU` responsável por
  pacientes, leitos e histórico; CLI apenas para interação.
- **Task 2 — Observer Pattern.** `IICUObserver`, `Dashboard` (`updateDashboard`)
  e `LED` (`updateRiskLevel`). Cada novo score notifica os observers, que
  imprimem a tabela do painel e `Highest Risk Level: <cor>`.
- **Task 3 — Alta.** Opção `D`: solicita ID; se não existir imprime
  `Patient not found. Please try again`; se existir confirma `Y/N`, remove o
  paciente, libera o leito e atualiza Dashboard e LED.
- **Task 4 — Testes.** GoogleTest com testes **unitários** (cálculo,
  classificação, fronteiras de todas as faixas, **validação de faixas**,
  cadastro/leitos, histórico, alta e observers) e testes **end-to-end** que
  rodam o binário real alimentando as demos e cobrindo todos os caminhos da CLI.

---

## Limitações conhecidas

- **Sem persistência**: os dados existem apenas em memória durante a execução.
- **Console apenas**: sem interface gráfica; cores dependem de terminal com
  suporte a ANSI.
- **Escopo fixo de 6 leitos** (constante `ICU::kMaxBeds`).
- **Sem internacionalização**: mensagens em inglês; textos de domínio fixos.
