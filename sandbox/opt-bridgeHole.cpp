#include "opt-bridgeHole.hpp"

#include <list>

#include "utils.hpp"

#import "ksconstants.tlb" no_namespace named_guids
#import "ksConstants3D.tlb" no_namespace named_guids
#import "kAPI5.tlb" no_namespace named_guids rename( "min", "Imin" ) rename( "max", "Imax" ) rename( "ksFragmentLibrary", "ksIFragmentLibrary" )

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

std::list<BridgeHoleTarget> getBridgeHoleTargets(ksPartPtr part, ksFaceDefinitionPtr printFace, HoleType holeType) {
    ksMeasurerPtr measurer(part->GetMeasurer());

    ksBodyPtr body = part->GetMainBody();
    ksFaceCollectionPtr faces = body->FaceCollection();
    int facesCount = faces->GetCount();

    std::list<BridgeHoleTarget> bridgeHoleTargets;

    for (int faceIndex = 0; faceIndex < facesCount; faceIndex++) {
        ksFaceDefinitionPtr face = faces->GetByIndex(faceIndex);
        if (!face->IsPlanar() || (face == printFace)) {
            continue;
        }
        measurer->SetObject1(printFace);
        measurer->SetObject2(face);
        measurer->Calc();
        double angle = measurer->angle;
        if (!(doubleEqual(angle, 0.0) || doubleEqual(angle, 180))) {
            continue;
        }

        ksLoopCollectionPtr loops(face->LoopCollection());
        for (int loopIndex = 0; loopIndex < loops->GetCount(); loopIndex++) {
            ksLoopPtr innerLoop(loops->GetByIndex(loopIndex));
            if (innerLoop->IsOuter()) {
                continue;
            }

            if (holeType != HoleType::ALL) {
                ksEdgeCollectionPtr edges(innerLoop->EdgeCollection());
                int edgesCount = edges->GetCount();
                if (holeType == HoleType::CIRCLE) {
                    if (edgesCount != 1) {
                        continue;
                    }
                    ksEdgeDefinitionPtr edge(edges->GetByIndex(0));
                    if (!edge->IsCircle()) {
                        continue;
                    }
                } else if (holeType == HoleType::NOT_CIRCLE) {
                    if (edgesCount == 1) {
                        continue;
                    }
                }
            }

            if (checkHoleLoop(innerLoop, printFace, measurer)) {
                bridgeHoleTargets.push_back(BridgeHoleTarget{ innerLoop, face });
            }
        }
    }
    return bridgeHoleTargets;
}

void fillBridgeHoles(ksPartPtr part, std::list<BridgeHoleTarget> bridgeHoleTargets, double extrusionDepth) {
    for (BridgeHoleTarget target : bridgeHoleTargets) {
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

void buildBridgeHoles(ksPartPtr part, std::list<BridgeHoleTarget> bridgeHoleTargets, double stepDepth) {
    
}

void bridgeHoleFillOptimization(ksPartPtr part, ksFaceDefinitionPtr printFace, double extrusionDepth, HoleType holeType) {
    std::list<BridgeHoleTarget> targets = getBridgeHoleTargets(part, printFace, holeType);
    fillBridgeHoles(part, targets, extrusionDepth);
}

void bridgeHoleBuildOptimization(ksPartPtr part, ksFaceDefinitionPtr printFace, double stepDepth) {
    std::list<BridgeHoleTarget> targets = getBridgeHoleTargets(part, printFace, HoleType::CIRCLE);
    buildBridgeHoles(part, targets, stepDepth);
}
