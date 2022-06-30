#include "optimizeElephantFoot.hpp"
#include "selectPlane.hpp"
#include <iostream>
#include <set>
#import "ksconstants.tlb" no_namespace named_guids
#import "ksConstants3D.tlb" no_namespace named_guids
#import "kAPI5.tlb" no_namespace named_guids rename( "min", "Imin" ) rename( "max", "Imax" ) rename( "ksFragmentLibrary", "ksIFragmentLibrary" )
#import "kAPI7.tlb" no_namespace named_guids rename( "CreateWindow", "ICreateWindow" ) rename( "PostMessage", "IPostMessage" ) rename( "MessageBoxEx", "IMessageBoxEx" )
#define EPS_PLANE_EQ 0.0001

void optimizeElephantFoot(KompasObjectPtr kompas, ksFaceDefinitionPtr face, PlaneEq planeEq, double width) {
    IApplicationPtr api7 = kompas->ksGetApplication7();
    IKompasDocument3DPtr document3d(api7->GetActiveDocument());
    IPart7Ptr topPart(document3d->GetTopPart());
    ksPartPtr part = kompas->TransferInterface(topPart, 1, 0);
    ksFeaturePtr feature(part->GetFeature());

    ksEntityPtr macroElementEntity(part->NewEntity(o3d_MacroObject));
    ksMacro3DDefinitionPtr macroElement(macroElementEntity->GetDefinition());
    macroElementEntity->name = "Оптимизирующие фаски слоновьей ноги";
    macroElement->StaffVisible = true;
    macroElementEntity->Create();

    ksEntityCollectionPtr entityCollection(feature->EntityCollection(o3d_face));                
    std::set<ksEdgeDefinitionPtr> edgesTargets;
    for (int i = 0; i < entityCollection->GetCount(); i++) {
        ksEntityPtr entity(entityCollection->GetByIndex(i));
        ksFaceDefinitionPtr currFace(entity->GetDefinition());
        if (currFace && currFace->IsPlanar()) {
            PlaneEq currPlaneEq(currFace);
            if (currPlaneEq.equals(planeEq)) {
                std::cout << "Найдена поверхность в плоскости печати \n";
                ksEdgeCollectionPtr edges(currFace->EdgeCollection());
                for (int i = 0; i < edges->GetCount(); i++) {
                    edgesTargets.insert(ksEdgeDefinitionPtr(edges->GetByIndex(i)));
                }
            }
        }
    }
    for (std::set<ksEdgeDefinitionPtr>::iterator iter = edgesTargets.begin(); iter != edgesTargets.end(); iter++) {
        ksEntityCollectionPtr allEdgesCollection(feature->EntityCollection(o3d_edge));
        if (allEdgesCollection->FindIt((*iter)->GetEntity()) != -1) {
            ksEntityPtr chamferEntity(part->NewEntity(o3d_chamfer));
            ksChamferDefinitionPtr chamfer(chamferEntity->GetDefinition());
            ksEntityCollectionPtr array(chamfer->array());
            chamfer->SetChamferParam(true, width, width);
            array->Add((*iter));
            chamferEntity->hidden = true;
            bool isCreated = chamferEntity->Create();
            if (isCreated) {
                macroElement->Add(chamferEntity);
            } else {
                array->Clear();
            }
        }
    }
    macroElementEntity->Update();
    document3d->RebuildDocument();
}