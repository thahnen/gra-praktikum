cmake_minimum_required(VERSION 2.8)

project(sampleDotNetCamInfo)

string(REGEX REPLACE "/" "\\\\" ROYALE_DOT_NET_SRC_DIR_CAMINFO ${CMAKE_CURRENT_SOURCE_DIR}) 

configure_file(
    "sampleDotNetCamInfo.csproj.in"
    "sampleDotNetCamInfo.csproj"
    )

include_external_msproject(sampleDotNetCamInfo 
    ${CMAKE_CURRENT_BINARY_DIR}/sampleDotNetCamInfo.csproj
    PLATFORM ${MACHINE}
    RoyaleDotNet)
   
set_target_properties(sampleDotNetCamInfo
    PROPERTIES
    OUTPUT_NAME sampleDotNetCamInfo
    FOLDER sampleDotNetCamInfo
    )
