option(USE_CLANG "build application with clang" OFF)

if(USE_CLANG)
  SET(CMAKE_C_COMPILER    "clang")
  SET(CMAKE_CXX_COMPILER  "clang++")
  SET(CMAKE_AR            "llvm-ar")
  SET(CMAKE_NM            "llvm-nm")
  SET(CMAKE_OBJDUMP       "llvm-objdump")
  SET(CMAKE_RANLIB        "llvm-ranlib")
endif(USE_CLANG)

option(ADDRESS_SANITIZER "Enable Clang AddressSanitizer" OFF)
if (ADDRESS_SANITIZER AND USE_CLANG)
    message(STATUS "AddressSanitizer enabled for debug build")
    set(CMAKE_CXX_FLAGS_DEBUG
        "${CMAKE_CXX_FLAGS_DEBUG} -O1 -fno-omit-frame-pointer -fsanitize=address")
elseif(ADDRESS_SANITIZER AND NOT USE_CLANG)
    message(ERROR "AddressSanitizer can only be enabled with USE_CLANG=ON")
endif ()

option(UNDEFINED_SANITIZER "Enable Clang UndefinedBehaviorSanitizer" OFF)
if (UNDEFINED_SANITIZER AND USE_CLANG)
    message(STATUS "UndefinedBehaviorSanitizer enabled for debug build")
    set(CMAKE_CXX_FLAGS_DEBUG
        "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=undefined -fsanitize=integer")
elseif(UNDEFINED_SANITIZER AND NOT USE_CLANG)
    message(ERROR "UndefinedBehaviorSanitizer can only be enabled with USE_CLANG=ON")
endif ()

option(CLANG_CODE_COVERAGE "Enable code coverage metrics in Clang" OFF)
if (CLANG_CODE_COVERAGE AND USE_CLANG)
    message(STATUS "Code coverage metrics enabled for debug build")
    set(CMAKE_CXX_FLAGS_DEBUG
        "${CMAKE_CXX_FLAGS_DEBUG} -fprofile-instr-generate -fcoverage-mapping")
elseif(CLANG_CODE_COVERAGE AND NOT USE_CLANG)
    message(ERROR "Code coverage metrics can only be enabled with USE_CLANG=ON")
endif ()
