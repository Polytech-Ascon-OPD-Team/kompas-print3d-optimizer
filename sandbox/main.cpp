#include <iostream>
#include <float.h>
#include <conio.h>

#include "kompasUtils.hpp"
#include "selectPlane.hpp"
#include "optimizeRounding.hpp"
#include "optimizeElephantFoot.hpp"
#include "optimizeOverhangingFaces.hpp"
#include "optimizeCircleHorizontalHoles.hpp"

#define _USE_MATH_DEFINES
#include <math.h>
#import "ksconstants.tlb" no_namespace named_guids
#import "ksConstants3D.tlb" no_namespace named_guids
#import "kAPI5.tlb" no_namespace named_guids rename( "min", "Imin" ) rename( "max", "Imax" ) rename( "ksFragmentLibrary", "ksIFragmentLibrary" )
#import "kAPI7.tlb" no_namespace named_guids rename( "CreateWindow", "ICreateWindow" ) rename( "PostMessage", "IPostMessage" ) rename( "MessageBoxEx", "IMessageBoxEx" )

void performRoundingOptimization(KompasObjectPtr kompas) {
    PlaneEq planeEq;
    ksFaceDefinitionPtr face = getSelectedPlane(kompas, &planeEq);
    double radius;
    double angle;
    if (face && kompas->ksReadDouble("Радиус: ", 0.0, -DBL_MIN, DBL_MAX, &radius) == 1 && kompas->ksReadDouble("Граничный угол: ", 60, -DBL_MIN, DBL_MAX, &angle) == 1 ) {
        if (radius > DBL_MIN) {
            optimizeByRounding(kompas, face, planeEq, radius, angle);
            kompas->ksMessage("Оптимизация модели была выполнена!");
        } else {
            kompas->ksMessage("Неверный радиус!");
        }
    }
}

void performAntiElephantFootOptimiztion(KompasObjectPtr kompas) {
    PlaneEq planeEq;
    ksFaceDefinitionPtr face = getSelectedPlane(kompas, &planeEq);
    double width;
    if (face && kompas->ksReadDouble("Размер оптимизирующей фаски: ", 0.0, -DBL_MIN, DBL_MAX, &width) == 1) {
        optimizeElephantFoot(kompas, face, planeEq, width);
        kompas->ksMessage("Оптимизация модели была выполнена!");
    }
}

void performAntiOverhangingOptimization(KompasObjectPtr kompas) {
    PlaneEq planeEq;
    ksFaceDefinitionPtr face = getSelectedPlane(kompas, &planeEq);
    double angle; //предельный угол нависания 
    if (face && kompas->ksReadDouble("Макс. угол нависания: ", 60, -DBL_MIN, DBL_MAX, &angle) == 1) {
        optimizeOverhangingFace(kompas, face, planeEq, angle);
    }
}

void performHorizontalHolesOptimization(KompasObjectPtr kompas) {
    PlaneEq planeEq;
    ksFaceDefinitionPtr face = getSelectedPlane(kompas, &planeEq);
    IApplicationPtr api7 = kompas->ksGetApplication7();
    IKompasDocument3DPtr document3d(api7->GetActiveDocument());

    IPart7Ptr topPart(document3d->GetTopPart());
    ksPartPtr part = kompas->TransferInterface(topPart, 1, 0);
    ksDocument3DPtr doc3d = kompas->ActiveDocument3D();
    //ksChooseMngPtr chooser(doc3d->GetChooseMng());
    std::set<ksFaceDefinitionPtr> holes = getHorizontalCircleHoles(part, face, planeEq);
    std::cout << "holes number:" << holes.size() << "\n";
    for (std::set<ksFaceDefinitionPtr>::iterator iter = holes.begin(); iter != holes.end(); iter++) {
        //chooser->Choose(*iter);
        ksFaceDefinitionPtr face = *iter;
        ksEntityPtr axisEntity(part->NewEntity(o3d_axisConeFace));
        ksAxisConefaceDefinitionPtr axis(axisEntity->GetDefinition());
        axis->SetFace(face);
        ksVertexDefinitionPtr vertex(ksEdgeDefinitionPtr(ksEdgeCollectionPtr(face->EdgeCollection())->First())->GetVertex(true));
        axisEntity->Create();
        ksEntityPtr mainPlaneEntity(part->NewEntity(o3d_planePerpendicular));
        ksPlanePerpendicularDefinitionPtr mainPlane(mainPlaneEntity->GetDefinition());
        mainPlane->SetPoint(vertex);
        mainPlane->SetEdge(axis);
        mainPlaneEntity->Create();

        /*
        ksEntityPtr planeMiddleEntity(part->NewEntity(o3d_planeMiddle));

    ksDocument3DPtr doc3d = kompas->ActiveDocument3D();
    ksChooseMngPtr chooseMng(doc3d->GetChooseMng());

         ksPlaneMiddleDefinitionPtr planeMiddle(planeMiddleEntity->GetDefinition());
         ksFaceCollectionPtr otherFaces(face->ConnectedFaceCollection());
        
        planeMiddle->SetObject(1, otherFaces->GetByIndex(0));
        planeMiddle->SetObject(2, otherFaces->GetByIndex(1));
        planeMiddleEntity->Create();
        */
        //planeMiddleEntity->Puthidden(true);
        //o3d_point3D


    }
}




int main() {
    short choise;
    std::setlocale(LC_ALL, "Russian");
    std::cout << "1 - Исправление выпирающих углов" << "\n";
    std::cout << "2 - Оптимизация вертикальных сквозных отверстий" << "\n";
    std::cout << "3 - Исправление слоновьей ноги" << "\n";
    std::cout << "4 - Исправление выступающих нависающих частей" << "\n";
    std::cout << "5 - Оптимизация горизонтальных круглых отверстий" << "\n";

    std::cout << "Ваш выбор:";
    std::cin >> choise;
    CoInitialize(nullptr);
    KompasObjectPtr kompas = kompasInit();
    if (kompas) {
        switch (choise) {
        case 1: {
            performRoundingOptimization(kompas);
            break;
        }
        case 3: {
            performAntiElephantFootOptimiztion(kompas);
            break;
        }
        case 4: {
            performAntiOverhangingOptimization(kompas);
            break;
        }
        case 5: {
            performHorizontalHolesOptimization(kompas);
            break;
        }
        default:
            break;
        }
    }
}
