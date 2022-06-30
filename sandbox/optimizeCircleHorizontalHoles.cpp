#include "optimizeCircleHorizontalHoles.hpp"
#include "selectPlane.hpp"
#include <iostream>
#include <set>
#define _USE_MATH_DEFINES
#include <math.h>
#include "utils.hpp"
#import "ksconstants.tlb" no_namespace named_guids
#import "ksConstants3D.tlb" no_namespace named_guids
#import "kAPI5.tlb" no_namespace named_guids rename( "min", "Imin" ) rename( "max", "Imax" ) rename( "ksFragmentLibrary", "ksIFragmentLibrary" )
#import "kAPI7.tlb" no_namespace named_guids rename( "CreateWindow", "ICreateWindow" ) rename( "PostMessage", "IPostMessage" ) rename( "MessageBoxEx", "IMessageBoxEx" )



std::set<ksFaceDefinitionPtr> getHorizontalCircleHoles(ksPartPtr part, ksFaceDefinitionPtr printFace, PlaneEq planeEq) {
    ksMeasurerPtr measurer(part->GetMeasurer()); 

    ksBodyPtr body = part->GetMainBody();
    ksFaceCollectionPtr faces = body->FaceCollection();
    int facesCount = faces->GetCount();

    std::set<ksFaceDefinitionPtr> holes;

    for (int faceIndex = 0; faceIndex < facesCount; faceIndex++) {
        ksFaceDefinitionPtr face = faces->GetByIndex(faceIndex);
        if (face->IsPlanar()) {
            ksLoopCollectionPtr loops(face->LoopCollection());
            for (int loopIndex = 0; loopIndex < loops->GetCount(); loopIndex++) {
                ksLoopPtr innerLoop(loops->GetByIndex(loopIndex));
                if (!(innerLoop->IsOuter())) {
                    ksEdgeCollectionPtr edges(innerLoop->EdgeCollection());
                    if (edges->GetCount() == 1) {
                        ksEdgeDefinitionPtr edge(edges->GetByIndex(0));
                        if ((edge->IsCircle() || edge->IsEllipse())) {
                            ksFaceDefinitionPtr otherFace = nullptr;
                            if (edge->GetAdjacentFace(true) != face) {
                                otherFace = edge->GetAdjacentFace(true);
                            } else {
                                otherFace = edge->GetAdjacentFace(false);
                            }
                            ksEdgeCollectionPtr otherFaceEdges(otherFace->EdgeCollection());
                            if (otherFaceEdges->GetCount() == 2) {

                                ksEdgeDefinitionPtr otherEdge = otherFaceEdges->GetByIndex(0);
                                if (otherEdge == edge) {
                                    otherEdge = otherFaceEdges->GetByIndex(1);
                                }
                                if ((otherEdge->IsCircle() || otherEdge->IsEllipse())) {
                                    holes.insert(otherFace);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return holes;
}