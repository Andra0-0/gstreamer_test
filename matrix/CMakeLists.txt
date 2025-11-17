set(
  MATRIX_HEADERS
  media_matrix.h
  input_intf.h
  input_impl_file.h
  input_impl_v4l2.h
)

set(
  MATRIX_SOURCES
  main.cpp
  media_matrix.cpp
  input_impl_file.cpp
  input_impl_v4l2.cpp
)

add_executable(matrix ${MATRIX_HEADERS} ${MATRIX_SOURCES})
target_link_libraries(matrix PUBLIC ${DEPS_LIBRARIES} pthread)