#include <iostream>
#include <conio.h>

#include "kompasUtils.hpp"

#import "ksconstants.tlb" no_namespace named_guids
#import "ksConstants3D.tlb" no_namespace named_guids
#import "kAPI5.tlb" no_namespace named_guids rename( "min", "Imin" ) rename( "max", "Imax" ) rename( "ksFragmentLibrary", "ksIFragmentLibrary" )
#import "kAPI7.tlb" no_namespace named_guids rename( "CreateWindow", "ICreateWindow" ) rename( "PostMessage", "IPostMessage" ) rename( "MessageBoxEx", "IMessageBoxEx" )

// Перебираем все ребра
int main1() {
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
    for (int k = 0; k < facesCount; k++) {
        ksFaceDefinitionPtr face = faces->GetByIndex(k);
        ksLoopCollectionPtr loops(face->LoopCollection());
        _getch();
        chooseMng->UnChooseAll(); chooseMng->Choose(face);
        std::cout << "face; skip? [s]\n";
        if (_getch() == 's') {
            continue;
        }

        for (int i = 0; i < loops->GetCount(); i++) {
            ksLoopPtr loop(loops->GetByIndex(i));
            chooseMng->UnChooseAll(); chooseMng->Choose(loop);
            if (loop->IsOuter()) {
                std::cout << "    outer loop " << i << "\n";
            }
            else {
                std::cout << "    NOT outer loop " << i << "\n";
            }

            ksEdgeCollectionPtr edges(loop->EdgeCollection());
            for (int j = 0; j < edges->GetCount(); j++) {

                ksEdgeDefinitionPtr edge(edges->GetByIndex(j));
                _getch();
                chooseMng->UnChooseAll(); chooseMng->Choose(edge);
                std::cout << "        edge " << j << "\n";
                _getch();
                chooseMng->UnChooseAll(); chooseMng->Choose(edge->GetAdjacentFace(true));
                std::cout << "            AdjacentFace(true)" << "\n";
                _getch();
                chooseMng->UnChooseAll(); chooseMng->Choose(edge->GetAdjacentFace(false));
                std::cout << "            AdjacentFace(false)" << "\n";
            }
        }
    }
    return 0;
}

// Перебираем все рабра отверстий
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
    for (int k = 0; k < facesCount; k++) {
        ksFaceDefinitionPtr face = faces->GetByIndex(k);
        if (!face->IsPlanar()) {
            continue;
        }

        ksLoopCollectionPtr loops(face->LoopCollection());
        for (int i = 0; i < loops->GetCount(); i++) {
            ksLoopPtr loop(loops->GetByIndex(i));
            if (loop->IsOuter()) {
                continue;
            }

            ksEdgeCollectionPtr edges(loop->EdgeCollection());
            for (int j = 0; j < edges->GetCount(); j++) {
                ksEdgeDefinitionPtr edge(edges->GetByIndex(j));
                if (face != edge->GetAdjacentFace(true)) {
                    continue;
                }
                chooseMng->Choose(edge);
            }
        }
    }
    return 0;
}
