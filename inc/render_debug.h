#ifndef _RENDER_DEBUG_H_
#define _RENDER_DEBUG_H_
//#define _DEBUG

#include <glad/glad.h>
#include <stdio.h>

inline void check_gl_error(char* tag) {
#ifdef _DEBUG
    const int error = glGetError();
    if (error != GL_NO_ERROR) {
        fprintf(stderr, "GL error [%s] error code: [%d]", tag, error);
    }
#endif
}

inline void check_gl_framebuffer_complete(char* tag) {
#ifdef _DEBUG
    const int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "Framebuffer isn't complete: %d", tag);
    }
#endif
}

#endif