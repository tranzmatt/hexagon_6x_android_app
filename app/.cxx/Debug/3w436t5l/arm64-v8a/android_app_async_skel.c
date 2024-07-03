#ifndef _ANDROID_APP_ASYNC_SKEL_H
#define _ANDROID_APP_ASYNC_SKEL_H
#include "android_app_async.h"

#include <string.h>
#ifndef _WIN32
#include "HAP_farf.h"
#endif //_WIN32 for HAP_farf
#ifndef _ALLOCATOR_H
#define _ALLOCATOR_H

#include <stdlib.h>
#include <stdint.h>

typedef struct _heap _heap;
struct _heap {
   _heap* pPrev;
   const char* loc;
   uint64_t buf;
};

typedef struct _allocator {
   _heap* pheap;
   uint8_t* stack;
   uint8_t* stackEnd;
   int nSize;
} _allocator;

_ATTRIBUTE_UNUSED
static __inline int _heap_alloc(_heap** ppa, const char* loc, int size, void** ppbuf) {
   _heap* pn = 0;
   pn = MALLOC((size_t)size + sizeof(_heap) - sizeof(uint64_t));
   if(pn != 0) {
      pn->pPrev = *ppa;
      pn->loc = loc;
      *ppa = pn;
      *ppbuf = (void*)&(pn->buf);
      return 0;
   } else {
      return -1;
   }
}
#define _ALIGN_SIZE(x, y) (((x) + (y-1)) & ~(y-1))

_ATTRIBUTE_UNUSED
static __inline int _allocator_alloc(_allocator* me,
                                    const char* loc,
                                    int size,
                                    unsigned int al,
                                    void** ppbuf) {
   if(size < 0) {
      return -1;
   } else if (size == 0) {
      *ppbuf = 0;
      return 0;
   }
   if((_ALIGN_SIZE((uintptr_t)me->stackEnd, al) + (size_t)size) < (uintptr_t)me->stack + (size_t)me->nSize) {
      *ppbuf = (uint8_t*)_ALIGN_SIZE((uintptr_t)me->stackEnd, al);
      me->stackEnd = (uint8_t*)_ALIGN_SIZE((uintptr_t)me->stackEnd, al) + size;
      return 0;
   } else {
      return _heap_alloc(&me->pheap, loc, size, ppbuf);
   }
}

_ATTRIBUTE_UNUSED
static __inline void _allocator_deinit(_allocator* me) {
   _heap* pa = me->pheap;
   while(pa != 0) {
      _heap* pn = pa;
      const char* loc = pn->loc;
      (void)loc;
      pa = pn->pPrev;
      FREE(pn);
   }
}

_ATTRIBUTE_UNUSED
static __inline void _allocator_init(_allocator* me, uint8_t* stack, int stackSize) {
   me->stack =  stack;
   me->stackEnd =  stack + stackSize;
   me->nSize = stackSize;
   me->pheap = 0;
}


#endif // _ALLOCATOR_H

#ifndef SLIM_H
#define SLIM_H

#include <stdint.h>

//a C data structure for the idl types that can be used to implement
//static and dynamic language bindings fairly efficiently.
//
//the goal is to have a minimal ROM and RAM footprint and without
//doing too many allocations.  A good way to package these things seemed
//like the module boundary, so all the idls within  one module can share
//all the type references.


#define PARAMETER_IN       0x0
#define PARAMETER_OUT      0x1
#define PARAMETER_INOUT    0x2
#define PARAMETER_ROUT     0x3
#define PARAMETER_INROUT   0x4

//the types that we get from idl
#define TYPE_OBJECT             0x0
#define TYPE_INTERFACE          0x1
#define TYPE_PRIMITIVE          0x2
#define TYPE_ENUM               0x3
#define TYPE_STRING             0x4
#define TYPE_WSTRING            0x5
#define TYPE_STRUCTURE          0x6
#define TYPE_UNION              0x7
#define TYPE_ARRAY              0x8
#define TYPE_SEQUENCE           0x9

