#include "concaveAngle.hpp"

#import "ksconstants.tlb" no_namespace named_guids
#import "ksConstants3D.tlb" no_namespace named_guids
#import "kAPI5.tlb" no_namespace named_guids rename( "min", "Imin" ) rename( "max", "Imax" ) rename( "ksFragmentLibrary", "ksIFragmentLibrary" )

#include <stdexcept>

const double CHAMFER_WIDTH = 0.01;

bool isConcaveAngle(ksDocument3DPtr document3d, ksEdgeDefinitionPtr edge) {
    ksPartPtr part(document3d->GetPart(pTop_Part));

    ksMassInertiaParamPtr massInertiaParam(part->CalcMassInertiaProperties(0x1 | 0x10)); // mm kg
    double startVolume = massInertiaParam->v;

    ksEntityPtr chamferEntity(part->NewEntity(Obj3dType::o3d_chamfer));
    ksChamferDefinitionPtr chamfer(chamferEntity->GetDefinition());
    chamfer->SetChamferParam(true, CHAMFER_WIDTH, CHAMFER_WIDTH);
    ksEntityCollectionPtr array(chamfer->array());
    array->Add(edge);
    chamferEntity->hidden = true;
    bool isCreated = chamferEntity->Create();

    if (!isCreated) {
        throw std::runtime_error("������ �������� �����");
    }

    massInertiaParam = part->CalcMassInertiaProperties(0x1 | 0x10); // mm kg
    bool isConcaveAngle = massInertiaParam->v > startVolume;

    document3d->DeleteObject(chamferEntity);

    return isConcaveAngle;
}
