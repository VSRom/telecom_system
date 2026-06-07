add_executable(ServerApp
    main.cpp
    serverwindow.h
    serverwindow.cpp
    serverworker.h
    serverworker.cpp
)

set_target_properties(ServerApp PROPERTIES
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(ServerApp PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Widgets
)