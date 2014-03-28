
/**
 * This file include the global configuration for the compilation of this project and centralizes
 * macro declarations.
 *
 * These are the macros expected to be declared externally to configure the build process:
 *
 * PROJECT_DEBUG_BUILD
 *     Is expected to be declared for debug compilation and not declared for release.
 *       - Debug compilation includes more control and raise exceptions when something is detected
 *         to be wrong. Slower but more suitable for developing.
 *       - Release compilation does not check in runtime for possible errors and exceptions are not
 *         raised. Faster but more insecure when developing.
 *
 * PROJECT_PLATFORM_WIN32
 *     Declared in case this program is compiled for Win32.
 *
 * PROJECT_PLATFORM_UNIX
 *     Declared in case this program is compiled for Unix variants.
 *
 */

#ifndef CONF_H_
#define CONF_H_

/**
 * PROJECT_VERSION_STR_
 * This is the centralized version for this compilation. All part of this software should be
 * depending on this value instead of redefining it somewhere else to avoid inconsistencies
 */
#define PROJECT_VERSION_STR_ "0.1"


/**
 * PROJECT_PATH_FOLDER_SEPARATOR
 * String used to separate folders in paths for local file systems.
 */
#ifdef PROJECT_PLATFORM_WIN32
# define PROJECT_PATH_FOLDER_SEPARATOR "\\"
#else //PROJECT_PLATFORM_WIN32
# ifdef PROJECT_PLATFORM_UNIX
#  define PROJECT_PATH_FOLDER_SEPARATOR "/"
# else //PROJECT_PLATFORM_UNIX
#  error "Platform not declared"
# endif //PROJECT_PLATFORM_UNIX
#endif //PROJECT_PLATFORM_WIN32


/**
 * PROJECT_VERSION_STR
 * This is the centralized version for this compilation which also includes if the compilation is
 * for Debug or not. When it is not for debug, no extra information is added and should match
 * PROJECT_VERSION_STR_
 */
#ifdef PROJECT_DEBUG_BUILD
# define PROJECT_VERSION_STR PROJECT_VERSION_STR_ " (Debug)"
#else //PROJECT_DEBUG_BUILD
# define PROJECT_VERSION_STR PROJECT_VERSION_STR_
#endif //PROJECT_DEBUG_BUILD

/**
 * BOUNDED_INTEGERS_STRICT
 * When declared, make all ranged integers to be wrapped in a class to check valid values.
 */
#ifdef PROJECT_DEBUG_BUILD
# define BOUNDED_INTEGERS_STRICT
#endif //PROJECT_DEBUG_BUILD

#endif /* CONF_H_ */
