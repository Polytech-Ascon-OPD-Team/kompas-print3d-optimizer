#ifndef SELECT_PLANE_H
#define SELECT_PLANE_H
#define PLANE_BORDER_EPS 0.001

#import <kAPI5.tlb> no_namespace named_guids rename( "min", "Imin" ) rename( "max", "Imax" ) rename( "ksFragmentLibrary", "ksIFragmentLibrary" )

class PlaneEq {
public:
    double a, b, c, d;
    bool isVertical(ksEdgeDefinitionPtr edge, double cos_angle);
    bool equals(PlaneEq other);
    PlaneEq(ksFaceDefinitionPtr face);
    PlaneEq();
};

void checkPlane(KompasObjectPtr kompas, double a, double b, double c, double d, int* s1, int* s2);

ksFaceDefinitionPtr getSelectedPlane(KompasObjectPtr kompas, PlaneEq* planeEq);

#endif