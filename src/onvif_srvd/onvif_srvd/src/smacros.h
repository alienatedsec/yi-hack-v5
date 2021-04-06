/*
 * smacros.h
 *
 *
 * version 1.1
 *
 *
 * BSD 3-Clause License
 *
 * Copyright (c) 2015, Koynov Stas - skojnov@yandex.ru
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * more details see https://github.com/KoynovStas/smacros
 */

#ifndef SMACROS_HEADER
#define SMACROS_HEADER


#include <stdint.h>
#include <stddef.h>  //for offsetof





#ifdef  __cplusplus
        #define  EXTERN_PREFIX extern "C"
#else
        #define  EXTERN_PREFIX extern
#endif





#define  SET_BIT(reg, num_bit)   ( (reg) |=  (1 << (num_bit)) )
#define  CLR_BIT(reg, num_bit)   ( (reg) &= ~(1 << (num_bit)) )
#define  INV_BIT(reg, num_bit)   ( (reg) ^=  (1 << (num_bit)) )


#define  SET_FLAG(reg, flag)   ( (reg) |=  (flag) )
#define  CLR_FLAG(reg, flag)   ( (reg) &= ~(flag) )
#define  INV_FLAG(reg, flag)   ( (reg) ^=  (flag) )





//The count of elements in the array.
#define  COUNT_ELEMENTS(array) (sizeof(array) / sizeof(array[0]))




/*
 * For GCC 4.6 or higher, in C++ you can use a standard right static_assert(exp, msg) in *.c and in *.h files.
 * For GCC 4.6 is required to add CFLAGS += -std="c++0x"
 * Simple C (gcc) have static_assert in std=c11.
 * A lot of variants, it is the most simple and intuitive
 * It can be used in *.c and in *.h files. (macros that use function style can be used in *.c files only)
 *
 * Disadvantages: you can not be set msg to display the console when compiling
 *
 * Example:
 *
 * STATIC_ASSERT( sizeof(char) == 1)  //good job
 * STATIC_ASSERT( sizeof(char) != 1)  //You will get a compilation error
*/
#define ASSERT_CONCAT_(a, b) a##b
#define ASSERT_CONCAT(a, b) ASSERT_CONCAT_(a, b)
#define STATIC_ASSERT(expr) \
    enum { ASSERT_CONCAT(assert_line_, __LINE__) = 1/(int)(!!(expr)) }





#define DEF_TO_STR_(text) #text
#define DEF_TO_STR(arg) DEF_TO_STR_(arg)





// align must be a power of two.
// Alignment takes place upwards.
#define  ALIGNMENT(val, align)  ( ((val) + (align)) & ~( (align) - 1) )





// Macros to convert the time in microseconds
// t  have to have  struct timespec type
// TIME_IN_USEC2 is fast variant (have not mul)
#define  TIME_IN_USEC(t)     (uint32_t)(t.tv_usec + t.tv_sec * 1000000)
#define  TIME_IN_USEC2(t)    (uint32_t)(t.tv_usec + (t.tv_sec << 20) )





#define  FREE_AND_NULL(ptr_var)  ({                  \
                                     free(ptr_var);  \
                                     ptr_var = NULL; \
                                  })



#define  DELETE_AND_NULL(ptr_var) ({                    \
                                      delete (ptr_var); \
                                      ptr_var = NULL;   \
                                   })



#define  DELETE_ARRAY_AND_NULL(ptr_var) ({                       \
                                            delete [] (ptr_var); \
                                            ptr_var = NULL;      \
                                        })





#ifdef  DEBUG
        #include <stdio.h>

        #define  DEBUG_MSG(...)       printf(__VA_ARGS__)
        #define  FDEBUG_MSG(fp, ...) fprintf(fp, __VA_ARGS__)
#else
        #define  DEBUG_MSG(...)       ({})
        #define  FDEBUG_MSG(fp, ...)  ({})
#endif



#define UNUSED(var) (void)var



// container_of - cast a member of a structure out to the containing structure
// ptr:    the pointer to the member.
// type:   the type of the container struct this is embedded in.
// member: the name of the member within the struct.
//
#define container_of( ptr, type, member ) \
    ({  (type *)( (const char *)ptr - offsetof(type, member) );  })




/*
 * gSOAP generate a lot of handlers(callbacks) for ONVIF
 * Most of them are empty. Almost everyone takes two arguments,
 * the second is named the first_name + Response.
 * This macro allows you to write the basic behavior in one line:
 * Disable the compiler warning about unused arguments and
 * display a message for the Debug mode.
 */
#define  SOAP_EMPTY_HANDLER(arg1, str_prefix)      \
    UNUSED(arg1);                                  \
    UNUSED(arg1##Response);                        \
    DEBUG_MSG(str_prefix ": %s\n", __FUNCTION__);  \
    return SOAP_OK                                 \





#endif //SMACROS_HEADER
