
/**
 * This file include the global configuration for the compilation of this project and centralizes
 * macro declarations.
 */

#ifndef CONF_H_
#define CONF_H_

#define PROJECT_VERSION_STR_ "0.1"

/**
 * PROJECT_DEBUG_BUILD is expected to be declared for debug compilation and not declared for release.
 *  - Debug compilation includes more control and raise exceptions when something is detected to be
 *    wrong. Slower but more suitable for developing.
 *  - Release compilation does not check in runtime for possible errors and exceptions are not
 *    raised. Faster but more insecure when developing.
 */
#ifdef PROJECT_DEBUG_BUILD

#define PROJECT_VERSION_STR PROJECT_VERSION_STR_ " (Debug)"

/** Make all ranged integers to be wrapped in a class to check valid values. */
#define BOUNDED_INTEGERS_STRICT

#else //PROJECT_DEBUG_BUILD

#define PROJECT_VERSION_STR PROJECT_VERSION_STR_

#endif //PROJECT_DEBUG_BUILD

#endif /* CONF_H_ */
