#include "optimizeElephantFoot.hpp"
#include "selectPlane.hpp"
#include <iostream>
#define _USE_MATH_DEFINES
#include <math.h>
#import "ksconstants.tlb" no_namespace named_guids
#import "ksConstants3D.tlb" no_namespace named_guids
#import "kAPI5.tlb" no_namespace named_guids rename( "min", "Imin" ) rename( "max", "Imax" ) rename( "ksFragmentLibrary", "ksIFragmentLibrary" )
#import "kAPI7.tlb" no_namespace named_guids rename( "CreateWindow", "ICreateWindow" ) rename( "PostMessage", "IPostMessage" ) rename( "MessageBoxEx", "IMessageBoxEx" )



bool checkOverhangingPlane(PlaneEq planeEqOfPrintPlane, ksFaceDefinitionPtr face, double angle) {
    double limit_cos_angle = cos(angle * M_PI / 180.0);
    ksTessellationPtr tessellation(face->GetTessellation());
    float x, y, z;
    tessellation->GetNormal(0, &x, &y, &z);
    double n1_mul_n0 = (x * planeEqOfPrintPlane.a) + (y * planeEqOfPrintPlane.b) + (z * planeEqOfPrintPlane.c);
    double n1_abs = sqrt((x * x) + (y * y) + (z * z));
    double n0_abs = sqrt((planeEqOfPrintPlane.a * planeEqOfPrintPlane.a) + (planeEqOfPrintPlane.b * planeEqOfPrintPlane.b) + (planeEqOfPrintPlane.c * planeEqOfPrintPlane.c));
    double cos_n1_n0 = n1_mul_n0 / (n1_abs * n0_abs);
    return cos_n1_n0 < -limit_cos_angle;
}

double getHeightOfLowestVertex(PlaneEq planeEqOfPrintPlane, ksFaceDefinitionPtr face) {
    double lowest = DBL_MAX;
    ksEdgeCollectionPtr edges(face->EdgeCollection());
    for (int i = 0; i < edges->GetCount(); i++) {
        ksEdgeDefinitionPtr edge(edges->GetByIndex(i));
        ksVertexDefinitionPtr v1(edge->GetVertex(true));
        ksVertexDefinitionPtr v2(edge->GetVertex(true));
        if (v1 && v2) {
            double x1, x2, y1, y2, z1, z2;
            v1->GetPoint(&x1, &y1, &z1);
            v2->GetPoint(&x2, &y2, &z2);
            double v1_h = (planeEqOfPrintPlane.a * x1) + (planeEqOfPrintPlane.b * y1) + (planeEqOfPrintPlane.c * z1) + planeEqOfPrintPlane.d;
            double v2_h = (planeEqOfPrintPlane.a * x2) + (planeEqOfPrintPlane.b * y2) + (planeEqOfPrintPlane.c * z2) + planeEqOfPrintPlane.d;
            if (v1_h < lowest) {
                lowest = v1_h;
            }
            if (v2_h < lowest) {
                lowest = v2_h;
            }
        }
    }
    return lowest;
}

//TODO: 
bool checkAngle(ksEdgeDefinitionPtr edge, ksFaceDefinitionPtr current, ksFaceDefinitionPtr other) {
    return true;
}

void optimizeOverhangingFace(KompasObjectPtr kompas, ksFaceDefinitionPtr face, PlaneEq planeEq, double angle) {
    IApplicationPtr api7 = kompas->ksGetApplication7();
    IKompasDocument3DPtr document3d(api7->GetActiveDocument());
    IPart7Ptr topPart(document3d->GetTopPart());
    ksPartPtr part = kompas->TransferInterface(topPart, 1, 0);
    ksFeaturePtr feature(part->GetFeature());
    ksMeasurerPtr measurer(part->GetMeasurer());
    ksEntityCollectionPtr entityCollection(feature->EntityCollection(o3d_face));
    ksDocument3DPtr doc3d = kompas->ActiveDocument3D();
    ksChooseMngPtr chooseMng(doc3d->GetChooseMng());
    chooseMng->UnChooseAll();
    for (int i = 0; i < entityCollection->GetCount(); i++) {
        ksEntityPtr entity(entityCollection->GetByIndex(i));
        ksFaceDefinitionPtr currFace(entity->GetDefinition()); //ðàññìàòðèâàåìàÿ íàâèñàþùàÿ ãðàíü
        if (currFace && currFace->IsPlanar() && checkOverhangingPlane(planeEq, currFace, angle) && !PlaneEq(currFace).equals(face)) {
            ksEdgeCollectionPtr edges(currFace->EdgeCollection());
            for (int j = 0; j < edges->GetCount(); j++) { //îáõîäèì âñå ðåáðà
                ksEdgeDefinitionPtr edge(edges->GetByIndex(j));
                ksFaceDefinitionPtr face1(edge->GetAdjacentFace(true));
                ksFaceDefinitionPtr face2(edge->GetAdjacentFace(false));
                ksFaceDefinitionPtr otherFace = nullptr;
                if (face1 == currFace) {
                    otherFace = face2;
                } else if (face2 == currFace) {
                    otherFace = face1;
                } else {
                    continue;
                }
                measurer->SetObject1(currFace);
                measurer->SetObject2(otherFace);
                measurer->Calc();
                std::cout << "angle:" << measurer->angle << "\n"; // Çäåñü âñåãäà ïîëó÷åì çíà÷åíèå óãëà ìåíüøå 180 ãðàäóñîâ  
                if (checkAngle(edge, currFace, otherFace)) {// Çäåñü ïûòàåìñÿ âûïîëíèòü ïðîâåðêó íà âûïóêëîñòü äâóõãðàííîãî óãëà
                    chooseMng->Choose(edge);
                    // ...
                }
            }
        }
    }
}

