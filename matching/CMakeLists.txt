set(SUBGRAPH_MATCHING_COMMON_FILES
        matchingcommand.cpp
        matchingcommand.h
        FilterVertices.cpp
        FilterVertices.h
        BuildTable.cpp
        BuildTable.h
        GenerateQueryPlan.cpp
        GenerateQueryPlan.h
        EvaluateQuery.cpp
        EvaluateQuery.h
        GenerateFilteringPlan.cpp
        GenerateFilteringPlan.h
        )

add_executable(main.out ${SUBGRAPH_MATCHING_COMMON_FILES} main.cpp)
target_link_libraries(main.out graph utility nd et gmpxx gmp)
