#include "optimizeRoundingHorizontalEdges.hpp"

#import "ksconstants.tlb" no_namespace named_guids
#import "ksConstants3D.tlb" no_namespace named_guids
#import "kAPI5.tlb" no_namespace named_guids rename( "min", "Imin" ) rename( "max", "Imax" ) rename( "ksFragmentLibrary", "ksIFragmentLibrary" )

#include "kompasUtils.hpp"
#include "utils.hpp"

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

std::list<RoundingHorizontalEdgeTarget> getRoundingHorizontalEdgesTargets(ksFaceDefinitionPtr printFace) {
    std::list<RoundingHorizontalEdgeTarget> targets;

    ksLoopCollectionPtr loops(printFace->LoopCollection());
    for (int loopIndex = 0; loopIndex < loops->GetCount(); loopIndex++) {
        ksLoopPtr loop(loops->GetByIndex(loopIndex));

        RoundingHorizontalEdgeTarget target;
        double radius = 0.0;

        bool firstEdgeInTarget = false;
        bool firstTargetInLoopCompleted = false;
        double firstEdgeRadius = 0.0;
        std::list<RoundingHorizontalEdgeTarget>::iterator targetWithFirstEdge;

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
                    target = RoundingHorizontalEdgeTarget();
                    
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
                target = RoundingHorizontalEdgeTarget();
                
                if (firstEdgeInTarget && !firstTargetInLoopCompleted) {
                    targetWithFirstEdge = --targets.end();
                }
                firstTargetInLoopCompleted = true;
            }
        }

        if (!target.trajectory.empty()) {
            if (firstEdgeInTarget && firstTargetInLoopCompleted && doubleEqual(firstEdgeRadius, radius)) {
                RoundingHorizontalEdgeTarget firstTarget = *(targetWithFirstEdge);
                targets.erase(targetWithFirstEdge);
                target.trajectory.insert(target.trajectory.cbegin(), firstTarget.trajectory.cbegin(), firstTarget.trajectory.cend());
            }
            targets.push_back(target);
        }
    }
    return targets;
}

void drawSketch(Sketch sketch, RoundingHorizontalEdgeTarget target) {
    IViewsAndLayersManagerPtr viewsAndLayersManager(sketch.document2d_api7->ViewsAndLayersManager);
    IViewsPtr views(viewsAndLayersManager->Views);
    IViewPtr view(views->ActiveView);
    IDrawingContainerPtr drawingContainer(view);
    
    sketch.definition->AddProjectionOf(target.trajectory.front()->GetVertex(true));
    IPointsPtr points(drawingContainer->Points);
    IPointPtr startPoint(points->GetPoint(0));

    sketch.definition->AddProjectionOf(target.roundingFace);
    IArcsPtr arcs(drawingContainer->Arcs);
    IArcPtr roundingArc = nullptr;
    for (int i = 0; i < arcs->GetCount(); i++) {
        IArcPtr arc(arcs->GetArc(i));
        if (!roundingArc && ((doubleEqual(startPoint->X, arc->X1) && doubleEqual(startPoint->Y, arc->Y1)) ||
                (doubleEqual(startPoint->X, arc->X2) && doubleEqual(startPoint->Y, arc->Y2)))) {
            roundingArc = arc;
        } else {
            arc->Style = ksCurveStyleEnum::ksCSThin;
            arc->Update();
        }
    }
    
    ILineSegmentsPtr lineSegments(drawingContainer->LineSegments);
    for (int i = 0; i < lineSegments->GetCount(); i++) {
        ILineSegmentPtr lineSegment(lineSegments->GetLineSegment(i));
        lineSegment->Style = ksCurveStyleEnum::ksCSThin;
        lineSegment->Update();
    }

}

void optimizeRoundingHorizontalEdges(KompasObjectPtr kompas, ksPartPtr part, ksFaceDefinitionPtr printFace) {
    std::list<RoundingHorizontalEdgeTarget> targets = getRoundingHorizontalEdgesTargets(printFace);

    // Подсвечиваем цели.   Отладка!
    /*
    ksDocument3DPtr document3d(kompas->ActiveDocument3D());
    ksChooseMngPtr chooseMng(document3d->GetChooseMng());
    for (RoundingHorizontalEdgeTarget target : targets) {
        chooseMng->UnChooseAll();
        for (ksEdgeDefinitionPtr edge : target.trajectory) {
            chooseMng->Choose(edge);
        }
        _getwch();
        chooseMng->UnChooseAll();
        chooseMng->Choose(target.roundingFace);
        _getwch();
    }
    */

    for (RoundingHorizontalEdgeTarget target : targets) {
        // Создаем плоскость для эскиза
        ksEntityPtr sketchPlane(part->NewEntity(Obj3dType::o3d_planePerpendicular));
        ksPlanePerpendicularDefinitionPtr sketchPlaneDef(sketchPlane->GetDefinition());
        sketchPlaneDef->SetEdge(target.trajectory.front());
        sketchPlaneDef->SetPoint(target.trajectory.front()->GetVertex(true));
        sketchPlane->Create();
        
        // Создаем эскиз
        Sketch sketch = createSketch(kompas, part, sketchPlane);
        drawSketch(sketch, target);
        sketch.definition->EndEdit();
        
        // Протягиваем эскиз по траектории
        // ksBossEvolutionDefinition "Обход с гладкой стыковкой"

    }

}
