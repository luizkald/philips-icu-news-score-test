#pragma once

#include "Models.hpp"

#include <string>

// Console concentra as sequências ANSI de formatação do terminal. Não conhece
// regras de risco; apenas expõe constantes e utilitários de cor. Models.hpp
// NÃO depende deste header (evita dependência circular / acoplamento de domínio
// com apresentação).
namespace icu {
namespace Console {

inline constexpr const char* reset  = "\033[0m";
inline constexpr const char* bold   = "\033[1m";
inline constexpr const char* dim    = "\033[2m";
inline constexpr const char* red    = "\033[31m";
inline constexpr const char* green  = "\033[32m";
inline constexpr const char* yellow = "\033[33m";
inline constexpr const char* cyan   = "\033[36m";
inline constexpr const char* orange = "\033[38;5;208m"; // 256-color

// Ponto de inicialização (no Linux/WSL as sequências ANSI já são nativas,
// portanto é um no-op; existe para simetria com o main e portabilidade futura).
inline void init() {}

// Cor associada a um nível de risco, para uso na apresentação.
inline const char* colorFor(RiskLevel level) {
    switch (level) {
        case RiskLevel::Low:       return green;
        case RiskLevel::LowMedium: return yellow;
        case RiskLevel::Medium:    return orange;
        case RiskLevel::High:      return red;
    }
    return reset;
}

} // namespace Console
} // namespace icu
