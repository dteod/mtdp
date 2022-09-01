/* Copyright (C) 2021-2022 Domenico Teodonio

mtdp is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

mtdp is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef MTDP_MEMORY_H
#define MTDP_MEMORY_H

#if !defined(MTDP_MEMORY_ACCESS)
#   define MTDP_MEMORY_ACCESS
#endif

#define MTDP_DECLARE_INSTANCE(Type)              \
MTDP_MEMORY_ACCESS Type* Type##_alloc();         \
MTDP_MEMORY_ACCESS void  Type##_dealloc(Type*);

#if MTDP_STATIC_THREADSAFE
#   if MTDP_STATIC_THREADSAFE_LOCKFREE
#       include "thread.h"
#       include "atomic.h"
#       define MTDP_DEFINE_STATIC_INSTANCE(Type, Name)                          \
        static Type Name;                                                       \
        static atomic_flag Name##_owned = ATOMIC_FLAG_INIT;                     \
        MTDP_MEMORY_ACCESS Type* Type##_alloc() {                               \
            if(!atomic_flag_test_and_set(&Name##_owned)) {                      \
                return &Name;                                                   \
            }                                                                   \
            return NULL;                                                        \
        }                                                                       \
        MTDP_MEMORY_ACCESS void Type##_dealloc(Type* MTDP_MACRO_ARG) {          \
            if(MTDP_MACRO_ARG == &Name) {                                       \
                atomic_flag_clear(&Name##_owned);                               \
            }                                                                   \
        }
#       define MTDP_DEFINE_STATIC_INSTANCES(Type, Name, Number)                 \
        static Type Name[Number];                                               \
        static atomic_uchar Name##_flags[Number/8 + 1] = {0};                   \
        MTDP_MEMORY_ACCESS Type* Type##_alloc() {                               \
            Type* MTDP_MACRO_OUT_VAR = NULL;                                    \
            atomic_uchar *MTDP_MACRO_FLAG_VAR;                                  \
            unsigned char MTDP_MACRO_MASK_VAR;                                  \
            unsigned MTDP_MACRO_N_VAR;                                          \
            for(                                                                \
                unsigned MTDP_MACRO_I_VAR = 0;                                  \
                !MTDP_MACRO_OUT_VAR && MTDP_MACRO_I_VAR < Number;               \
                ++MTDP_MACRO_I_VAR                                              \
            ) {                                                                 \
                MTDP_MACRO_N_VAR = MTDP_MACRO_I_VAR/8;                          \
                flag = Name##_flags + MTDP_MACRO_N_VAR;                         \
                MTDP_MACRO_MASK_VAR = 1 << (i - MTDP_MACRO_N_VAR*8);            \
                if(!(atomic_fetch_or(MTDP_MACRO_FLAG_VAR,                       \
                    MTDP_MACRO_MASK_VAR) & MTDP_MACRO_MASK_VAR)                 \
                ) {                                                             \
                    MTDP_MACRO_OUT_VAR = &Name[i];                              \
                }                                                               \
            }                                                                   \
            return MTDP_MACRO_OUT_VAR;                                          \
        }                                                                       \
        MTDP_MEMORY_ACCESS void Type##_dealloc(Type* MTDP_MACRO_ARG) {          \
            ptrdiff_t MTDP_MACRO_TMP_VAR;                                       \
            if(MTDP_MACRO_ARG >= Name && MTDP_MACRO_ARG < Name + Number) {      \
                MTDP_MACRO_TMP_VAR = (MTDP_MACRO_ARG - Name)/8;                 \
                atomic_fetch_and(Name##_flags + MTDP_MACRO_TMP_VAR/8,           \
                    ~(1 << MTDP_MACRO_TMP_VAR%8));                              \
            }                                                                   \
        }
#   else
#       define MTDP_DEFINE_STATIC_INSTANCE(Type, Name)                          \
        static Type Name;                                                       \
        static bool Name##_owned = false;                                       \
        static mtx_t Name##_mutex;                                              \
        static once_flag Name##_init_flag = ONCE_FLAG_INIT;                     \
        static void Name##_init_once() { mtx_init(&Name##_mutex, mtx_plain); }  \
        MTDP_MEMORY_ACCESS Type* Type##_alloc() {                               \
            call_once(&Name##_init_flag, Name##_init_once);                     \
            mtx_lock(&Name##_mutex);                                            \
            if(!Name##_owned) {                                                 \
                Name##_owned = true;                                            \
                return &Name;                                                   \
            }                                                                   \
            mtx_unlock(&Name##_mutex);                                          \
            return NULL;                                                        \
        }                                                                       \
        MTDP_MEMORY_ACCESS void Type##_dealloc(Type* MTDP_MACRO_ARG) {          \
            if(DEFINE_STATIC_INSTANCE_ARG == &Name) {                           \
                mtx_lock(&Name##_mutex);                                        \
                Name##_owned = false;                                           \
                mtx_unlock(&Name##_mutex);                                      \
            }                                                                   \
        }
#       define MTDP_DEFINE_STATIC_INSTANCES(Type, Name, Number)                 \
        static Type Name[Number];                                               \
        static uint8_t Name##_flags[Number/8 + 1] = {0};                        \
        static mtx_t Name##_mutex;                                              \
        static once_flag Name##_init_flag = ONCE_FLAG_INIT;                     \
        static void Name##_init_once() { mtx_init(&Name##_mutex, mtx_plain); }  \
        MTDP_MEMORY_ACCESS Type* Type##_alloc() {                               \
            Type* out = NULL;                                                   \
            uint8_t *flag;                                                      \
            unsigned char mask;                                                 \
            unsigned n;                                                         \
            call_once(&Name##_init_flag, Name##_init_once);                     \
            for(unsigned i = 0; !out && i < Number; ++i) {                      \
                n = i/8;                                                        \
                flag = Name##_flags + n;                                        \
                mask = 1 << (i - n*8);                                          \
                mtx_lock(&Name##_mutex);                                        \
                if(!(*flag & mask)) {                                           \
                    *flag |= mask;                                              \
                    out = &Name[i];                                             \
                }                                                               \
                mtx_unlock(&Name##_mutex);                                      \
            }                                                                   \
            return out;                                                         \
        }                                                                       \
        MTDP_MEMORY_ACCESS void Type##_dealloc(Type* MTDP_MACRO_ARG) {          \
            ptrdiff_t MTDP_MACRO_TMP_VAR;                                       \
            if(DEFINE_STATIC_INSTANCE_ARG >= Name                               \
                && MTDP_MACRO_ARG < Name + Number                               \
            ) {                                                                 \
                MTDP_MACRO_TMP_VAR = MTDP_MACRO_ARG - Name;                     \
                mtx_lock(&Name##_mutex);                                        \
                Name##_flags[DEFINE_STATIC_INSTANCE_COUNTER/8] &=               \
                   ~(1 << MTDP_MACRO_TMP_VAR%8);                                \
                mtx_unlock(&Name##_mutex);                                      \
            }                                                                   \
        }
#   endif
#else
#   define MTDP_DEFINE_STATIC_INSTANCE(Type, Name)                              \
    static Type Name;                                                           \
    static bool Name##_owned = false;                                           \
    static mtx_t Name##_mutex;                                                  \
    static once_flag Name##_init_flag = ONCE_FLAG_INIT;                         \
    static void Name##_init_once() { mtx_init(&Name##_mutex, mtx_plain); }      \
    Type* Type##_alloc() {                                                      \
        call_once(&Name##_init_flag, Name##_init_once);                         \
        mtx_lock(&Name##_mutex);                                                \
        if(!Name##_owned) {                                                     \
            Name##_owned = true;                                                \
            return &Name;                                                       \
        }                                                                       \
        mtx_unlock(&Name##_mutex);                                              \
        return NULL;                                                            \
    }                                                                           \
    void Type##_dealloc(Type* MTDP_MACRO_ARG) {                                 \
        if(DEFINE_STATIC_INSTANCE_ARG == &Name) {                               \
            mtx_lock(&Name##_mutex);                                            \
            Name##_owned = false;                                               \
            mtx_unlock(&Name##_mutex);                                          \
        }                                                                       \
    }
#   define MTDP_DEFINE_STATIC_INSTANCES(Type, Name, Number)                     \
    static Type Name[Number];                                                   \
    static uint8_t Name##_flags[Number/8 + 1] = {0};                            \
    Type* Type##_alloc() {                                                      \
        Type* out = NULL;                                                       \
        uint8_t *flag;                                                          \
        unsigned char mask;                                                     \
        unsigned n;                                                             \
        for(unsigned i = 0; !out && i < Number; ++i) {                          \
            n = i/8;                                                            \
            flag = Name##_flags + n;                                            \
            mask = 1 << (i - n*8);                                              \
            if(!(*flag & mask)) {                                               \
                *flag |= mask;                                                  \
                out = &Name[i];                                                 \
            }                                                                   \
        }                                                                       \
        return out;                                                             \
    }                                                                           \
    void Type##_dealloc(Type* MTDP_MACRO_ARG) {                                 \
        ptrdiff_t MTDP_MACRO_TMP_VAR;                                           \
        if(DEFINE_STATIC_INSTANCE_ARG >= Name                                   \
            && MTDP_MACRO_ARG < Name + Number                                   \
        ) {                                                                     \
            MTDP_MACRO_TMP_VAR = MTDP_MACRO_ARG - Name;                         \
            Name##_flags[DEFINE_STATIC_INSTANCE_COUNTER/8] &=                   \
                ~(1 << MTDP_MACRO_TMP_VAR%8));                                  \
        }                                                                       \
    }
#endif

#define MTDP_DEFINE_DYNAMIC_INSTANCE(Type) \
MTDP_MEMORY_ACCESS Type* Type##_alloc() {  \
    return (Type*) malloc(sizeof(Type));   \
}                                          \
MTDP_MEMORY_ACCESS void                    \
Type##_dealloc(Type* MTDP_MACRO_VAR) {     \
    free(MTDP_MACRO_VAR);                  \
}

#endif
