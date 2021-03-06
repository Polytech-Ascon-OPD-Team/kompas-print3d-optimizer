#include "stdafx.h"
#include "optimizeRoundingEdgesOnPrintFace.hpp"

#include <sstream>
#include <atlbase.h>

#include "kompasUtils.hpp"
#include "utils.hpp"

const char* MACRO_NAME_ROUNDING_EDGES_ON_PRINT_FACE = "??????????? ??????????? ????? ?? ????????? ??????";
const char* MACRO_NAME_ROUNDING_EDGES_ON_PRINT_FACE_ELEMENT = "??????";
const char* MACRO_NAME_ROUNDING_EDGES_ON_PRINT_FACE_ELEMENT_WITH_REWORK = "?????? - ?????????";

double getCylinderOrTorusRadius(ksFaceDefinitionPtr face) {
    if (face->IsCylinder()) {
        double height = 0.0, radius = 0.0;
        face->GetCylinderParam(&height, &radius);
        return radius;
    } else if (face->IsTorus()) {
        ksSurfacePtr surface(face->GetSurface());
        ksTorusParamPtr torusParam(surface->GetSurfaceParam());
        return torusParam->radius;
    }
    return 0.0;
}

bool faceNeedRework(ksFaceDefinitionPtr roundingFace) {
    if (!roundingFace->IsCylinder() && !roundingFace->IsTorus()) {
        return true;
    }

    ksEdgeCollectionPtr edges(roundingFace->EdgeCollection());
    int edgesCount = edges->GetCount();
    if (roundingFace->IsCylinder()) {
        // ???? ????? ??????????????, ?? ??? ????? ??????, ? ?????? ??? ????
        if (edgesCount != 4) {
            return true;
        }
        int straightCount = 0, arcCount = 0;
        for (int i = 0; i < edgesCount; i++) {
            ksEdgeDefinitionPtr edge(edges->GetByIndex(i));
            if (edge->IsStraight()) {
                straightCount++;
            } else if (edge->IsArc()) {
                arcCount++;
            }
        }
        return !((straightCount == 2) && (arcCount == 2));
    } else {
        // ???? ????? ????????????
        if ((edgesCount == 2) || (edgesCount == 3)) {
            // ??? ??? ??? ????? ??????? ?? ????????? ?????????
            // ??? ????? ? ????? ??????? ??????? ?????
            return false;
        } else if (edgesCount == 4) {
            // ???? ????? ??????, ?? ??? ??? ?????? ???? ??????
            for (int i = 0; i < edgesCount; i++) {
                ksEdgeDefinitionPtr edge(edges->GetByIndex(i));
                double length = edge->GetLength(ksLengthUnitsEnum::ksLUnMM);
                // ? ?????, ??? ?????? ???? 3 ?????, ??????-?? ???????????? ??? ???? ?????, ????? ???????? 0
                // ??? ????? ????? ??? ??????? ??????????? ????? ????? ????? Is...() ?????????? false
                if (!edge->IsArc() && !doubleEqual(length, 0.0)) {
                    return true;
                }
            }
            return false;
        }
        return true;
    }
}

bool targetNeedRework(RoundingEdgeOnPrintFaceTarget target) {
    std::list<ksEdgeDefinitionPtr> firstAndLastEdge;
    firstAndLastEdge.push_back(target.trajectory.front()); firstAndLastEdge.push_back(target.trajectory.back());
    for (ksEdgeDefinitionPtr edge : firstAndLastEdge) {
        ksFaceDefinitionPtr roundingFace(edge->GetAdjacentFace(false));
        if (!roundingFace->IsCylinder() && !roundingFace->IsTorus()) {
            roundingFace = edge->GetAdjacentFace(true);
        }
        if (faceNeedRework(roundingFace)) {
            return true;
        }
    }
    return false;
}

