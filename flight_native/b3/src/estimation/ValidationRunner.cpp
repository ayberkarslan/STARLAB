#include "estimation/ValidationRunner.hpp"
#include <algorithm>
#include <cmath>
#include <map>
#include <numeric>
#include <stdexcept>

namespace estimation::validation {
ConsistencySummary consistency(const std::vector<double>& values,unsigned dof) {
    // Central 95% chi-square intervals, DOF 1 (pressure NIS) and 2 (marginal h/v NEES).
    if(dof!=1 && dof!=2) throw std::invalid_argument("supported consistency DOF: 1 or 2");
    ConsistencySummary result;
    result.degrees_of_freedom=dof;
    result.lower=dof==1?0.0009820691171752555:0.05063561596857975;
    result.upper=dof==1?5.023886187314888:7.377758908227871;
    result.count=values.size();
    if(values.empty()) return result;
    for(double x:values) {
        if(!std::isfinite(x) || x<0) throw std::invalid_argument("invalid normalized error");
        result.mean+=x;
        result.inside_fraction+=(x>=result.lower && x<=result.upper)?1:0;
    }
    result.mean/=values.size(); result.inside_fraction/=values.size(); return result;
}
namespace {
Metrics score(const sensors::TruthTrajectory& truth, const std::vector<EstimateRecord>& history,
              const std::vector<Innovation>& innovations) {
    if(history.empty()) throw std::invalid_argument("no valid estimates to score");
    // One most recently delivered estimate per acquisition horizon, avoiding duplicate-time weighting.
    std::map<double,EstimateRecord> epochs;
    for(const auto& row:history) epochs[row.state.timestamp]=row;
    std::vector<double> h,v,nees,nis;
    double true_peak=-1e300, estimated_peak=-1e300;
    std::optional<double> truth_apogee,estimated_apogee;
    std::optional<EstimateRecord> previous;
    for(const auto& kv:epochs) {
        const auto& row=kv.second; const auto t=truth.at(row.state.timestamp);
        h.push_back(row.altitude_error_m); v.push_back(row.velocity_error_mps);
        if(row.state.covariance) nees.push_back(row.nees_hv);
        true_peak=std::max(true_peak,t.position.z()); estimated_peak=std::max(estimated_peak,row.state.altitude_m);
        if(previous) {
            const auto t0=truth.at(previous->state.timestamp);
            if(!truth_apogee && t0.velocity.z()>0 && t.velocity.z()<=0)
                truth_apogee=t0.time+(t.time-t0.time)*t0.velocity.z()/(t0.velocity.z()-t.velocity.z());
        }
        previous=row;
    }
    // Robust local peak: first + to - velocity crossing within 0.5 s of the global
    // estimated altitude maximum, rejecting spurious pad-noise crossings.
    double peak_time=epochs.begin()->first;
    for(const auto& kv:epochs) if(kv.second.state.altitude_m==estimated_peak) peak_time=kv.first;
    previous.reset();
    for(const auto& kv:epochs) {
        const auto& row=kv.second;
        if(previous && !estimated_apogee && previous->state.vertical_velocity_mps>0 &&
           row.state.vertical_velocity_mps<=0 && std::abs(row.state.timestamp-peak_time)<=.5) {
            const double v0=previous->state.vertical_velocity_mps,v1=row.state.vertical_velocity_mps;
            estimated_apogee=previous->state.timestamp+(row.state.timestamp-previous->state.timestamp)*v0/(v0-v1);
        }
        previous=row;
    }
    for(const auto& x:innovations) nis.push_back(x.nis);
    Metrics result;
    result.altitude=sensors::validation::errors(h); result.velocity=sensors::validation::errors(v);
    result.apogee_altitude_error_m=estimated_peak-true_peak;
    if(truth_apogee && estimated_apogee) result.apogee_time_error_s=*estimated_apogee-*truth_apogee;
    result.nis=consistency(nis,1); result.nees_hv=consistency(nees,2);
    return result;
}
}
std::vector<EstimatorResult> compare(const sensors::TruthTrajectory& truth,
    const sensors::MeasurementStream& measurements,
    const std::vector<std::pair<std::string,Factory>>& factories,const InitialState& initial) {
    if(factories.empty()) throw std::invalid_argument("no estimators supplied");
    std::vector<std::unique_ptr<StateEstimator>> estimators;
    std::vector<EstimatorResult> results;
    for(const auto& factory:factories) {
        auto e=factory.second(); if(!e) throw std::invalid_argument("null estimator factory");
        e->reset(initial); estimators.push_back(std::move(e));
        EstimatorResult result; result.name=factory.first; results.push_back(std::move(result));
    }
    double last_delivery=-1e300;
    for(const auto& m:measurements) {
        const auto stamp=std::visit([](const auto& sample){return sample.stamp;},m.sample);
        if(!std::isfinite(stamp.measurement_timestamp) || stamp.measurement_timestamp<last_delivery)
            throw std::invalid_argument("comparison requires delivery-sorted measurements");
        last_delivery=stamp.measurement_timestamp;
        for(std::size_t i=0;i<estimators.size();++i) {
            estimators[i]->consume(m); const auto s=estimators[i]->state();
            if(!s.initialized || !s.valid) continue;
            if(m.kind!=sensors::SensorKind::Accelerometer && m.kind!=sensors::SensorKind::Barometer) continue;
            const auto t=truth.at(s.timestamp);
            EstimateRecord row{stamp.measurement_timestamp,s,s.altitude_m-t.position.z(),s.vertical_velocity_mps-t.velocity.z(),0};
            if(s.covariance) {
                // Bias truth is not in GroundTruth. Score only unambiguous marginal h/v.
                const Eigen::Vector2d error(row.altitude_error_m,row.velocity_error_mps);
                row.nees_hv=sensors::validation::normalizedSquaredError(error,s.covariance->topLeftCorner<2,2>());
            }
            results[i].history.push_back(std::move(row));
        }
    }
    for(std::size_t i=0;i<estimators.size();++i) {
        results[i].innovations=estimators[i]->innovations();
        results[i].metrics=score(truth,results[i].history,results[i].innovations);
    }
    return results;
}
Aggregate aggregate(const std::vector<double>& values) {
    const auto metrics=sensors::validation::errors(values);
    // All aggregate inputs are nonnegative performance measures.
    for(double v:values) if(v<0) throw std::invalid_argument("aggregate requires nonnegative values");
    return {metrics.mean,metrics.p50_absolute,metrics.standard_deviation,metrics.p95_absolute,metrics.max_absolute};
}
MonteCarloResult monteCarlo(const sensors::TruthTrajectory& truth,const Scenario& scenario,
                          std::uint64_t master,std::size_t count) {
    if(count==0) throw std::invalid_argument("Monte Carlo trial count must be positive");
    MonteCarloResult result;
    std::vector<double> hr,vr,hm,vm,ap,ni,nf;
    for(std::size_t k=0;k<count;++k) {
        auto config=scenario.sensors; config.seed=sensors::deriveSeed(master,k);
        result.seeds.push_back(config.seed);
        const auto realization=sensors::simulate(truth,config);
        InitialState initial; initial.timestamp=truth.begin();
        const auto runs=compare(truth,realization.measurements,{{"VerticalKalman",[&](){return std::make_unique<VerticalKalman>(scenario.estimator);}}},initial);
        const auto& m=runs[0].metrics; result.trials.push_back(m);
        hr.push_back(m.altitude.rmse); vr.push_back(m.velocity.rmse);
        hm.push_back(m.altitude.max_absolute); vm.push_back(m.velocity.max_absolute);
        ap.push_back(std::abs(m.apogee_altitude_error_m)); ni.push_back(m.nis.mean); nf.push_back(m.nis.inside_fraction);
    }
    result.altitude_rmse=aggregate(hr); result.velocity_rmse=aggregate(vr);
    result.altitude_max=aggregate(hm); result.velocity_max=aggregate(vm);
    result.apogee_absolute_error=aggregate(ap); result.mean_nis=aggregate(ni); result.nis_inside_fraction=aggregate(nf);
    result.nis_inside_fraction.worst=*std::min_element(nf.begin(),nf.end());
    return result;
}
nlohmann::json metricsJson(const Metrics& m) {
    auto errors=[](const sensors::validation::ErrorMetrics& e) {
        return nlohmann::json{{"rmse",e.rmse},{"mae",e.mae},{"mean_error",e.mean},{"standard_deviation",e.standard_deviation},
            {"max_absolute",e.max_absolute},{"p50_absolute",e.p50_absolute},{"p95_absolute",e.p95_absolute},{"p99_absolute",e.p99_absolute}};
    };
    auto consistency=[](const ConsistencySummary& c) {
        return nlohmann::json{{"count",c.count},{"dof",c.degrees_of_freedom},{"mean",c.mean},
            {"inside_fraction",c.inside_fraction},{"lower_95",c.lower},{"upper_95",c.upper}};
    };
    nlohmann::json j={{"altitude",errors(m.altitude)},{"velocity",errors(m.velocity)},
        {"apogee_altitude_error_m",m.apogee_altitude_error_m},{"nis",consistency(m.nis)},{"nees_hv",consistency(m.nees_hv)}};
    j["apogee_time_error_s"]=m.apogee_time_error_s?nlohmann::json(*m.apogee_time_error_s):nlohmann::json(nullptr);
    return j;
}
nlohmann::json monteCarloJson(const MonteCarloResult& m) {
    auto a=[](const Aggregate& x) {return nlohmann::json{{"mean",x.mean},{"median",x.median},{"standard_deviation",x.standard_deviation},{"p95",x.p95},{"worst",x.worst}};};
    return {{"trial_count",m.trials.size()},{"seeds",m.seeds},{"altitude_rmse",a(m.altitude_rmse)},
        {"velocity_rmse",a(m.velocity_rmse)},{"altitude_max",a(m.altitude_max)},{"velocity_max",a(m.velocity_max)},
        {"apogee_absolute_error",a(m.apogee_absolute_error)},{"mean_nis",a(m.mean_nis)},{"nis_inside_fraction",a(m.nis_inside_fraction)}};
}
}
