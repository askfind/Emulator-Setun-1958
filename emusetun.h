/**
  ******************************************************************************
  * @file    emusetun.h
  *
  * @brief
  ******************************************************************************
  */

#ifndef EMUSETUN_H_
#define EMUSETUN_H_

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

#endif /* EMUSETUN_H_ */

