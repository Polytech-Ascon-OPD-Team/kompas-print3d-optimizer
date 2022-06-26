#include "opt-bridgeHole.hpp"

#include <list>

#include "utils.hpp"
#include "kompasUtils.hpp"

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

/* Закрытие нависающих отверстий тонким слоем материала */

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

void bridgeHoleFillOptimization(ksPartPtr part, ksFaceDefinitionPtr printFace, double extrusionDepth, HoleType holeType) {
    std::list<BridgeHoleFillTarget> targets = getBridgeHoleFillTargets(part, printFace, holeType);
    fillBridgeHoles(part, targets, extrusionDepth);
}

/* Достройка нависающих отверстий для печати мостами */

bool isOuterLoopForBuild(ksLoopPtr loop) {
    if (loopIsCircle(loop)) {
        return true;
    }
    ksEdgeCollectionPtr edges(loop->EdgeCollection());
    for (int edgeIndex = 0; edgeIndex < edges->GetCount(); edgeIndex++) {
        ksEdgeDefinitionPtr edge(edges->GetByIndex(edgeIndex));
        if (!edge->IsStraight()) {
            return false;
        }
    }
    return true;
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

        if (!loopIsCircle(innerLoop) || !isOuterLoopForBuild(outerLoop)) {
            continue;
        }

        if (checkHoleLoop(innerLoop, printFace, measurer)) {
            bridgeHoleBuildTargets.push_back(BridgeHoleBuildTarget{ innerLoop, outerLoop, face });
        }
    }
    return bridgeHoleBuildTargets;
}

void drawLoopProjection(ksSketchDefinitionPtr sketchDef, ksLoopPtr loop) {
    ksEdgeCollectionPtr edges(loop->EdgeCollection());
    for (int i = 0; i < edges->GetCount(); i++) {
        ksEdgeDefinitionPtr edge(edges->GetByIndex(i));
        sketchDef->AddProjectionOf(edge);
    }
}

ICirclePtr drawThinCircleProjection(Sketch sketch, ksPartPtr part, BridgeHoleBuildTarget target) {

    IViewsAndLayersManagerPtr viewsAndLayersManager(sketch.document2d_api7->ViewsAndLayersManager);
    IViewsPtr views(viewsAndLayersManager->Views);
    IViewPtr view(views->ActiveView);
    IDrawingContainerPtr drawingContainer(view);

    ksEdgeCollectionPtr innerEdges(target.innerLoop->EdgeCollection());
    ksEdgeDefinitionPtr innerEdge(innerEdges->GetByIndex(0));
    sketch.definition->AddProjectionOf(innerEdge);

    ICirclesPtr circles(drawingContainer->Circles);
    ICirclePtr innerCircle(circles->GetCircle(0));
    innerCircle->Style = ksCurveStyleEnum::ksCSThin;
    innerCircle->Update();

    return innerCircle;
}

