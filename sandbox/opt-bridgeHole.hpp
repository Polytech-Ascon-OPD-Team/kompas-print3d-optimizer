#ifndef OPT_BRIDGE_HOLE_HPP
#define OPT_BRIDGE_HOLE_HPP

#include <list>

#import "ksconstants.tlb" no_namespace named_guids
#import "ksConstants3D.tlb" no_namespace named_guids
#import "kAPI5.tlb" no_namespace named_guids rename( "min", "Imin" ) rename( "max", "Imax" ) rename( "ksFragmentLibrary", "ksIFragmentLibrary" )
#import "kAPI7.tlb" no_namespace named_guids rename( "CreateWindow", "ICreateWindow" ) rename( "PostMessage", "IPostMessage" ) rename( "MessageBoxEx", "IMessageBoxEx" )

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
void bridgeHoleFillOptimization(ksPartPtr part, ksFaceDefinitionPtr printFace, double extrusionDepth, HoleType holeType);

bool isOuterLoopForBuild(ksLoopPtr loop);
void drawLoopProjection(ksSketchDefinitionPtr sketchDef, ksLoopPtr loop);
std::list<BridgeHoleBuildTarget> getBridgeHoleBuildTargets(ksPartPtr part, ksFaceDefinitionPtr printFace);
ICirclePtr drawThinCircleProjection(Sketch sketch, ksPartPtr part, BridgeHoleBuildTarget target);
void bridgeHoleBuildCircleDrawSketch1(Sketch sketch, ICirclePtr innerCircle, BridgeHoleBuildTarget target);
void closeContour(ILineSegmentsPtr lineSegments, std::list<double> points, double y);
void bridgeHoleBuildNotCircleDrawSketch1(KompasObjectPtr kompas, Sketch sketch, ICirclePtr innerCircle, BridgeHoleBuildTarget target);
void buildBridgeHole1(ksEntityPtr sketchEntity, ksPartPtr part, double stepDepth);
void buildBridgeHole2(KompasObjectPtr kompas, ksPartPtr part, BridgeHoleBuildTarget target, double stepDepth,
        int polygonAngleCount, double centerX, double centerY, double radius);
void buildBridgeHoles(KompasObjectPtr kompas, ksPartPtr part, std::list<BridgeHoleBuildTarget> bridgeHoleTargets, double stepDepth);
void bridgeHoleBuildOptimization(KompasObjectPtr kompas, ksPartPtr part, ksFaceDefinitionPtr printFace, double stepDepth);

#endif /* OPT_BRIDGE_HOLE_HPP */
