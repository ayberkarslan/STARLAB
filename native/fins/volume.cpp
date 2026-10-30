#include "../export.hpp"
// RoketSiM analyzeComponent(Fins): root tab participates in physical volume.
STARLAB_EXPORT double fin_volume(double count,double area,double tabHeight,double tabLength,double thickness,double sectionFactor) {
 const double tabArea=tabHeight*tabLength;
 return count*(area+tabArea)*thickness*sectionFactor;
}
