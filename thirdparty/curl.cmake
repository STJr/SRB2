if(TARGET CURL::libcurl)
    return()
endif()

message(STATUS "Third-party: creating target 'CURL::libcurl'")

set(CURL_ENABLE_EXPORT_TARGET OFF CACHE BOOL "" FORCE)

set(
	internal_curl_options

	"BUILD_CURL_EXE OFF"
	"BUILD_SHARED_LIBS OFF"
	"CURL_DISABLE_TESTS ON"
	"HTTP_ONLY ON"
	"CURL_DISABLE_CRYPTO_AUTH ON"
	"CURL_DISABLE_NTLM ON"
	"ENABLE_MANUAL OFF"
	"ENABLE_THREADED_RESOLVER OFF"
	"CURL_USE_LIBPSL OFF"
	"CURL_USE_LIBSSH2 OFF"
	"USE_LIBIDN2 OFF"
	"CURL_ENABLE_EXPORT_TARGET OFF"
)
if(${CMAKE_SYSTEM} MATCHES Windows)
	list(APPEND internal_curl_options "CURL_USE_OPENSSL OFF")
	list(APPEND internal_curl_options "CURL_USE_SCHANNEL ON")
endif()
if(${CMAKE_SYSTEM} MATCHES Darwin)
	list(APPEND internal_curl_options "CURL_USE_OPENSSL OFF")
	list(APPEND internal_curl_options "CURL_USE_SECTRANSP ON")
endif()
if(${CMAKE_SYSTEM} MATCHES Linux)
	list(APPEND internal_curl_options "CURL_USE_OPENSSL ON")
endif()

include(FetchContent)

if (CURL_USE_THIRDPARTY)
	FetchContent_Declare(
		curl
		VERSION 7.88.1
		GITHUB_REPOSITORY "curl/curl"
		GIT_TAG curl-7_88_1
		EXCLUDE_FROM_ALL ON
		OPTIONS ${internal_curl_options}
	)
else()
	FetchContent_Declare(
		curl
		SOURCE_DIR "${CMAKE_SOURCE_DIR}/thirdparty/curl/"
		EXCLUDE_FROM_ALL ON
		OPTIONS ${internal_curl_options}
	)
endif()

FetchContent_MakeAvailable(curl)

