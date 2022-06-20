#include <iostream>
#include <conio.h>

#include "kompasUtils.hpp"

#import "ksconstants.tlb" no_namespace named_guids
#import "ksConstants3D.tlb" no_namespace named_guids
#import "kAPI5.tlb" no_namespace named_guids rename( "min", "Imin" ) rename( "max", "Imax" ) rename( "ksFragmentLibrary", "ksIFragmentLibrary" )
#import "kAPI7.tlb" no_namespace named_guids rename( "CreateWindow", "ICreateWindow" ) rename( "PostMessage", "IPostMessage" ) rename( "MessageBoxEx", "IMessageBoxEx" )

bool doubleEqual(double a, double b, double epsilon = 0.00001) {
    return (abs(a - b) < epsilon);
}

int main() {
    CoInitialize(nullptr);
    KompasObjectPtr kompas = kompasInit();

    IApplicationPtr api7 = kompas->ksGetApplication7();
    IKompasDocument3DPtr document3d(api7->GetActiveDocument());
    IPart7Ptr topPart(document3d->GetTopPart());

    ksDocument3DPtr doc3d = kompas->ActiveDocument3D();
    ksChooseMngPtr chooseMng(doc3d->GetChooseMng());

    ksPartPtr part = kompas->TransferInterface(topPart, 1, 0);
    ksBodyPtr body = part->GetMainBody();

    ksMeasurerPtr measurer(part->GetMeasurer());

    ksFaceCollectionPtr faces = body->FaceCollection();
    int facesCount = faces->GetCount();

    ksFaceDefinitionPtr printFace;

    // Задаем грань стола 3д принтера. Только для тестирования
    for (int printFaceIndex = 0; printFaceIndex < facesCount; printFaceIndex++) {
        printFace = faces->GetByIndex(printFaceIndex);
        if (abs(printFace->GetArea(ksLUnMM) - 968) < 2) {
            break;
        }
    }

    chooseMng->Choose(printFace);
    std::cout << "print face\n";
    _getch();
    chooseMng->UnChooseAll();

    for (int faceIndex = 0; faceIndex < facesCount; faceIndex++) {
        ksFaceDefinitionPtr face = faces->GetByIndex(faceIndex);
        if (!face->IsPlanar() || (face == printFace)) {
            continue;
        }
        measurer->SetObject1(printFace); measurer->SetObject2(face);
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

            if (edges->GetCount() == 1) {
                ksEdgeDefinitionPtr holeEdge(edges->GetByIndex(0));
                if (!holeEdge->IsCircle()) {
                    continue;
                }
                ksFaceDefinitionPtr cylinderFace(holeEdge->GetAdjacentFace(false));
                if (!cylinderFace->IsCylinder()) {
                    continue;
                }

                measurer->SetObject1(printFace);
                measurer->SetObject2(holeEdge);
                measurer->Calc();
                double distanceToMainEdge = measurer->MinDistance;
                bool isMainEdgeLower = true;

                ksEdgeCollectionPtr edges2(cylinderFace->EdgeCollection());
                for (int edge2Index = 0; edge2Index < edges2->GetCount(); edge2Index++) {
                    ksEdgeDefinitionPtr edge2(edges2->GetByIndex(edge2Index));
                    if (edge2 == holeEdge) {
                        continue;
                    }

                    measurer->SetObject2(edge2);
                    measurer->Calc();
                    double distanceToEdge2 = measurer->MinDistance;
                    if (distanceToEdge2 < distanceToMainEdge) {
                        isMainEdgeLower = false;
                        break;
                    }
                }
                if (isMainEdgeLower) {
                    chooseMng->Choose(holeEdge);
                }

            } else {
                for (int holeEdgeIndex = 0; holeEdgeIndex < edges->GetCount(); holeEdgeIndex++) {
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

                        measurer->SetObject2(edge2);
                        measurer->Calc();
                        double angle = measurer->angle;
                        if (doubleEqual(angle, 90) || doubleEqual(angle, 270)) {
                            continue;
                        }
                        double distanceToEdge2 = measurer->distance;
                        if (distanceToEdge2 < distanceToHoleEdge) {
                            isHoleEdgeLower = false;
                            break;
                        }
                    }
                    if (isHoleEdgeLower) {
                        chooseMng->Choose(holeEdge);
                    } else {
                        break;
                    }
                }
            }

        }
    }
    return 0;
}
