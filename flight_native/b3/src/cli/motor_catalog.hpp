#pragma once

#include <memory>
#include <stdexcept>
#include <string>

#include <sqlite3.h>

#include "nlohmann/json.hpp"

namespace motor_catalog {

using json = nlohmann::json;

struct DatabaseCloser {
    void operator()(sqlite3* database) const {
        if (database != nullptr) sqlite3_close(database);
    }
};

using Database = std::unique_ptr<sqlite3, DatabaseCloser>;

inline Database openReadOnly(const std::string& databasePath) {
    sqlite3* raw = nullptr;
    const int status = sqlite3_open_v2(databasePath.c_str(), &raw,
        SQLITE_OPEN_READONLY, nullptr);
    if (status != SQLITE_OK) {
        const std::string message = raw != nullptr
            ? sqlite3_errmsg(raw) : "SQLite database could not be opened.";
        if (raw != nullptr) sqlite3_close(raw);
        throw std::runtime_error("Motor database error: " + message);
    }
    return Database(raw);
}

inline json catalog(const std::string& databasePath) {
    const Database database = openReadOnly(databasePath);
    constexpr const char* query = R"sql(
        SELECT m.id, mf.name, m.designation, COALESCE(m.common_name, m.designation),
               COALESCE(m.impulse_class, ''), COALESCE(m.diameter, 0),
               COALESCE(m.length, 0), COALESCE(m.total_impulse, 0),
               COALESCE(m.avg_thrust, 0), COALESCE(m.max_thrust, 0),
               COALESCE(m.burn_time, 0), COALESCE(m.propellant_weight, 0),
               COALESCE(m.total_weight, 0), COALESCE(m.type, ''),
               COALESCE(m.prop_info, ''), COALESCE(m.case_info, '')
          FROM motors AS m
          JOIN manufacturers AS mf ON mf.id = m.manufacturer_id
         ORDER BY mf.name COLLATE NOCASE, m.impulse_class, m.designation COLLATE NOCASE
    )sql";
    sqlite3_stmt* rawStatement = nullptr;
    if (sqlite3_prepare_v2(database.get(), query, -1, &rawStatement, nullptr)
        != SQLITE_OK) {
        throw std::runtime_error("Motor catalog query failed: "
            + std::string(sqlite3_errmsg(database.get())));
    }
    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> statement(
        rawStatement, sqlite3_finalize);

    json motors = json::array();
    while (sqlite3_step(statement.get()) == SQLITE_ROW) {
        motors.push_back({
            {"motor_id", sqlite3_column_int(statement.get(), 0)},
            {"manufacturer", reinterpret_cast<const char*>(sqlite3_column_text(statement.get(), 1))},
            {"designation", reinterpret_cast<const char*>(sqlite3_column_text(statement.get(), 2))},
            {"name", reinterpret_cast<const char*>(sqlite3_column_text(statement.get(), 3))},
            {"class", reinterpret_cast<const char*>(sqlite3_column_text(statement.get(), 4))},
            {"diameter_mm", sqlite3_column_double(statement.get(), 5)},
            {"length_mm", sqlite3_column_double(statement.get(), 6)},
            {"total_impulse_Ns", sqlite3_column_double(statement.get(), 7)},
            {"avg_thrust_N", sqlite3_column_double(statement.get(), 8)},
            {"max_thrust_N", sqlite3_column_double(statement.get(), 9)},
            {"burn_time_s", sqlite3_column_double(statement.get(), 10)},
            {"propellant_mass_g", sqlite3_column_double(statement.get(), 11)},
            {"total_mass_g", sqlite3_column_double(statement.get(), 12)},
            {"motor_type", reinterpret_cast<const char*>(sqlite3_column_text(statement.get(), 13))},
            {"propellant", reinterpret_cast<const char*>(sqlite3_column_text(statement.get(), 14))},
            {"case_info", reinterpret_cast<const char*>(sqlite3_column_text(statement.get(), 15))},
            {"database_motor", true},
        });
    }
    return {{"motors", motors}};
}

inline json curve(const std::string& databasePath, int motorId) {
    const Database database = openReadOnly(databasePath);
    constexpr const char* curveQuery = R"sql(
        SELECT id FROM thrust_curves WHERE motor_id = ? ORDER BY id LIMIT 1
    )sql";
    sqlite3_stmt* rawCurve = nullptr;
    if (sqlite3_prepare_v2(database.get(), curveQuery, -1, &rawCurve, nullptr)
        != SQLITE_OK) throw std::runtime_error("Motor curve query could not be prepared.");
    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> curveStatement(
        rawCurve, sqlite3_finalize);
    sqlite3_bind_int(curveStatement.get(), 1, motorId);
    if (sqlite3_step(curveStatement.get()) != SQLITE_ROW) {
        throw std::runtime_error("No thrust curve exists for selected motor.");
    }
    const int curveId = sqlite3_column_int(curveStatement.get(), 0);

    constexpr const char* pointQuery = R"sql(
        SELECT time_seconds, force_newtons FROM thrust_data
         WHERE curve_id = ? ORDER BY time_seconds, id
    )sql";
    sqlite3_stmt* rawPoints = nullptr;
    if (sqlite3_prepare_v2(database.get(), pointQuery, -1, &rawPoints, nullptr)
        != SQLITE_OK) throw std::runtime_error("Thrust-data query could not be prepared.");
    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> pointStatement(
        rawPoints, sqlite3_finalize);
    sqlite3_bind_int(pointStatement.get(), 1, curveId);
    json points = json::array();
    while (sqlite3_step(pointStatement.get()) == SQLITE_ROW) {
        points.push_back({sqlite3_column_double(pointStatement.get(), 0),
            sqlite3_column_double(pointStatement.get(), 1)});
    }
    if (points.size() < 2) throw std::runtime_error("Selected motor curve has fewer than two points.");
    return {{"motor_id", motorId}, {"curve_id", curveId}, {"curve_points", points}};
}

}  // namespace motor_catalog
