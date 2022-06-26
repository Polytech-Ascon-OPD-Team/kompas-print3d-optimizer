#include "kompasUtils.hpp"

#include <Windows.h>
#include <iostream>

#import "ksconstants.tlb" no_namespace named_guids
#import "ksConstants3D.tlb" no_namespace named_guids
#import "kAPI5.tlb" no_namespace named_guids rename( "min", "Imin" ) rename( "max", "Imax" ) rename( "ksFragmentLibrary", "ksIFragmentLibrary" )
#import "kAPI7.tlb" no_namespace named_guids rename( "CreateWindow", "ICreateWindow" ) rename( "PostMessage", "IPostMessage" ) rename( "MessageBoxEx", "IMessageBoxEx" )

const wchar_t objectName[] = L"KOMPAS.Application.5";

bool isKompasInstalled()
{
    CLSID clsid;
    HRESULT res;
    res = CLSIDFromProgID(objectName, &clsid);
    return (res == S_OK);
}

bool isKompasRun()
{
    CLSID clsid;
    CLSIDFromProgID(objectName, &clsid);
    HRESULT res;
    IUnknown* pIUnknown;
    res = GetActiveObject(clsid, NULL, &pIUnknown);
    if (res == S_OK) {
        pIUnknown->Release();
        return true;
    }
    return false;
}

KompasObjectPtr kompasInit() {
    if (!isKompasInstalled()) {
        std::cerr << "Компас не установлен" << "\n";
        return nullptr;
    }
    KompasObjectPtr kompas;
    if (isKompasRun()) {
        kompas.GetActiveObject(objectName);
    } else {
        kompas.CreateInstance(objectName);
    }
    kompas->Visible = true;
    return kompas;
}

Sketch createSketch(KompasObjectPtr kompas, ksPartPtr part, ksFaceDefinitionPtr face) {
    ksEntityPtr sketchEntity(part->NewEntity(o3d_sketch));
    ksSketchDefinitionPtr sketchDef(sketchEntity->GetDefinition());
    sketchDef->SetPlane(face);
    sketchEntity->Create();
    ksDocument2DPtr sketchEdit(sketchDef->BeginEdit());
    IKompasDocument2DPtr sketchEdit_api7(kompas->TransferInterface(sketchEdit, ksAPI7Dual, 0));

    return Sketch{ sketchEntity, sketchDef, sketchEdit, sketchEdit_api7 };
}
