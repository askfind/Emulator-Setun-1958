/**
  ******************************************************************************
  * @file    setun1958_vm.h
  *
  * @brief
  ******************************************************************************
  */

#ifndef SETUN1958_VM_H_
#define SETUN1958_VM_H_

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdbool.h>
#include <stdint.h>

#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* SETUN1958_VM_H_ */

