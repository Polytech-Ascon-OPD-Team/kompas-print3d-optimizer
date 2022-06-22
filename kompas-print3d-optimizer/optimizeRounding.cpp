#include "StdAfx.h"
#include "optimizeRounding.hpp"
#include "selectPlane.hpp"

#include <iostream>
#define _USE_MATH_DEFINES
#include <math.h>

#import "ksconstants.tlb" no_namespace named_guids
#import "ksConstants3D.tlb" no_namespace named_guids
#import "kAPI5.tlb" no_namespace named_guids rename( "min", "Imin" ) rename( "max", "Imax" ) rename( "ksFragmentLibrary", "ksIFragmentLibrary" )
#import "kAPI7.tlb" no_namespace named_guids rename( "CreateWindow", "ICreateWindow" ) rename( "PostMessage", "IPostMessage" ) rename( "MessageBoxEx", "IMessageBoxEx" )

bool checkEdge(ksEdgeDefinitionPtr edge, double minWidth) {
    /*try {*/
    if (edge) {
        ksFaceDefinitionPtr face1(edge->GetAdjacentFace(true));
        ksFaceDefinitionPtr face2(edge->GetAdjacentFace(false));
        if (face1 && face2) {
            double area1 = face1->GetArea(0x1);
            double area2 = face2->GetArea(0x1);
            double lenOfEdge = edge->GetLength(0x1);
            return edge->IsStraight() && face1->IsPlanar() && face2->IsPlanar() && (area1 / lenOfEdge >= minWidth) && (area2 / lenOfEdge >= minWidth);
        }
    }
    return false;
    /* }
    catch (_com_error& e)
    {
        return false;
    }*/
}

void optimizeByRounding(KompasObjectPtr kompas, ksFaceDefinitionPtr face, PlaneEq planeEq, double radius, double angle) {
    double cos_angle = sin(angle * M_PI / 180.0); //да, тут смежные углы
    IApplicationPtr api7 = kompas->ksGetApplication7();
    IKompasDocument3DPtr document3d(api7->GetActiveDocument());
    IPart7Ptr topPart(document3d->GetTopPart());
    ksPartPtr part = kompas->TransferInterface(topPart, 1, 0);
    ksFeaturePtr feature(part->GetFeature());
    ksEntityCollectionPtr entityCollection(feature->EntityCollection(o3d_edge));
    entityCollection->GetCount();
    for (int i = 0; i < entityCollection->GetCount(); i++) {
        ksEntityPtr entity(entityCollection->GetByIndex(i));
        ksEdgeDefinitionPtr edge(entity->GetDefinition());
        if (edge && checkEdge(edge, radius)) {
            bool isVerical = planeEq.isVertical(edge, cos_angle);
            if (isVerical) {
                ksEntityPtr filletEntity(part->NewEntity(o3d_fillet));
                ksFilletDefinitionPtr fillet(filletEntity->GetDefinition());
                ksEntityCollectionPtr array(fillet->array());
                fillet->radius = radius;
                array->Add(entity);
                filletEntity->Create();
                std::cout << "найдено вертикальное ребро" << "\n";
            } else {
                std::cout << "найдено невертикальное ребро" << "\n";
            }
        }
    }
}