#include "../export.hpp"
// Production section factors, including rounded=0.99 (FinSet.cpp uses a different path).
STARLAB_EXPORT double fin_section(double section) {
 if(section==1) return 0.99;
 if(section==2) return 0.65;
 if(section==3) return 0.5;
 if(section==4) return 0.6;
 return 1.0;
}
