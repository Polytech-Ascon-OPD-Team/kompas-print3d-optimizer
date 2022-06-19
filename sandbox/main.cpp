#include <iostream>

#include "kompasUtils.hpp"

#import "ksconstants.tlb" no_namespace named_guids
#import "ksConstants3D.tlb" no_namespace named_guids
#import "kAPI5.tlb" no_namespace named_guids rename( "min", "Imin" ) rename( "max", "Imax" ) rename( "ksFragmentLibrary", "ksIFragmentLibrary" )
#import "kAPI7.tlb" no_namespace named_guids rename( "CreateWindow", "ICreateWindow" ) rename( "PostMessage", "IPostMessage" ) rename( "MessageBoxEx", "IMessageBoxEx" )


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
    ksFaceCollectionPtr faces = body->FaceCollection();
    int facesCount = faces->GetCount();
    for (int i = 0; i < facesCount; i++) {
        ksFaceDefinitionPtr face = faces->GetByIndex(i);
        ksLoopCollectionPtr loops(face->LoopCollection());
        chooseMng->Choose(face);
        kompas->ksMessage("face");
        chooseMng->UnChooseAll();


        for (int i = 0; i < loops->GetCount(); i++) {
            ksLoopPtr loop(loops->GetByIndex(i));

            chooseMng->Choose(loop);
            if (loop->IsOuter()) {
                kompas->ksMessage("outer loop");
            }
            else {
                kompas->ksMessage("NOT outer loop");
            }

            chooseMng->UnChooseAll();


            ksEdgeCollectionPtr edges(loop->EdgeCollection());
            for (int i = 0; i < edges->GetCount(); i++) {
                ksEdgeDefinitionPtr edge(edges->GetByIndex(i));
                chooseMng->Choose(edge);
                kompas->ksMessage("edge");
                chooseMng->UnChooseAll();
            }
        }
    }
}