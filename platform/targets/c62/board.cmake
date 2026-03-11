# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=Venus")
board_runner_args(pyocd "--target=csk6001")
board_runner_args(csk "--chip=6")

set_ifndef(BOARD_DEBUG_RUNNER pyocd)
set_ifndef(BOARD_FLASH_RUNNER pyocd)

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)

get_filename_component(PARENT_DIR "${CMAKE_CURRENT_LIST_DIR}" DIRECTORY)
include(${ZEPHYR_CSK_MODULE_DIR}/boards/common/csk.board.cmake)

