# Try to find the QScintilla2 library
#  QSCINTILLA_FOUND - system has QSCINTILLA lib
#  QScintilla_INCLUDE_DIRS - the include directories needed
#  QScintilla_LIBRARIES - libraries needed
#
# Also create an imported CMake target named "QScintilla::QScintilla".

FIND_PATH(QSCINTILLA_INCLUDE_DIR NAMES Qsci/qsciscintilla.h PATH_SUFFIXES qt5 qt)
FIND_LIBRARY(QSCINTILLA_LIBRARY NAMES qsciscintilla qt5scintilla2 qscintilla2 qscintilla2_qt5)

SET(QScintilla_INCLUDE_DIRS ${QSCINTILLA_INCLUDE_DIR})
SET(QScintilla_LIBRARIES ${QSCINTILLA_LIBRARY})

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(QScintilla DEFAULT_MSG QSCINTILLA_LIBRARY QSCINTILLA_INCLUDE_DIR)

MARK_AS_ADVANCED(QSCINTILLA_INCLUDE_DIR QSCINTILLA_LIBRARY)

# Create imported target for the library.
IF(QSCINTILLA_FOUND AND NOT TARGET QScintilla::QScintilla)
	ADD_LIBRARY(QScintilla::QScintilla UNKNOWN IMPORTED)
	SET_TARGET_PROPERTIES(QScintilla::QScintilla PROPERTIES
		IMPORTED_LOCATION "${QSCINTILLA_LIBRARY}"
		INTERFACE_INCLUDE_DIRECTORIES "${QSCINTILLA_INCLUDE_DIR}"
		INTERFACE_COMPILE_DEFINITIONS "QSCINTILLA_DLL" # required when linking against QScintilla on Windows.
	)
ENDIF()
