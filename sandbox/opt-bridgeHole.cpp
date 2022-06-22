#include "opt-bridgeHole.hpp"

#include <list>

#include "utils.hpp"

#import "ksconstants.tlb" no_namespace named_guids
#import "ksConstants3D.tlb" no_namespace named_guids
#import "kAPI5.tlb" no_namespace named_guids rename( "min", "Imin" ) rename( "max", "Imax" ) rename( "ksFragmentLibrary", "ksIFragmentLibrary" )

bool loopIsCircle(ksLoopPtr loop) {
    ksEdgeCollectionPtr edges(loop->EdgeCollection());
    int edgesCount = edges->GetCount();
    if (edgesCount != 1) {
        return false;
    }
    ksEdgeDefinitionPtr edge(edges->GetByIndex(0));
    if (!edge->IsCircle()) {
        return false;
    }
    return true;
}

bool checkFaceWithHole(ksFaceDefinitionPtr face, ksFaceDefinitionPtr printFace, ksMeasurerPtr measurer) {
    if (!face->IsPlanar() || (face == printFace)) {
        return false;
    }
    measurer->SetObject1(printFace);
    measurer->SetObject2(face);
    measurer->Calc();
    double angle = measurer->angle;
    if (!(doubleEqual(angle, 0.0) || doubleEqual(angle, 180))) {
        return false;
    }
    return true;
}

bool isHoleDirect(ksLoopPtr loop, ksFaceDefinitionPtr printFace, ksMeasurerPtr measurer) {
    ksEdgeCollectionPtr edges(loop->EdgeCollection());
    int edgesCount = edges->GetCount();
    for (int holeEdgeIndex = 0; holeEdgeIndex < edgesCount; holeEdgeIndex++) {
        ksEdgeDefinitionPtr holeEdge(edges->GetByIndex(holeEdgeIndex));
        ksFaceDefinitionPtr holeFace(holeEdge->GetAdjacentFace(false));

        if (!holeFace->IsCylinder()) {
            if (!holeFace->IsPlanar()) {
                return false;
            } else {
                measurer->SetObject1(printFace);
                measurer->SetObject2(holeFace);
                measurer->Calc();
                double angle = measurer->angle;
                if (!(doubleEqual(angle, 90.0) || doubleEqual(angle, 270.0))) {
                    return false;
                }
            }
        }
    }
    return true;
}

bool checkHoleLoop(ksLoopPtr loop, ksFaceDefinitionPtr printFace, ksMeasurerPtr measurer) {
    if (!isHoleDirect(loop, printFace, measurer)) {
        return false;
    }

    ksEdgeCollectionPtr edges(loop->EdgeCollection());
    int edgesCount = edges->GetCount();

    for (int holeEdgeIndex = 0; holeEdgeIndex < edgesCount; holeEdgeIndex++) {
        ksEdgeDefinitionPtr holeEdge(edges->GetByIndex(holeEdgeIndex));
        ksFaceDefinitionPtr holeFace(holeEdge->GetAdjacentFace(false));

        measurer->SetObject1(printFace);
        measurer->SetObject2(holeEdge);
        measurer->Calc();
        double distanceToHoleEdge = measurer->distance;
        bool isHoleEdgeLower = true;

        ksEdgeCollectionPtr edges2(holeFace->EdgeCollection());
        for (int edge2Index = 0; edge2Index < edges2->GetCount(); edge2Index++) {
            ksEdgeDefinitionPtr edge2(edges2->GetByIndex(edge2Index));
            if (edge2 == holeEdge) {
                continue;
            }

            measurer->SetObject1(printFace);
            measurer->SetObject2(edge2);
            measurer->Calc();
            if (edge2->IsStraight()) {
                double angle = measurer->angle;
                if (doubleEqual(angle, 90.0) || doubleEqual(angle, 270.0)) {
                    continue;
                }
            }
            double distanceToEdge2 = measurer->distance;
            if (distanceToEdge2 < distanceToHoleEdge) {
                isHoleEdgeLower = false;
                break;
            }
        }
        return isHoleEdgeLower;
    }
    return false;
}

std::list<BridgeHoleFillTarget> getBridgeHoleFillTargets(ksPartPtr part, ksFaceDefinitionPtr printFace, HoleType holeType) {
    ksMeasurerPtr measurer(part->GetMeasurer());

    ksBodyPtr body = part->GetMainBody();
    ksFaceCollectionPtr faces = body->FaceCollection();
    int facesCount = faces->GetCount();

    std::list<BridgeHoleFillTarget> bridgeHoleFillTargets;

    for (int faceIndex = 0; faceIndex < facesCount; faceIndex++) {
        ksFaceDefinitionPtr face = faces->GetByIndex(faceIndex);
        if (!checkFaceWithHole(face, printFace, measurer)) {
            continue;
        }

        ksLoopCollectionPtr loops(face->LoopCollection());
        for (int loopIndex = 0; loopIndex < loops->GetCount(); loopIndex++) {
            ksLoopPtr innerLoop(loops->GetByIndex(loopIndex));
            if (innerLoop->IsOuter()) {
                continue;
            }

            if (holeType == HoleType::CIRCLE) {
                if (!loopIsCircle(innerLoop)) {
                    continue;
                }
            } else if (holeType == HoleType::NOT_CIRCLE) {
                if (loopIsCircle(innerLoop)) {
                    continue;
                }
            }

            if (checkHoleLoop(innerLoop, printFace, measurer)) {
                bridgeHoleFillTargets.push_back(BridgeHoleFillTarget{ innerLoop, face });
            }
        }
    }
    return bridgeHoleFillTargets;
}