std::list<RoundingEdgeOnPrintFaceTarget> getRoundingEdgesOnPrintFaceTargets(ksFaceDefinitionPtr printFace, ReworkType reworkType) {
    std::list<RoundingEdgeOnPrintFaceTarget> targets;

    ksLoopCollectionPtr loops(printFace->LoopCollection());
    for (int loopIndex = 0; loopIndex < loops->GetCount(); loopIndex++) {
        ksLoopPtr loop(loops->GetByIndex(loopIndex));

        RoundingEdgeOnPrintFaceTarget target;
        double radius = 0.0;

        bool firstEdgeInTarget = false;
        bool firstTargetInLoopCompleted = false;
        double firstEdgeRadius = 0.0;
        std::list<RoundingEdgeOnPrintFaceTarget>::iterator targetWithFirstEdge;

        ksEdgeCollectionPtr edges(loop->EdgeCollection());
        for (int edgeIndex = 0; edgeIndex < edges->GetCount(); edgeIndex++) {
            ksEdgeDefinitionPtr edge(edges->GetByIndex(edgeIndex));

            ksFaceDefinitionPtr roundingFace(edge->GetAdjacentFace(false));
            if (roundingFace == printFace) {
                roundingFace = edge->GetAdjacentFace(true);
            }

            if ((edge->IsStraight() && roundingFace->IsCylinder()) ||
                    ((edge->IsCircle() || edge->IsArc()) && roundingFace->IsTorus())) {
                if (target.trajectory.empty()) {
                    radius = getCylinderOrTorusRadius(roundingFace);
                    target.roundingFace = roundingFace;
                } else if (!doubleEqual(radius, getCylinderOrTorusRadius(roundingFace))) {
                    targets.push_back(target);
                    target = RoundingEdgeOnPrintFaceTarget();
                    
                    if (firstEdgeInTarget && !firstTargetInLoopCompleted) {
                        targetWithFirstEdge = --targets.end();
                    }
                    firstTargetInLoopCompleted = true;
                }
                target.trajectory.push_back(edge);
                
                if (edgeIndex == 0) {
                    firstEdgeInTarget = true;
                    firstEdgeRadius = radius;
                }
            } else if (!target.trajectory.empty()) {
                targets.push_back(target);
                target = RoundingEdgeOnPrintFaceTarget();
                
                if (firstEdgeInTarget && !firstTargetInLoopCompleted) {
                    targetWithFirstEdge = --targets.end();
                }
                firstTargetInLoopCompleted = true;
            }
        }

        if (!target.trajectory.empty()) {
            if (firstEdgeInTarget && firstTargetInLoopCompleted && doubleEqual(firstEdgeRadius, radius)) {
                RoundingEdgeOnPrintFaceTarget firstTarget = *(targetWithFirstEdge);
                targets.erase(targetWithFirstEdge);
                target.trajectory.insert(target.trajectory.cbegin(), firstTarget.trajectory.cbegin(), firstTarget.trajectory.cend());
                target.roundingFace = firstTarget.roundingFace;
            }
            targets.push_back(target);
        }
    }

    for (std::list<RoundingEdgeOnPrintFaceTarget>::iterator it = targets.begin(); it != targets.end();) {
        if (targetNeedRework(*it)) {
            if (reworkType == ReworkType::ONLY_WITHOUT_REWORK) {
                it = targets.erase(it);
            } else {
                it->needRework = true;
                it++;
            }
        } else {
            if (reworkType == ReworkType::ONLY_WITH_REWORK) {
                it = targets.erase(it);
            } else {
                it->needRework = false;
                it++;
            }
        }
    }

    return targets;
}

