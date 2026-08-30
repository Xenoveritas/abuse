#ifndef SDLPORT_UTIL_H_

#define SDLPORT_UTIL_H_

/*
 * Basic utility to join strings. Returned string is allocated via SDL_malloc
 * and should be freed via SDL_free.
 */
char* join_strings(const char* s1, const char* s2);

#endif /* SDLPORT_UTIL_H_ */