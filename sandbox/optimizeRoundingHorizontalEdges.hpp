#ifndef OPTIMIZE_ROUNDING_HORIZONTAL_EDGES_HPP
#define OPTIMIZE_ROUNDING_HORIZONTAL_EDGES_HPP

#include <list>

#import "ksconstants.tlb" no_namespace named_guids
#import "ksConstants3D.tlb" no_namespace named_guids
#import "kAPI5.tlb" no_namespace named_guids rename( "min", "Imin" ) rename( "max", "Imax" ) rename( "ksFragmentLibrary", "ksIFragmentLibrary" )

struct RoundingHorizontalEdgeTarget {
    std::list<ksEdgeDefinitionPtr> trajectory;
    ksFaceDefinitionPtr roundingFace;
};

double getCylinderOrTorusRadius(ksFaceDefinitionPtr face);

std::list<RoundingHorizontalEdgeTarget> getRoundingHorizontalEdgesTargets(ksFaceDefinitionPtr printFace);
void optimizeRoundingHorizontalEdges(ksPartPtr part, ksFaceDefinitionPtr printFace);

#endif /* OPTIMIZE_ROUNDING_HORIZONTAL_EDGES_HPP */
