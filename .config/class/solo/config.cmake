# ============================================================================
#       item            : CMake additional configuration for a node class
#       made by         : René van Paassen
#       date            : 180326
#       copyright       : (c) 2018 TUDelft-AE-C&S
# ============================================================================

# A "node" is a computer participating in a DUECA distributed process
# Each node requires a specific configuration, e.g., to show instruments,
# out-of-the-window view, perform IO with hardware. The software
# configuration for a node (chosen modules, libraries) is determined by
# it's "machine class"
#
# The "machine class" thus indicates what part of the application runs on this
# computer, examples are control_loading, ig, efis
#
# Per machine class, specify what libraries to link, additional DUECA
# components, compile flags etc.

# This file is "sourced" when dueca_adapt_machine is called.

# extend DUECA_COMPONENTS with additional components
option(USE_GTK3      "Use GTK3 for graphic interface + GL winows" ON)
option(USE_GTK4      "Use GTK3 for graphic interface + GL windows" OFF)

if(USE_GTK3)
    set(GUI_COMPONENT "gtk3")
endif()
if(USE_GTK4)
    set(GUI_COMPONENT "gtk4")
endif()

if(GUI_COMPONENT)
    list(APPEND DUECA_COMPONENTS ${GUI_COMPONENT})
endif()

# define PROJECT_LIBRARIES with libraries needed on the current platform,
# use CMAKE to detect these if needed
#set(PROJECT_LIBRARIES )

# define PROJECT_INCLUDE_DIRS with include directories common to all on
# the current platform
#set(PROJECT_INCLUDE_DIRS )

# define PROJECT_COMPILE_FLAGS with the flags needed for compiling
#set(PROJECT_COMPILE_FLAGS )