void bridgeHoleBuildCircleDrawSketch1(Sketch sketch, ICirclePtr innerCircle, BridgeHoleBuildTarget target) {
    drawLoopProjection(sketch.definition, target.outerLoop);

    IViewsAndLayersManagerPtr viewsAndLayersManager(sketch.document2d_api7->ViewsAndLayersManager);
    IViewsPtr views(viewsAndLayersManager->Views);
    IViewPtr view(views->ActiveView);
    IDrawingContainerPtr drawingContainer(view);

    ICirclesPtr circles(drawingContainer->Circles);
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
    lineSegment1->X1 = outerCircle->Xc + 1; lineSegment1->Y1 = outerCircle->Yc - 1;
    lineSegment1->X2 = outerCircle->Xc - 1; lineSegment1->Y2 = outerCircle->Yc - 1;
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
    lineSegment2->X1 = outerCircle->Xc + 1; lineSegment2->Y1 = outerCircle->Yc + 1;
    lineSegment2->X2 = outerCircle->Xc - 1; lineSegment2->Y2 = outerCircle->Yc + 1;
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
    IArcsPtr arcs(drawingContainer->Arcs);
    {
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

void bridgeHoleBuildNotCircleDrawSketch1(KompasObjectPtr kompas, Sketch sketch, ICirclePtr innerCircle, BridgeHoleBuildTarget target) {

    // TODO Алгоритм нужно оптимизировать

    drawLoopProjection(sketch.definition, target.outerLoop);

    IViewsAndLayersManagerPtr viewsAndLayersManager(sketch.document2d_api7->ViewsAndLayersManager);
    IViewsPtr views(viewsAndLayersManager->Views);
    IViewPtr view(views->ActiveView);
    IDrawingContainerPtr drawingContainer(view);

    double yMin = innerCircle->Yc - innerCircle->Radius, yMax = innerCircle->Yc + innerCircle->Radius;

    // Отрезки, в которых одна точка вне промежутка, другая в промежутке
    std::list<ILineSegmentPtr> lineSegmentList;

    // Точки для достраивания отрезков
    std::list<double> pointsMin; // x'ы точек на линии yMin
    std::list<double> pointsMax; // x'ы точек на линии yMax

    ILineSegmentsPtr lineSegments(drawingContainer->LineSegments);
    for (int lineSegmentIndex = 0; lineSegmentIndex < lineSegments->Count; lineSegmentIndex++) {
        ILineSegmentPtr lineSegment(lineSegments->GetLineSegment(lineSegmentIndex));
        
        // Отрезок полностью вне промежутка
        if (((lineSegment->Y1 <= yMin) && (lineSegment->Y2 <= yMin)) ||
                ((lineSegment->Y1 >= yMax) && (lineSegment->Y2 >= yMax))) {
            lineSegment->Style = ksCurveStyleEnum::ksCSThin;
            lineSegment->Update();
            continue;
        }

        // Одна точка отрезка вне промежутка, другая в промежутке
        if ((((lineSegment->Y1 > yMin) && (lineSegment->Y1 < yMax)) && ((lineSegment->Y2 < yMin) || (lineSegment->Y2 > yMax))) ||
                (((lineSegment->Y2 > yMin) && (lineSegment->Y2 < yMax)) && ((lineSegment->Y1 < yMin) || (lineSegment->Y1 > yMax)))) {
            lineSegment->Style = ksCurveStyleEnum::ksCSThin;
            lineSegment->Update();
            lineSegmentList.push_back(lineSegment);
        }

        // Отрезок пересекает промежуток
        if (((lineSegment->Y1 < yMin) && (lineSegment->Y2 > yMax)) ||
                ((lineSegment->Y2 < yMin) && (lineSegment->Y1 > yMax))) {
            lineSegment->Style = ksCurveStyleEnum::ksCSThin;
            lineSegment->Update();
            lineSegmentList.push_back(lineSegment);
        }

        // Одна из точек отрезка лежит на линии yMin или yMax
        if (doubleEqual(lineSegment->Y1, yMin)) { pointsMin.push_back(lineSegment->X1); }
        if (doubleEqual(lineSegment->Y2, yMin)) { pointsMin.push_back(lineSegment->X2); }
        if (doubleEqual(lineSegment->Y1, yMax)) { pointsMax.push_back(lineSegment->X1); }
        if (doubleEqual(lineSegment->Y2, yMax)) { pointsMax.push_back(lineSegment->X2); }

        // TODO Если обе лежат на одной линии, то не добавляем
    }

    // Строим вспомогательные линии
    ILinesPtr lines(drawingContainer->Lines);
    ILinePtr line1(lines->Add());
    line1->X1 = innerCircle->Xc + 1; line1->Y1 = yMin;
    line1->X2 = innerCircle->Xc - 1; line1->Y2 = yMin;
    line1->Update();
    IDrawingObjectPtr line1DrawingObject(line1);
    IDrawingObject1Ptr line1DrawingObject1(line1DrawingObject);
    {
        IParametriticConstraintPtr constraint(line1DrawingObject1->NewConstraint());
        constraint->ConstraintType = ksCHorizontal;
        constraint->Create();
    }
    {
        IParametriticConstraintPtr constraint(line1DrawingObject1->NewConstraint());
        constraint->ConstraintType = ksCTangentTwoCurves;
        constraint->Partner = static_cast<IDispatch*>(innerCircle);
        constraint->Create();
    }

    ILinePtr line2(lines->Add());
    line2->X1 = innerCircle->Xc + 1; line2->Y1 = yMax;
    line2->X2 = innerCircle->Xc - 1; line2->Y2 = yMax;
    line2->Update();
    IDrawingObjectPtr line2DrawingObject(line2);
    IDrawingObject1Ptr line2DrawingObject1(line2DrawingObject);
    {
        IParametriticConstraintPtr constraint(line2DrawingObject1->NewConstraint());
        constraint->ConstraintType = ksCHorizontal;
        constraint->Create();
    }
    {
        IParametriticConstraintPtr constraint(line2DrawingObject1->NewConstraint());
        constraint->ConstraintType = ksCTangentTwoCurves;
        constraint->Partner = static_cast<IDispatch*>(innerCircle);
        constraint->Create();
    }

    // Строим части прямых, лежащие в нужной области
    ksMathematic2DPtr math2d = kompas->GetMathematic2D();
    for (ILineSegmentPtr lineSegment : lineSegmentList) {
        ksDynamicArrayPtr dynArr1(kompas->GetDynamicArray(2)); // Точка
        ksDynamicArrayPtr dynArr2(kompas->GetDynamicArray(2));
        int res1 = math2d->ksIntersectCurvCurv(lineSegment->GetReference(), line1->GetReference(), dynArr1);
        int res2 = math2d->ksIntersectCurvCurv(lineSegment->GetReference(), line2->GetReference(), dynArr2);
        
        if ((res1 == 1) && (res2 == 1)) {
            ksMathPointParamPtr point1 = kompas->GetParamStruct(ko_MathPointParam);
            dynArr1->ksGetArrayItem(0, point1);
            ksMathPointParamPtr point2 = kompas->GetParamStruct(ko_MathPointParam);
            dynArr2->ksGetArrayItem(0, point2);

            ILineSegmentPtr newLineSegment(lineSegments->Add());
            newLineSegment->X1 = point1->x; newLineSegment->Y1 = point1->y;
            newLineSegment->X2 = point2->x; newLineSegment->Y2 = point2->y;
            newLineSegment->Update();

            pointsMin.push_back(point1->x);
            pointsMax.push_back(point2->x);

            continue;
        }

        ksMathPointParamPtr point = kompas->GetParamStruct(ko_MathPointParam);
        if (res1 == 1) {
            dynArr1->ksGetArrayItem(0, point);
            pointsMin.push_back(point->x);
        } else {
            dynArr2->ksGetArrayItem(0, point);
            pointsMax.push_back(point->x);
        }

        ILineSegmentPtr newLineSegment(lineSegments->Add());

        if ((lineSegment->Y1 > yMin) && (lineSegment->Y1 < yMax)) {
            newLineSegment->X1 = lineSegment->X1; newLineSegment->Y1 = lineSegment->Y1;
        } else {
            newLineSegment->X1 = lineSegment->X2; newLineSegment->Y1 = lineSegment->Y2;
        }
        newLineSegment->X2 = point->x; newLineSegment->Y2 = point->y;
        newLineSegment->Update();

    }

    // Достраиваем отрезки
    if ((pointsMin.size() % 2 == 0) && (pointsMax.size() % 2 == 0)) { // Размеры всегда должны быть четными
        pointsMin.sort();
        pointsMax.sort();

        for (std::list<double>::const_iterator it = pointsMin.cbegin(); it != pointsMin.cend(); it++) {
            ILineSegmentPtr newLineSegment(lineSegments->Add());
            newLineSegment->X1 = *it; newLineSegment->Y1 = yMin;
            it++;
            newLineSegment->X2 = *it; newLineSegment->Y2 = yMin;
            newLineSegment->Update();
        }

        for (std::list<double>::const_iterator it = pointsMax.cbegin(); it != pointsMax.cend(); it++) {
            ILineSegmentPtr newLineSegment(lineSegments->Add());
            newLineSegment->X1 = *it; newLineSegment->Y1 = yMax;
            it++;
            newLineSegment->X2 = *it; newLineSegment->Y2 = yMax;
            newLineSegment->Update();
        }
    }

}

void buildBridgeHole1(ksEntityPtr sketchEntity, ksPartPtr part, double stepDepth) {
    // Выдавливаем эскиз
    ksEntityPtr extrusionEntity(part->NewEntity(o3d_cutExtrusion));
    ksCutExtrusionDefinitionPtr extrusionDef(extrusionEntity->GetDefinition());
    extrusionDef->cut = true;
    extrusionDef->chooseType = ksChBodiesAndParts;
    extrusionDef->directionType = dtNormal;
    extrusionDef->SetSideParam(true, etBlind, stepDepth, 0, false);
    extrusionDef->SetSketch(sketchEntity);
    extrusionEntity->Create();
}

void buildBridgeHole2(KompasObjectPtr kompas, ksPartPtr part, BridgeHoleBuildTarget target, double stepDepth,
        int polygonAngleCount, double centerX, double centerY, double radius) {
    Sketch sketch = createSketch(kompas, part, target.face);

    ksRegularPolygonParamPtr polygonParam(kompas->GetParamStruct(ko_RegularPolygonParam));
    polygonParam->xc = centerX; polygonParam->yc = centerY;
    polygonParam->count = polygonAngleCount;
    polygonParam->describe = true;
    polygonParam->radius = radius;
    polygonParam->style = 1;

    sketch.document2d->ksRegularPolygon(polygonParam, 0);
    sketch.definition->EndEdit();

    ksEntityPtr extrusionEntity(part->NewEntity(o3d_cutExtrusion));
    ksCutExtrusionDefinitionPtr extrusionDef(extrusionEntity->GetDefinition());
    extrusionDef->cut = true;
    extrusionDef->chooseType = ksChBodiesAndParts;
    extrusionDef->directionType = dtNormal;
    extrusionDef->SetSideParam(true, etBlind, stepDepth, 0, false);
    extrusionDef->SetSketch(sketch.entity);
    extrusionEntity->Create();
}

void buildBridgeHoles(KompasObjectPtr kompas, ksPartPtr part, std::list<BridgeHoleBuildTarget> bridgeHoleBuildTargets, double stepDepth) {
    for (BridgeHoleBuildTarget target : bridgeHoleBuildTargets) {

        Sketch sketch = createSketch(kompas, part, target.face);
        ICirclePtr innerCircle = drawThinCircleProjection(sketch, part, target);
        double centerX = innerCircle->Xc, centerY = innerCircle->Yc, radius = innerCircle->Radius;

        if (loopIsCircle(target.outerLoop)) {
            bridgeHoleBuildCircleDrawSketch1(sketch, innerCircle, target);
        } else {
            bridgeHoleBuildNotCircleDrawSketch1(kompas, sketch, innerCircle, target);
        }
        sketch.definition->EndEdit();
        buildBridgeHole1(sketch.entity, part, stepDepth);

        buildBridgeHole2(kompas, part, target, stepDepth * 2, 4, centerX, centerY, radius);
        buildBridgeHole2(kompas, part, target, stepDepth * 3, 8, centerX, centerY, radius);
    }
}

void bridgeHoleBuildOptimization(KompasObjectPtr kompas, ksPartPtr part, ksFaceDefinitionPtr printFace, double stepDepth) {
    std::list<BridgeHoleBuildTarget> targets = getBridgeHoleBuildTargets(part, printFace);
    buildBridgeHoles(kompas, part, targets, stepDepth);
}
