
#include <glad/glad.h>
#include <stdio.h>
#include <render_debug.h>

void check_gl_error(char* tag) {
#ifdef _DEBUG
    const int error = glGetError();
    if (error != GL_NO_ERROR) {
        fprintf(stderr, "GL error [%s] error code: [%d]", tag, error);
    }
#endif
}

void check_gl_framebuffer_complete(char* tag) {
#ifdef _DEBUG
    const int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "Framebuffer isn't complete: %s", tag);
    }
#endif
}

