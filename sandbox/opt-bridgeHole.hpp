#ifndef OPT_BRIDGE_HOLE_HPP
#define OPT_BRIDGE_HOLE_HPP

#include <list>

#import "kAPI5.tlb" no_namespace named_guids rename( "min", "Imin" ) rename( "max", "Imax" ) rename( "ksFragmentLibrary", "ksIFragmentLibrary" )

struct BridgeHoleFillTarget {
    ksLoopPtr loop;
    ksFaceDefinitionPtr face;
};

struct BridgeHoleBuildTarget {
    ksLoopPtr innerLoop;
    ksLoopPtr outerLoop;
    ksFaceDefinitionPtr face;
};

enum class HoleType {
    CIRCLE,
    NOT_CIRCLE,
    ALL,
};

bool loopIsCircle(ksLoopPtr loop);
bool checkFaceWithHole(ksFaceDefinitionPtr face, ksFaceDefinitionPtr printFace, ksMeasurerPtr measurer);
bool isHoleDirect(ksLoopPtr loop, ksFaceDefinitionPtr printFace, ksMeasurerPtr measurer);

bool checkHoleLoop(ksLoopPtr loop, ksFaceDefinitionPtr printFace, ksMeasurerPtr measurer);
std::list<BridgeHoleFillTarget> getBridgeHoleFillTargets(ksPartPtr part, ksFaceDefinitionPtr printFace, HoleType holeType);
void fillBridgeHoles(ksPartPtr part, std::list<BridgeHoleFillTarget> bridgeHoleTargets, double extrusionDepth);

std::list<BridgeHoleBuildTarget> getBridgeHoleBuildTargets(ksPartPtr part, ksFaceDefinitionPtr printFace);
void buildBridgeHoles(KompasObjectPtr kompas, ksPartPtr part, std::list<BridgeHoleBuildTarget> bridgeHoleTargets, double stepDepth);

void bridgeHoleFillOptimization(ksPartPtr part, ksFaceDefinitionPtr printFace, double extrusionDepth, HoleType holeType);
void bridgeHoleBuildOptimization(KompasObjectPtr kompas, ksPartPtr part, ksFaceDefinitionPtr printFace, double stepDepth);

#endif /* OPT_BRIDGE_HOLE_HPP */
