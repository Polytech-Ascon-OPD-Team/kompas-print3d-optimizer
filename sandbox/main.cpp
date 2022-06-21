#include <iostream>
#include <conio.h>
#include <list>

#include "kompasUtils.hpp"

#import "ksconstants.tlb" no_namespace named_guids
#import "ksConstants3D.tlb" no_namespace named_guids
#import "kAPI5.tlb" no_namespace named_guids rename( "min", "Imin" ) rename( "max", "Imax" ) rename( "ksFragmentLibrary", "ksIFragmentLibrary" )
#import "kAPI7.tlb" no_namespace named_guids rename( "CreateWindow", "ICreateWindow" ) rename( "PostMessage", "IPostMessage" ) rename( "MessageBoxEx", "IMessageBoxEx" )

bool doubleEqual(double a, double b, double epsilon = 0.00001) {
    return (abs(a - b) < epsilon);
}

struct BridgeHoleTarget {
    ksLoopPtr loop;
    ksFaceDefinitionPtr face;
};

int main() {
    CoInitialize(nullptr);
    KompasObjectPtr kompas = kompasInit();

    ksDocument3DPtr document3d = kompas->ActiveDocument3D();
    ksChooseMngPtr chooseMng(document3d->GetChooseMng());

    ksPartPtr part(document3d->GetPart(pTop_Part));
    ksBodyPtr body = part->GetMainBody();

    ksMeasurerPtr measurer(part->GetMeasurer());

    ksFaceCollectionPtr faces = body->FaceCollection();
    int facesCount = faces->GetCount();

    ksFaceDefinitionPtr printFace;

    // Задаем грань стола 3д принтера. Только для тестирования
    for (int printFaceIndex = 0; printFaceIndex < facesCount; printFaceIndex++) {
        printFace = faces->GetByIndex(printFaceIndex);
        if (abs(printFace->GetArea(ksLUnMM) - 1146) < 2) {
            break;
        }
    }

    chooseMng->Choose(printFace);
    std::cout << "print face\n";
    _getch();
    chooseMng->UnChooseAll();

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
            ksEdgeCollectionPtr edges(innerLoop->EdgeCollection());
            int edgesCount = edges->GetCount();

            for (int holeEdgeIndex = 0; holeEdgeIndex < edgesCount; holeEdgeIndex++) {
                ksEdgeDefinitionPtr holeEdge(edges->GetByIndex(holeEdgeIndex));
                ksFaceDefinitionPtr holeFace(holeEdge->GetAdjacentFace(false));

                if (!holeFace->IsCylinder()) {
                    if (!holeFace->IsPlanar()) {
                        break;
                    } else {
                        measurer->SetObject1(printFace);
                        measurer->SetObject2(holeFace);
                        measurer->Calc();
                        double angle = measurer->angle;
                        if (!(doubleEqual(angle, 90.0) || doubleEqual(angle, 270.0))) {
                            break;
                        }
                    }
                }

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
                    if (edgesCount != 1) {
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
                if (isHoleEdgeLower) {
                    BridgeHoleTarget target{ innerLoop, face };
                    bridgeHoleTargets.push_back(target);
                }
                break;
            }
        }
    }

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
        extrusionDef->SetSideParam(false, etBlind, 0.4, 0, false);
        extrusionDef->SetSketch(sketchEntity);
        extrusionEntity->Create();
    }

    document3d->RebuildDocument(); // Нужно чтобы исправить странные ошибки "Вырожденная проекция ребра"

    return 0;
}
