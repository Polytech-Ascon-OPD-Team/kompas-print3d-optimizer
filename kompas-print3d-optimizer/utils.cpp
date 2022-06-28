#include "utils.hpp"

#include <cmath>

#import "ksconstants.tlb" no_namespace named_guids
#import "ksConstants3D.tlb" no_namespace named_guids
#import "kAPI5.tlb" no_namespace named_guids rename( "min", "Imin" ) rename( "max", "Imax" ) rename( "ksFragmentLibrary", "ksIFragmentLibrary" )
#import "kAPI7.tlb" no_namespace named_guids rename( "CreateWindow", "ICreateWindow" ) rename( "PostMessage", "IPostMessage" ) rename( "MessageBoxEx", "IMessageBoxEx" )

bool doubleEqual(double a, double b, double epsilon) {
    return (abs(a - b) < epsilon);
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
