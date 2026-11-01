find_package(Qt6 REQUIRED COMPONENTS Core Test)

set(CMAKE_AUTOMOC ON)

add_executable(tst_dprscaling
  tst_dprscaling.cpp
  ${CMAKE_SOURCE_DIR}/src/utils/dprscaling.cpp
)

target_include_directories(tst_dprscaling PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(tst_dprscaling PRIVATE Qt6::Core Qt6::Test)

add_test(NAME dprscaling COMMAND tst_dprscaling)

add_executable(tst_resizehandles
  tst_resizehandles.cpp
  ${CMAKE_SOURCE_DIR}/src/utils/resizehandles.cpp
)

target_include_directories(tst_resizehandles PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(tst_resizehandles PRIVATE Qt6::Core Qt6::Test)

add_test(NAME resizehandles COMMAND tst_resizehandles)
