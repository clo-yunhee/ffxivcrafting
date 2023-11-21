set(SOURCES
    actions/Action.cc
    actions/ActionTable.cc
    model/Crafter.cc
    model/LevelTable.cc
    model/State.cc
    model/Synth.cc
    solver/montecarlo/MonteCarloSim.cc
    solver/simulation/SimSynth.cc
    solver/Fitness.cc
    solver/Solver.cc
    main.cc
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} csprng openGA)

if (CMAKE_BUILD_TYPE MATCHES "Debug")
    target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=address)
    target_link_options   (${PROJECT_NAME} PRIVATE -fsanitize=address)
elseif (CMAKE_BUILD_TYPE MATCHES "Release")
    target_compile_options(${PROJECT_NAME} PRIVATE -O3 -Ofast)
    target_link_options(${PROJECT_NAME} PRIVATE -O3 -Ofast -flto)
endif()