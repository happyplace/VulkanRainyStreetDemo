include(CheckCXXCompilerFlag)

function(Check_And_Add_Flag)
	set(TARGET ${ARGV0})
	set(FLAG ${ARGV1})

	# Check if it's a -Wno- flag
	if (FLAG MATCHES "^-Wno-")
		# special handling for -Wno- flags due to gcc weirdness
		# as we have to check the reverse flag before applying the one we want
		# Extract the name after -Wno-
		string (REGEX REPLACE "^-Wno-" "" NO_FLAG ${FLAG})

		# Sanitize to become cmake variable name
		string (REPLACE "-" "_" NAME ${NO_FLAG})
		string (FIND ${NAME} "=" EQUALS)
		string (SUBSTRING ${NAME} 0 ${EQUALS} NAME)
		set(NAME "Wno_${NAME}")
		CHECK_CXX_COMPILER_FLAG("-W${NO_FLAG}" ${NAME})
		if (${NAME})
			target_compile_options(${TARGET} PRIVATE ${FLAG})
		endif()
	else()
		# Sanitize flag to become cmake variable name
		string (REPLACE "-" "_" NAME ${FLAG})
		string (SUBSTRING ${NAME} 1 -1 NAME)
		string (FIND ${NAME} "=" EQUALS)
		string (SUBSTRING ${NAME} 0 ${EQUALS} CAN_USE_FLAG_NAME)

		CHECK_CXX_COMPILER_FLAG(${FLAG} ${NAME})
		if (${NAME})
			target_compile_options(${TARGET} PRIVATE ${FLAG})
		endif()
	endif()
endfunction()

function(AddCompilerFlags)
	set(TARGET ${ARGV0})

	if (("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU") OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
		Check_And_Add_Flag(${TARGET} -fdiagnostics-color=always)
		Check_And_Add_Flag(${TARGET} -Wall)
		Check_And_Add_Flag(${TARGET} -Wextra)
		Check_And_Add_Flag(${TARGET} -Wpedantic)
		Check_And_Add_Flag(${TARGET} -Wconversion)
		Check_And_Add_Flag(${TARGET} -Wsign-conversion)
		Check_And_Add_Flag(${TARGET} -Wcast-align)
		Check_And_Add_Flag(${TARGET} -Wcast-qual)
		# Check_And_Add_Flag(${TARGET} -Wctor-dtor-privacy) # causes issues on gcc 5/9 with catch2 under linux
		Check_And_Add_Flag(${TARGET} -Wdisabled-optimization)
		Check_And_Add_Flag(${TARGET} -Wdouble-promotion)
		Check_And_Add_Flag(${TARGET} -Wduplicated-branches)
		Check_And_Add_Flag(${TARGET} -Wduplicated-cond)
		Check_And_Add_Flag(${TARGET} -Wformat=2)
		Check_And_Add_Flag(${TARGET} -Wlogical-op)
		Check_And_Add_Flag(${TARGET} -Wmissing-include-dirs)
		Check_And_Add_Flag(${TARGET} -Wnoexcept)
		Check_And_Add_Flag(${TARGET} -Wnull-dereference)
		Check_And_Add_Flag(${TARGET} -Wold-style-cast)
		Check_And_Add_Flag(${TARGET} -Woverloaded-virtual)
		Check_And_Add_Flag(${TARGET} -Wshadow)
		Check_And_Add_Flag(${TARGET} -Wstrict-aliasing=1)
		Check_And_Add_Flag(${TARGET} -Wstrict-null-sentinel)
		Check_And_Add_Flag(${TARGET} -Wstrict-overflow=2)
		Check_And_Add_Flag(${TARGET} -Wswitch-default)
		Check_And_Add_Flag(${TARGET} -Wundef)
		Check_And_Add_Flag(${TARGET} -Wuseless-cast)
		Check_And_Add_Flag(${TARGET} -Wno-unknown-pragmas)
		Check_And_Add_Flag(${TARGET} -Wno-aligned-new)
		Check_And_Add_Flag(${TARGET} -Wno-aligned-new)
		if(("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU") AND (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.0))
			# Useless flag that hits a bunch of valid code
			Check_And_Add_Flag(${TARGET} -Wno-missing-field-initializers)
		endif()
		if(FTL_WERROR)
			Check_And_Add_Flag(${TARGET} -Werror)
		endif()
	else()
		Check_And_Add_Flag(${TARGET} /W4)
		Check_And_Add_Flag(${TARGET} /wd4324)
		if(FTL_WERROR)
			Check_And_Add_Flag(${TARGET} /WX)
		endif()
	endif()
endfunction()