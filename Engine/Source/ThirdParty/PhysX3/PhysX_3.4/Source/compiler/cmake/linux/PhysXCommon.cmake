#
# Build PhysXCommon
#

FIND_PACKAGE(nvToolsExt REQUIRED)

SET(PHYSX_SOURCE_DIR ${PROJECT_SOURCE_DIR}/../../../)

SET(COMMON_SRC_DIR ${PHYSX_SOURCE_DIR}/Common/src)
SET(GU_SOURCE_DIR ${PHYSX_SOURCE_DIR}/GeomUtils)

SET(PXCOMMON_PLATFORM_SRC_FILES 
)

SET(PXCOMMON_PLATFORM_INCLUDES
	${NVTOOLSEXT_INCLUDE_DIRS}
	${PHYSX_SOURCE_DIR}/Common/src/linux
)

IF(DEFINED PX_STATIC_LIBRARIES)
	SET(PHYSXCOMMON_LIBTYPE STATIC)
ELSE()
	SET(PHYSXCOMMON_LIBTYPE SHARED)
ENDIF()


SET(PXCOMMON_COMPILE_DEFS
	# Common to all configurations
	${PHYSX_LINUX_COMPILE_DEFS};${PHYSXCOMMON_SHARED_LIBRARY_FLAGS}
)

IF(DEFINED PX_STATIC_LIBRARIES)
	LIST(APPEND PXCOMMON_COMPILE_DEFS
		PX_PHYSX_STATIC_LIB;
	)
ELSE()
	LIST(APPEND PXCOMMON_COMPILE_DEFS
		PX_FOUNDATION_DLL=1;PX_PHYSX_COMMON_EXPORTS;
	)
ENDIF()


if(${CMAKE_BUILD_TYPE_LOWERCASE} STREQUAL "debug")
	LIST(APPEND PXCOMMON_COMPILE_DEFS
		${PHYSX_LINUX_DEBUG_COMPILE_DEFS};
	)
elseif(${CMAKE_BUILD_TYPE_LOWERCASE} STREQUAL "checked")
	LIST(APPEND PXCOMMON_COMPILE_DEFS
		${PHYSX_LINUX_CHECKED_COMPILE_DEFS};
	)
elseif(${CMAKE_BUILD_TYPE_LOWERCASE} STREQUAL "profile")
	LIST(APPEND PXCOMMON_COMPILE_DEFS
		${PHYSX_LINUX_PROFILE_COMPILE_DEFS};
	)
elseif(${CMAKE_BUILD_TYPE_LOWERCASE} STREQUAL "release")
	LIST(APPEND PXCOMMON_COMPILE_DEFS
		${PHYSX_LINUX_RELEASE_COMPILE_DEFS}
	)
else(${CMAKE_BUILD_TYPE_LOWERCASE} STREQUAL "debug")
	MESSAGE(FATAL_ERROR "Unknown configuration ${CMAKE_BUILD_TYPE}")
endif(${CMAKE_BUILD_TYPE_LOWERCASE} STREQUAL "debug")

# include common PhysXCommon settings
INCLUDE(../common/PhysXCommon.cmake)

# Do final direct sets after the target has been defined
IF(DEFINED PX_STATIC_LIBRARIES)
	TARGET_LINK_LIBRARIES(PhysXCommon PUBLIC ${NVTOOLSEXT_LIBRARIES} PxFoundation)
ELSE()
	TARGET_LINK_LIBRARIES(PhysXCommon PUBLIC ${NVTOOLSEXT_LIBRARIES} PxFoundation rt)
ENDIF()
SET_TARGET_PROPERTIES(PhysXCommon PROPERTIES LINK_FLAGS ""	)

# enable -fPIC so we can link static libs with the editor
SET_TARGET_PROPERTIES(PhysXCommon PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
