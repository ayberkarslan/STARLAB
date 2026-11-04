#pragma once

// Whole-vehicle aerodynamic tables exported by pyrasaero/RASAero II.
// The format is Mach, Reynolds, AoA_deg, CA_off, CA_on, CN, CP_m,
// CN_alpha_per_rad.  This reader intentionally has no dependency on the
// SU2 CFD database: it is an optional flight-solver input only.

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace rasaero_table {

struct Coefficients {
    double caOff = 0.0;
    double caOn = 0.0;
    double cn = 0.0;
    double cpM = 0.0;
    double cnAlphaPerRad = 0.0;
};

class Table {
public:
    bool empty() const noexcept { return values_.empty(); }

    void loadCsv(const std::string& path) {
        std::ifstream input(path);
        if (!input.is_open()) {
            throw std::runtime_error("Cannot open RASAero aerodynamic table: " + path);
        }
        std::string line;
        if (!std::getline(input, line)) {
            throw std::runtime_error("RASAero aerodynamic table is empty: " + path);
        }
        if (line.size() >= 3 && static_cast<unsigned char>(line[0]) == 0xEF) {
            line.erase(0, 3); // UTF-8 BOM
        }
        const auto header = split(line);
        const auto col = [&](const char* name) {
            const auto found = std::find(header.begin(), header.end(), name);
            if (found == header.end()) {
                throw std::runtime_error(std::string("RASAero table missing column: ") + name);
            }
            return static_cast<std::size_t>(found - header.begin());
        };
        const auto machCol = col("Mach"), reCol = col("Reynolds"), aoaCol = col("AoA_deg");
        const auto offCol = col("CA_off"), onCol = col("CA_on"), cnCol = col("CN");
        const auto cpCol = col("CP_m"), cnaCol = col("CN_alpha_per_rad");

        struct Raw { double mach, re, aoa; Coefficients coeffs; };
        std::vector<Raw> rows;
        while (std::getline(input, line)) {
            if (line.empty()) continue;
            const auto fields = split(line);
            const auto at = [&](std::size_t index) {
                if (index >= fields.size()) throw std::runtime_error("Malformed RASAero table row");
                return std::stod(fields[index]);
            };
            rows.push_back({at(machCol), at(reCol), at(aoaCol),
                {at(offCol), at(onCol), at(cnCol), at(cpCol), at(cnaCol)}});
        }
        if (rows.empty()) throw std::runtime_error("RASAero aerodynamic table has no data rows: " + path);

        mach_.clear(); reynolds_.clear(); aoa_.clear(); values_.clear();
        for (const auto& row : rows) {
            mach_.push_back(row.mach); reynolds_.push_back(row.re); aoa_.push_back(row.aoa);
        }
        uniqueSort(mach_); uniqueSort(reynolds_); uniqueSort(aoa_);
        const std::size_t count = mach_.size() * reynolds_.size() * aoa_.size();
        values_.assign(count, {});
        present_.assign(count, false);
        for (const auto& row : rows) {
            const auto index = flatIndex(indexOf(mach_, row.mach), indexOf(reynolds_, row.re), indexOf(aoa_, row.aoa));
            values_[index] = row.coeffs;
            present_[index] = true;
        }
        if (std::find(present_.begin(), present_.end(), false) != present_.end()) {
            throw std::runtime_error("RASAero table must contain a complete Mach x Reynolds x AoA grid");
        }
    }

    Coefficients interpolate(double mach, double reynolds, double aoaDeg) const {
        if (empty()) throw std::runtime_error("RASAero table was not loaded");
        const auto mx = bracket(mach_, mach);
        const auto rx = bracket(reynolds_, reynolds);
        const auto ax = bracket(aoa_, aoaDeg);
        Coefficients result;
        for (int im = 0; im < 2; ++im) for (int ir = 0; ir < 2; ++ir) for (int ia = 0; ia < 2; ++ia) {
            const double weight = (im ? mx.fraction : 1.0 - mx.fraction)
                * (ir ? rx.fraction : 1.0 - rx.fraction)
                * (ia ? ax.fraction : 1.0 - ax.fraction);
            const auto& v = values_[flatIndex(im ? mx.upper : mx.lower, ir ? rx.upper : rx.lower, ia ? ax.upper : ax.lower)];
            result.caOff += weight * v.caOff; result.caOn += weight * v.caOn;
            result.cn += weight * v.cn; result.cpM += weight * v.cpM;
            result.cnAlphaPerRad += weight * v.cnAlphaPerRad;
        }
        return result;
    }

private:
    struct Bracket { std::size_t lower, upper; double fraction; };
    static std::vector<std::string> split(const std::string& line) {
        std::vector<std::string> fields; std::stringstream stream(line); std::string field;
        while (std::getline(stream, field, ',')) fields.push_back(field);
        return fields;
    }
    static void uniqueSort(std::vector<double>& values) {
        std::sort(values.begin(), values.end());
        values.erase(std::unique(values.begin(), values.end()), values.end());
    }
    static std::size_t indexOf(const std::vector<double>& values, double value) {
        const auto it = std::lower_bound(values.begin(), values.end(), value);
        if (it == values.end() || *it != value) throw std::runtime_error("Invalid RASAero grid value");
        return static_cast<std::size_t>(it - values.begin());
    }
    static Bracket bracket(const std::vector<double>& axis, double value) {
        if (axis.size() == 1 || value <= axis.front()) return {0, 0, 0.0};
        if (value >= axis.back()) { const auto last = axis.size() - 1; return {last, last, 0.0}; }
        const auto upper = static_cast<std::size_t>(std::upper_bound(axis.begin(), axis.end(), value) - axis.begin());
        const auto lower = upper - 1;
        return {lower, upper, (value - axis[lower]) / (axis[upper] - axis[lower])};
    }
    std::size_t flatIndex(std::size_t mach, std::size_t re, std::size_t aoa) const {
        return (mach * reynolds_.size() + re) * aoa_.size() + aoa;
    }
    std::vector<double> mach_, reynolds_, aoa_;
    std::vector<Coefficients> values_;
    std::vector<bool> present_;
};

} // namespace rasaero_table
