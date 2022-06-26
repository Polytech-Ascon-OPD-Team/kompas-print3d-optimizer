#include <iostream>
#include <float.h>
#include "kompasUtils.hpp"
#include "selectPlane.hpp"
#include "optimizeRounding.hpp"
#include "optimizeElephantFoot.hpp"
#include "optimizeOverhangingFaces.hpp"
#define _USE_MATH_DEFINES
#include <math.h>
#import "ksconstants.tlb" no_namespace named_guids
#import "ksConstants3D.tlb" no_namespace named_guids
#import "kAPI5.tlb" no_namespace named_guids rename( "min", "Imin" ) rename( "max", "Imax" ) rename( "ksFragmentLibrary", "ksIFragmentLibrary" )
#import "kAPI7.tlb" no_namespace named_guids rename( "CreateWindow", "ICreateWindow" ) rename( "PostMessage", "IPostMessage" ) rename( "MessageBoxEx", "IMessageBoxEx" )

void performRoundingOptimization(KompasObjectPtr kompas) {
    PlaneEq planeEq;
    ksFaceDefinitionPtr face = getSelectedPlane(kompas, &planeEq);
    double radius;
    double angle;
    if (face && kompas->ksReadDouble("Радиус: ", 0.0, -DBL_MIN, DBL_MAX, &radius) == 1 && kompas->ksReadDouble("Граничный угол: ", 60, -DBL_MIN, DBL_MAX, &angle) == 1) {
        optimizeByRounding(kompas, face, planeEq, radius, angle);
        kompas->ksMessage("Оптимизация модели была выполнена!");
    }
}

void performAntiElephantFootOptimiztion(KompasObjectPtr kompas) {
    PlaneEq planeEq;
    ksFaceDefinitionPtr face = getSelectedPlane(kompas, &planeEq);
    double width;
    if (face && kompas->ksReadDouble("Размер оптимизирующей фаски: ", 0.0, -DBL_MIN, DBL_MAX, &width) == 1) {
        optimizeElephantFoot(kompas, face, planeEq, width);
        kompas->ksMessage("Оптимизация модели была выполнена!");
    }
}

void performAntiOverhangingOptimization(KompasObjectPtr kompas) {
    PlaneEq planeEq;
    ksFaceDefinitionPtr face = getSelectedPlane(kompas, &planeEq);
    double angle; //предельный угол нависания 
    if (face && kompas->ksReadDouble("Макс. угол нависания: ", 60, -DBL_MIN, DBL_MAX, &angle) == 1) {
        optimizeElephantFoot(kompas, face, planeEq, angle);
    }
}



int main() {
    short choise;
    std::setlocale(LC_ALL, "Russian");
    std::cout << "1 - Исправление выпирающих углов" << "\n";
    std::cout << "2 - Оптимизация вертикальных сквозных отверстий" << "\n";
    std::cout << "3 - Исправление слоновьей ноги" << "\n";
    std::cout << "4 - Исправление выступающих нависающих частей" << "\n";
    std::cout << "Ваш выбор:";
    std::cin >> choise;
    CoInitialize(nullptr);
    KompasObjectPtr kompas = kompasInit();
    if (kompas) {
        switch (choise) {
        case 1: {
            performRoundingOptimization(kompas);
            break;
        }
        case 3: {
            performAntiElephantFootOptimiztion(kompas);
            break;
        }
        case 4: {
            performAntiOverhangingOptimization(kompas);
            break;

        }
        default:
            break;
        }
    }
}
