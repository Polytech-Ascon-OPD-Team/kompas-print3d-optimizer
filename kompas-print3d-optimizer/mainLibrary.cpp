#include "StdAfx.h"
#include "resources/resource.h"
#include "kompasUtils.hpp"
#include "selectPlane.hpp"
#include "optimizeRounding.hpp"
#include "optimizeElephantFoot.hpp"
#include "optimizeBridgeHole.hpp"

#import "ksconstants.tlb" no_namespace named_guids
#import "ksConstants3D.tlb" no_namespace named_guids
#import <kAPI5.tlb> no_namespace named_guids rename( "min", "Imin" ) rename( "max", "Imax" ) rename( "ksFragmentLibrary", "ksIFragmentLibrary" )
#import <kAPI7.tlb> no_namespace named_guids rename( "CreateWindow", "ICreateWindow" ) rename( "PostMessage", "IPostMessage" ) rename( "MessageBoxEx", "IMessageBoxEx" )

ksFaceDefinitionPtr printFace = nullptr;
PlaneEq printPlaneEq;
ksDocument3DPtr oldDocument = nullptr;

bool checkSelectedFace(KompasObjectPtr kompas) {
	ksDocument3DPtr doc3d = kompas->ActiveDocument3D();
	return doc3d == oldDocument && printFace;
}


void performRoundingOptimization(KompasObjectPtr kompas) {

	if (!checkSelectedFace(kompas)) {
		kompas->ksError("��������� ������ �� �������!");
		return;
	}

	double radius;
	double angle;
	if (kompas->ksReadDouble("������: ", 0.0, -DBL_MIN, DBL_MAX, &radius) == 1 && kompas->ksReadDouble("��������� ����: ", 60, -DBL_MIN, DBL_MAX, &angle) == 1) {
		optimizeByRounding(kompas, printFace, printPlaneEq, radius, angle);
		kompas->ksMessage("����������� ������ ���� ���������!");
	}

}

void performAntiElephantFootOptimiztion(KompasObjectPtr kompas) {
	if (!checkSelectedFace(kompas)) {
		kompas->ksError("��������� ������ �� �������!");
		return;
	}
	double width;
	if (kompas->ksReadDouble("������ ���� ������: ", 0.0, -DBL_MIN, DBL_MAX, &width) == 1) {
		optimizeElephantFoot(kompas, printFace, printPlaneEq, 2*width);
		kompas->ksMessage("����������� ������ ���� ���������!");
	} else {
		kompas->ksMessage("��������� ������ �� �������!");
	}
}

void performBridgeHolesBuildOptimization(KompasObjectPtr kompas) {
	if (!checkSelectedFace(kompas)) {
		kompas->ksError("��������� ������ �� �������!");
		return;
	}
	double depth;
	if (kompas->ksReadDouble("������ ���� ������: ", 0.2, DBL_MIN, DBL_MAX, &depth) == 1) {
		optimizeBridgeHoleBuild(kompas, oldDocument->GetPart(pTop_Part), printFace, depth);
		oldDocument->RebuildDocument();
		kompas->ksMessage("����������� ������ ���� ���������!");
	}
}

void performBridgeHolesFillOptimization(KompasObjectPtr kompas) {
	if (!checkSelectedFace(kompas)) {
		kompas->ksError("��������� ������ �� �������!");
		return;
	}
	double depth;
	if (kompas->ksReadDouble("������ ���� ������: ", 0.2, DBL_MIN, DBL_MAX, &depth) == 1) {
		long choise = 0;
		kompas->ksReadInt("1-�������;2-���������;3-���", 1, 1, 3, &choise);
		HoleType choisedType;
		switch (choise)
		{
		case 1:
			choisedType = HoleType::CIRCLE;
			break;

		case 2:
			choisedType = HoleType::NOT_CIRCLE;
			break;

		case 3:
			choisedType = HoleType::ALL;
			break;
		default:
			return;
			break;
		}
		optimizeBridgeHoleFill(oldDocument->GetPart(pTop_Part), printFace, depth, choisedType);
		oldDocument->RebuildDocument();
		kompas->ksMessage("����������� ������ ���� ���������!");
	}
}


unsigned int WINAPI LIBRARYID() {
	return IDR_LIBID;
}

void WINAPI LIBRARYENTRY(unsigned int comm) {
	KompasObjectPtr kompas = getKompasObjectPtr();
	if (kompas) {
		switch (comm) {
		case 1: {
			printFace = getSelectedPlane(kompas, &printPlaneEq);
			oldDocument = kompas->ActiveDocument3D();
			break;
		}
		case 2: {
			performRoundingOptimization(kompas);
			break;
		}
		case 3: {
			performAntiElephantFootOptimiztion(kompas);
			break;
		}
		case 4: {
			performBridgeHolesFillOptimization(kompas);
			break;
		}
		case 5: {
			performBridgeHolesBuildOptimization(kompas);
			break;
		}
		default:
			break;
		}
	}
}