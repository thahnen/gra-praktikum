cmake_minimum_required(VERSION 2.8)

project(sampleDotNetRetrieveData)

string(REGEX REPLACE "/" "\\\\" ROYALE_DOT_NET_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}) 

configure_file(
    "sampleDotNetRetrieveData.csproj.in"
    "sampleDotNetRetrieveData.csproj"
    )

include_external_msproject(sampleDotNetRetrieveData 
    ${CMAKE_CURRENT_BINARY_DIR}/sampleDotNetRetrieveData.csproj
    PLATFORM ${MACHINE}
    RoyaleDotNet)
   
set_target_properties(sampleDotNetRetrieveData
    PROPERTIES
    OUTPUT_NAME sampleDotNetRetrieveData
    FOLDER sampleDotNetRetrieveData
    )
