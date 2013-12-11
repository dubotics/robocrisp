get_filename_component(SELF_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
include("${SELF_DIR}/RoboCRISPTargets.cmake")
get_filename_component(RoboCRISP_INCLUDE_DIRS "${SELF_DIR}/../include/" ABSOLUTE)
