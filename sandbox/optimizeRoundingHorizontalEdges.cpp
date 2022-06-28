#include "optimizeRoundingHorizontalEdges.hpp"

#import "ksconstants.tlb" no_namespace named_guids
#import "ksConstants3D.tlb" no_namespace named_guids
#import "kAPI5.tlb" no_namespace named_guids rename( "min", "Imin" ) rename( "max", "Imax" ) rename( "ksFragmentLibrary", "ksIFragmentLibrary" )

#include "kompasUtils.hpp"

std::list<RoundingHorizontalEdgeTarget> getRoundingHorizontalEdgesTargets(ksFaceDefinitionPtr printFace) {
    std::list<RoundingHorizontalEdgeTarget> targets;

    ksLoopCollectionPtr loops(printFace->LoopCollection());
    for (int loopIndex = 0; loopIndex < loops->GetCount(); loopIndex++) {
        ksLoopPtr loop(loops->GetByIndex(loopIndex));

        RoundingHorizontalEdgeTarget target;

        ksEdgeCollectionPtr edges(loop->EdgeCollection());
        for (int edgeIndex = 0; edgeIndex < edges->GetCount(); edgeIndex++) {
            ksEdgeDefinitionPtr edge(edges->GetByIndex(edgeIndex));

            ksFaceDefinitionPtr roundingFace(edge->GetAdjacentFace(false));
            if (roundingFace == printFace) {
                roundingFace = edge->GetAdjacentFace(true);
            }

            if ((edge->IsStraight() && roundingFace->IsCylinder()) ||
                ((edge->IsCircle() || edge->IsArc()) && roundingFace->IsTorus())) {
                target.push_back(edge);
            } else if (!target.empty()) {
                targets.push_back(target);
                target = RoundingHorizontalEdgeTarget();
            }
        }

        if (!target.empty()) {
            targets.push_back(target);
        }
    }
    return targets;
}

void optimizeRoundingHorizontalEdges(ksPartPtr part, ksFaceDefinitionPtr printFace) {
    std::list<RoundingHorizontalEdgeTarget> targets = getRoundingHorizontalEdgesTargets(printFace);

    for (RoundingHorizontalEdgeTarget target : targets) {
        // Создаем плоскость для эскиза
        ksEntityPtr sketchPlane(part->NewEntity(Obj3dType::o3d_planePerpendicular));
        ksPlanePerpendicularDefinitionPtr sketchPlaneDef(sketchPlane->GetDefinition());
        sketchPlaneDef->SetEdge(target.front());
        sketchPlaneDef->SetPoint(target.front()->GetVertex(true));
        sketchPlane->Create();

        // Создаем эскиз
        
        // Протягиваем эскиз по траектории
        // ksBossEvolutionDefinition "Обход с гладкой стыковкой"

    }

}