void drawSketch(Sketch sketch, RoundingEdgeOnPrintFaceTarget target, double overhangThreshold) {
    std::ostringstream oss;
    oss << (180.0 - overhangThreshold);
    CComBSTR temp(oss.str().c_str());
    _bstr_t expression = temp.Detach();

    IViewsAndLayersManagerPtr viewsAndLayersManager(sketch.document2d_api7->ViewsAndLayersManager);
    IViewsPtr views(viewsAndLayersManager->Views);
    IViewPtr view(views->ActiveView);
    IDrawingContainerPtr drawingContainer(view);
    
    // ????????? ????????
    sketch.definition->AddProjectionOf(target.trajectory.front()->GetVertex(true));
    IPointsPtr points(drawingContainer->Points);
    IPointPtr startPoint(points->GetPoint(0));

    sketch.definition->AddProjectionOf(target.roundingFace);
    IArcsPtr arcs(drawingContainer->Arcs);
    IArcPtr roundingArc = nullptr;
    bool startPointIs1 = false;
    for (int i = 0; i < arcs->GetCount(); i++) {
        IArcPtr arc(arcs->GetArc(i));
        if (!roundingArc && ((doubleEqual(startPoint->X, arc->X1) && doubleEqual(startPoint->Y, arc->Y1)) ||
                (doubleEqual(startPoint->X, arc->X2) && doubleEqual(startPoint->Y, arc->Y2)))) {
            roundingArc = arc;
            if (doubleEqual(startPoint->X, arc->X1)) {
                startPointIs1 = true;
            }
        }
        arc->Style = ksCurveStyleEnum::ksCSThin;
        arc->Update();
    }
    
    ILineSegmentsPtr lineSegments(drawingContainer->LineSegments);
    for (int i = 0; i < lineSegments->GetCount(); i++) {
        ILineSegmentPtr lineSegment(lineSegments->GetLineSegment(i));
        lineSegment->Style = ksCurveStyleEnum::ksCSThin;
        lineSegment->Update();
    }

    // ?????? ??? ???????
    ILineSegmentPtr lineSeg1(lineSegments->Add());
    lineSeg1->X1 = startPoint->X; lineSeg1->Y1 = startPoint->Y;
    if (startPointIs1) {
        lineSeg1->X2 = roundingArc->X2; lineSeg1->Y2 = roundingArc->Y2;
    } else {
        lineSeg1->X2 = roundingArc->X1; lineSeg1->Y2 = roundingArc->Y1;
    }
    lineSeg1->Update();

    IDrawingObjectPtr lineSeg1DrawingObject(lineSeg1);
    IDrawingObject1Ptr lineSeg1DrawingObject1(lineSeg1DrawingObject);
    {
        IParametriticConstraintPtr constraint(lineSeg1DrawingObject1->NewConstraint());
        constraint->ConstraintType = ksConstraintTypeEnum::ksCMergePoints;
        constraint->Index = 0;
        constraint->Partner = static_cast<IDispatch*>(roundingArc);
        if (startPointIs1) {
            constraint->PartnerIndex = 1;
        } else {
            constraint->PartnerIndex = 2;
        }
        constraint->Create();
    }
    {
        IParametriticConstraintPtr constraint(lineSeg1DrawingObject1->NewConstraint());
        constraint->ConstraintType = ksConstraintTypeEnum::ksCTangentTwoCurves;
        constraint->Partner = static_cast<IDispatch*>(roundingArc);
        constraint->Create();
    }
    {
        IParametriticConstraintPtr constraint(lineSeg1DrawingObject1->NewConstraint());
        constraint->ConstraintType = ksConstraintTypeEnum::ksCMergePoints;
        constraint->Index = 0;
        constraint->Partner = static_cast<IDispatch*>(startPoint);
        constraint->Create();
    }

    ILineSegmentPtr lineSeg2(lineSegments->Add());
    lineSeg2->X1 = lineSeg1->X2; lineSeg2->Y1 = lineSeg1->Y2;
    if (startPointIs1) {
        lineSeg2->X2 = roundingArc->X2; lineSeg2->Y2 = roundingArc->Y2;
    } else {
        lineSeg2->X2 = roundingArc->X1; lineSeg2->Y2 = roundingArc->Y1;
    }
    lineSeg2->Update();

    IDrawingObjectPtr lineSeg2DrawingObject(lineSeg2);
    IDrawingObject1Ptr lineSeg2DrawingObject1(lineSeg2DrawingObject);
    {
        IParametriticConstraintPtr constraint(lineSeg2DrawingObject1->NewConstraint());
        constraint->ConstraintType = ksConstraintTypeEnum::ksCMergePoints;
        constraint->Index = 0;
        constraint->Partner = static_cast<IDispatch*>(lineSeg1);
        constraint->PartnerIndex = 1;
        constraint->Create();
    }
    {
        IParametriticConstraintPtr constraint(lineSeg2DrawingObject1->NewConstraint());
        constraint->ConstraintType = ksConstraintTypeEnum::ksCTangentTwoCurves;
        constraint->Partner = static_cast<IDispatch*>(roundingArc);
        constraint->Create();
    }
    {
        IParametriticConstraintPtr constraint(lineSeg2DrawingObject1->NewConstraint());
        constraint->ConstraintType = ksConstraintTypeEnum::ksCPointOnCurve;
        constraint->Index = 1;
        constraint->Partner = static_cast<IDispatch*>(roundingArc);
        constraint->Create();
    }

    // ????????????? ???????
    ISymbols2DContainerPtr symbols2dContainer(view);
    IAngleDimensionsPtr angleDimensions(symbols2dContainer->AngleDimensions);

    IAngleDimensionPtr angleDim(angleDimensions->Add(DrawingObjectTypeEnum::ksDrADimension));
    angleDim->DimensionType = ksAngleDimTypeEnum::ksADMinAngle;
    angleDim->BaseObject1 = lineSeg1DrawingObject;
    angleDim->BaseObject2 = lineSeg2DrawingObject;
    angleDim->Radius = 0;
    angleDim->X3 = (lineSeg1->X1 + lineSeg2->X2) / 2;
    angleDim->Y3 = (lineSeg1->Y1 + lineSeg2->Y2) / 2;
    angleDim->Update();
    IDrawingObject1Ptr angleDimDrawingObject1(angleDim);
    {
        IParametriticConstraintPtr constraint(angleDimDrawingObject1->NewConstraint());
        constraint->ConstraintType = ksConstraintTypeEnum::ksCFixedDim;
        constraint->Create();
    }
    {
        IParametriticConstraintPtr constraint(angleDimDrawingObject1->NewConstraint());
        constraint->ConstraintType = ksConstraintTypeEnum::ksCDimWithVariable;
        constraint->Expression = expression;
        constraint->Create();
    }

    // ??????????? ????? ?????
    IArcPtr arc(arcs->Add());
    arc->Xc = roundingArc->Xc; arc->Yc = roundingArc->Yc;
    arc->X1 = startPoint->X; arc->Y1 = startPoint->Y;
    arc->X2 = lineSeg2->X2; arc->Y2 = lineSeg2->Y2;
    arc->Radius = roundingArc->Radius;
    if (startPointIs1) {
        arc->Direction = roundingArc->Direction;
    } else {
        arc->Direction = !roundingArc->Direction;
    }
    arc->Update();

    IDrawingObjectPtr arcDrawingObject(arc);
    IDrawingObject1Ptr arcDrawingObject1(arcDrawingObject);
    {
        IParametriticConstraintPtr constraint(arcDrawingObject1->NewConstraint());
        constraint->ConstraintType = ksConstraintTypeEnum::ksCMergePoints;
        constraint->Index = 1;
        constraint->Partner = static_cast<IDispatch*>(lineSeg1);
        constraint->PartnerIndex = 0;
        constraint->Create();
    }
    {
        IParametriticConstraintPtr constraint(arcDrawingObject1->NewConstraint());
        constraint->ConstraintType = ksConstraintTypeEnum::ksCMergePoints;
        constraint->Index = 2;
        constraint->Partner = static_cast<IDispatch*>(lineSeg2);
        constraint->PartnerIndex = 1;
        constraint->Create();
    }
    {
        IParametriticConstraintPtr constraint(arcDrawingObject1->NewConstraint());
        constraint->ConstraintType = ksConstraintTypeEnum::ksCEqualRadius;
        constraint->Partner = static_cast<IDispatch*>(roundingArc);
        constraint->Create();
    }
}

