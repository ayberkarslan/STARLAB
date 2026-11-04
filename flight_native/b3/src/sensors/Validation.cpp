#include "sensors/Validation.hpp"
#include <unsupported/Eigen/FFT>
#include <algorithm>
#include <numeric>
#include <stdexcept>
namespace sensors::validation {
WelchResult welch(const std::vector<double>& x,double rate,std::size_t n) {
    if(!std::isfinite(rate)||rate<=0||n<4||n%2||x.size()<n) throw std::invalid_argument("invalid Welch grid");
    for(double v:x) if(!std::isfinite(v)) throw std::invalid_argument("nonfinite Welch input");
    WelchResult r; r.frequency_hz.resize(n/2+1);r.density.assign(n/2+1,0);
    std::vector<double> window(n),segment(n); double energy=0;
    for(std::size_t i=0;i<n;++i) {window[i]=0.5-0.5*std::cos(2*pi*i/n);energy+=window[i]*window[i];}
    Eigen::FFT<double> fft;std::vector<std::complex<double>> spectrum;std::size_t count=0;
    for(std::size_t start=0;start+n<=x.size();start+=n/2) {
        double mean=std::accumulate(x.begin()+start,x.begin()+start+n,0.0)/n;
        for(std::size_t i=0;i<n;++i) segment[i]=(x[start+i]-mean)*window[i];
        fft.fwd(spectrum,segment);
        for(std::size_t i=0;i<=n/2;++i) r.density[i]+=std::norm(spectrum[i])*(i==0||i==n/2?1:2)/(rate*energy);
        ++count;
    }
    for(std::size_t i=0;i<=n/2;++i) {r.frequency_hz[i]=i*rate/n;r.density[i]/=count;r.integrated_power+=r.density[i]*rate/n;}
    return r;
}
PsdConsistency checkPsd(const std::vector<double>& x,double rate,std::size_t n,double target,double tolerance) {
    if(!std::isfinite(target)||target<0||!std::isfinite(tolerance)||tolerance<0) throw std::invalid_argument("invalid PSD check");
    auto w=welch(x,rate,n); double power=0;for(double v:x) power+=v*v;power/=x.size();
    double rel=target>0?std::abs(power-target)/target:(power==0?0:std::numeric_limits<double>::infinity());
    const bool spectral=target>0?std::abs(w.integrated_power-target)/target<=tolerance:w.integrated_power==0;
    return {power,w.integrated_power,target,rel,rel<=tolerance&&spectral};
}
ErrorMetrics errors(const std::vector<double>& x) {
    if(x.empty()) throw std::invalid_argument("empty error series");
    double sum=0,squares=0,absolute=0;std::vector<double> a;
    for(double v:x) {if(!std::isfinite(v)) throw std::invalid_argument("nonfinite error");sum+=v;squares+=v*v;absolute+=std::abs(v);a.push_back(std::abs(v));}
    const double mean=sum/x.size();double variance=0;for(double v:x) variance+=(v-mean)*(v-mean);variance/=x.size();
    std::sort(a.begin(),a.end());auto percentile=[&](double p){double k=p*(a.size()-1);auto lo=std::size_t(k),hi=std::min(lo+1,a.size()-1);return a[lo]+(a[hi]-a[lo])*(k-lo);};
    return {std::sqrt(squares/x.size()),absolute/x.size(),mean,std::sqrt(variance),a.back(),percentile(.5),percentile(.95),percentile(.99)};
}
double attitudeError(const Eigen::Quaterniond& a,const Eigen::Quaterniond& b) {
    if(!a.coeffs().allFinite()||!b.coeffs().allFinite()||a.norm()<1e-12||b.norm()<1e-12) throw std::invalid_argument("invalid attitude");
    return 2*std::acos(std::clamp(std::abs(a.normalized().dot(b.normalized())),0.0,1.0));
}
double normalizedSquaredError(const Eigen::VectorXd& e,const Eigen::MatrixXd& c) {
    if(e.size()==0||c.rows()!=e.size()||c.cols()!=e.size()||!e.allFinite()||!c.allFinite()||!c.isApprox(c.transpose(),1e-12)) throw std::invalid_argument("invalid covariance");
    Eigen::LLT<Eigen::MatrixXd> llt(c);if(llt.info()!=Eigen::Success) throw std::invalid_argument("covariance must be positive definite");return e.dot(llt.solve(e));
}
}
