#pragma once

/**
 * @file
 * Selects exactly one rcan backend based on the platform macros the build defines, and
 * exposes that choice as a single RCAN_BACKEND_* macro. Every other file in rcan tests
 * RCAN_BACKEND_HOST / RCAN_BACKEND_BXCAN / RCAN_BACKEND_BXCANFD instead of repeating the
 * full chip/OS lists below - add a new supported chip or OS here, once.
 */

#if defined(STM32F767xx) || defined(STM32F765xx) || defined(STM32F072xB) || defined(STM32F091xC) || \
    defined(STM32F103xB) || defined(STM32F429xx) || defined(STM32F407xx) || defined(STM32F103xE)
#    define RCAN_BACKEND_BXCAN 1
#elif defined(STM32G474xx) || defined(STM32G0B1xx)
#    define RCAN_BACKEND_BXCANFD 1
#elif defined(RCAN_MACOS) || defined(RCAN_UNIX)
#    define RCAN_BACKEND_HOST 1
#else
#    error "rcan: no backend selected - define one of the supported STM32 chip macros, or RCAN_MACOS/RCAN_UNIX"
#endif
