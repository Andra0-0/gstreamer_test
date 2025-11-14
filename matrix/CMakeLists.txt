set(
  MATRIX_HEADERS
  media_matrix.h
)

set(
  MATRIX_SOURCES
  main.cpp
  media_matrix.cpp
)

add_executable(matrix ${MATRIX_SOURCES} ${MATRIX_HEADERS})
target_link_libraries(matrix PUBLIC ${DEPS_LIBRARIES} pthread)