#pragma once

#include <sstream>
#include <string>
#include <vector>

#include "aero/FinPlanform.hpp"
#include "nlohmann/json.hpp"

namespace freeform_planform {

using json = nlohmann::json;

inline double number(
    const json& object,
    const char* key,
    double fallback = 0.0
) {
    if (!object.is_object() || !object.contains(key)
        || object[key].is_null()) {
        return fallback;
    }
    return object[key].get<double>();
}

inline std::vector<FinPlanform::Point> pointsFromParams(
    const json& params
) {
    std::vector<FinPlanform::Point> points;
    if (params.is_object() && params.contains("source_points")) {
        const json& source = params["source_points"];
        if (source.is_string()) {
            std::stringstream tokens(source.get<std::string>());
            std::string token;
            while (std::getline(tokens, token, ';')) {
                const std::size_t comma = token.find(',');
                if (comma == std::string::npos) {
                    continue;
                }
                try {
                    points.push_back({
                        std::stod(token.substr(0, comma)),
                        std::stod(token.substr(comma + 1)),
                    });
                } catch (const std::exception&) {
                    continue;
                }
            }
        } else if (source.is_array()) {
            for (const json& point : source) {
                if (point.is_array() && point.size() >= 2) {
                    points.push_back({
                        point[0].get<double>(), point[1].get<double>()});
                } else if (point.is_object()) {
                    points.push_back({
                        number(point, "x"), number(point, "y")});
                }
            }
        }
    }
    if (points.size() < 3) {
        const double root = number(params, "root_chord", 0.25);
        const double tip = number(params, "tip_chord", 0.1);
        const double span = number(params, "span", 0.12);
        const double sweep = number(params, "sweep_length", 0.1);
        points = {
            {0.0, 0.0},
            {root, 0.0},
            {sweep + tip, span},
            {sweep, span},
        };
    }
    return points;
}

inline FinPlanform::Metrics metricsFromParams(const json& params) {
    return FinPlanform::calculate(pointsFromParams(params));
}

}  // namespace freeform_planform
