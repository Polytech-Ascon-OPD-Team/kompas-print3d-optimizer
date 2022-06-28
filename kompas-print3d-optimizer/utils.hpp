#ifndef UTILS_HPP
#define UTILS_HPP

#include <list>
#include <algorithm>
#include <comutil.h>
#include <OleAuto.h>

#import "ksconstants.tlb" no_namespace named_guids
#import "ksConstants3D.tlb" no_namespace named_guids
#import "kAPI5.tlb" no_namespace named_guids rename( "min", "Imin" ) rename( "max", "Imax" ) rename( "ksFragmentLibrary", "ksIFragmentLibrary" )
#import "kAPI7.tlb" no_namespace named_guids rename( "CreateWindow", "ICreateWindow" ) rename( "PostMessage", "IPostMessage" ) rename( "MessageBoxEx", "IMessageBoxEx" )


bool doubleEqual(double a, double b, double epsilon = 0.00001);

template<typename ElementsType>
std::list<ElementsType> variantToList(const _variant_t& arr) {
    std::list<ElementsType> result;

    if (arr.vt == (VT_ARRAY | VT_DISPATCH)) {
        ElementsType obj = NULL;
        long count = arr.parray->rgsabound[0].cElements;
        for (long i = 0; i < count; i++) {
            SafeArrayGetElement(arr.parray, &i, &obj);
            result.push_back(obj);
        }
    } else if (arr.vt == VT_DISPATCH) {
        result.push_back(arr.pdispVal);
    }
    return result;
}

template<typename OutputType, typename InputType>
std::list<OutputType> castElementTypes(std::list<InputType> inputList) {
    std::list<OutputType> outputList(inputList.size());
    std::transform(inputList.cbegin(), inputList.cend(), outputList.begin(),
        [](const InputType& obj) -> OutputType {
            return OutputType(obj);
        }
    );
    return outputList;
}

struct Sketch {
    ksEntityPtr entity;
    ksSketchDefinitionPtr definition;
    ksDocument2DPtr document2d;
    IKompasDocument2DPtr document2d_api7;
};

Sketch createSketch(KompasObjectPtr kompas, ksPartPtr part, ksFaceDefinitionPtr face);

#endif /* UTILS_HPP */
