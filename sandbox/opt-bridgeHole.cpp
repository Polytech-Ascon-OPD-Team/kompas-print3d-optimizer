#include "opt-bridgeHole.hpp"

#include <list>
#include <iostream>

#include "utils.hpp"

#import "ksconstants.tlb" no_namespace named_guids
#import "ksConstants3D.tlb" no_namespace named_guids
#import "kAPI5.tlb" no_namespace named_guids rename( "min", "Imin" ) rename( "max", "Imax" ) rename( "ksFragmentLibrary", "ksIFragmentLibrary" )
#import "kAPI7.tlb" no_namespace named_guids rename( "CreateWindow", "ICreateWindow" ) rename( "PostMessage", "IPostMessage" ) rename( "MessageBoxEx", "IMessageBoxEx" )

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

void buildBridgeHole1Circle(ksSketchDefinitionPtr sketchDef, IDrawingContainerPtr drawingContainer, BridgeHoleBuildTarget target, double stepDepth) {
    // Проецируем окружности
    ksEdgeCollectionPtr innerEdges(target.innerLoop->EdgeCollection());
    ksEdgeDefinitionPtr innerEdge(innerEdges->GetByIndex(0));
    sketchDef->AddProjectionOf(innerEdge);

    ICirclesPtr circles(drawingContainer->Circles);
    ICirclePtr innerCircle(circles->GetCircle(0));
    innerCircle->Style = ksCurveStyleEnum::ksCSThin;
    innerCircle->Update();

    ksEdgeCollectionPtr outerEdges(target.outerLoop->EdgeCollection());
    ksEdgeDefinitionPtr outerEdge(outerEdges->GetByIndex(0));
    sketchDef->AddProjectionOf(outerEdge);

    ICirclePtr outerCircle;
    circles = drawingContainer->Circles;
    for (int circleIndex = 0; circleIndex < circles->Count; circleIndex++) {
        ICirclePtr circle(circles->GetCircle(circleIndex));
        if (circle != innerCircle) {
            outerCircle = circle;
        }
    }
    outerCircle->Style = ksCurveStyleEnum::ksCSThin;
    outerCircle->Update();

    // Устанавливаем ограничения
    ILineSegmentsPtr lineSegments(drawingContainer->LineSegments);
    ILineSegmentPtr lineSegment1(lineSegments->Add());
    lineSegment1->X1 = innerCircle->Xc + 1; lineSegment1->Y1 = innerCircle->Yc - 1;
    lineSegment1->X2 = innerCircle->Xc - 1; lineSegment1->Y2 = innerCircle->Yc - 1;
    lineSegment1->Update();
    IDrawingObjectPtr lineSegment1DrawingObject(lineSegment1);
    IDrawingObject1Ptr lineSegment1DrawingObject1(lineSegment1DrawingObject);
    {
        IParametriticConstraintPtr constraint(lineSegment1DrawingObject1->NewConstraint());
        constraint->ConstraintType = ksCPointOnCurve;
        constraint->Index = 0;
        constraint->Partner = static_cast<IDispatch*>(outerCircle);
        constraint->Create();
    }
    {
        IParametriticConstraintPtr constraint(lineSegment1DrawingObject1->NewConstraint());
        constraint->ConstraintType = ksCPointOnCurve;
        constraint->Index = 1;
        constraint->Partner = static_cast<IDispatch*>(outerCircle);
        constraint->Create();
    }
    {
        IParametriticConstraintPtr constraint(lineSegment1DrawingObject1->NewConstraint());
        constraint->ConstraintType = ksCHorizontal;
        constraint->Create();
    }
    {
        IParametriticConstraintPtr constraint(lineSegment1DrawingObject1->NewConstraint());
        constraint->ConstraintType = ksCTangentTwoCurves;
        constraint->Partner = static_cast<IDispatch*>(innerCircle);
        constraint->Create();
    }

    ILineSegmentPtr lineSegment2(lineSegments->Add());
    lineSegment2->X1 = innerCircle->Xc + 1; lineSegment2->Y1 = innerCircle->Yc + 1;
    lineSegment2->X2 = innerCircle->Xc - 1; lineSegment2->Y2 = innerCircle->Yc + 1;
    lineSegment2->Update();
    IDrawingObjectPtr lineSegment2DrawingObject(lineSegment2);
    IDrawingObject1Ptr lineSegment2DrawingObject1(lineSegment2DrawingObject);
    {
        IParametriticConstraintPtr constraint(lineSegment2DrawingObject1->NewConstraint());
        constraint->ConstraintType = ksCPointOnCurve;
        constraint->Index = 0;
        constraint->Partner = static_cast<IDispatch*>(outerCircle);
        constraint->Create();
    }
    {
        IParametriticConstraintPtr constraint(lineSegment2DrawingObject1->NewConstraint());
        constraint->ConstraintType = ksCPointOnCurve;
        constraint->Index = 1;
        constraint->Partner = static_cast<IDispatch*>(outerCircle);
        constraint->Create();
    }
    {
        IParametriticConstraintPtr constraint(lineSegment2DrawingObject1->NewConstraint());
        constraint->ConstraintType = ksCParallel;
        constraint->Partner = static_cast<IDispatch*>(lineSegment1);
        constraint->Create();
    }
    {
        IParametriticConstraintPtr constraint(lineSegment2DrawingObject1->NewConstraint());
        constraint->ConstraintType = ksCTangentTwoCurves;
        constraint->Partner = static_cast<IDispatch*>(innerCircle);
        constraint->Create();
    }

    // Рисуем дуги
    {
        IArcsPtr arcs(drawingContainer->Arcs);
        IArcPtr arc(arcs->Add());
        arc->Xc = outerCircle->Xc; arc->Yc = outerCircle->Yc;
        arc->Radius = outerCircle->Radius;
        arc->X1 = lineSegment1->X1; arc->Y1 = lineSegment1->Y1;
        arc->X2 = lineSegment2->X1; arc->Y2 = lineSegment2->Y1;
        arc->Direction = false;
        arc->Update();

        IDrawingObjectPtr arcDrawingObject(arc);
        IDrawingObject1Ptr arcDrawingObject1(arcDrawingObject);
        {
            IParametriticConstraintPtr constraint(arcDrawingObject1->NewConstraint());
            constraint->ConstraintType = ksCMergePoints;
            constraint->Index = 0;
            constraint->Partner = static_cast<IDispatch*>(outerCircle);
            constraint->PartnerIndex = 0;
            constraint->Create();
        }
        {
            IParametriticConstraintPtr constraint(arcDrawingObject1->NewConstraint());
            constraint->ConstraintType = ksCMergePoints;
            constraint->Index = 1;
            constraint->Partner = static_cast<IDispatch*>(lineSegment1);
            constraint->PartnerIndex = 0;
            constraint->Create();
        }
        {
            IParametriticConstraintPtr constraint(arcDrawingObject1->NewConstraint());
            constraint->ConstraintType = ksCMergePoints;
            constraint->Index = 2;
            constraint->Partner = static_cast<IDispatch*>(lineSegment2);
            constraint->PartnerIndex = 0;
            constraint->Create();
        }
    }
    {
        IArcsPtr arcs(drawingContainer->Arcs);
        IArcPtr arc(arcs->Add());
        arc->Xc = outerCircle->Xc; arc->Yc = outerCircle->Yc;
        arc->Radius = outerCircle->Radius;
        arc->X1 = lineSegment1->X2; arc->Y1 = lineSegment1->Y2;
        arc->X2 = lineSegment2->X2; arc->Y2 = lineSegment2->Y2;
        arc->Direction = true;
        arc->Update();

        IDrawingObjectPtr arcDrawingObject(arc);
        IDrawingObject1Ptr arcDrawingObject1(arcDrawingObject);
        {
            IParametriticConstraintPtr constraint(arcDrawingObject1->NewConstraint());
            constraint->ConstraintType = ksCMergePoints;
            constraint->Index = 0;
            constraint->Partner = static_cast<IDispatch*>(outerCircle);
            constraint->PartnerIndex = 0;
            constraint->Create();
        }
        {
            IParametriticConstraintPtr constraint(arcDrawingObject1->NewConstraint());
            constraint->ConstraintType = ksCMergePoints;
            constraint->Index = 1;
            constraint->Partner = static_cast<IDispatch*>(lineSegment1);
            constraint->PartnerIndex = 1;
            constraint->Create();
        }
        {
            IParametriticConstraintPtr constraint(arcDrawingObject1->NewConstraint());
            constraint->ConstraintType = ksCMergePoints;
            constraint->Index = 2;
            constraint->Partner = static_cast<IDispatch*>(lineSegment2);
            constraint->PartnerIndex = 1;
            constraint->Create();
        }
    }

}

