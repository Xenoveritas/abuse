#include <SDL3/SDL_stdinc.h>

#include "util.h"

char* join_strings(const char* s1, const char* s2)
{
    char* result;
    size_t len1, len2;
    // calculate needed length
    len1 = SDL_strlen(s1);
    len2 = SDL_strlen(s2);
    result = (char*) SDL_malloc(len1 + len2 + 1);
    // quit early if the alloc failed
    if (result == NULL)
        return result;
    SDL_strlcpy(result, s1, len1 + 1);
    SDL_strlcpy(result + len1, s2, len2 + 1);
    return result;
}