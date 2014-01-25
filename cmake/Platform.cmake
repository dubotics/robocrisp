# This file checks for various platforms and sets variables/flags as necessary.

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  if(EXISTS "/etc/issue")
    file(READ "/etc/issue" ISSUE)

    # ================================================================
    # Raspberry Pi

    if("${ISSUE}" MATCHES "^Raspbian")
      set(RoboCRISP_BUILD_INPUT OFF)
      add_definitions("-march=armv6zk")
      add_definitions("-mcpu=arm1176jzf-s")
      add_definitions("-mfloat-abi=hard")
      set(CMAKE_EXE_LINKER_FLAGS "-Wl,--fix-arm1176 -Wl,--defsym,_Unwind_SjLj_RaiseException=abort -Wl,--defsym,_Unwind_SetIP=abort -Wl,--defsym,_Unwind_GetIP=abort -Wl,--defsym,_Unwind_SetGR=abort")
    endif("${ISSUE}" MATCHES "^Raspbian")
  endif(EXISTS "/etc/issue")
endif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