void fillBridgeHoles(ksPartPtr part, std::list<BridgeHoleFillTarget> bridgeHoleFillTargets, double extrusionDepth) {
    for (BridgeHoleFillTarget target : bridgeHoleFillTargets) {
        ksEntityPtr sketchEntity(part->NewEntity(o3d_sketch));
        ksSketchDefinitionPtr sketchDef(sketchEntity->GetDefinition());
        sketchDef->SetPlane(target.face);
        sketchEntity->Create();

        sketchDef->BeginEdit();
        ksEdgeCollectionPtr edges(target.loop->EdgeCollection());
        int edgesCount = edges->GetCount();
        for (int edgeIndex = 0; edgeIndex < edgesCount; edgeIndex++) {
            sketchDef->AddProjectionOf(edges->GetByIndex(edgeIndex));
        }
        sketchDef->EndEdit();

        ksEntityPtr extrusionEntity(part->NewEntity(o3d_bossExtrusion));
        ksBossExtrusionDefinitionPtr extrusionDef(extrusionEntity->GetDefinition());
        extrusionDef->chooseType = ksChBodiesAndParts;
        extrusionDef->directionType = dtReverse;
        extrusionDef->SetSideParam(false, etBlind, extrusionDepth, 0, false);
        extrusionDef->SetSketch(sketchEntity);
        extrusionEntity->Create();
    }
}

std::list<BridgeHoleBuildTarget> getBridgeHoleBuildTargets(ksPartPtr part, ksFaceDefinitionPtr printFace) {
    ksMeasurerPtr measurer(part->GetMeasurer());

    ksBodyPtr body = part->GetMainBody();
    ksFaceCollectionPtr faces = body->FaceCollection();
    int facesCount = faces->GetCount();

    std::list<BridgeHoleBuildTarget> bridgeHoleBuildTargets;

    for (int faceIndex = 0; faceIndex < facesCount; faceIndex++) {
        ksFaceDefinitionPtr face = faces->GetByIndex(faceIndex);
        if (!checkFaceWithHole(face, printFace, measurer)) {
            continue;
        }

        ksLoopCollectionPtr loops(face->LoopCollection());
        int loopsCount = loops->GetCount();

        if (loopsCount != 2) {
            continue;
        }

        ksLoopPtr testLoop(loops->GetByIndex(0));
        ksLoopPtr innerLoop, outerLoop;
        if (testLoop->IsOuter()) {
            outerLoop = testLoop;
            innerLoop = loops->GetByIndex(1);
        } else {
            innerLoop = testLoop;
            outerLoop = loops->GetByIndex(1);
        }

        if (!loopIsCircle(innerLoop)) {
            continue;
        }

        if (checkHoleLoop(innerLoop, printFace, measurer)) {
            bridgeHoleBuildTargets.push_back(BridgeHoleBuildTarget{ innerLoop, outerLoop, face });
        }
    }
    return bridgeHoleBuildTargets;
}

void buildBridgeHoles(ksPartPtr part, std::list<BridgeHoleBuildTarget> bridgeHoleBuildTargets, double stepDepth) {
    for (BridgeHoleBuildTarget target : bridgeHoleBuildTargets) {
        ksEntityPtr sketchEntity(part->NewEntity(o3d_sketch));
        ksSketchDefinitionPtr sketchDef(sketchEntity->GetDefinition());
        sketchDef->SetPlane(target.face);
        sketchEntity->Create();

        sketchDef->BeginEdit();

        if (!loopIsCircle(target.innerLoop)) {
            continue;
        }
        ksEdgeCollectionPtr innerEdges(target.innerLoop->EdgeCollection());
        ksEdgeDefinitionPtr innerEdge(innerEdges->GetByIndex(0));
        sketchDef->AddProjectionOf(innerEdge);

        ksEdgeCollectionPtr outerEdges(target.outerLoop->EdgeCollection());
        for (int outerEdgeIndex = 0; outerEdgeIndex < outerEdges->GetCount(); outerEdgeIndex++) {
            ksEdgeDefinitionPtr outerEdge(outerEdges->GetByIndex(outerEdgeIndex));
            sketchDef->AddProjectionOf(outerEdge);
        }
        
        sketchDef->EndEdit();
    }
}

void bridgeHoleFillOptimization(ksPartPtr part, ksFaceDefinitionPtr printFace, double extrusionDepth, HoleType holeType) {
    std::list<BridgeHoleFillTarget> targets = getBridgeHoleFillTargets(part, printFace, holeType);
    fillBridgeHoles(part, targets, extrusionDepth);
}

void bridgeHoleBuildOptimization(ksPartPtr part, ksFaceDefinitionPtr printFace, double stepDepth) {
    std::list<BridgeHoleBuildTarget> targets = getBridgeHoleBuildTargets(part, printFace);
    buildBridgeHoles(part, targets, stepDepth);
}
