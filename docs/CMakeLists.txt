cmake_minimum_required(VERSION 3.22 FATAL_ERROR)

find_package(Doxygen)

if(DOXYGEN_FOUND)
	set(DOXYGEN_GENERATE_MAN NO)
	set(DOXYGEN_GENERATE_HTML YES)
	set(DOXYGEN_SHOW_NAMESPACES YES)
	set(DOXYGEN_EXTRACT_ALL YES)
	set(DOXYGEN_EXTRACT_PRIVATE YES)
	set(DOXYGEN_OUTPUT_LANGUAGE Russian)
	set(DOXYGEN_USE_MDFILE_AS_MAINPAGE README.md)

	doxygen_add_docs(docsbuild ALL ${PROJECT_SOURCE_DIR}/source ${PROJECT_SOURCE_DIR}/README.md)
	execute_process(COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.docsbuild)

	doxygen_add_docs(ghpages ALL source README.md)
	execute_process(
		COMMAND ${CMAKE_COMMAND} -E copy
		${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.ghpages
		${PROJECT_SOURCE_DIR}/Doxyfile.ghpages)
	
else(DOXYGEN_FOUND)
	message(STATUS "WARNING: Doxygen not found - Documentation will not be created")
endif(DOXYGEN_FOUND)