void optimizeRoundingEdgesOnPrintFace(KompasObjectPtr kompas, ksPartPtr part, ksFaceDefinitionPtr printFace, double overhangThreshold,
        ReworkType reworkType) {
    std::list<RoundingEdgeOnPrintFaceTarget> targets = getRoundingEdgesOnPrintFaceTargets(printFace, reworkType);

    ksEntityPtr macroEntity(part->NewEntity(o3d_MacroObject));
    ksMacro3DDefinitionPtr macro(macroEntity->GetDefinition());
    macroEntity->name = MACRO_NAME_ROUNDING_EDGES_ON_PRINT_FACE;
    macro->StaffVisible = true;
    macroEntity->Create();

    for (RoundingEdgeOnPrintFaceTarget target : targets) {
        ksEntityPtr macroElementEntity(part->NewEntity(o3d_MacroObject));
        ksMacro3DDefinitionPtr macroElement(macroElementEntity->GetDefinition());
        if (target.needRework) {
            macroElementEntity->name = MACRO_NAME_ROUNDING_EDGES_ON_PRINT_FACE_ELEMENT_WITH_REWORK;
        } else {
            macroElementEntity->name = MACRO_NAME_ROUNDING_EDGES_ON_PRINT_FACE_ELEMENT;
        }
        macroElement->StaffVisible = true;
        macroElementEntity->Create();

        // ??????? ????????? ??? ??????
        ksEntityPtr sketchPlane(part->NewEntity(Obj3dType::o3d_planePerpendicular));
        ksPlanePerpendicularDefinitionPtr sketchPlaneDef(sketchPlane->GetDefinition());
        sketchPlaneDef->SetEdge(target.trajectory.front());
        sketchPlaneDef->SetPoint(target.trajectory.front()->GetVertex(true));
        sketchPlane->hidden = true;
        sketchPlane->Create();
        macroElement->Add(sketchPlane);
        
        // ??????? ?????
        Sketch sketch = createSketch(kompas, part, sketchPlane);
        drawSketch(sketch, target, overhangThreshold);
        sketch.definition->EndEdit();
        macroElement->Add(sketch.entity);
        
        // ??????????? ????? ?? ??????????
        ksEntityPtr evolutionEntity(part->NewEntity(Obj3dType::o3d_bossEvolution));
        ksBossEvolutionDefinitionPtr evolutionDef(evolutionEntity->GetDefinition());
        evolutionDef->chooseType = ksChooseType::ksChBodiesAndParts;
        evolutionDef->sketchShiftType = 1;
        evolutionDef->SetSketch(sketch.entity);
        ksEntityCollectionPtr trajectory(evolutionDef->PathPartArray());
        for (ksEdgeDefinitionPtr edge : target.trajectory) {
            trajectory->Add(edge);
        }
        evolutionEntity->Create();
        macroElement->Add(evolutionEntity);
        macroElementEntity->Update();
        
        macro->Add(macroElementEntity);
    }
    macroEntity->Update();
}
