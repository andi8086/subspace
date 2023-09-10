PLATFORM_LINUX=1
PLATFORM_WIN32=2

BUILD_PLATFORM=Unknown
ifeq ($(OS),Windows_NT)
        BUILD_PLATFORM=Win32
else
        UNAME_S := $(shell uname -s)
        ifeq ($(UNAME_S),Linux)
                BUILD_PLATFORM=Linux
        endif
        ifeq ($(UNAME_S),Darwin)
                BUILD_PLATFORM=OSX
                $(error OSX is unsupported)
        endif
endif

ifeq ($(BUILD_PLATFORM),Unknown)
        $(error Unknown build platform)
endif

ifeq ($(BUILD_PLATFORM),Win32)
LDFLAGS += -lws2_32
CFLAGS += -DBUILD_PLATFORM=$(PLATFORM_WIN32)

RM = del /Q
endif

ifeq ($(BUILD_PLATFORM),Linux)
CFLAGS += -DHAVE_BSD_STDLIB
CFLAGS += -DBUILD_PLATFORM=$(PLATFORM_LINUX)

RM = rm -rf
endif


