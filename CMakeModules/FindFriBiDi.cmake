# - Find the native FriBiDI includes and library
#
# This module defines
#  FRIBIDI_INCLUDE_DIR, where to find fribidi.h, etc.
#  FRIBIDI_LIBRARIES, the libraries to link against to use FriBiDi.
#  FRIBIDI_FOUND, If false, do not try to use fribidi.
# also defined, but not for general use are
#  FRIBIDI_LIBRARY, where to find the FriBiDi library.

include(CheckFunctionExists)
SET(FRIBIDI_FOUND "NO")

FIND_PATH(FRIBIDI_INCLUDE_DIR fribidi/fribidi.h
  /usr/local/include
  /usr/include
  )

SET(FRIBIDI_NAMES ${FRIBIDI_NAMES} fribidi libfribidi)
FIND_LIBRARY(FRIBIDI_LIBRARY
  NAMES ${FRIBIDI_NAMES}
  PATHS /usr/lib /usr/local/lib
  )

IF (FRIBIDI_LIBRARY AND FRIBIDI_INCLUDE_DIR)
  SET(CMAKE_REQUIRED_INCLUDES ${FRIBIDI_INCLUDE_DIR})
  SET(CMAKE_REQUIRED_LIBRARIES ${FRIBIDI_LIBRARY})
  CHECK_FUNCTION_EXISTS(fribidi_utf8_to_unicode FOUND_fribidi_utf8_to_unicode)
  IF(FOUND_fribidi_utf8_to_unicode)
    SET(FRIBIDI_LIBRARIES ${FRIBIDI_LIBRARY})
    SET(FRIBIDI_FOUND "YES")
  ELSE(FOUND_fribidi_utf8_to_unicode)
    SET(FRIBIDI_LIBRARIES "NOTFOUND")
    SET(FRIBIDI_INCLUDE_DIR "NOTFOUND")
    SET(FRIBIDI_FOUND "NO")
  ENDIF(FOUND_fribidi_utf8_to_unicode)
ENDIF (FRIBIDI_LIBRARY AND FRIBIDI_INCLUDE_DIR)

IF (FRIBIDI_FOUND)
  IF (NOT FRIBIDI_FIND_QUIETLY)
    MESSAGE(STATUS "Found FriBiDi: ${FRIBIDI_LIBRARY}")
  ENDIF (NOT FRIBIDI_FIND_QUIETLY)
ELSE (FRIBIDI_FOUND)
  IF (FRIBIDI_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "Could not find FriBiDi library")
  ENDIF (FRIBIDI_FIND_REQUIRED)
ENDIF (FRIBIDI_FOUND)