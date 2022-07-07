#include "StdAfx.h"

#import "ksconstants.tlb" no_namespace named_guids
#import "ksConstants3D.tlb" no_namespace named_guids
#import "kAPI5.tlb" no_namespace named_guids rename( "min", "Imin" ) rename( "max", "Imax" ) rename( "ksFragmentLibrary", "ksIFragmentLibrary" )
#import "kAPI7.tlb" no_namespace named_guids rename( "CreateWindow", "ICreateWindow" ) rename( "PostMessage", "IPostMessage" ) rename( "MessageBoxEx", "IMessageBoxEx" )

#include <sstream>

#include "resources/resource.h"

#include "kompasUtils.hpp"
#include "selectPlane.hpp"
#include "optimizeRounding.hpp"
#include "optimizeElephantFoot.hpp"
#include "optimizeBridgeHole.hpp"
#include "optimizeRoundingEdgesOnPrintFace.hpp"

struct PrintSettings {
    double nozzleDiameter;
    double layerHeight;
    double overhangThreshold;
};


ksFaceDefinitionPtr printFace = nullptr;
PlaneEq printPlaneEq;
ksDocument3DPtr oldDocument = nullptr;

PrintSettings printSettings{ 0.4, 0.2, 40.0 };


bool checkSelectedFace(KompasObjectPtr kompas) {
    ksDocument3DPtr doc3d = kompas->ActiveDocument3D();
    return doc3d == oldDocument && printFace;
}

PrintSettings inputPrintSettings(KompasObjectPtr kompas) {
    PrintSettings ps;
    if (kompas->ksReadDouble("������� �����:", 0.4, 0.05, 2.0, &ps.nozzleDiameter) != 1) {
        return printSettings;
    }
    if (kompas->ksReadDouble("������ ����:", 0.2, 0.01, 0.8, &ps.layerHeight) != 1) {
        return printSettings;
    }
    if (kompas->ksReadDouble("������������ ���� ���������:", 40.0, 0.0, 90.0, &ps.overhangThreshold) != 1) {
        return printSettings;
    }
    return ps;
}


unsigned int WINAPI LIBRARYID() {
    return IDR_LIBID;
}

void WINAPI LIBRARYENTRY(unsigned int comm) {
    KompasObjectPtr kompas = getKompasObjectPtr();
    if (!kompas) {
        return;
    }

    if ((comm > 2) && !checkSelectedFace(kompas)) {
        kompas->ksError("��������� ������ �� �������!");
        return;
    }
    switch (comm) {
        case 1: {
              printSettings = inputPrintSettings(kompas);
              return;
        }
        case 2: {
            printFace = getSelectedPlane(kompas, &printPlaneEq);
            oldDocument = kompas->ActiveDocument3D();
            return;
        }
        case 3:
        {
            std::ostringstream oss;
            oss << "������� �����: " << printSettings.nozzleDiameter << "\n"
                << "������ ����: " << printSettings.layerHeight << "\n"
                << "������������ ���� ���������: " << printSettings.overhangThreshold << "\n"
                << "��������� ������ ��������";
            ksChooseMngPtr chooseMng(oldDocument->GetChooseMng());
            chooseMng->UnChooseAll();
            chooseMng->Choose(printFace);
            kompas->ksMessage(oss.str().c_str());
            return;
        }
        case 4: {
              double radius = 0.0;
              if (kompas->ksReadDouble("������:", 0.0, 0.0, DBL_MAX, &radius) != 1) {
                  return;
              }
              optimizeByRounding(kompas, printFace, printPlaneEq, radius, printSettings.overhangThreshold);
              break;
        }
        case 5: {
            optimizeElephantFoot(kompas, printFace, printPlaneEq, 2 * printSettings.layerHeight);
            break;
        }
        case 6: {
            optimizeRoundingEdgesOnPrintFace(kompas, oldDocument->GetPart(pTop_Part), printFace,
                printSettings.overhangThreshold, ReworkType::ALL);
            break;
        }
        case 7: {
            optimizeRoundingEdgesOnPrintFace(kompas, oldDocument->GetPart(pTop_Part), printFace,
                printSettings.overhangThreshold, ReworkType::ONLY_WITHOUT_REWORK);
            break;
        }
        case 8: {
            optimizeBridgeHoleFill(oldDocument->GetPart(pTop_Part), printFace, printSettings.layerHeight, HoleType::NOT_CIRCLE);
            optimizeBridgeHoleBuild(kompas, oldDocument->GetPart(pTop_Part), printFace, printSettings.layerHeight);
            break;
        }
        case 9: {
            optimizeBridgeHoleFill(oldDocument->GetPart(pTop_Part), printFace, printSettings.layerHeight, HoleType::ALL);
            break;
        }
        case 10: {
            optimizeBridgeHoleBuild(kompas, oldDocument->GetPart(pTop_Part), printFace, printSettings.layerHeight);
            break;
        }
        case 11: {
            break;
        }
        case 12: {
            break;
        }
    }
    oldDocument->RebuildDocument();
    kompas->ksMessage("����������� ������ ���� ���������!");
}
