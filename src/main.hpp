#ifndef MDC_RAW_MAIN_HPP
#define MDC_RAW_MAIN_HPP

#include <lak/architecture.hpp>

#include "git.hpp"
#define APP_VERSION GIT_TAG "-" GIT_HASH
#define APP_NAME    "MDC RAW " STRINGIFY(LAK_ARCH) " " APP_VERSION

#endif
