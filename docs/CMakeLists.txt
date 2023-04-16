cmake_minimum_required(VERSION 3.22 FATAL_ERROR)

find_package(Doxygen)

if(DOXYGEN_FOUND)
#	set(DOXYGEN_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/docs)
#	set(doxy_html_index_file 	 ${PROJECT_SOURCE_DIR}/docs/html/index.html)
#	set(doxy_output_root     	 ${PROJECT_SOURCE_DIR}/docs)
#	set(DOXYGEN_README_FILE 	 ${PROJECT_SOURCE_DIR}/README.md)

	doxygen_add_docs(documentation ALL ../source)

	execute_process(COMMAND ${DOXYGEN_EXECUTABLE} ${PROJECT_SOURCE_DIR}/build/docs/Doxyfile.documentation)
else(DOXYGEN_FOUND)
	message(STATUS "WARNING: Doxygen not found - Documentation will not be created")
endif(DOXYGEN_FOUND)

