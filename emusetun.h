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

#include <stdint.h>

typedef uint8_t bool ;

#ifndef false
  #define false 0
#endif
#ifndef true
  #define true  !false
#endif

#ifndef FALSE
  #define FALSE 0
#endif
#ifndef TRUE
  #define TRUE  !FALSE
#endif

#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

/* --------------------
*  Test call C-function
*/
int greet(const char *name, int year, char *out);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* EMUSETUN_H_ */

