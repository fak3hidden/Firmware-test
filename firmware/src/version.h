#pragma once

#define FINOS_NAME            "FinOS"
#define FINOS_VERSION         "0.1.0"
#define FINOS_VERSION_MAJOR   0
#define FINOS_VERSION_MINOR   1
#define FINOS_VERSION_PATCH   0
#define FINOS_BUILD_DATE      __DATE__
#define FINOS_BUILD_TIME      __TIME__

#if FINOS_PLUS
#define FINOS_BOARD_NAME      "T-Embed CC1101 Plus"
#define FINOS_BOARD_ID        "t-embed-cc1101-plus"
#else
#define FINOS_BOARD_NAME      "T-Embed CC1101"
#define FINOS_BOARD_ID        "t-embed-cc1101"
#endif
