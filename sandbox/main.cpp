﻿#include "stdafx.h"

#include <iostream>
#include <cstdlib>
#include <float.h>

#define _USE_MATH_DEFINES
#include <math.h>

#include "kompasUtils.hpp"
#include "selectPlane.hpp"
#include "optimizeRounding.hpp"
#include "optimizeElephantFoot.hpp"
#include "optimizeOverhangingFaces.hpp"
#include "optimizeCircleHorizontalHoles.hpp"
#include "optimizeBridgeHole.hpp"
#include "optimizeRoundingEdgesOnPrintFace.hpp"

ksFaceDefinitionPtr printFace = nullptr;
PlaneEq printPlaneEq;
ksDocument3DPtr oldDocument = nullptr;

bool checkSelectedFace(KompasObjectPtr kompas) {
    ksDocument3DPtr doc3d = kompas->ActiveDocument3D();
    return doc3d == oldDocument && printFace;
}

void performRoundingOptimization(KompasObjectPtr kompas) {
    double radius;
    double angle;
    if (checkSelectedFace(kompas) && kompas->ksReadDouble("Радиус: ", 0.0, -DBL_MIN, DBL_MAX, &radius) == 1 && kompas->ksReadDouble("Граничный угол: ", 60, -DBL_MIN, DBL_MAX, &angle) == 1 ) {
        if (radius > DBL_MIN) {
            optimizeByRounding(kompas, printFace, printPlaneEq, radius, angle);
            kompas->ksMessage("Оптимизация модели была выполнена!");
        } else {
            kompas->ksMessage("Неверный радиус!");
        }
    }
}

void performAntiElephantFootOptimiztion(KompasObjectPtr kompas) {
    double depth;
    if (checkSelectedFace(kompas) && kompas->ksReadDouble("Высота слоя печати: ", 0.0, -DBL_MIN, DBL_MAX, &depth) == 1) {
        optimizeElephantFoot(kompas, printFace, printPlaneEq, depth*2);
        kompas->ksMessage("Оптимизация модели была выполнена!");
    }
}

void performAntiOverhangingOptimization(KompasObjectPtr kompas) {
    double angle; //предельный угол нависания 
    if (checkSelectedFace(kompas) && kompas->ksReadDouble("Макс. угол нависания: ", 60, -DBL_MIN, DBL_MAX, &angle) == 1) {
        optimizeOverhangingFace(kompas, printFace, printPlaneEq, angle);
    }
}



void performHorizontalHolesOptimization(KompasObjectPtr kompas) {
    double maxAngle; //угол нависания
    if (checkSelectedFace(kompas) && kompas->ksReadDouble("Макс. угол нависания: ", 60, -DBL_MIN, DBL_MAX, &maxAngle) == 1) {
        optimizeCircleHorizontalHoles(kompas, maxAngle, printFace, printPlaneEq);
    }
}

void performBridgeHolesBuildOptimization(KompasObjectPtr kompas) {
    double depth;
    if (kompas->ksReadDouble("Высота слоя печати: ", 0.2, DBL_MIN, DBL_MAX, &depth) == 1) {
        optimizeBridgeHoleBuild(kompas, kompas->ActiveDocument3D(), oldDocument->GetPart(pTop_Part), printFace, depth);
        oldDocument->RebuildDocument();
        kompas->ksMessage("Оптимизация модели была выполнена!");
    }
}

void performBridgeHolesFillOptimization(KompasObjectPtr kompas) {
    double depth;
    if (kompas->ksReadDouble("Высота слоя печати: ", 0.2, DBL_MIN, DBL_MAX, &depth) == 1) {
        long choise = 0;
        kompas->ksReadInt("1-круглые;2-некруглые;3-все", 0, 1, 3, &choise);
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
        optimizeBridgeHoleFill(oldDocument, oldDocument->GetPart(pTop_Part), printFace, depth, choisedType);
        oldDocument->RebuildDocument();
        kompas->ksMessage("Оптимизация модели была выполнена!");
    }
}

int main() {
    CoInitialize(nullptr);
    KompasObjectPtr kompas = kompasInit();
    if (!kompas) {
        return 0;
    }
    while (true) {
        std::setlocale(LC_ALL, "Russian");
        std::cout << "Выберите плоскость печати!\n";
        system("pause");

        printFace = getSelectedPlane(kompas, &printPlaneEq);
        oldDocument = kompas->ActiveDocument3D();
        if (!checkSelectedFace(kompas)) {
            std::cout << "Плоскость печати не выбрана!\n";
            continue;
        }
        while (true) {
            if (kompas->ActiveDocument3D() != oldDocument) {
                std::cout << "Старый документ был закрыт, нужно заново выбрать плоскость печати.\n";
                break;
            }
            short choise;
            std::cout << "1 - Исправление выпирающих углов" << "\n";
            std::cout << "2 - Исправление слоновьей ноги" << "\n";
            std::cout << "3 - Исправление выступающих нависающих частей (в разработке, пока не работает)" << "\n";
            std::cout << "4 - Оптимизация горизонтальных круглых отверстий (в разработке, пока не работает)" << "\n";
            std::cout << "5 - Закрытие нависающих отверстий диафрагмой" << "\n";
            std::cout << "6 - Достройка нависающих круглых отверстий до набора мостов" << "\n";

            std::cout << "Ваш выбор:";
            std::cin >> choise;
            if (kompas) {
                switch (choise) {
                case 1: {
                    performRoundingOptimization(kompas);
                    break;
                }
                case 2: {
                    performAntiElephantFootOptimiztion(kompas);
                    break;
                }
                case 3: {
                    performAntiOverhangingOptimization(kompas);
                    break;
                }
                case 4: {
                    performHorizontalHolesOptimization(kompas);
                    break;
                }
                case 5: {
                    performBridgeHolesFillOptimization(kompas);
                    break;
                }
                case 6: {
                    performBridgeHolesBuildOptimization(kompas);
                    break;
                }
                default:
                    std::cout << "Ошибка\n";
                    return 0;
                    break;
                }
            }
        }
    }
}
