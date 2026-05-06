#ifndef __RNG_H
#define __RNG_H

#include <stdint.h>
#include <stddef.h>
#include "../../GPIO/inc/gpio.h"   /* RCC_TypeDef / RCC */

#define __IO volatile

/* RCC clock enable for RNG (AHB2ENR bit 6) */
#define RCC_AHB2ENR_RNGEN  (1U << 6)

#endif /* __RNG_H */