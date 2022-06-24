#ifndef UTILS_HPP
#define UTILS_HPP

#include <list>
#include <algorithm>
#include <comutil.h>
#include <OleAuto.h>

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

#endif /* UTILS_HPP */
