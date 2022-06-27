#include <iostream>
#include <conio.h>

#include "kompasUtils.hpp"
#include "utils.hpp"
#include "opt-bridgeHole.hpp"

#import "ksconstants.tlb" no_namespace named_guids
#import "ksConstants3D.tlb" no_namespace named_guids
#import "kAPI5.tlb" no_namespace named_guids rename( "min", "Imin" ) rename( "max", "Imax" ) rename( "ksFragmentLibrary", "ksIFragmentLibrary" )
#import "kAPI7.tlb" no_namespace named_guids rename( "CreateWindow", "ICreateWindow" ) rename( "PostMessage", "IPostMessage" ) rename( "MessageBoxEx", "IMessageBoxEx" )

int main() {
    CoInitialize(nullptr);
    KompasObjectPtr kompas = kompasInit();

    ksDocument3DPtr document3d = kompas->ActiveDocument3D();
    ksChooseMngPtr chooseMng(document3d->GetChooseMng());

    ksPartPtr part(document3d->GetPart(pTop_Part));

    ksBodyPtr body = part->GetMainBody();
    ksFaceCollectionPtr faces = body->FaceCollection();
    int facesCount = faces->GetCount();

    ksFaceDefinitionPtr printFace;

    // Задаем грань стола 3д принтера. Только для тестирования
    for (int printFaceIndex = 0; printFaceIndex < facesCount; printFaceIndex++) {
        printFace = faces->GetByIndex(printFaceIndex);
        if (abs(printFace->GetArea(ksLUnMM) - 1146) < 1) {
            break;
        }
    }

    chooseMng->Choose(printFace);
    std::cout << "print face\n";
    _getch();
    chooseMng->UnChooseAll();
    
    //bridgeHoleFillOptimization(part, printFace, 0.2, HoleType::ALL);
    bridgeHoleBuildOptimization(kompas, part, printFace, 0.2);

    //document3d->RebuildDocument(); // Нужно чтобы исправить странные ошибки "Вырожденная проекция ребра"

    return 0;
}
