#include "estimation/ValidationRunner.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

#ifndef ESTIMATION_SCENARIO_FILE
#define ESTIMATION_SCENARIO_FILE "validation/estimation/scenarios.json"
#endif
int main(int argc,char** argv) {
    try {
        std::string scenario_path=ESTIMATION_SCENARIO_FILE,output="build/estimation-results",golden;
        std::size_t count=16; std::uint64_t master=20260905;
        for(int i=1;i<argc;++i) {
            const std::string key=argv[i];
            if(key=="--help") {std::cout<<"estimation_validate [--scenarios file] [--output directory] [--trials N] [--seed N] [--golden directory]\n";return 0;}
            if(i+1==argc) throw std::invalid_argument("option requires value");
            const std::string value=argv[++i];
            if(key=="--scenarios") scenario_path=value;
            else if(key=="--output") output=value;
            else if(key=="--golden") golden=value;
            else if(key=="--trials" || key=="--seed") {
                if(value.empty() || value.find_first_not_of("0123456789")!=std::string::npos) throw std::invalid_argument("unsigned integer required");
                if(key=="--trials") count=std::stoull(value); else master=std::stoull(value);
            } else throw std::invalid_argument("unknown option: "+key);
        }
        if(count==0) throw std::invalid_argument("trial count must be positive");
        using namespace estimation::validation;
        auto fixture=verticalSixDofFixture(); auto truth=fixture.trajectory();
        auto scenarios=loadScenarios(scenario_path); nlohmann::json report;
        report["schema_version"]=1; report["master_seed"]=master; report["trials_per_scenario"]=count;
        for(auto& scenario:scenarios) {
            scenario.sensors.seed=master;
            const auto realization=sensors::simulate(truth,scenario.sensors);
            const auto runs=compare(truth,realization.measurements,{{"VerticalKalman",[&](){return std::make_unique<estimation::VerticalKalman>(scenario.estimator);}}});
            report["scenarios"][scenario.name]=metricsJson(runs[0].metrics);
            report["monte_carlo"][scenario.name]=monteCarloJson(monteCarlo(truth,scenario,master,count));
            exportDataset((std::filesystem::path(output)/scenario.name).string(),fixture,scenario,realization,runs[0]);
            if(!golden.empty() && scenario.name=="NOMINAL") exportDataset(golden,fixture,scenario,realization,runs[0]);
            const auto& m=runs[0].metrics;
            std::cout<<scenario.name<<" h_rmse="<<m.altitude.rmse<<" v_rmse="<<m.velocity.rmse
                <<" apogee_error="<<m.apogee_altitude_error_m<<" NIS_inside="<<m.nis.inside_fraction<<'\n';
        }
        std::filesystem::create_directories(output);
        std::ofstream file(std::filesystem::path(output)/"summary.json");
        file.exceptions(std::ios::badbit|std::ios::failbit); file<<report.dump(2)<<'\n';
    } catch(const std::exception& e) {std::cerr<<"estimation validation failed: "<<e.what()<<'\n';return 1;}
    return 0;
}
