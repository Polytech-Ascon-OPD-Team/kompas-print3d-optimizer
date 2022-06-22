#ifndef OPT_BRIDGE_HOLE_HPP
#define OPT_BRIDGE_HOLE_HPP

#include <list>

#import "kAPI5.tlb" no_namespace named_guids rename( "min", "Imin" ) rename( "max", "Imax" ) rename( "ksFragmentLibrary", "ksIFragmentLibrary" )

struct BridgeHoleTarget {
    ksLoopPtr loop;
    ksFaceDefinitionPtr face;
};

enum class HoleType {
    CIRCLE,
    NOT_CIRCLE,
    ALL,
};

bool checkHoleLoop(ksLoopPtr loop, ksFaceDefinitionPtr printFace, ksMeasurerPtr measurer);
std::list<BridgeHoleTarget> getBridgeHoleTargets(ksPartPtr part, ksFaceDefinitionPtr printFace, HoleType holeType);
void fillBridgeHoles(ksPartPtr part, std::list<BridgeHoleTarget> bridgeHoleTargets, double extrusionDepth);

void buildBridgeHoles(ksPartPtr part, std::list<BridgeHoleTarget> bridgeHoleTargets, double stepDepth);

void bridgeHoleFillOptimization(ksPartPtr part, ksFaceDefinitionPtr printFace, double extrusionDepth, HoleType holeType);
void bridgeHoleBuildOptimization(ksPartPtr part, ksFaceDefinitionPtr printFace, double stepDepth);

#endif /* OPT_BRIDGE_HOLE_HPP */