//these require the pack/unpack to recurse
//so it's a hint to those languages that can optimize in cases where
//recursion isn't necessary.
#define TYPE_COMPLEX_STRUCTURE  (0x10 | TYPE_STRUCTURE)
#define TYPE_COMPLEX_UNION      (0x10 | TYPE_UNION)
#define TYPE_COMPLEX_ARRAY      (0x10 | TYPE_ARRAY)
#define TYPE_COMPLEX_SEQUENCE   (0x10 | TYPE_SEQUENCE)


typedef struct Type Type;

#define INHERIT_TYPE\
   int32_t nativeSize;                /*in the simple case its the same as wire size and alignment*/\
   union {\
      struct {\
         const uintptr_t         p1;\
         const uintptr_t         p2;\
      } _cast;\
      struct {\
         uint32_t  iid;\
         uint32_t  bNotNil;\
      } object;\
      struct {\
         const Type  *arrayType;\
         int32_t      nItems;\
      } array;\
      struct {\
         const Type *seqType;\
         int32_t      nMaxLen;\
      } seqSimple; \
      struct {\
         uint32_t bFloating;\
         uint32_t bSigned;\
      } prim; \
      const SequenceType* seqComplex;\
      const UnionType  *unionType;\
      const StructType *structType;\
      int32_t         stringMaxLen;\
      uint8_t        bInterfaceNotNil;\
   } param;\
   uint8_t    type;\
   uint8_t    nativeAlignment\

typedef struct UnionType UnionType;
typedef struct StructType StructType;
typedef struct SequenceType SequenceType;
struct Type {
   INHERIT_TYPE;
};

struct SequenceType {
   const Type *         seqType;
   uint32_t               nMaxLen;
   uint32_t               inSize;
   uint32_t               routSizePrimIn;
   uint32_t               routSizePrimROut;
};

//byte offset from the start of the case values for
//this unions case value array.  it MUST be aligned
//at the alignment requrements for the descriptor
//
//if negative it means that the unions cases are
//simple enumerators, so the value read from the descriptor
//can be used directly to find the correct case
typedef union CaseValuePtr CaseValuePtr;
union CaseValuePtr {
   const uint8_t*   value8s;
   const uint16_t*  value16s;
   const uint32_t*  value32s;
   const uint64_t*  value64s;
};

//these are only used in complex cases
//so I pulled them out of the type definition as references to make
//the type smaller
struct UnionType {
   const Type           *descriptor;
   uint32_t               nCases;
   const CaseValuePtr   caseValues;
   const Type * const   *cases;
   int32_t               inSize;
   int32_t               routSizePrimIn;
   int32_t               routSizePrimROut;
   uint8_t                inAlignment;
   uint8_t                routAlignmentPrimIn;
   uint8_t                routAlignmentPrimROut;
   uint8_t                inCaseAlignment;
   uint8_t                routCaseAlignmentPrimIn;
   uint8_t                routCaseAlignmentPrimROut;
   uint8_t                nativeCaseAlignment;
   uint8_t              bDefaultCase;
};

struct StructType {
   uint32_t               nMembers;
   const Type * const   *members;
   int32_t               inSize;
   int32_t               routSizePrimIn;
   int32_t               routSizePrimROut;
   uint8_t                inAlignment;
   uint8_t                routAlignmentPrimIn;
   uint8_t                routAlignmentPrimROut;
};

typedef struct Parameter Parameter;
struct Parameter {
   INHERIT_TYPE;
   uint8_t    mode;
   uint8_t  bNotNil;
};

#define SLIM_IFPTR32(is32,is64) (sizeof(uintptr_t) == 4 ? (is32) : (is64))
#define SLIM_SCALARS_IS_DYNAMIC(u) (((u) & 0x00ffffff) == 0x00ffffff)

