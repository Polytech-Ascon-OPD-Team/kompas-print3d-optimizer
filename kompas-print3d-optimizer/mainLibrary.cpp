#include "StdAfx.h"
#include "resources/resource.h"
#include "kompasUtils.hpp"
#include "selectPlane.hpp"
#include "optimizeRounding.hpp"
#include "optimizeElephantFoot.hpp"

#import "ksconstants.tlb" no_namespace named_guids
#import "ksConstants3D.tlb" no_namespace named_guids
#import <kAPI5.tlb> no_namespace named_guids rename( "min", "Imin" ) rename( "max", "Imax" ) rename( "ksFragmentLibrary", "ksIFragmentLibrary" )
#import <kAPI7.tlb> no_namespace named_guids rename( "CreateWindow", "ICreateWindow" ) rename( "PostMessage", "IPostMessage" ) rename( "MessageBoxEx", "IMessageBoxEx" )

void performRoundingOptimization(KompasObjectPtr kompas) {
    PlaneEq planeEq;
    ksFaceDefinitionPtr face = getSelectedPlane(kompas, &planeEq);
    if (face) {
        double radius;
        double angle;
        if (kompas->ksReadDouble("Радиус: ", 0.0, -DBL_MIN, DBL_MAX, &radius) == 1 && kompas->ksReadDouble("Граничный угол: ", 60, -DBL_MIN, DBL_MAX, &angle) == 1) {
            optimizeByRounding(kompas, face, planeEq, radius, angle);
            kompas->ksMessage("Оптимизация модели была выполнена!");
        }
    }
}

void performAntiElephantFootOptimiztion(KompasObjectPtr kompas) {
    PlaneEq planeEq;
    ksFaceDefinitionPtr face = getSelectedPlane(kompas, &planeEq);
    if (face) {
        double width;
        if (kompas->ksReadDouble("Размер оптимизирующей фаски: ", 0.0, -DBL_MIN, DBL_MAX, &width) == 1) {
            optimizeElephantFoot(kompas, face, planeEq, width);
            kompas->ksMessage("Оптимизация модели была выполнена!");
        }
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
            performRoundingOptimization(kompas);
        }
        case 3: {
            performAntiElephantFootOptimiztion(kompas);
        }
        default:
            break;
        }
    }
}