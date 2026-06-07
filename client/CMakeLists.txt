add_executable(ClientApp
    main.cpp
    main.h
)

target_link_libraries(ClientApp PRIVATE
    Qt6::Core
    Qt6::Network
)