typedef struct Method Method;
struct Method {
   uint32_t                    uScalars;            //no method index
   int32_t                     primInSize;
   int32_t                     primROutSize;
   int                         maxArgs;
   int                         numParams;
   const Parameter * const     *params;
   uint8_t                       primInAlignment;
   uint8_t                       primROutAlignment;
};

typedef struct Interface Interface;

struct Interface {
   int                            nMethods;
   const Method  * const          *methodArray;
   int                            nIIds;
   const uint32_t                   *iids;
   const uint16_t*                  methodStringArray;
   const uint16_t*                  methodStrings;
   const char*                    strings;
};


#endif //SLIM_H


#ifndef _ANDROID_APP_ASYNC_SLIM_H
#define _ANDROID_APP_ASYNC_SLIM_H
#include <stdint.h>

#ifndef __QAIC_SLIM
#define __QAIC_SLIM(ff) ff
#endif
#ifndef __QAIC_SLIM_EXPORT
#define __QAIC_SLIM_EXPORT
#endif

static const Type types[3];
static const Type* const typeArrays[27] = {&(types[0]),&(types[0]),&(types[0]),&(types[0]),&(types[0]),&(types[0]),&(types[0]),&(types[0]),&(types[0]),&(types[0]),&(types[0]),&(types[0]),&(types[0]),&(types[0]),&(types[0]),&(types[0]),&(types[0]),&(types[0]),&(types[1]),&(types[1]),&(types[1]),&(types[1]),&(types[0]),&(types[0]),&(types[0]),&(types[0]),&(types[0])};
static const StructType structTypes[2] = {{0xb,&(typeArrays[16]),0x40,0x0,0x40,0x8,0x1,0x8},{0x10,&(typeArrays[0]),0x40,0x0,0x40,0x4,0x1,0x4}};
static const Type types[3] = {{0x4,{{(const uintptr_t)0,(const uintptr_t)1}}, 2,0x4},{0x8,{{(const uintptr_t)0,(const uintptr_t)1}}, 2,0x8},{0x1,{{(const uintptr_t)0,(const uintptr_t)1}}, 2,0x1}};
static const Parameter parameters[8] = {{SLIM_IFPTR32(0x8,0x10),{{(const uintptr_t)0x0,0}}, 4,SLIM_IFPTR32(0x4,0x8),0,0},{SLIM_IFPTR32(0x4,0x8),{{(const uintptr_t)0xdeadc0de,(const uintptr_t)0}}, 0,SLIM_IFPTR32(0x4,0x8),3,0},{SLIM_IFPTR32(0x4,0x8),{{(const uintptr_t)0xdeadc0de,(const uintptr_t)0}}, 0,SLIM_IFPTR32(0x4,0x8),0,0},{0x4,{{(const uintptr_t)0,(const uintptr_t)1}}, 2,0x4,0,0},{0x40,{{(const uintptr_t)&(structTypes[0]),0}}, 6,0x8,3,0},{0x40,{{(const uintptr_t)&(structTypes[1]),0}}, 6,0x4,0,0},{SLIM_IFPTR32(0x8,0x10),{{(const uintptr_t)&(types[2]),(const uintptr_t)0x0}}, 9,SLIM_IFPTR32(0x4,0x8),0,0},{SLIM_IFPTR32(0x8,0x10),{{(const uintptr_t)&(types[2]),(const uintptr_t)0x0}}, 9,SLIM_IFPTR32(0x4,0x8),3,0}};
static const Parameter* const parameterArrays[10] = {(&(parameters[5])),(&(parameters[6])),(&(parameters[7])),(&(parameters[3])),(&(parameters[3])),(&(parameters[3])),(&(parameters[0])),(&(parameters[1])),(&(parameters[4])),(&(parameters[2]))};
static const Method methods[6] = {{REMOTE_SCALARS_MAKEX(0,0,0x2,0x0,0x0,0x1),0x4,0x0,2,2,(&(parameterArrays[6])),0x4,0x1},{REMOTE_SCALARS_MAKEX(0,0,0x0,0x0,0x1,0x0),0x0,0x0,1,1,(&(parameterArrays[9])),0x1,0x0},{REMOTE_SCALARS_MAKEX(0,0,0x0,0x0,0x0,0x0),0x0,0x0,0,0,0,0x0,0x0},{REMOTE_SCALARS_MAKEX(0,0,0x1,0x0,0x0,0x0),0xc,0x0,3,3,(&(parameterArrays[3])),0x4,0x0},{REMOTE_SCALARS_MAKEX(0,0,0x0,0x1,0x0,0x0),0x0,0x40,1,1,(&(parameterArrays[8])),0x1,0x8},{REMOTE_SCALARS_MAKEX(0,0,0x2,0x1,0x0,0x0),0x48,0x0,6,3,(&(parameterArrays[0])),0x4,0x1}};
static const Method* const methodArrays[7] = {&(methods[0]),&(methods[1]),&(methods[2]),&(methods[3]),&(methods[4]),&(methods[5]),&(methods[2])};
static const char strings[513] = "resource_acquire_timeout_us\0waiting_on_resource_time_us\0num_tasks_interrupted\0mega_cycles_per_task\0num_tasks_completed\0trigger_period_ms\0num_interruptions\0num_tasks_aborted\0num_tasks_failed\0num_failed_tests\0is_asynchronous\0thread_priority\0is_serialized\0verbose_mask\0array_height\0task_trigger\0vtcm_size_mb\0test_time_us\0dcvs_enabled\0array_width\0num_workers\0dsp_version\0dsp_time_us\0cpu_time_us\0power_level\0session_id\0set_clocks\0terminate\0num_tasks\0get_stats\0out_ptr\0latency\0in_ptr\0is_off\0dummy\0close\0open\0run\0uri\0c\0";
static const uint16_t methodStrings[44] = {502,510,403,223,435,478,355,292,343,78,239,207,119,0,279,331,266,253,471,455,445,26,403,190,379,305,367,28,99,155,173,56,137,414,391,463,318,497,506,341,491,341,425,485};
static const uint16_t methodStringsArrays[7] = {37,40,43,33,20,0,42};
__QAIC_SLIM_EXPORT const Interface __QAIC_SLIM(android_app_async_slim) = {7,&(methodArrays[0]),0,0,&(methodStringsArrays [0]),methodStrings,strings};
#endif //_ANDROID_APP_ASYNC_SLIM_H
extern int adsp_mmap_fd_getinfo(int, uint32_t *);
#ifdef __cplusplus
extern "C" {
#endif
_ATTRIBUTE_VISIBILITY uint32_t android_app_async_skel_handle_invoke_qaic_version = 10048;
_ATTRIBUTE_VISIBILITY char android_app_async_skel_handle_invoke_uri[85+1]="file:///libandroid_app_async_skel.so?android_app_async_skel_handle_invoke&_modver=1.0";
static __inline int _skel_method(int (*_pfn)(remote_handle64), remote_handle64 _h, uint32_t _sc, remote_arg* _pra) {
   remote_arg* _praEnd = 0;
   int _nErr = 0;
   _praEnd = ((_pra + REMOTE_SCALARS_INBUFS(_sc)) + REMOTE_SCALARS_OUTBUFS(_sc) + REMOTE_SCALARS_INHANDLES(_sc) + REMOTE_SCALARS_OUTHANDLES(_sc));
   _QAIC_ASSERT(_nErr, REMOTE_SCALARS_INBUFS(_sc)==0);
   _QAIC_ASSERT(_nErr, REMOTE_SCALARS_OUTBUFS(_sc)==0);
   _QAIC_ASSERT(_nErr, REMOTE_SCALARS_INHANDLES(_sc)==0);
   _QAIC_ASSERT(_nErr, REMOTE_SCALARS_OUTHANDLES(_sc)==0);
   _QAIC_ASSERT(_nErr, (_pra + ((0 + 0) + (((0 + 0) + 0) + 0))) <= _praEnd);
   _TRY(_nErr, _pfn(_h));
   _QAIC_CATCH(_nErr) {}
   return _nErr;
}
static __inline int _skel_method_1(int (*_pfn)(remote_handle64, fastrpc_async_descriptor_t* asyncDesc, const android_app_configs*, const uint8*, int, uint8*, int), remote_handle64 _h, uint32_t _sc, remote_arg* _pra) {
   remote_arg* _praEnd = 0;
   uint32_t _in0[16] = {0};
   char* _in1[1] = {0};
   uint32_t _in1Len[1] = {0};
   char* _rout2[1] = {0};
   uint32_t _rout2Len[1] = {0};
   uint32_t* _primIn= 0;
   int _numIn[1] = {0};
   remote_arg* _praIn = 0;
   remote_arg* _praROut = 0;
   int _nErr = 0;
   _praEnd = ((_pra + REMOTE_SCALARS_INBUFS(_sc)) + REMOTE_SCALARS_OUTBUFS(_sc) + REMOTE_SCALARS_INHANDLES(_sc) + REMOTE_SCALARS_OUTHANDLES(_sc));
   _QAIC_ASSERT(_nErr, REMOTE_SCALARS_INBUFS(_sc)==2);
   _QAIC_ASSERT(_nErr, REMOTE_SCALARS_OUTBUFS(_sc)==1);
   _QAIC_ASSERT(_nErr, REMOTE_SCALARS_INHANDLES(_sc)==0);
   _QAIC_ASSERT(_nErr, REMOTE_SCALARS_OUTHANDLES(_sc)==0);
   _QAIC_ASSERT(_nErr, (_pra + ((2 + 1) + (((0 + 0) + 0) + 0))) <= _praEnd);
   _numIn[0] = (REMOTE_SCALARS_INBUFS(_sc) - 1);
   _QAIC_ASSERT(_nErr, _pra[0].buf.nLen >= 72);
   _primIn = _pra[0].buf.pv;
   _COPY(_in0, 0, _primIn, 0, 64);
   _COPY(_in1Len, 0, _primIn, 64, 4);
   _praIn = (_pra + 1);
   _QAIC_ASSERT(_nErr, ((_praIn[0].buf.nLen / 1)) >= (size_t)(_in1Len[0]));
   _in1[0] = _praIn[0].buf.pv;
   _COPY(_rout2Len, 0, _primIn, 68, 4);
   _praROut = (_praIn + _numIn[0] + 0);
   _QAIC_ASSERT(_nErr, ((_praROut[0].buf.nLen / 1)) >= (size_t)(_rout2Len[0]));
   _rout2[0] = _praROut[0].buf.pv;
   _TRY(_nErr, _pfn(_h, NULL, (const android_app_configs*)_in0, (const uint8*)*_in1, (int)*_in1Len, (uint8*)*_rout2, (int)*_rout2Len));
   _QAIC_CATCH(_nErr) {}
   return _nErr;
}
static __inline int _skel_method_2(int (*_pfn)(remote_handle64, android_app_stats*), remote_handle64 _h, uint32_t _sc, remote_arg* _pra) {
   remote_arg* _praEnd = 0;
   uint64_t _rout0[8] = {0};
   uint64_t* _primROut= 0;
   int _numIn[1] = {0};
   int _nErr = 0;
   _praEnd = ((_pra + REMOTE_SCALARS_INBUFS(_sc)) + REMOTE_SCALARS_OUTBUFS(_sc) + REMOTE_SCALARS_INHANDLES(_sc) + REMOTE_SCALARS_OUTHANDLES(_sc));
   _QAIC_ASSERT(_nErr, REMOTE_SCALARS_INBUFS(_sc)==0);
   _QAIC_ASSERT(_nErr, REMOTE_SCALARS_OUTBUFS(_sc)==1);
   _QAIC_ASSERT(_nErr, REMOTE_SCALARS_INHANDLES(_sc)==0);
   _QAIC_ASSERT(_nErr, REMOTE_SCALARS_OUTHANDLES(_sc)==0);
   _QAIC_ASSERT(_nErr, (_pra + ((0 + 1) + (((0 + 0) + 0) + 0))) <= _praEnd);
   _numIn[0] = (REMOTE_SCALARS_INBUFS(_sc) - 0);
   _QAIC_ASSERT(_nErr, _pra[(_numIn[0] + 0)].buf.nLen >= 64);
   _primROut = _pra[(_numIn[0] + 0)].buf.pv;
   _TRY(_nErr, _pfn(_h, (android_app_stats*)_rout0));
   _COPY(_primROut, 0, _rout0, 0, 64);
   _QAIC_CATCH(_nErr) {}
   return _nErr;
}
static __inline int _skel_method_3(int (*_pfn)(remote_handle64, int32, int32, int32), remote_handle64 _h, uint32_t _sc, remote_arg* _pra) {
   remote_arg* _praEnd = 0;
   uint32_t _in0[1] = {0};
   uint32_t _in1[1] = {0};
   uint32_t _in2[1] = {0};
   uint32_t* _primIn= 0;
   int _nErr = 0;
   _praEnd = ((_pra + REMOTE_SCALARS_INBUFS(_sc)) + REMOTE_SCALARS_OUTBUFS(_sc) + REMOTE_SCALARS_INHANDLES(_sc) + REMOTE_SCALARS_OUTHANDLES(_sc));
   _QAIC_ASSERT(_nErr, REMOTE_SCALARS_INBUFS(_sc)==1);
   _QAIC_ASSERT(_nErr, REMOTE_SCALARS_OUTBUFS(_sc)==0);
   _QAIC_ASSERT(_nErr, REMOTE_SCALARS_INHANDLES(_sc)==0);
   _QAIC_ASSERT(_nErr, REMOTE_SCALARS_OUTHANDLES(_sc)==0);
   _QAIC_ASSERT(_nErr, (_pra + ((1 + 0) + (((0 + 0) + 0) + 0))) <= _praEnd);
   _QAIC_ASSERT(_nErr, _pra[0].buf.nLen >= 12);
   _primIn = _pra[0].buf.pv;
   _COPY(_in0, 0, _primIn, 0, 4);
   _COPY(_in1, 0, _primIn, 4, 4);
   _COPY(_in2, 0, _primIn, 8, 4);
   _TRY(_nErr, _pfn(_h, (int32)*_in0, (int32)*_in1, (int32)*_in2));
   _QAIC_CATCH(_nErr) {}
   return _nErr;
}
static __inline int _skel_method_4(int (*_pfn)(remote_handle64), uint32_t _sc, remote_arg* _pra) {
   remote_arg* _praEnd = 0;
   remote_handle64 _in0[1] = {0};
   remote_arg* _praRHandleIn = _pra + REMOTE_SCALARS_INBUFS(_sc) +  REMOTE_SCALARS_OUTBUFS(_sc);
   int _nErr = 0;
   _praEnd = ((_pra + REMOTE_SCALARS_INBUFS(_sc)) + REMOTE_SCALARS_OUTBUFS(_sc) + REMOTE_SCALARS_INHANDLES(_sc) + REMOTE_SCALARS_OUTHANDLES(_sc));
   _QAIC_ASSERT(_nErr, REMOTE_SCALARS_INBUFS(_sc)==0);
   _QAIC_ASSERT(_nErr, REMOTE_SCALARS_OUTBUFS(_sc)==0);
   _QAIC_ASSERT(_nErr, REMOTE_SCALARS_INHANDLES(_sc)==1);
   _QAIC_ASSERT(_nErr, REMOTE_SCALARS_OUTHANDLES(_sc)==0);
   _QAIC_ASSERT(_nErr, (_pra + ((0 + 0) + (((1 + 0) + 0) + 0))) <= _praEnd);
   _COPY(_in0, 0, &(_praRHandleIn[0].h64), 0, sizeof(remote_handle64));
   _TRY(_nErr, _pfn((remote_handle64)*_in0));
   _QAIC_CATCH(_nErr) {}
   return _nErr;
}
static __inline int _skel_method_5(int (*_pfn)(const char*, remote_handle64*), uint32_t _sc, remote_arg* _pra) {
   remote_arg* _praEnd = 0;
   char* _in0[1] = {0};
   uint32_t _in0Len[1] = {0};
   remote_handle64 _rout1[1] = {0};
   uint32_t* _primIn= 0;
   remote_arg* _praRHandleROut = _pra + REMOTE_SCALARS_INBUFS(_sc) + REMOTE_SCALARS_OUTBUFS(_sc) + REMOTE_SCALARS_INHANDLES(_sc) ;
   remote_arg* _praIn = 0;
   int _nErr = 0;
   _praEnd = ((_pra + REMOTE_SCALARS_INBUFS(_sc)) + REMOTE_SCALARS_OUTBUFS(_sc) + REMOTE_SCALARS_INHANDLES(_sc) + REMOTE_SCALARS_OUTHANDLES(_sc));
   _QAIC_ASSERT(_nErr, REMOTE_SCALARS_INBUFS(_sc)==2);
   _QAIC_ASSERT(_nErr, REMOTE_SCALARS_OUTBUFS(_sc)==0);
   _QAIC_ASSERT(_nErr, REMOTE_SCALARS_INHANDLES(_sc)==0);
   _QAIC_ASSERT(_nErr, REMOTE_SCALARS_OUTHANDLES(_sc)==1);
   _QAIC_ASSERT(_nErr, (_pra + ((2 + 0) + (((0 + 1) + 0) + 0))) <= _praEnd);
   _QAIC_ASSERT(_nErr, _pra[0].buf.nLen >= 4);
   _primIn = _pra[0].buf.pv;
   _COPY(_in0Len, 0, _primIn, 0, 4);
   _praIn = (_pra + 1);
   _QAIC_ASSERT(_nErr, ((_praIn[0].buf.nLen / 1)) >= (size_t)(_in0Len[0]));
   _in0[0] = _praIn[0].buf.pv;
   _QAIC_ASSERT(_nErr, (_in0Len[0] > 0) && (_in0[0][(_in0Len[0] - 1)] == 0));
   _TRY(_nErr, _pfn((const char*)*_in0, (remote_handle64*)_rout1));
   _COPY(&(_praRHandleROut[0].h64), 0, _rout1, 0, sizeof(remote_handle64));
   _QAIC_CATCH(_nErr) {}
   return _nErr;
}
__QAIC_SKEL_EXPORT int __QAIC_SKEL(android_app_async_skel_handle_invoke)(remote_handle64 _h, uint32_t _sc, remote_arg* _pra) __QAIC_SKEL_ATTRIBUTE {
   switch(REMOTE_SCALARS_METHOD(_sc)){
      case 0:
      return _skel_method_5(__QAIC_IMPL(android_app_async_open), _sc, _pra);
      case 1:
      return _skel_method_4(__QAIC_IMPL(android_app_async_close), _sc, _pra);
      case 2:
      return _skel_method(__QAIC_IMPL(android_app_async_dummy), _h, _sc, _pra);
      case 3:
      return _skel_method_3(__QAIC_IMPL(android_app_async_set_clocks), _h, _sc, _pra);
      case 4:
      return _skel_method_2(__QAIC_IMPL(android_app_async_get_stats), _h, _sc, _pra);
      case 5:
      return _skel_method_1(__QAIC_IMPL(android_app_async_run), _h, _sc, _pra);
      case 6:
      return _skel_method(__QAIC_IMPL(android_app_async_terminate), _h, _sc, _pra);
   }
   return AEE_EUNSUPPORTED;
}
#ifdef __cplusplus
}
#endif
#endif //_ANDROID_APP_ASYNC_SKEL_H
