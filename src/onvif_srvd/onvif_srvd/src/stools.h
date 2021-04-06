#ifndef STOOLS_H
#define STOOLS_H

#include <memory>
#include <limits>

#include "soapH.h"





template<typename T>
T* soap_new_ptr(struct soap* soap, T value)
{
    T* ptr = (T*)soap_malloc(soap, sizeof(T));
    *ptr = value;

    return ptr;
}





#endif // STOOLS_H
