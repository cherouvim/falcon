find_path(Editline_INCLUE_DIR editline/readline.h)
find_library(Editline_LIBRARY edit)

include(CheckSymbolExists)
set(CMAKE_REQUIRED_INCLUDES ${Editline_INCLUE_DIR})
set(CMAKE_REQUIRED_LIBRARIES ${Editline_LIBRARY})
check_symbol_exists(rl_getc_function "stdio.h;editline/readline.h" 
  HAVE_RL_GETC_FUNCTION 
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Editline DEFAULT_MSG Editline_LIBRARY Editline_INCLUE_DIR HAVE_RL_GETC_FUNCTION)

set(Editline_INCLUDE_DIRS ${Editline_INCLUE_DIR})
set(Editline_LIBRARIES ${Editline_LIBRARY})