void buildBridgeHole1(KompasObjectPtr kompas, ksPartPtr part, BridgeHoleBuildTarget target, double stepDepth) {
    ksEntityPtr sketchEntity(part->NewEntity(o3d_sketch));
    ksSketchDefinitionPtr sketchDef(sketchEntity->GetDefinition());
    sketchDef->SetPlane(target.face);
    sketchEntity->Create();

    ksDocument2DPtr sketchEdit(sketchDef->BeginEdit());

    IKompasDocument2DPtr sketchEdit_api7(kompas->TransferInterface(sketchEdit, ksAPI7Dual, 0));
    IViewsAndLayersManagerPtr viewsAndLayersManager(sketchEdit_api7->ViewsAndLayersManager);
    IViewsPtr views(viewsAndLayersManager->Views);
    IViewPtr view(views->ActiveView);
    IDrawingContainerPtr drawingContainer(view);

    if (!loopIsCircle(target.innerLoop)) {
        return;
    }

    if (loopIsCircle(target.outerLoop)) {
        buildBridgeHole1Circle(sketchDef, drawingContainer, target, stepDepth);

    }

    sketchDef->EndEdit();
}

void buildBridgeHoles(KompasObjectPtr kompas, ksPartPtr part, std::list<BridgeHoleBuildTarget> bridgeHoleBuildTargets, double stepDepth) {
    for (BridgeHoleBuildTarget target : bridgeHoleBuildTargets) {
        buildBridgeHole1(kompas, part, target, stepDepth);


    }
}

void bridgeHoleFillOptimization(ksPartPtr part, ksFaceDefinitionPtr printFace, double extrusionDepth, HoleType holeType) {
    std::list<BridgeHoleFillTarget> targets = getBridgeHoleFillTargets(part, printFace, holeType);
    fillBridgeHoles(part, targets, extrusionDepth);
}

void bridgeHoleBuildOptimization(KompasObjectPtr kompas, ksPartPtr part, ksFaceDefinitionPtr printFace, double stepDepth) {
    std::list<BridgeHoleBuildTarget> targets = getBridgeHoleBuildTargets(part, printFace);
    buildBridgeHoles(kompas, part, targets, stepDepth);
}
