#pragma once
#include <algorithm>
#include <cctype>
#include <string>
namespace RoketSiM::DatcomIntegration {
// Product semantics only. These identifiers do not select new numerical methods.
enum class FinSectionFamily { Square, Rounded, Airfoil, Diamond, Biconvex, Hex, Unknown };
enum class FinSectionSupport { Exact, ApproximatedWithWarning, NotImplemented };
inline FinSectionFamily finSectionFromString(std::string value) {
    const auto begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) return FinSectionFamily::Unknown;
    value = value.substr(begin, value.find_last_not_of(" \t\r\n") - begin + 1);
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return std::tolower(c); });
    if (value == "square") return FinSectionFamily::Square;
    if (value == "rounded") return FinSectionFamily::Rounded;
    if (value == "airfoil") return FinSectionFamily::Airfoil;
    if (value == "diamond") return FinSectionFamily::Diamond;
    if (value == "biconvex") return FinSectionFamily::Biconvex;
    if (value == "hex" || value == "hexagonal") return FinSectionFamily::Hex;
    return FinSectionFamily::Unknown;
}
inline std::string toString(FinSectionFamily value) {
    switch (value) {
    case FinSectionFamily::Square: return "square";
    case FinSectionFamily::Rounded: return "rounded";
    case FinSectionFamily::Airfoil: return "airfoil";
    case FinSectionFamily::Diamond: return "diamond";
    case FinSectionFamily::Biconvex: return "biconvex";
    case FinSectionFamily::Hex: return "hex";
    default: return "unknown";
    }
}
inline FinSectionSupport finSectionSupport(FinSectionFamily value) {
    // Diamond is the only product family whose persisted definition supplies
    // the complete sharp, symmetric, mid-chord, zero-flat provider geometry.
    // Generic HEX has no persisted LMAX/LFLAT/LER definition and must not
    // inherit Diamond's values merely because the provider token is "HEX".
    return value == FinSectionFamily::Diamond
        ? FinSectionSupport::Exact : FinSectionSupport::NotImplemented;
}
inline std::string toString(FinSectionSupport value) {
    switch (value) {
    case FinSectionSupport::Exact: return "EXACT";
    case FinSectionSupport::ApproximatedWithWarning: return "APPROXIMATED_WITH_WARNING";
    default: return "NOT_IMPLEMENTED";
    }
}
}
