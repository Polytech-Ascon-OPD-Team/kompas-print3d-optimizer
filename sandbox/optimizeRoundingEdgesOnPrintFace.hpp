#ifndef OPTIMIZE_ROUNDING_EDGES_ON_PRINT_FACE_HPP
#define OPTIMIZE_ROUNDING_EDGES_ON_PRINT_FACE_HPP

#include <list>

#include "utils.hpp"

#import "ksconstants.tlb" no_namespace named_guids
#import "ksConstants3D.tlb" no_namespace named_guids
#import "kAPI5.tlb" no_namespace named_guids rename( "min", "Imin" ) rename( "max", "Imax" ) rename( "ksFragmentLibrary", "ksIFragmentLibrary" )

struct RoundingEdgeOnPrintFaceTarget {
    std::list<ksEdgeDefinitionPtr> trajectory;
    ksFaceDefinitionPtr roundingFace;
};

double getCylinderOrTorusRadius(ksFaceDefinitionPtr face);

std::list<RoundingEdgeOnPrintFaceTarget> getRoundingEdgesOnPrintFaceTargets(ksFaceDefinitionPtr printFace);
void drawSketch(Sketch sketch, RoundingEdgeOnPrintFaceTarget target, double overhangThreshold);
void optimizeRoundingEdgesOnPrintFace(KompasObjectPtr kompas, ksPartPtr part, ksFaceDefinitionPtr printFace, double overhangThreshold);

#endif /* OPTIMIZE_ROUNDING_EDGES_ON_PRINT_FACE_HPP */
