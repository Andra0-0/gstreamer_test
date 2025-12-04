set(
  MATRIX_HEADERS
  media_matrix.h
  noncopyable.h
  ipad_prober.h
  ithread.h
  meta_message.h
  output_intf.h
  output_impl_kms.h
  media_input_intf.h
  media_input_impl_image.h
  media_input_impl_hdmi.h
  media_input_impl_uridb.h
  media_input_module.h
)

set(
  MATRIX_SOURCES
  main.cpp
  media_matrix.cpp
  ipad_prober.cpp
  ithread.cpp
  meta_message.cpp
  output_impl_kms.cpp
  media_input_intf.cpp
  media_input_impl_image.cpp
  media_input_impl_hdmi.cpp
  media_input_impl_uridb.cpp
  media_input_module.cpp
)

add_executable(matrix ${MATRIX_HEADERS} ${MATRIX_SOURCES})
target_link_libraries(matrix PUBLIC ${DEPS_LIBRARIES} pthread drm)

set(
  MMXTERM_HEADERS
  noncopyable.h
  ithread.h
  console_terminal.h
)

set(
  MMXTERM_SOURCES
  ithread.cpp
  console_terminal.cpp
)

add_executable(mmxterm ${MMXTERM_HEADERS} ${MMXTERM_SOURCES})
target_link_libraries(mmxterm PUBLIC pthread)