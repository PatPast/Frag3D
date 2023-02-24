#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glad/glad.h>

static void* get_proc(const char *namez);

#if defined(_WIN32) || defined(__CYGWIN__)
#ifndef _WINDOWS_
#undef APIENTRY
#endif
#include <windows.h>
static HMODULE libGL;

typedef void* (APIENTRYP PFNWGLGETPROCADDRESSPROC_PRIVATE)(const char*);
static PFNWGLGETPROCADDRESSPROC_PRIVATE gladGetProcAddressPtr;

#ifdef _MSC_VER
#ifdef __has_include
  #if __has_include(<winapifamily.h>)
    #define HAVE_WINAPIFAMILY 1
  #endif
#elif _MSC_VER >= 1700 && !_USING_V110_SDK71_
  #define HAVE_WINAPIFAMILY 1
#endif
#endif

#ifdef HAVE_WINAPIFAMILY
  #include <winapifamily.h>
  #if !WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)
    #define IS_UWP 1
  #endif
#endif

static
int open_gl(void) {
#ifndef IS_UWP
    libGL = LoadLibraryW(L"opengl32.dll");
    if(libGL != NULL) {
        void (* tmp)(void);
        tmp = (void(*)(void)) GetProcAddress(libGL, "wglGetProcAddress");
        gladGetProcAddressPtr = (PFNWGLGETPROCADDRESSPROC_PRIVATE) tmp;
        return gladGetProcAddressPtr != NULL;
    }
#endif

    return 0;
}

static
void close_gl(void) {
    if(libGL != NULL) {
        FreeLibrary((HMODULE) libGL);
        libGL = NULL;
    }
}
#else
#include <dlfcn.h>
static void* libGL;

#if !defined(__APPLE__) && !defined(__HAIKU__)
typedef void* (APIENTRYP PFNGLXGETPROCADDRESSPROC_PRIVATE)(const char*);
static PFNGLXGETPROCADDRESSPROC_PRIVATE gladGetProcAddressPtr;
#endif

static
int open_gl(void) {
#ifdef __APPLE__
    static const char *NAMES[] = {
        "../Frameworks/OpenGL.framework/OpenGL",
        "/Library/Frameworks/OpenGL.framework/OpenGL",
        "/System/Library/Frameworks/OpenGL.framework/OpenGL",
        "/System/Library/Frameworks/OpenGL.framework/Versions/Current/OpenGL"
    };
#else
    static const char *NAMES[] = {"libGL.so.1", "libGL.so"};
#endif

    unsigned int index = 0;
    for(index = 0; index < (sizeof(NAMES) / sizeof(NAMES[0])); index++) {
        libGL = dlopen(NAMES[index], RTLD_NOW | RTLD_GLOBAL);

        if(libGL != NULL) {
#if defined(__APPLE__) || defined(__HAIKU__)
            return 1;
#else
            gladGetProcAddressPtr = (PFNGLXGETPROCADDRESSPROC_PRIVATE)dlsym(libGL,
                "glXGetProcAddressARB");
            return gladGetProcAddressPtr != NULL;
#endif
        }
    }

    return 0;
}

static
void close_gl(void) {
    if(libGL != NULL) {
        dlclose(libGL);
        libGL = NULL;
    }
}
#endif

static
void* get_proc(const char *namez) {
    void* result = NULL;
    if(libGL == NULL) return NULL;

#if !defined(__APPLE__) && !defined(__HAIKU__)
    if(gladGetProcAddressPtr != NULL) {
        result = gladGetProcAddressPtr(namez);
    }
#endif
    if(result == NULL) {
#if defined(_WIN32) || defined(__CYGWIN__)
        result = (void*)GetProcAddress((HMODULE) libGL, namez);
#else
        result = dlsym(libGL, namez);
#endif
    }

    return result;
}

int gladLoadGL(void) {
    int status = 0;

    if(open_gl()) {
        status = gladLoadGLLoader(&get_proc);
        close_gl();
    }

    return status;
}

struct gladGLversionStruct GLVersion = { 0, 0 };

#if defined(GL_ES_VERSION_3_0) || defined(GL_VERSION_3_0)
#define _GLAD_IS_SOME_NEW_VERSION 1
#endif

static int max_loaded_major;
static int max_loaded_minor;

static const char *exts = NULL;
static int num_exts_i = 0;
static char **exts_i = NULL;

static int get_exts(void) {
#ifdef _GLAD_IS_SOME_NEW_VERSION
    if(max_loaded_major < 3) {
#endif
        exts = (const char *)glGetString(GL_EXTENSIONS);
#ifdef _GLAD_IS_SOME_NEW_VERSION
    } else {
        unsigned int index;

        num_exts_i = 0;
        glGetIntegerv(GL_NUM_EXTENSIONS, &num_exts_i);
        if (num_exts_i > 0) {
            exts_i = (char **)malloc((size_t)num_exts_i * (sizeof *exts_i));
        }

        if (exts_i == NULL) {
            return 0;
        }

        for(index = 0; index < (unsigned)num_exts_i; index++) {
            const char *gl_str_tmp = (const char*)glGetStringi(GL_EXTENSIONS, index);
            size_t len = strlen(gl_str_tmp);

            char *local_str = (char*)malloc((len+1) * sizeof(char));
            if(local_str != NULL) {
                memcpy(local_str, gl_str_tmp, (len+1) * sizeof(char));
            }
            exts_i[index] = local_str;
        }
    }
#endif
    return 1;
}

static void free_exts(void) {
    if (exts_i != NULL) {
        int index;
        for(index = 0; index < num_exts_i; index++) {
            free((char *)exts_i[index]);
        }
        free((void *)exts_i);
        exts_i = NULL;
    }
}

static int has_ext(const char *ext) {
#ifdef _GLAD_IS_SOME_NEW_VERSION
    if(max_loaded_major < 3) {
#endif
        const char *extensions;
        const char *loc;
        const char *terminator;
        extensions = exts;
        if(extensions == NULL || ext == NULL) {
            return 0;
        }

        while(1) {
            loc = strstr(extensions, ext);
            if(loc == NULL) {
                return 0;
            }

            terminator = loc + strlen(ext);
            if((loc == extensions || *(loc - 1) == ' ') &&
                (*terminator == ' ' || *terminator == '\0')) {
                return 1;
            }
            extensions = terminator;
        }
#ifdef _GLAD_IS_SOME_NEW_VERSION
    } else {
        int index;
        if(exts_i == NULL) return 0;
        for(index = 0; index < num_exts_i; index++) {
            const char *e = exts_i[index];

            if(exts_i[index] != NULL && strcmp(e, ext) == 0) {
                return 1;
            }
        }
    }
#endif

    return 0;
}
int GLAD_GL_VERSION_1_0 = 0;
int GLAD_GL_VERSION_1_1 = 0;
int GLAD_GL_VERSION_1_2 = 0;
int GLAD_GL_VERSION_1_3 = 0;
int GLAD_GL_VERSION_1_4 = 0;
int GLAD_GL_VERSION_1_5 = 0;
int GLAD_GL_VERSION_2_0 = 0;
int GLAD_GL_VERSION_2_1 = 0;
int GLAD_GL_VERSION_3_0 = 0;
int GLAD_GL_VERSION_3_1 = 0;
int GLAD_GL_VERSION_3_2 = 0;
int GLAD_GL_VERSION_3_3 = 0;
PFNGLACTIVETEXTUREPROC glad_glActiveTexture = NULL;
PFNGLATTACHSHADERPROC glad_glAttachShader = NULL;
PFNGLBEGINCONDITIONALRENDERPROC glad_glBeginConditionalRender = NULL;
PFNGLBEGINQUERYPROC glad_glBeginQuery = NULL;
PFNGLBEGINTRANSFORMFEEDBACKPROC glad_glBeginTransformFeedback = NULL;
PFNGLBINDATTRIBLOCATIONPROC glad_glBindAttribLocation = NULL;
PFNGLBINDBUFFERPROC glad_glBindBuffer = NULL;
PFNGLBINDBUFFERBASEPROC glad_glBindBufferBase = NULL;
PFNGLBINDBUFFERRANGEPROC glad_glBindBufferRange = NULL;
PFNGLBINDFRAGDATALOCATIONPROC glad_glBindFragDataLocation = NULL;
PFNGLBINDFRAGDATALOCATIONINDEXEDPROC glad_glBindFragDataLocationIndexed = NULL;
PFNGLBINDFRAMEBUFFERPROC glad_glBindFramebuffer = NULL;
PFNGLBINDRENDERBUFFERPROC glad_glBindRenderbuffer = NULL;
PFNGLBINDSAMPLERPROC glad_glBindSampler = NULL;
PFNGLBINDTEXTUREPROC glad_glBindTexture = NULL;
PFNGLBINDVERTEXARRAYPROC glad_glBindVertexArray = NULL;
PFNGLBLENDCOLORPROC glad_glBlendColor = NULL;
PFNGLBLENDEQUATIONPROC glad_glBlendEquation = NULL;
PFNGLBLENDEQUATIONSEPARATEPROC glad_glBlendEquationSeparate = NULL;
PFNGLBLENDFUNCPROC glad_glBlendFunc = NULL;
PFNGLBLENDFUNCSEPARATEPROC glad_glBlendFuncSeparate = NULL;
PFNGLBLITFRAMEBUFFERPROC glad_glBlitFramebuffer = NULL;
PFNGLBUFFERDATAPROC glad_glBufferData = NULL;
PFNGLBUFFERSUBDATAPROC glad_glBufferSubData = NULL;
PFNGLCHECKFRAMEBUFFERSTATUSPROC glad_glCheckFramebufferStatus = NULL;
PFNGLCLAMPCOLORPROC glad_glClampColor = NULL;
PFNGLCLEARPROC glad_glClear = NULL;
PFNGLCLEARBUFFERFIPROC glad_glClearBufferfi = NULL;
PFNGLCLEARBUFFERFVPROC glad_glClearBufferfv = NULL;
PFNGLCLEARBUFFERIVPROC glad_glClearBufferiv = NULL;
PFNGLCLEARBUFFERUIVPROC glad_glClearBufferuiv = NULL;
PFNGLCLEARCOLORPROC glad_glClearColor = NULL;
PFNGLCLEARDEPTHPROC glad_glClearDepth = NULL;
PFNGLCLEARSTENCILPROC glad_glClearStencil = NULL;
PFNGLCLIENTWAITSYNCPROC glad_glClientWaitSync = NULL;
PFNGLCOLORMASKPROC glad_glColorMask = NULL;
PFNGLCOLORMASKIPROC glad_glColorMaski = NULL;
PFNGLCOLORP3UIPROC glad_glColorP3ui = NULL;
PFNGLCOLORP3UIVPROC glad_glColorP3uiv = NULL;
PFNGLCOLORP4UIPROC glad_glColorP4ui = NULL;
PFNGLCOLORP4UIVPROC glad_glColorP4uiv = NULL;
PFNGLCOMPILESHADERPROC glad_glCompileShader = NULL;
PFNGLCOMPRESSEDTEXIMAGE1DPROC glad_glCompressedTexImage1D = NULL;
PFNGLCOMPRESSEDTEXIMAGE2DPROC glad_glCompressedTexImage2D = NULL;
PFNGLCOMPRESSEDTEXIMAGE3DPROC glad_glCompressedTexImage3D = NULL;
PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC glad_glCompressedTexSubImage1D = NULL;
PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC glad_glCompressedTexSubImage2D = NULL;
PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC glad_glCompressedTexSubImage3D = NULL;
PFNGLCOPYBUFFERSUBDATAPROC glad_glCopyBufferSubData = NULL;
PFNGLCOPYTEXIMAGE1DPROC glad_glCopyTexImage1D = NULL;
PFNGLCOPYTEXIMAGE2DPROC glad_glCopyTexImage2D = NULL;
PFNGLCOPYTEXSUBIMAGE1DPROC glad_glCopyTexSubImage1D = NULL;
PFNGLCOPYTEXSUBIMAGE2DPROC glad_glCopyTexSubImage2D = NULL;
PFNGLCOPYTEXSUBIMAGE3DPROC glad_glCopyTexSubImage3D = NULL;
PFNGLCREATEPROGRAMPROC glad_glCreateProgram = NULL;
PFNGLCREATESHADERPROC glad_glCreateShader = NULL;
PFNGLCULLFACEPROC glad_glCullFace = NULL;
PFNGLDELETEBUFFERSPROC glad_glDeleteBuffers = NULL;
PFNGLDELETEFRAMEBUFFERSPROC glad_glDeleteFramebuffers = NULL;
PFNGLDELETEPROGRAMPROC glad_glDeleteProgram = NULL;
PFNGLDELETEQUERIESPROC glad_glDeleteQueries = NULL;
PFNGLDELETERENDERBUFFERSPROC glad_glDeleteRenderbuffers = NULL;
PFNGLDELETESAMPLERSPROC glad_glDeleteSamplers = NULL;
PFNGLDELETESHADERPROC glad_glDeleteShader = NULL;
PFNGLDELETESYNCPROC glad_glDeleteSync = NULL;
PFNGLDELETETEXTURESPROC glad_glDeleteTextures = NULL;
PFNGLDELETEVERTEXARRAYSPROC glad_glDeleteVertexArrays = NULL;
PFNGLDEPTHFUNCPROC glad_glDepthFunc = NULL;
PFNGLDEPTHMASKPROC glad_glDepthMask = NULL;
PFNGLDEPTHRANGEPROC glad_glDepthRange = NULL;
PFNGLDETACHSHADERPROC glad_glDetachShader = NULL;
PFNGLDISABLEPROC glad_glDisable = NULL;
PFNGLDISABLEVERTEXATTRIBARRAYPROC glad_glDisableVertexAttribArray = NULL;
PFNGLDISABLEIPROC glad_glDisablei = NULL;
PFNGLDRAWARRAYSPROC glad_glDrawArrays = NULL;
PFNGLDRAWARRAYSINSTANCEDPROC glad_glDrawArraysInstanced = NULL;
PFNGLDRAWBUFFERPROC glad_glDrawBuffer = NULL;
PFNGLDRAWBUFFERSPROC glad_glDrawBuffers = NULL;
PFNGLDRAWELEMENTSPROC glad_glDrawElements = NULL;
PFNGLDRAWELEMENTSBASEVERTEXPROC glad_glDrawElementsBaseVertex = NULL;
PFNGLDRAWELEMENTSINSTANCEDPROC glad_glDrawElementsInstanced = NULL;
PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC glad_glDrawElementsInstancedBaseVertex = NULL;
PFNGLDRAWRANGEELEMENTSPROC glad_glDrawRangeElements = NULL;
PFNGLDRAWRANGEELEMENTSBASEVERTEXPROC glad_glDrawRangeElementsBaseVertex = NULL;
PFNGLENABLEPROC glad_glEnable = NULL;
PFNGLENABLEVERTEXATTRIBARRAYPROC glad_glEnableVertexAttribArray = NULL;
PFNGLENABLEIPROC glad_glEnablei = NULL;
PFNGLENDCONDITIONALRENDERPROC glad_glEndConditionalRender = NULL;
PFNGLENDQUERYPROC glad_glEndQuery = NULL;
PFNGLENDTRANSFORMFEEDBACKPROC glad_glEndTransformFeedback = NULL;
PFNGLFENCESYNCPROC glad_glFenceSync = NULL;
PFNGLFINISHPROC glad_glFinish = NULL;
PFNGLFLUSHPROC glad_glFlush = NULL;
PFNGLFLUSHMAPPEDBUFFERRANGEPROC glad_glFlushMappedBufferRange = NULL;
PFNGLFRAMEBUFFERRENDERBUFFERPROC glad_glFramebufferRenderbuffer = NULL;
PFNGLFRAMEBUFFERTEXTUREPROC glad_glFramebufferTexture = NULL;
PFNGLFRAMEBUFFERTEXTURE1DPROC glad_glFramebufferTexture1D = NULL;
PFNGLFRAMEBUFFERTEXTURE2DPROC glad_glFramebufferTexture2D = NULL;
PFNGLFRAMEBUFFERTEXTURE3DPROC glad_glFramebufferTexture3D = NULL;
PFNGLFRAMEBUFFERTEXTURELAYERPROC glad_glFramebufferTextureLayer = NULL;
PFNGLFRONTFACEPROC glad_glFrontFace = NULL;
PFNGLGENBUFFERSPROC glad_glGenBuffers = NULL;
PFNGLGENFRAMEBUFFERSPROC glad_glGenFramebuffers = NULL;
PFNGLGENQUERIESPROC glad_glGenQueries = NULL;
PFNGLGENRENDERBUFFERSPROC glad_glGenRenderbuffers = NULL;
PFNGLGENSAMPLERSPROC glad_glGenSamplers = NULL;
PFNGLGENTEXTURESPROC glad_glGenTextures = NULL;
PFNGLGENVERTEXARRAYSPROC glad_glGenVertexArrays = NULL;
PFNGLGENERATEMIPMAPPROC glad_glGenerateMipmap = NULL;
PFNGLGETACTIVEATTRIBPROC glad_glGetActiveAttrib = NULL;
PFNGLGETACTIVEUNIFORMPROC glad_glGetActiveUniform = NULL;
PFNGLGETACTIVEUNIFORMBLOCKNAMEPROC glad_glGetActiveUniformBlockName = NULL;
PFNGLGETACTIVEUNIFORMBLOCKIVPROC glad_glGetActiveUniformBlockiv = NULL;
PFNGLGETACTIVEUNIFORMNAMEPROC glad_glGetActiveUniformName = NULL;
PFNGLGETACTIVEUNIFORMSIVPROC glad_glGetActiveUniformsiv = NULL;
PFNGLGETATTACHEDSHADERSPROC glad_glGetAttachedShaders = NULL;
PFNGLGETATTRIBLOCATIONPROC glad_glGetAttribLocation = NULL;
PFNGLGETBOOLEANI_VPROC glad_glGetBooleani_v = NULL;
PFNGLGETBOOLEANVPROC glad_glGetBooleanv = NULL;
PFNGLGETBUFFERPARAMETERI64VPROC glad_glGetBufferParameteri64v = NULL;
PFNGLGETBUFFERPARAMETERIVPROC glad_glGetBufferParameteriv = NULL;
PFNGLGETBUFFERPOINTERVPROC glad_glGetBufferPointerv = NULL;
PFNGLGETBUFFERSUBDATAPROC glad_glGetBufferSubData = NULL;
PFNGLGETCOMPRESSEDTEXIMAGEPROC glad_glGetCompressedTexImage = NULL;
PFNGLGETDOUBLEVPROC glad_glGetDoublev = NULL;
PFNGLGETERRORPROC glad_glGetError = NULL;
PFNGLGETFLOATVPROC glad_glGetFloatv = NULL;
PFNGLGETFRAGDATAINDEXPROC glad_glGetFragDataIndex = NULL;
PFNGLGETFRAGDATALOCATIONPROC glad_glGetFragDataLocation = NULL;
PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC glad_glGetFramebufferAttachmentParameteriv = NULL;
PFNGLGETINTEGER64I_VPROC glad_glGetInteger64i_v = NULL;
PFNGLGETINTEGER64VPROC glad_glGetInteger64v = NULL;
PFNGLGETINTEGERI_VPROC glad_glGetIntegeri_v = NULL;
PFNGLGETINTEGERVPROC glad_glGetIntegerv = NULL;
PFNGLGETMULTISAMPLEFVPROC glad_glGetMultisamplefv = NULL;
PFNGLGETPROGRAMINFOLOGPROC glad_glGetProgramInfoLog = NULL;
PFNGLGETPROGRAMIVPROC glad_glGetProgramiv = NULL;
PFNGLGETQUERYOBJECTI64VPROC glad_glGetQueryObjecti64v = NULL;
PFNGLGETQUERYOBJECTIVPROC glad_glGetQueryObjectiv = NULL;
PFNGLGETQUERYOBJECTUI64VPROC glad_glGetQueryObjectui64v = NULL;
PFNGLGETQUERYOBJECTUIVPROC glad_glGetQueryObjectuiv = NULL;
PFNGLGETQUERYIVPROC glad_glGetQueryiv = NULL;
PFNGLGETRENDERBUFFERPARAMETERIVPROC glad_glGetRenderbufferParameteriv = NULL;
PFNGLGETSAMPLERPARAMETERIIVPROC glad_glGetSamplerParameterIiv = NULL;
PFNGLGETSAMPLERPARAMETERIUIVPROC glad_glGetSamplerParameterIuiv = NULL;
PFNGLGETSAMPLERPARAMETERFVPROC glad_glGetSamplerParameterfv = NULL;
PFNGLGETSAMPLERPARAMETERIVPROC glad_glGetSamplerParameteriv = NULL;
PFNGLGETSHADERINFOLOGPROC glad_glGetShaderInfoLog = NULL;
PFNGLGETSHADERSOURCEPROC glad_glGetShaderSource = NULL;
PFNGLGETSHADERIVPROC glad_glGetShaderiv = NULL;
PFNGLGETSTRINGPROC glad_glGetString = NULL;
PFNGLGETSTRINGIPROC glad_glGetStringi = NULL;
PFNGLGETSYNCIVPROC glad_glGetSynciv = NULL;
PFNGLGETTEXIMAGEPROC glad_glGetTexImage = NULL;
PFNGLGETTEXLEVELPARAMETERFVPROC glad_glGetTexLevelParameterfv = NULL;
PFNGLGETTEXLEVELPARAMETERIVPROC glad_glGetTexLevelParameteriv = NULL;
PFNGLGETTEXPARAMETERIIVPROC glad_glGetTexParameterIiv = NULL;
PFNGLGETTEXPARAMETERIUIVPROC glad_glGetTexParameterIuiv = NULL;
PFNGLGETTEXPARAMETERFVPROC glad_glGetTexParameterfv = NULL;
PFNGLGETTEXPARAMETERIVPROC glad_glGetTexParameteriv = NULL;
PFNGLGETTRANSFORMFEEDBACKVARYINGPROC glad_glGetTransformFeedbackVarying = NULL;
PFNGLGETUNIFORMBLOCKINDEXPROC glad_glGetUniformBlockIndex = NULL;
PFNGLGETUNIFORMINDICESPROC glad_glGetUniformIndices = NULL;
PFNGLGETUNIFORMLOCATIONPROC glad_glGetUniformLocation = NULL;
PFNGLGETUNIFORMFVPROC glad_glGetUniformfv = NULL;
PFNGLGETUNIFORMIVPROC glad_glGetUniformiv = NULL;
PFNGLGETUNIFORMUIVPROC glad_glGetUniformuiv = NULL;
PFNGLGETVERTEXATTRIBIIVPROC glad_glGetVertexAttribIiv = NULL;
PFNGLGETVERTEXATTRIBIUIVPROC glad_glGetVertexAttribIuiv = NULL;
PFNGLGETVERTEXATTRIBPOINTERVPROC glad_glGetVertexAttribPointerv = NULL;
PFNGLGETVERTEXATTRIBDVPROC glad_glGetVertexAttribdv = NULL;
PFNGLGETVERTEXATTRIBFVPROC glad_glGetVertexAttribfv = NULL;
PFNGLGETVERTEXATTRIBIVPROC glad_glGetVertexAttribiv = NULL;
PFNGLHINTPROC glad_glHint = NULL;
PFNGLISBUFFERPROC glad_glIsBuffer = NULL;
PFNGLISENABLEDPROC glad_glIsEnabled = NULL;
PFNGLISENABLEDIPROC glad_glIsEnabledi = NULL;
PFNGLISFRAMEBUFFERPROC glad_glIsFramebuffer = NULL;
PFNGLISPROGRAMPROC glad_glIsProgram = NULL;
PFNGLISQUERYPROC glad_glIsQuery = NULL;
PFNGLISRENDERBUFFERPROC glad_glIsRenderbuffer = NULL;
PFNGLISSAMPLERPROC glad_glIsSampler = NULL;
PFNGLISSHADERPROC glad_glIsShader = NULL;
PFNGLISSYNCPROC glad_glIsSync = NULL;
PFNGLISTEXTUREPROC glad_glIsTexture = NULL;
PFNGLISVERTEXARRAYPROC glad_glIsVertexArray = NULL;
PFNGLLINEWIDTHPROC glad_glLineWidth = NULL;
PFNGLLINKPROGRAMPROC glad_glLinkProgram = NULL;
PFNGLLOGICOPPROC glad_glLogicOp = NULL;
PFNGLMAPBUFFERPROC glad_glMapBuffer = NULL;
PFNGLMAPBUFFERRANGEPROC glad_glMapBufferRange = NULL;
PFNGLMULTIDRAWARRAYSPROC glad_glMultiDrawArrays = NULL;
PFNGLMULTIDRAWELEMENTSPROC glad_glMultiDrawElements = NULL;
PFNGLMULTIDRAWELEMENTSBASEVERTEXPROC glad_glMultiDrawElementsBaseVertex = NULL;
PFNGLMULTITEXCOORDP1UIPROC glad_glMultiTexCoordP1ui = NULL;
PFNGLMULTITEXCOORDP1UIVPROC glad_glMultiTexCoordP1uiv = NULL;
PFNGLMULTITEXCOORDP2UIPROC glad_glMultiTexCoordP2ui = NULL;
PFNGLMULTITEXCOORDP2UIVPROC glad_glMultiTexCoordP2uiv = NULL;
PFNGLMULTITEXCOORDP3UIPROC glad_glMultiTexCoordP3ui = NULL;
PFNGLMULTITEXCOORDP3UIVPROC glad_glMultiTexCoordP3uiv = NULL;
PFNGLMULTITEXCOORDP4UIPROC glad_glMultiTexCoordP4ui = NULL;
PFNGLMULTITEXCOORDP4UIVPROC glad_glMultiTexCoordP4uiv = NULL;
PFNGLNORMALP3UIPROC glad_glNormalP3ui = NULL;
PFNGLNORMALP3UIVPROC glad_glNormalP3uiv = NULL;
PFNGLPIXELSTOREFPROC glad_glPixelStoref = NULL;
PFNGLPIXELSTOREIPROC glad_glPixelStorei = NULL;
PFNGLPOINTPARAMETERFPROC glad_glPointParameterf = NULL;
PFNGLPOINTPARAMETERFVPROC glad_glPointParameterfv = NULL;
PFNGLPOINTPARAMETERIPROC glad_glPointParameteri = NULL;
PFNGLPOINTPARAMETERIVPROC glad_glPointParameteriv = NULL;
PFNGLPOINTSIZEPROC glad_glPointSize = NULL;
PFNGLPOLYGONMODEPROC glad_glPolygonMode = NULL;
PFNGLPOLYGONOFFSETPROC glad_glPolygonOffset = NULL;
PFNGLPRIMITIVERESTARTINDEXPROC glad_glPrimitiveRestartIndex = NULL;
PFNGLPROVOKINGVERTEXPROC glad_glProvokingVertex = NULL;
PFNGLQUERYCOUNTERPROC glad_glQueryCounter = NULL;
PFNGLREADBUFFERPROC glad_glReadBuffer = NULL;
PFNGLREADPIXELSPROC glad_glReadPixels = NULL;
PFNGLRENDERBUFFERSTORAGEPROC glad_glRenderbufferStorage = NULL;
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC glad_glRenderbufferStorageMultisample = NULL;
PFNGLSAMPLECOVERAGEPROC glad_glSampleCoverage = NULL;
PFNGLSAMPLEMASKIPROC glad_glSampleMaski = NULL;
PFNGLSAMPLERPARAMETERIIVPROC glad_glSamplerParameterIiv = NULL;
PFNGLSAMPLERPARAMETERIUIVPROC glad_glSamplerParameterIuiv = NULL;
PFNGLSAMPLERPARAMETERFPROC glad_glSamplerParameterf = NULL;
PFNGLSAMPLERPARAMETERFVPROC glad_glSamplerParameterfv = NULL;
PFNGLSAMPLERPARAMETERIPROC glad_glSamplerParameteri = NULL;
PFNGLSAMPLERPARAMETERIVPROC glad_glSamplerParameteriv = NULL;
PFNGLSCISSORPROC glad_glScissor = NULL;
PFNGLSECONDARYCOLORP3UIPROC glad_glSecondaryColorP3ui = NULL;
PFNGLSECONDARYCOLORP3UIVPROC glad_glSecondaryColorP3uiv = NULL;
PFNGLSHADERSOURCEPROC glad_glShaderSource = NULL;
PFNGLSTENCILFUNCPROC glad_glStencilFunc = NULL;
PFNGLSTENCILFUNCSEPARATEPROC glad_glStencilFuncSeparate = NULL;
PFNGLSTENCILMASKPROC glad_glStencilMask = NULL;
PFNGLSTENCILMASKSEPARATEPROC glad_glStencilMaskSeparate = NULL;
PFNGLSTENCILOPPROC glad_glStencilOp = NULL;
PFNGLSTENCILOPSEPARATEPROC glad_glStencilOpSeparate = NULL;
PFNGLTEXBUFFERPROC glad_glTexBuffer = NULL;
PFNGLTEXCOORDP1UIPROC glad_glTexCoordP1ui = NULL;
PFNGLTEXCOORDP1UIVPROC glad_glTexCoordP1uiv = NULL;
PFNGLTEXCOORDP2UIPROC glad_glTexCoordP2ui = NULL;
PFNGLTEXCOORDP2UIVPROC glad_glTexCoordP2uiv = NULL;
PFNGLTEXCOORDP3UIPROC glad_glTexCoordP3ui = NULL;
PFNGLTEXCOORDP3UIVPROC glad_glTexCoordP3uiv = NULL;
PFNGLTEXCOORDP4UIPROC glad_glTexCoordP4ui = NULL;
PFNGLTEXCOORDP4UIVPROC glad_glTexCoordP4uiv = NULL;
PFNGLTEXIMAGE1DPROC glad_glTexImage1D = NULL;
PFNGLTEXIMAGE2DPROC glad_glTexImage2D = NULL;
PFNGLTEXIMAGE2DMULTISAMPLEPROC glad_glTexImage2DMultisample = NULL;
PFNGLTEXIMAGE3DPROC glad_glTexImage3D = NULL;
PFNGLTEXIMAGE3DMULTISAMPLEPROC glad_glTexImage3DMultisample = NULL;
PFNGLTEXPARAMETERIIVPROC glad_glTexParameterIiv = NULL;
PFNGLTEXPARAMETERIUIVPROC glad_glTexParameterIuiv = NULL;
PFNGLTEXPARAMETERFPROC glad_glTexParameterf = NULL;
PFNGLTEXPARAMETERFVPROC glad_glTexParameterfv = NULL;
PFNGLTEXPARAMETERIPROC glad_glTexParameteri = NULL;
PFNGLTEXPARAMETERIVPROC glad_glTexParameteriv = NULL;
PFNGLTEXSUBIMAGE1DPROC glad_glTexSubImage1D = NULL;
PFNGLTEXSUBIMAGE2DPROC glad_glTexSubImage2D = NULL;
PFNGLTEXSUBIMAGE3DPROC glad_glTexSubImage3D = NULL;
PFNGLTRANSFORMFEEDBACKVARYINGSPROC glad_glTransformFeedbackVaryings = NULL;
PFNGLUNIFORM1FPROC glad_glUniform1f = NULL;
PFNGLUNIFORM1FVPROC glad_glUniform1fv = NULL;
PFNGLUNIFORM1IPROC glad_glUniform1i = NULL;
PFNGLUNIFORM1IVPROC glad_glUniform1iv = NULL;
PFNGLUNIFORM1UIPROC glad_glUniform1ui = NULL;
PFNGLUNIFORM1UIVPROC glad_glUniform1uiv = NULL;
PFNGLUNIFORM2FPROC glad_glUniform2f = NULL;
PFNGLUNIFORM2FVPROC glad_glUniform2fv = NULL;
PFNGLUNIFORM2IPROC glad_glUniform2i = NULL;
PFNGLUNIFORM2IVPROC glad_glUniform2iv = NULL;
PFNGLUNIFORM2UIPROC glad_glUniform2ui = NULL;
PFNGLUNIFORM2UIVPROC glad_glUniform2uiv = NULL;
PFNGLUNIFORM3FPROC glad_glUniform3f = NULL;
PFNGLUNIFORM3FVPROC glad_glUniform3fv = NULL;
PFNGLUNIFORM3IPROC glad_glUniform3i = NULL;
PFNGLUNIFORM3IVPROC glad_glUniform3iv = NULL;
PFNGLUNIFORM3UIPROC glad_glUniform3ui = NULL;
PFNGLUNIFORM3UIVPROC glad_glUniform3uiv = NULL;
PFNGLUNIFORM4FPROC glad_glUniform4f = NULL;
PFNGLUNIFORM4FVPROC glad_glUniform4fv = NULL;
PFNGLUNIFORM4IPROC glad_glUniform4i = NULL;
PFNGLUNIFORM4IVPROC glad_glUniform4iv = NULL;
PFNGLUNIFORM4UIPROC glad_glUniform4ui = NULL;
PFNGLUNIFORM4UIVPROC glad_glUniform4uiv = NULL;
PFNGLUNIFORMBLOCKBINDINGPROC glad_glUniformBlockBinding = NULL;
PFNGLUNIFORMMATRIX2FVPROC glad_glUniformMatrix2fv = NULL;
PFNGLUNIFORMMATRIX2X3FVPROC glad_glUniformMatrix2x3fv = NULL;
PFNGLUNIFORMMATRIX2X4FVPROC glad_glUniformMatrix2x4fv = NULL;
PFNGLUNIFORMMATRIX3FVPROC glad_glUniformMatrix3fv = NULL;
PFNGLUNIFORMMATRIX3X2FVPROC glad_glUniformMatrix3x2fv = NULL;
PFNGLUNIFORMMATRIX3X4FVPROC glad_glUniformMatrix3x4fv = NULL;
PFNGLUNIFORMMATRIX4FVPROC glad_glUniformMatrix4fv = NULL;
PFNGLUNIFORMMATRIX4X2FVPROC glad_glUniformMatrix4x2fv = NULL;
PFNGLUNIFORMMATRIX4X3FVPROC glad_glUniformMatrix4x3fv = NULL;
PFNGLUNMAPBUFFERPROC glad_glUnmapBuffer = NULL;
PFNGLUSEPROGRAMPROC glad_glUseProgram = NULL;
PFNGLVALIDATEPROGRAMPROC glad_glValidateProgram = NULL;
PFNGLVERTEXATTRIB1DPROC glad_glVertexAttrib1d = NULL;
PFNGLVERTEXATTRIB1DVPROC glad_glVertexAttrib1dv = NULL;
PFNGLVERTEXATTRIB1FPROC glad_glVertexAttrib1f = NULL;
PFNGLVERTEXATTRIB1FVPROC glad_glVertexAttrib1fv = NULL;
PFNGLVERTEXATTRIB1SPROC glad_glVertexAttrib1s = NULL;
PFNGLVERTEXATTRIB1SVPROC glad_glVertexAttrib1sv = NULL;
PFNGLVERTEXATTRIB2DPROC glad_glVertexAttrib2d = NULL;
PFNGLVERTEXATTRIB2DVPROC glad_glVertexAttrib2dv = NULL;
PFNGLVERTEXATTRIB2FPROC glad_glVertexAttrib2f = NULL;
PFNGLVERTEXATTRIB2FVPROC glad_glVertexAttrib2fv = NULL;
PFNGLVERTEXATTRIB2SPROC glad_glVertexAttrib2s = NULL;
PFNGLVERTEXATTRIB2SVPROC glad_glVertexAttrib2sv = NULL;
PFNGLVERTEXATTRIB3DPROC glad_glVertexAttrib3d = NULL;
PFNGLVERTEXATTRIB3DVPROC glad_glVertexAttrib3dv = NULL;
PFNGLVERTEXATTRIB3FPROC glad_glVertexAttrib3f = NULL;
PFNGLVERTEXATTRIB3FVPROC glad_glVertexAttrib3fv = NULL;
PFNGLVERTEXATTRIB3SPROC glad_glVertexAttrib3s = NULL;
PFNGLVERTEXATTRIB3SVPROC glad_glVertexAttrib3sv = NULL;
PFNGLVERTEXATTRIB4NBVPROC glad_glVertexAttrib4Nbv = NULL;
PFNGLVERTEXATTRIB4NIVPROC glad_glVertexAttrib4Niv = NULL;
PFNGLVERTEXATTRIB4NSVPROC glad_glVertexAttrib4Nsv = NULL;
PFNGLVERTEXATTRIB4NUBPROC glad_glVertexAttrib4Nub = NULL;
PFNGLVERTEXATTRIB4NUBVPROC glad_glVertexAttrib4Nubv = NULL;
PFNGLVERTEXATTRIB4NUIVPROC glad_glVertexAttrib4Nuiv = NULL;
PFNGLVERTEXATTRIB4NUSVPROC glad_glVertexAttrib4Nusv = NULL;
PFNGLVERTEXATTRIB4BVPROC glad_glVertexAttrib4bv = NULL;
PFNGLVERTEXATTRIB4DPROC glad_glVertexAttrib4d = NULL;
PFNGLVERTEXATTRIB4DVPROC glad_glVertexAttrib4dv = NULL;
PFNGLVERTEXATTRIB4FPROC glad_glVertexAttrib4f = NULL;
PFNGLVERTEXATTRIB4FVPROC glad_glVertexAttrib4fv = NULL;
PFNGLVERTEXATTRIB4IVPROC glad_glVertexAttrib4iv = NULL;
PFNGLVERTEXATTRIB4SPROC glad_glVertexAttrib4s = NULL;
PFNGLVERTEXATTRIB4SVPROC glad_glVertexAttrib4sv = NULL;
PFNGLVERTEXATTRIB4UBVPROC glad_glVertexAttrib4ubv = NULL;
PFNGLVERTEXATTRIB4UIVPROC glad_glVertexAttrib4uiv = NULL;
PFNGLVERTEXATTRIB4USVPROC glad_glVertexAttrib4usv = NULL;
PFNGLVERTEXATTRIBDIVISORPROC glad_glVertexAttribDivisor = NULL;
PFNGLVERTEXATTRIBI1IPROC glad_glVertexAttribI1i = NULL;
PFNGLVERTEXATTRIBI1IVPROC glad_glVertexAttribI1iv = NULL;
PFNGLVERTEXATTRIBI1UIPROC glad_glVertexAttribI1ui = NULL;
PFNGLVERTEXATTRIBI1UIVPROC glad_glVertexAttribI1uiv = NULL;
PFNGLVERTEXATTRIBI2IPROC glad_glVertexAttribI2i = NULL;
PFNGLVERTEXATTRIBI2IVPROC glad_glVertexAttribI2iv = NULL;
PFNGLVERTEXATTRIBI2UIPROC glad_glVertexAttribI2ui = NULL;
PFNGLVERTEXATTRIBI2UIVPROC glad_glVertexAttribI2uiv = NULL;
PFNGLVERTEXATTRIBI3IPROC glad_glVertexAttribI3i = NULL;
PFNGLVERTEXATTRIBI3IVPROC glad_glVertexAttribI3iv = NULL;
PFNGLVERTEXATTRIBI3UIPROC glad_glVertexAttribI3ui = NULL;
PFNGLVERTEXATTRIBI3UIVPROC glad_glVertexAttribI3uiv = NULL;
PFNGLVERTEXATTRIBI4BVPROC glad_glVertexAttribI4bv = NULL;
PFNGLVERTEXATTRIBI4IPROC glad_glVertexAttribI4i = NULL;
PFNGLVERTEXATTRIBI4IVPROC glad_glVertexAttribI4iv = NULL;
PFNGLVERTEXATTRIBI4SVPROC glad_glVertexAttribI4sv = NULL;
PFNGLVERTEXATTRIBI4UBVPROC glad_glVertexAttribI4ubv = NULL;
PFNGLVERTEXATTRIBI4UIPROC glad_glVertexAttribI4ui = NULL;
PFNGLVERTEXATTRIBI4UIVPROC glad_glVertexAttribI4uiv = NULL;
PFNGLVERTEXATTRIBI4USVPROC glad_glVertexAttribI4usv = NULL;
PFNGLVERTEXATTRIBIPOINTERPROC glad_glVertexAttribIPointer = NULL;
PFNGLVERTEXATTRIBP1UIPROC glad_glVertexAttribP1ui = NULL;
PFNGLVERTEXATTRIBP1UIVPROC glad_glVertexAttribP1uiv = NULL;
PFNGLVERTEXATTRIBP2UIPROC glad_glVertexAttribP2ui = NULL;
PFNGLVERTEXATTRIBP2UIVPROC glad_glVertexAttribP2uiv = NULL;
PFNGLVERTEXATTRIBP3UIPROC glad_glVertexAttribP3ui = NULL;
PFNGLVERTEXATTRIBP3UIVPROC glad_glVertexAttribP3uiv = NULL;
PFNGLVERTEXATTRIBP4UIPROC glad_glVertexAttribP4ui = NULL;
PFNGLVERTEXATTRIBP4UIVPROC glad_glVertexAttribP4uiv = NULL;
PFNGLVERTEXATTRIBPOINTERPROC glad_glVertexAttribPointer = NULL;
PFNGLVERTEXP2UIPROC glad_glVertexP2ui = NULL;
PFNGLVERTEXP2UIVPROC glad_glVertexP2uiv = NULL;
PFNGLVERTEXP3UIPROC glad_glVertexP3ui = NULL;
PFNGLVERTEXP3UIVPROC glad_glVertexP3uiv = NULL;
PFNGLVERTEXP4UIPROC glad_glVertexP4ui = NULL;
PFNGLVERTEXP4UIVPROC glad_glVertexP4uiv = NULL;
PFNGLVIEWPORTPROC glad_glViewport = NULL;
PFNGLWAITSYNCPROC glad_glWaitSync = NULL;
int GLAD_GL_3DFX_multisample = 0;
int GLAD_GL_3DFX_tbuffer = 0;
int GLAD_GL_3DFX_texture_compression_FXT1 = 0;
int GLAD_GL_AMD_blend_minmax_factor = 0;
int GLAD_GL_AMD_conservative_depth = 0;
int GLAD_GL_AMD_debug_output = 0;
int GLAD_GL_AMD_depth_clamp_separate = 0;
int GLAD_GL_AMD_draw_buffers_blend = 0;
int GLAD_GL_AMD_framebuffer_multisample_advanced = 0;
int GLAD_GL_AMD_framebuffer_sample_positions = 0;
int GLAD_GL_AMD_gcn_shader = 0;
int GLAD_GL_AMD_gpu_shader_half_float = 0;
int GLAD_GL_AMD_gpu_shader_int16 = 0;
int GLAD_GL_AMD_gpu_shader_int64 = 0;
int GLAD_GL_AMD_interleaved_elements = 0;
int GLAD_GL_AMD_multi_draw_indirect = 0;
int GLAD_GL_AMD_name_gen_delete = 0;
int GLAD_GL_AMD_occlusion_query_event = 0;
int GLAD_GL_AMD_performance_monitor = 0;
int GLAD_GL_AMD_pinned_memory = 0;
int GLAD_GL_AMD_query_buffer_object = 0;
int GLAD_GL_AMD_sample_positions = 0;
int GLAD_GL_AMD_seamless_cubemap_per_texture = 0;
int GLAD_GL_AMD_shader_atomic_counter_ops = 0;
int GLAD_GL_AMD_shader_ballot = 0;
int GLAD_GL_AMD_shader_explicit_vertex_parameter = 0;
int GLAD_GL_AMD_shader_gpu_shader_half_float_fetch = 0;
int GLAD_GL_AMD_shader_image_load_store_lod = 0;
int GLAD_GL_AMD_shader_stencil_export = 0;
int GLAD_GL_AMD_shader_trinary_minmax = 0;
int GLAD_GL_AMD_sparse_texture = 0;
int GLAD_GL_AMD_stencil_operation_extended = 0;
int GLAD_GL_AMD_texture_gather_bias_lod = 0;
int GLAD_GL_AMD_texture_texture4 = 0;
int GLAD_GL_AMD_transform_feedback3_lines_triangles = 0;
int GLAD_GL_AMD_transform_feedback4 = 0;
int GLAD_GL_AMD_vertex_shader_layer = 0;
int GLAD_GL_AMD_vertex_shader_tessellator = 0;
int GLAD_GL_AMD_vertex_shader_viewport_index = 0;
int GLAD_GL_APPLE_aux_depth_stencil = 0;
int GLAD_GL_APPLE_client_storage = 0;
int GLAD_GL_APPLE_element_array = 0;
int GLAD_GL_APPLE_fence = 0;
int GLAD_GL_APPLE_float_pixels = 0;
int GLAD_GL_APPLE_flush_buffer_range = 0;
int GLAD_GL_APPLE_object_purgeable = 0;
int GLAD_GL_APPLE_rgb_422 = 0;
int GLAD_GL_APPLE_row_bytes = 0;
int GLAD_GL_APPLE_specular_vector = 0;
int GLAD_GL_APPLE_texture_range = 0;
int GLAD_GL_APPLE_transform_hint = 0;
int GLAD_GL_APPLE_vertex_array_object = 0;
int GLAD_GL_APPLE_vertex_array_range = 0;
int GLAD_GL_APPLE_vertex_program_evaluators = 0;
int GLAD_GL_APPLE_ycbcr_422 = 0;
int GLAD_GL_ARB_ES2_compatibility = 0;
int GLAD_GL_ARB_ES3_1_compatibility = 0;
int GLAD_GL_ARB_ES3_2_compatibility = 0;
int GLAD_GL_ARB_ES3_compatibility = 0;
int GLAD_GL_ARB_arrays_of_arrays = 0;
int GLAD_GL_ARB_base_instance = 0;
int GLAD_GL_ARB_bindless_texture = 0;
int GLAD_GL_ARB_blend_func_extended = 0;
int GLAD_GL_ARB_buffer_storage = 0;
int GLAD_GL_ARB_cl_event = 0;
int GLAD_GL_ARB_clear_buffer_object = 0;
int GLAD_GL_ARB_clear_texture = 0;
int GLAD_GL_ARB_clip_control = 0;
int GLAD_GL_ARB_color_buffer_float = 0;
int GLAD_GL_ARB_compatibility = 0;
int GLAD_GL_ARB_compressed_texture_pixel_storage = 0;
int GLAD_GL_ARB_compute_shader = 0;
int GLAD_GL_ARB_compute_variable_group_size = 0;
int GLAD_GL_ARB_conditional_render_inverted = 0;
int GLAD_GL_ARB_conservative_depth = 0;
int GLAD_GL_ARB_copy_buffer = 0;
int GLAD_GL_ARB_copy_image = 0;
int GLAD_GL_ARB_cull_distance = 0;
int GLAD_GL_ARB_debug_output = 0;
int GLAD_GL_ARB_depth_buffer_float = 0;
int GLAD_GL_ARB_depth_clamp = 0;
int GLAD_GL_ARB_depth_texture = 0;
int GLAD_GL_ARB_derivative_control = 0;
int GLAD_GL_ARB_direct_state_access = 0;
int GLAD_GL_ARB_draw_buffers = 0;
int GLAD_GL_ARB_draw_buffers_blend = 0;
int GLAD_GL_ARB_draw_elements_base_vertex = 0;
int GLAD_GL_ARB_draw_indirect = 0;
int GLAD_GL_ARB_draw_instanced = 0;
int GLAD_GL_ARB_enhanced_layouts = 0;
int GLAD_GL_ARB_explicit_attrib_location = 0;
int GLAD_GL_ARB_explicit_uniform_location = 0;
int GLAD_GL_ARB_fragment_coord_conventions = 0;
int GLAD_GL_ARB_fragment_layer_viewport = 0;
int GLAD_GL_ARB_fragment_program = 0;
int GLAD_GL_ARB_fragment_program_shadow = 0;
int GLAD_GL_ARB_fragment_shader = 0;
int GLAD_GL_ARB_fragment_shader_interlock = 0;
int GLAD_GL_ARB_framebuffer_no_attachments = 0;
int GLAD_GL_ARB_framebuffer_object = 0;
int GLAD_GL_ARB_framebuffer_sRGB = 0;
int GLAD_GL_ARB_geometry_shader4 = 0;
int GLAD_GL_ARB_get_program_binary = 0;
int GLAD_GL_ARB_get_texture_sub_image = 0;
int GLAD_GL_ARB_gl_spirv = 0;
int GLAD_GL_ARB_gpu_shader5 = 0;
int GLAD_GL_ARB_gpu_shader_fp64 = 0;
int GLAD_GL_ARB_gpu_shader_int64 = 0;
int GLAD_GL_ARB_half_float_pixel = 0;
int GLAD_GL_ARB_half_float_vertex = 0;
int GLAD_GL_ARB_imaging = 0;
int GLAD_GL_ARB_indirect_parameters = 0;
int GLAD_GL_ARB_instanced_arrays = 0;
int GLAD_GL_ARB_internalformat_query = 0;
int GLAD_GL_ARB_internalformat_query2 = 0;
int GLAD_GL_ARB_invalidate_subdata = 0;
int GLAD_GL_ARB_map_buffer_alignment = 0;
int GLAD_GL_ARB_map_buffer_range = 0;
int GLAD_GL_ARB_matrix_palette = 0;
int GLAD_GL_ARB_multi_bind = 0;
int GLAD_GL_ARB_multi_draw_indirect = 0;
int GLAD_GL_ARB_multisample = 0;
int GLAD_GL_ARB_multitexture = 0;
int GLAD_GL_ARB_occlusion_query = 0;
int GLAD_GL_ARB_occlusion_query2 = 0;
int GLAD_GL_ARB_parallel_shader_compile = 0;
int GLAD_GL_ARB_pipeline_statistics_query = 0;
int GLAD_GL_ARB_pixel_buffer_object = 0;
int GLAD_GL_ARB_point_parameters = 0;
int GLAD_GL_ARB_point_sprite = 0;
int GLAD_GL_ARB_polygon_offset_clamp = 0;
int GLAD_GL_ARB_post_depth_coverage = 0;
int GLAD_GL_ARB_program_interface_query = 0;
int GLAD_GL_ARB_provoking_vertex = 0;
int GLAD_GL_ARB_query_buffer_object = 0;
int GLAD_GL_ARB_robust_buffer_access_behavior = 0;
int GLAD_GL_ARB_robustness = 0;
int GLAD_GL_ARB_robustness_isolation = 0;
int GLAD_GL_ARB_sample_locations = 0;
int GLAD_GL_ARB_sample_shading = 0;
int GLAD_GL_ARB_sampler_objects = 0;
int GLAD_GL_ARB_seamless_cube_map = 0;
int GLAD_GL_ARB_seamless_cubemap_per_texture = 0;
int GLAD_GL_ARB_separate_shader_objects = 0;
int GLAD_GL_ARB_shader_atomic_counter_ops = 0;
int GLAD_GL_ARB_shader_atomic_counters = 0;
int GLAD_GL_ARB_shader_ballot = 0;
int GLAD_GL_ARB_shader_bit_encoding = 0;
int GLAD_GL_ARB_shader_clock = 0;
int GLAD_GL_ARB_shader_draw_parameters = 0;
int GLAD_GL_ARB_shader_group_vote = 0;
int GLAD_GL_ARB_shader_image_load_store = 0;
int GLAD_GL_ARB_shader_image_size = 0;
int GLAD_GL_ARB_shader_objects = 0;
int GLAD_GL_ARB_shader_precision = 0;
int GLAD_GL_ARB_shader_stencil_export = 0;
int GLAD_GL_ARB_shader_storage_buffer_object = 0;
int GLAD_GL_ARB_shader_subroutine = 0;
int GLAD_GL_ARB_shader_texture_image_samples = 0;
int GLAD_GL_ARB_shader_texture_lod = 0;
int GLAD_GL_ARB_shader_viewport_layer_array = 0;
int GLAD_GL_ARB_shading_language_100 = 0;
int GLAD_GL_ARB_shading_language_420pack = 0;
int GLAD_GL_ARB_shading_language_include = 0;
int GLAD_GL_ARB_shading_language_packing = 0;
int GLAD_GL_ARB_shadow = 0;
int GLAD_GL_ARB_shadow_ambient = 0;
int GLAD_GL_ARB_sparse_buffer = 0;
int GLAD_GL_ARB_sparse_texture = 0;
int GLAD_GL_ARB_sparse_texture2 = 0;
int GLAD_GL_ARB_sparse_texture_clamp = 0;
int GLAD_GL_ARB_spirv_extensions = 0;
int GLAD_GL_ARB_stencil_texturing = 0;
int GLAD_GL_ARB_sync = 0;
int GLAD_GL_ARB_tessellation_shader = 0;
int GLAD_GL_ARB_texture_barrier = 0;
int GLAD_GL_ARB_texture_border_clamp = 0;
int GLAD_GL_ARB_texture_buffer_object = 0;
int GLAD_GL_ARB_texture_buffer_object_rgb32 = 0;
int GLAD_GL_ARB_texture_buffer_range = 0;
int GLAD_GL_ARB_texture_compression = 0;
int GLAD_GL_ARB_texture_compression_bptc = 0;
int GLAD_GL_ARB_texture_compression_rgtc = 0;
int GLAD_GL_ARB_texture_cube_map = 0;
int GLAD_GL_ARB_texture_cube_map_array = 0;
int GLAD_GL_ARB_texture_env_add = 0;
int GLAD_GL_ARB_texture_env_combine = 0;
int GLAD_GL_ARB_texture_env_crossbar = 0;
int GLAD_GL_ARB_texture_env_dot3 = 0;
int GLAD_GL_ARB_texture_filter_anisotropic = 0;
int GLAD_GL_ARB_texture_filter_minmax = 0;
int GLAD_GL_ARB_texture_float = 0;
int GLAD_GL_ARB_texture_gather = 0;
int GLAD_GL_ARB_texture_mirror_clamp_to_edge = 0;
int GLAD_GL_ARB_texture_mirrored_repeat = 0;
int GLAD_GL_ARB_texture_multisample = 0;
int GLAD_GL_ARB_texture_non_power_of_two = 0;
int GLAD_GL_ARB_texture_query_levels = 0;
int GLAD_GL_ARB_texture_query_lod = 0;
int GLAD_GL_ARB_texture_rectangle = 0;
int GLAD_GL_ARB_texture_rg = 0;
int GLAD_GL_ARB_texture_rgb10_a2ui = 0;
int GLAD_GL_ARB_texture_stencil8 = 0;
int GLAD_GL_ARB_texture_storage = 0;
int GLAD_GL_ARB_texture_storage_multisample = 0;
int GLAD_GL_ARB_texture_swizzle = 0;
int GLAD_GL_ARB_texture_view = 0;
int GLAD_GL_ARB_timer_query = 0;
int GLAD_GL_ARB_transform_feedback2 = 0;
int GLAD_GL_ARB_transform_feedback3 = 0;
int GLAD_GL_ARB_transform_feedback_instanced = 0;
int GLAD_GL_ARB_transform_feedback_overflow_query = 0;
int GLAD_GL_ARB_transpose_matrix = 0;
int GLAD_GL_ARB_uniform_buffer_object = 0;
int GLAD_GL_ARB_vertex_array_bgra = 0;
int GLAD_GL_ARB_vertex_array_object = 0;
int GLAD_GL_ARB_vertex_attrib_64bit = 0;
int GLAD_GL_ARB_vertex_attrib_binding = 0;
int GLAD_GL_ARB_vertex_blend = 0;
int GLAD_GL_ARB_vertex_buffer_object = 0;
int GLAD_GL_ARB_vertex_program = 0;
int GLAD_GL_ARB_vertex_shader = 0;
int GLAD_GL_ARB_vertex_type_10f_11f_11f_rev = 0;
int GLAD_GL_ARB_vertex_type_2_10_10_10_rev = 0;
int GLAD_GL_ARB_viewport_array = 0;
int GLAD_GL_ARB_window_pos = 0;
int GLAD_GL_ATI_draw_buffers = 0;
int GLAD_GL_ATI_element_array = 0;
int GLAD_GL_ATI_envmap_bumpmap = 0;
int GLAD_GL_ATI_fragment_shader = 0;
int GLAD_GL_ATI_map_object_buffer = 0;
int GLAD_GL_ATI_meminfo = 0;
int GLAD_GL_ATI_pixel_format_float = 0;
int GLAD_GL_ATI_pn_triangles = 0;
int GLAD_GL_ATI_separate_stencil = 0;
int GLAD_GL_ATI_text_fragment_shader = 0;
int GLAD_GL_ATI_texture_env_combine3 = 0;
int GLAD_GL_ATI_texture_float = 0;
int GLAD_GL_ATI_texture_mirror_once = 0;
int GLAD_GL_ATI_vertex_array_object = 0;
int GLAD_GL_ATI_vertex_attrib_array_object = 0;
int GLAD_GL_ATI_vertex_streams = 0;
int GLAD_GL_EXT_422_pixels = 0;
int GLAD_GL_EXT_EGL_image_storage = 0;
int GLAD_GL_EXT_EGL_sync = 0;
int GLAD_GL_EXT_abgr = 0;
int GLAD_GL_EXT_bgra = 0;
int GLAD_GL_EXT_bindable_uniform = 0;
int GLAD_GL_EXT_blend_color = 0;
int GLAD_GL_EXT_blend_equation_separate = 0;
int GLAD_GL_EXT_blend_func_separate = 0;
int GLAD_GL_EXT_blend_logic_op = 0;
int GLAD_GL_EXT_blend_minmax = 0;
int GLAD_GL_EXT_blend_subtract = 0;
int GLAD_GL_EXT_clip_volume_hint = 0;
int GLAD_GL_EXT_cmyka = 0;
int GLAD_GL_EXT_color_subtable = 0;
int GLAD_GL_EXT_compiled_vertex_array = 0;
int GLAD_GL_EXT_convolution = 0;
int GLAD_GL_EXT_coordinate_frame = 0;
int GLAD_GL_EXT_copy_texture = 0;
int GLAD_GL_EXT_cull_vertex = 0;
int GLAD_GL_EXT_debug_label = 0;
int GLAD_GL_EXT_debug_marker = 0;
int GLAD_GL_EXT_depth_bounds_test = 0;
int GLAD_GL_EXT_direct_state_access = 0;
int GLAD_GL_EXT_draw_buffers2 = 0;
int GLAD_GL_EXT_draw_instanced = 0;
int GLAD_GL_EXT_draw_range_elements = 0;
int GLAD_GL_EXT_external_buffer = 0;
int GLAD_GL_EXT_fog_coord = 0;
int GLAD_GL_EXT_framebuffer_blit = 0;
int GLAD_GL_EXT_framebuffer_blit_layers = 0;
int GLAD_GL_EXT_framebuffer_multisample = 0;
int GLAD_GL_EXT_framebuffer_multisample_blit_scaled = 0;
int GLAD_GL_EXT_framebuffer_object = 0;
int GLAD_GL_EXT_framebuffer_sRGB = 0;
int GLAD_GL_EXT_geometry_shader4 = 0;
int GLAD_GL_EXT_gpu_program_parameters = 0;
int GLAD_GL_EXT_gpu_shader4 = 0;
int GLAD_GL_EXT_histogram = 0;
int GLAD_GL_EXT_index_array_formats = 0;
int GLAD_GL_EXT_index_func = 0;
int GLAD_GL_EXT_index_material = 0;
int GLAD_GL_EXT_index_texture = 0;
int GLAD_GL_EXT_light_texture = 0;
int GLAD_GL_EXT_memory_object = 0;
int GLAD_GL_EXT_memory_object_fd = 0;
int GLAD_GL_EXT_memory_object_win32 = 0;
int GLAD_GL_EXT_misc_attribute = 0;
int GLAD_GL_EXT_multi_draw_arrays = 0;
int GLAD_GL_EXT_multisample = 0;
int GLAD_GL_EXT_multiview_tessellation_geometry_shader = 0;
int GLAD_GL_EXT_multiview_texture_multisample = 0;
int GLAD_GL_EXT_multiview_timer_query = 0;
int GLAD_GL_EXT_packed_depth_stencil = 0;
int GLAD_GL_EXT_packed_float = 0;
int GLAD_GL_EXT_packed_pixels = 0;
int GLAD_GL_EXT_paletted_texture = 0;
int GLAD_GL_EXT_pixel_buffer_object = 0;
int GLAD_GL_EXT_pixel_transform = 0;
int GLAD_GL_EXT_pixel_transform_color_table = 0;
int GLAD_GL_EXT_point_parameters = 0;
int GLAD_GL_EXT_polygon_offset = 0;
int GLAD_GL_EXT_polygon_offset_clamp = 0;
int GLAD_GL_EXT_post_depth_coverage = 0;
int GLAD_GL_EXT_provoking_vertex = 0;
int GLAD_GL_EXT_raster_multisample = 0;
int GLAD_GL_EXT_rescale_normal = 0;
int GLAD_GL_EXT_secondary_color = 0;
int GLAD_GL_EXT_semaphore = 0;
int GLAD_GL_EXT_semaphore_fd = 0;
int GLAD_GL_EXT_semaphore_win32 = 0;
int GLAD_GL_EXT_separate_shader_objects = 0;
int GLAD_GL_EXT_separate_specular_color = 0;
int GLAD_GL_EXT_shader_framebuffer_fetch = 0;
int GLAD_GL_EXT_shader_framebuffer_fetch_non_coherent = 0;
int GLAD_GL_EXT_shader_image_load_formatted = 0;
int GLAD_GL_EXT_shader_image_load_store = 0;
int GLAD_GL_EXT_shader_integer_mix = 0;
int GLAD_GL_EXT_shader_samples_identical = 0;
int GLAD_GL_EXT_shadow_funcs = 0;
int GLAD_GL_EXT_shared_texture_palette = 0;
int GLAD_GL_EXT_sparse_texture2 = 0;
int GLAD_GL_EXT_stencil_clear_tag = 0;
int GLAD_GL_EXT_stencil_two_side = 0;
int GLAD_GL_EXT_stencil_wrap = 0;
int GLAD_GL_EXT_subtexture = 0;
int GLAD_GL_EXT_texture = 0;
int GLAD_GL_EXT_texture3D = 0;
int GLAD_GL_EXT_texture_array = 0;
int GLAD_GL_EXT_texture_buffer_object = 0;
int GLAD_GL_EXT_texture_compression_latc = 0;
int GLAD_GL_EXT_texture_compression_rgtc = 0;
int GLAD_GL_EXT_texture_compression_s3tc = 0;
int GLAD_GL_EXT_texture_cube_map = 0;
int GLAD_GL_EXT_texture_env_add = 0;
int GLAD_GL_EXT_texture_env_combine = 0;
int GLAD_GL_EXT_texture_env_dot3 = 0;
int GLAD_GL_EXT_texture_filter_anisotropic = 0;
int GLAD_GL_EXT_texture_filter_minmax = 0;
int GLAD_GL_EXT_texture_integer = 0;
int GLAD_GL_EXT_texture_lod_bias = 0;
int GLAD_GL_EXT_texture_mirror_clamp = 0;
int GLAD_GL_EXT_texture_object = 0;
int GLAD_GL_EXT_texture_perturb_normal = 0;
int GLAD_GL_EXT_texture_sRGB = 0;
int GLAD_GL_EXT_texture_sRGB_R8 = 0;
int GLAD_GL_EXT_texture_sRGB_RG8 = 0;
int GLAD_GL_EXT_texture_sRGB_decode = 0;
int GLAD_GL_EXT_texture_shadow_lod = 0;
int GLAD_GL_EXT_texture_shared_exponent = 0;
int GLAD_GL_EXT_texture_snorm = 0;
int GLAD_GL_EXT_texture_storage = 0;
int GLAD_GL_EXT_texture_swizzle = 0;
int GLAD_GL_EXT_timer_query = 0;
int GLAD_GL_EXT_transform_feedback = 0;
int GLAD_GL_EXT_vertex_array = 0;
int GLAD_GL_EXT_vertex_array_bgra = 0;
int GLAD_GL_EXT_vertex_attrib_64bit = 0;
int GLAD_GL_EXT_vertex_shader = 0;
int GLAD_GL_EXT_vertex_weighting = 0;
int GLAD_GL_EXT_win32_keyed_mutex = 0;
int GLAD_GL_EXT_window_rectangles = 0;
int GLAD_GL_EXT_x11_sync_object = 0;
int GLAD_GL_GREMEDY_frame_terminator = 0;
int GLAD_GL_GREMEDY_string_marker = 0;
int GLAD_GL_HP_convolution_border_modes = 0;
int GLAD_GL_HP_image_transform = 0;
int GLAD_GL_HP_occlusion_test = 0;
int GLAD_GL_HP_texture_lighting = 0;
int GLAD_GL_IBM_cull_vertex = 0;
int GLAD_GL_IBM_multimode_draw_arrays = 0;
int GLAD_GL_IBM_rasterpos_clip = 0;
int GLAD_GL_IBM_static_data = 0;
int GLAD_GL_IBM_texture_mirrored_repeat = 0;
int GLAD_GL_IBM_vertex_array_lists = 0;
int GLAD_GL_INGR_blend_func_separate = 0;
int GLAD_GL_INGR_color_clamp = 0;
int GLAD_GL_INGR_interlace_read = 0;
int GLAD_GL_INTEL_blackhole_render = 0;
int GLAD_GL_INTEL_conservative_rasterization = 0;
int GLAD_GL_INTEL_fragment_shader_ordering = 0;
int GLAD_GL_INTEL_framebuffer_CMAA = 0;
int GLAD_GL_INTEL_map_texture = 0;
int GLAD_GL_INTEL_parallel_arrays = 0;
int GLAD_GL_INTEL_performance_query = 0;
int GLAD_GL_KHR_blend_equation_advanced = 0;
int GLAD_GL_KHR_blend_equation_advanced_coherent = 0;
int GLAD_GL_KHR_context_flush_control = 0;
int GLAD_GL_KHR_debug = 0;
int GLAD_GL_KHR_no_error = 0;
int GLAD_GL_KHR_parallel_shader_compile = 0;
int GLAD_GL_KHR_robust_buffer_access_behavior = 0;
int GLAD_GL_KHR_robustness = 0;
int GLAD_GL_KHR_shader_subgroup = 0;
int GLAD_GL_KHR_texture_compression_astc_hdr = 0;
int GLAD_GL_KHR_texture_compression_astc_ldr = 0;
int GLAD_GL_KHR_texture_compression_astc_sliced_3d = 0;
int GLAD_GL_MESAX_texture_stack = 0;
int GLAD_GL_MESA_framebuffer_flip_x = 0;
int GLAD_GL_MESA_framebuffer_flip_y = 0;
int GLAD_GL_MESA_framebuffer_swap_xy = 0;
int GLAD_GL_MESA_pack_invert = 0;
int GLAD_GL_MESA_program_binary_formats = 0;
int GLAD_GL_MESA_resize_buffers = 0;
int GLAD_GL_MESA_shader_integer_functions = 0;
int GLAD_GL_MESA_tile_raster_order = 0;
int GLAD_GL_MESA_window_pos = 0;
int GLAD_GL_MESA_ycbcr_texture = 0;
int GLAD_GL_NVX_blend_equation_advanced_multi_draw_buffers = 0;
int GLAD_GL_NVX_conditional_render = 0;
int GLAD_GL_NVX_gpu_memory_info = 0;
int GLAD_GL_NVX_gpu_multicast2 = 0;
int GLAD_GL_NVX_linked_gpu_multicast = 0;
int GLAD_GL_NVX_progress_fence = 0;
int GLAD_GL_NV_alpha_to_coverage_dither_control = 0;
int GLAD_GL_NV_bindless_multi_draw_indirect = 0;
int GLAD_GL_NV_bindless_multi_draw_indirect_count = 0;
int GLAD_GL_NV_bindless_texture = 0;
int GLAD_GL_NV_blend_equation_advanced = 0;
int GLAD_GL_NV_blend_equation_advanced_coherent = 0;
int GLAD_GL_NV_blend_minmax_factor = 0;
int GLAD_GL_NV_blend_square = 0;
int GLAD_GL_NV_clip_space_w_scaling = 0;
int GLAD_GL_NV_command_list = 0;
int GLAD_GL_NV_compute_program5 = 0;
int GLAD_GL_NV_compute_shader_derivatives = 0;
int GLAD_GL_NV_conditional_render = 0;
int GLAD_GL_NV_conservative_raster = 0;
int GLAD_GL_NV_conservative_raster_dilate = 0;
int GLAD_GL_NV_conservative_raster_pre_snap = 0;
int GLAD_GL_NV_conservative_raster_pre_snap_triangles = 0;
int GLAD_GL_NV_conservative_raster_underestimation = 0;
int GLAD_GL_NV_copy_depth_to_color = 0;
int GLAD_GL_NV_copy_image = 0;
int GLAD_GL_NV_deep_texture3D = 0;
int GLAD_GL_NV_depth_buffer_float = 0;
int GLAD_GL_NV_depth_clamp = 0;
int GLAD_GL_NV_draw_texture = 0;
int GLAD_GL_NV_draw_vulkan_image = 0;
int GLAD_GL_NV_evaluators = 0;
int GLAD_GL_NV_explicit_multisample = 0;
int GLAD_GL_NV_fence = 0;
int GLAD_GL_NV_fill_rectangle = 0;
int GLAD_GL_NV_float_buffer = 0;
int GLAD_GL_NV_fog_distance = 0;
int GLAD_GL_NV_fragment_coverage_to_color = 0;
int GLAD_GL_NV_fragment_program = 0;
int GLAD_GL_NV_fragment_program2 = 0;
int GLAD_GL_NV_fragment_program4 = 0;
int GLAD_GL_NV_fragment_program_option = 0;
int GLAD_GL_NV_fragment_shader_barycentric = 0;
int GLAD_GL_NV_fragment_shader_interlock = 0;
int GLAD_GL_NV_framebuffer_mixed_samples = 0;
int GLAD_GL_NV_framebuffer_multisample_coverage = 0;
int GLAD_GL_NV_geometry_program4 = 0;
int GLAD_GL_NV_geometry_shader4 = 0;
int GLAD_GL_NV_geometry_shader_passthrough = 0;
int GLAD_GL_NV_gpu_multicast = 0;
int GLAD_GL_NV_gpu_program4 = 0;
int GLAD_GL_NV_gpu_program5 = 0;
int GLAD_GL_NV_gpu_program5_mem_extended = 0;
int GLAD_GL_NV_gpu_shader5 = 0;
int GLAD_GL_NV_half_float = 0;
int GLAD_GL_NV_internalformat_sample_query = 0;
int GLAD_GL_NV_light_max_exponent = 0;
int GLAD_GL_NV_memory_attachment = 0;
int GLAD_GL_NV_memory_object_sparse = 0;
int GLAD_GL_NV_mesh_shader = 0;
int GLAD_GL_NV_multisample_coverage = 0;
int GLAD_GL_NV_multisample_filter_hint = 0;
int GLAD_GL_NV_occlusion_query = 0;
int GLAD_GL_NV_packed_depth_stencil = 0;
int GLAD_GL_NV_parameter_buffer_object = 0;
int GLAD_GL_NV_parameter_buffer_object2 = 0;
int GLAD_GL_NV_path_rendering = 0;
int GLAD_GL_NV_path_rendering_shared_edge = 0;
int GLAD_GL_NV_pixel_data_range = 0;
int GLAD_GL_NV_point_sprite = 0;
int GLAD_GL_NV_present_video = 0;
int GLAD_GL_NV_primitive_restart = 0;
int GLAD_GL_NV_primitive_shading_rate = 0;
int GLAD_GL_NV_query_resource = 0;
int GLAD_GL_NV_query_resource_tag = 0;
int GLAD_GL_NV_register_combiners = 0;
int GLAD_GL_NV_register_combiners2 = 0;
int GLAD_GL_NV_representative_fragment_test = 0;
int GLAD_GL_NV_robustness_video_memory_purge = 0;
int GLAD_GL_NV_sample_locations = 0;
int GLAD_GL_NV_sample_mask_override_coverage = 0;
int GLAD_GL_NV_scissor_exclusive = 0;
int GLAD_GL_NV_shader_atomic_counters = 0;
int GLAD_GL_NV_shader_atomic_float = 0;
int GLAD_GL_NV_shader_atomic_float64 = 0;
int GLAD_GL_NV_shader_atomic_fp16_vector = 0;
int GLAD_GL_NV_shader_atomic_int64 = 0;
int GLAD_GL_NV_shader_buffer_load = 0;
int GLAD_GL_NV_shader_buffer_store = 0;
int GLAD_GL_NV_shader_storage_buffer_object = 0;
int GLAD_GL_NV_shader_subgroup_partitioned = 0;
int GLAD_GL_NV_shader_texture_footprint = 0;
int GLAD_GL_NV_shader_thread_group = 0;
int GLAD_GL_NV_shader_thread_shuffle = 0;
int GLAD_GL_NV_shading_rate_image = 0;
int GLAD_GL_NV_stereo_view_rendering = 0;
int GLAD_GL_NV_tessellation_program5 = 0;
int GLAD_GL_NV_texgen_emboss = 0;
int GLAD_GL_NV_texgen_reflection = 0;
int GLAD_GL_NV_texture_barrier = 0;
int GLAD_GL_NV_texture_compression_vtc = 0;
int GLAD_GL_NV_texture_env_combine4 = 0;
int GLAD_GL_NV_texture_expand_normal = 0;
int GLAD_GL_NV_texture_multisample = 0;
int GLAD_GL_NV_texture_rectangle = 0;
int GLAD_GL_NV_texture_rectangle_compressed = 0;
int GLAD_GL_NV_texture_shader = 0;
int GLAD_GL_NV_texture_shader2 = 0;
int GLAD_GL_NV_texture_shader3 = 0;
int GLAD_GL_NV_timeline_semaphore = 0;
int GLAD_GL_NV_transform_feedback = 0;
int GLAD_GL_NV_transform_feedback2 = 0;
int GLAD_GL_NV_uniform_buffer_unified_memory = 0;
int GLAD_GL_NV_vdpau_interop = 0;
int GLAD_GL_NV_vdpau_interop2 = 0;
int GLAD_GL_NV_vertex_array_range = 0;
int GLAD_GL_NV_vertex_array_range2 = 0;
int GLAD_GL_NV_vertex_attrib_integer_64bit = 0;
int GLAD_GL_NV_vertex_buffer_unified_memory = 0;
int GLAD_GL_NV_vertex_program = 0;
int GLAD_GL_NV_vertex_program1_1 = 0;
int GLAD_GL_NV_vertex_program2 = 0;
int GLAD_GL_NV_vertex_program2_option = 0;
int GLAD_GL_NV_vertex_program3 = 0;
int GLAD_GL_NV_vertex_program4 = 0;
int GLAD_GL_NV_video_capture = 0;
int GLAD_GL_NV_viewport_array2 = 0;
int GLAD_GL_NV_viewport_swizzle = 0;
int GLAD_GL_OES_byte_coordinates = 0;
int GLAD_GL_OES_compressed_paletted_texture = 0;
int GLAD_GL_OES_fixed_point = 0;
int GLAD_GL_OES_query_matrix = 0;
int GLAD_GL_OES_read_format = 0;
int GLAD_GL_OES_single_precision = 0;
int GLAD_GL_OML_interlace = 0;
int GLAD_GL_OML_resample = 0;
int GLAD_GL_OML_subsample = 0;
int GLAD_GL_OVR_multiview = 0;
int GLAD_GL_OVR_multiview2 = 0;
int GLAD_GL_PGI_misc_hints = 0;
int GLAD_GL_PGI_vertex_hints = 0;
int GLAD_GL_REND_screen_coordinates = 0;
int GLAD_GL_S3_s3tc = 0;
int GLAD_GL_SGIS_detail_texture = 0;
int GLAD_GL_SGIS_fog_function = 0;
int GLAD_GL_SGIS_generate_mipmap = 0;
int GLAD_GL_SGIS_multisample = 0;
int GLAD_GL_SGIS_pixel_texture = 0;
int GLAD_GL_SGIS_point_line_texgen = 0;
int GLAD_GL_SGIS_point_parameters = 0;
int GLAD_GL_SGIS_sharpen_texture = 0;
int GLAD_GL_SGIS_texture4D = 0;
int GLAD_GL_SGIS_texture_border_clamp = 0;
int GLAD_GL_SGIS_texture_color_mask = 0;
int GLAD_GL_SGIS_texture_edge_clamp = 0;
int GLAD_GL_SGIS_texture_filter4 = 0;
int GLAD_GL_SGIS_texture_lod = 0;
int GLAD_GL_SGIS_texture_select = 0;
int GLAD_GL_SGIX_async = 0;
int GLAD_GL_SGIX_async_histogram = 0;
int GLAD_GL_SGIX_async_pixel = 0;
int GLAD_GL_SGIX_blend_alpha_minmax = 0;
int GLAD_GL_SGIX_calligraphic_fragment = 0;
int GLAD_GL_SGIX_clipmap = 0;
int GLAD_GL_SGIX_convolution_accuracy = 0;
int GLAD_GL_SGIX_depth_pass_instrument = 0;
int GLAD_GL_SGIX_depth_texture = 0;
int GLAD_GL_SGIX_flush_raster = 0;
int GLAD_GL_SGIX_fog_offset = 0;
int GLAD_GL_SGIX_fragment_lighting = 0;
int GLAD_GL_SGIX_framezoom = 0;
int GLAD_GL_SGIX_igloo_interface = 0;
int GLAD_GL_SGIX_instruments = 0;
int GLAD_GL_SGIX_interlace = 0;
int GLAD_GL_SGIX_ir_instrument1 = 0;
int GLAD_GL_SGIX_list_priority = 0;
int GLAD_GL_SGIX_pixel_texture = 0;
int GLAD_GL_SGIX_pixel_tiles = 0;
int GLAD_GL_SGIX_polynomial_ffd = 0;
int GLAD_GL_SGIX_reference_plane = 0;
int GLAD_GL_SGIX_resample = 0;
int GLAD_GL_SGIX_scalebias_hint = 0;
int GLAD_GL_SGIX_shadow = 0;
int GLAD_GL_SGIX_shadow_ambient = 0;
int GLAD_GL_SGIX_sprite = 0;
int GLAD_GL_SGIX_subsample = 0;
int GLAD_GL_SGIX_tag_sample_buffer = 0;
int GLAD_GL_SGIX_texture_add_env = 0;
int GLAD_GL_SGIX_texture_coordinate_clamp = 0;
int GLAD_GL_SGIX_texture_lod_bias = 0;
int GLAD_GL_SGIX_texture_multi_buffer = 0;
int GLAD_GL_SGIX_texture_scale_bias = 0;
int GLAD_GL_SGIX_vertex_preclip = 0;
int GLAD_GL_SGIX_ycrcb = 0;
int GLAD_GL_SGIX_ycrcb_subsample = 0;
int GLAD_GL_SGIX_ycrcba = 0;
int GLAD_GL_SGI_color_matrix = 0;
int GLAD_GL_SGI_color_table = 0;
int GLAD_GL_SGI_texture_color_table = 0;
int GLAD_GL_SUNX_constant_data = 0;
int GLAD_GL_SUN_convolution_border_modes = 0;
int GLAD_GL_SUN_global_alpha = 0;
int GLAD_GL_SUN_mesh_array = 0;
int GLAD_GL_SUN_slice_accum = 0;
int GLAD_GL_SUN_triangle_list = 0;
int GLAD_GL_SUN_vertex = 0;
int GLAD_GL_WIN_phong_shading = 0;
int GLAD_GL_WIN_specular_fog = 0;
PFNGLTBUFFERMASK3DFXPROC glad_glTbufferMask3DFX = NULL;
PFNGLDEBUGMESSAGEENABLEAMDPROC glad_glDebugMessageEnableAMD = NULL;
PFNGLDEBUGMESSAGEINSERTAMDPROC glad_glDebugMessageInsertAMD = NULL;
PFNGLDEBUGMESSAGECALLBACKAMDPROC glad_glDebugMessageCallbackAMD = NULL;
PFNGLGETDEBUGMESSAGELOGAMDPROC glad_glGetDebugMessageLogAMD = NULL;
PFNGLBLENDFUNCINDEXEDAMDPROC glad_glBlendFuncIndexedAMD = NULL;
PFNGLBLENDFUNCSEPARATEINDEXEDAMDPROC glad_glBlendFuncSeparateIndexedAMD = NULL;
PFNGLBLENDEQUATIONINDEXEDAMDPROC glad_glBlendEquationIndexedAMD = NULL;
PFNGLBLENDEQUATIONSEPARATEINDEXEDAMDPROC glad_glBlendEquationSeparateIndexedAMD = NULL;
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEADVANCEDAMDPROC glad_glRenderbufferStorageMultisampleAdvancedAMD = NULL;
PFNGLNAMEDRENDERBUFFERSTORAGEMULTISAMPLEADVANCEDAMDPROC glad_glNamedRenderbufferStorageMultisampleAdvancedAMD = NULL;
PFNGLFRAMEBUFFERSAMPLEPOSITIONSFVAMDPROC glad_glFramebufferSamplePositionsfvAMD = NULL;
PFNGLNAMEDFRAMEBUFFERSAMPLEPOSITIONSFVAMDPROC glad_glNamedFramebufferSamplePositionsfvAMD = NULL;
PFNGLGETFRAMEBUFFERPARAMETERFVAMDPROC glad_glGetFramebufferParameterfvAMD = NULL;
PFNGLGETNAMEDFRAMEBUFFERPARAMETERFVAMDPROC glad_glGetNamedFramebufferParameterfvAMD = NULL;
PFNGLUNIFORM1I64NVPROC glad_glUniform1i64NV = NULL;
PFNGLUNIFORM2I64NVPROC glad_glUniform2i64NV = NULL;
PFNGLUNIFORM3I64NVPROC glad_glUniform3i64NV = NULL;
PFNGLUNIFORM4I64NVPROC glad_glUniform4i64NV = NULL;
PFNGLUNIFORM1I64VNVPROC glad_glUniform1i64vNV = NULL;
PFNGLUNIFORM2I64VNVPROC glad_glUniform2i64vNV = NULL;
PFNGLUNIFORM3I64VNVPROC glad_glUniform3i64vNV = NULL;
PFNGLUNIFORM4I64VNVPROC glad_glUniform4i64vNV = NULL;
PFNGLUNIFORM1UI64NVPROC glad_glUniform1ui64NV = NULL;
PFNGLUNIFORM2UI64NVPROC glad_glUniform2ui64NV = NULL;
PFNGLUNIFORM3UI64NVPROC glad_glUniform3ui64NV = NULL;
PFNGLUNIFORM4UI64NVPROC glad_glUniform4ui64NV = NULL;
PFNGLUNIFORM1UI64VNVPROC glad_glUniform1ui64vNV = NULL;
PFNGLUNIFORM2UI64VNVPROC glad_glUniform2ui64vNV = NULL;
PFNGLUNIFORM3UI64VNVPROC glad_glUniform3ui64vNV = NULL;
PFNGLUNIFORM4UI64VNVPROC glad_glUniform4ui64vNV = NULL;
PFNGLGETUNIFORMI64VNVPROC glad_glGetUniformi64vNV = NULL;
PFNGLGETUNIFORMUI64VNVPROC glad_glGetUniformui64vNV = NULL;
PFNGLPROGRAMUNIFORM1I64NVPROC glad_glProgramUniform1i64NV = NULL;
PFNGLPROGRAMUNIFORM2I64NVPROC glad_glProgramUniform2i64NV = NULL;
PFNGLPROGRAMUNIFORM3I64NVPROC glad_glProgramUniform3i64NV = NULL;
PFNGLPROGRAMUNIFORM4I64NVPROC glad_glProgramUniform4i64NV = NULL;
PFNGLPROGRAMUNIFORM1I64VNVPROC glad_glProgramUniform1i64vNV = NULL;
PFNGLPROGRAMUNIFORM2I64VNVPROC glad_glProgramUniform2i64vNV = NULL;
PFNGLPROGRAMUNIFORM3I64VNVPROC glad_glProgramUniform3i64vNV = NULL;
PFNGLPROGRAMUNIFORM4I64VNVPROC glad_glProgramUniform4i64vNV = NULL;
PFNGLPROGRAMUNIFORM1UI64NVPROC glad_glProgramUniform1ui64NV = NULL;
PFNGLPROGRAMUNIFORM2UI64NVPROC glad_glProgramUniform2ui64NV = NULL;
PFNGLPROGRAMUNIFORM3UI64NVPROC glad_glProgramUniform3ui64NV = NULL;
PFNGLPROGRAMUNIFORM4UI64NVPROC glad_glProgramUniform4ui64NV = NULL;
PFNGLPROGRAMUNIFORM1UI64VNVPROC glad_glProgramUniform1ui64vNV = NULL;
PFNGLPROGRAMUNIFORM2UI64VNVPROC glad_glProgramUniform2ui64vNV = NULL;
PFNGLPROGRAMUNIFORM3UI64VNVPROC glad_glProgramUniform3ui64vNV = NULL;
PFNGLPROGRAMUNIFORM4UI64VNVPROC glad_glProgramUniform4ui64vNV = NULL;
PFNGLVERTEXATTRIBPARAMETERIAMDPROC glad_glVertexAttribParameteriAMD = NULL;
PFNGLMULTIDRAWARRAYSINDIRECTAMDPROC glad_glMultiDrawArraysIndirectAMD = NULL;
PFNGLMULTIDRAWELEMENTSINDIRECTAMDPROC glad_glMultiDrawElementsIndirectAMD = NULL;
PFNGLGENNAMESAMDPROC glad_glGenNamesAMD = NULL;
PFNGLDELETENAMESAMDPROC glad_glDeleteNamesAMD = NULL;
PFNGLISNAMEAMDPROC glad_glIsNameAMD = NULL;
PFNGLQUERYOBJECTPARAMETERUIAMDPROC glad_glQueryObjectParameteruiAMD = NULL;
PFNGLGETPERFMONITORGROUPSAMDPROC glad_glGetPerfMonitorGroupsAMD = NULL;
PFNGLGETPERFMONITORCOUNTERSAMDPROC glad_glGetPerfMonitorCountersAMD = NULL;
PFNGLGETPERFMONITORGROUPSTRINGAMDPROC glad_glGetPerfMonitorGroupStringAMD = NULL;
PFNGLGETPERFMONITORCOUNTERSTRINGAMDPROC glad_glGetPerfMonitorCounterStringAMD = NULL;
PFNGLGETPERFMONITORCOUNTERINFOAMDPROC glad_glGetPerfMonitorCounterInfoAMD = NULL;
PFNGLGENPERFMONITORSAMDPROC glad_glGenPerfMonitorsAMD = NULL;
PFNGLDELETEPERFMONITORSAMDPROC glad_glDeletePerfMonitorsAMD = NULL;
PFNGLSELECTPERFMONITORCOUNTERSAMDPROC glad_glSelectPerfMonitorCountersAMD = NULL;
PFNGLBEGINPERFMONITORAMDPROC glad_glBeginPerfMonitorAMD = NULL;
PFNGLENDPERFMONITORAMDPROC glad_glEndPerfMonitorAMD = NULL;
PFNGLGETPERFMONITORCOUNTERDATAAMDPROC glad_glGetPerfMonitorCounterDataAMD = NULL;
PFNGLSETMULTISAMPLEFVAMDPROC glad_glSetMultisamplefvAMD = NULL;
PFNGLTEXSTORAGESPARSEAMDPROC glad_glTexStorageSparseAMD = NULL;
PFNGLTEXTURESTORAGESPARSEAMDPROC glad_glTextureStorageSparseAMD = NULL;
PFNGLSTENCILOPVALUEAMDPROC glad_glStencilOpValueAMD = NULL;
PFNGLTESSELLATIONFACTORAMDPROC glad_glTessellationFactorAMD = NULL;
PFNGLTESSELLATIONMODEAMDPROC glad_glTessellationModeAMD = NULL;
PFNGLELEMENTPOINTERAPPLEPROC glad_glElementPointerAPPLE = NULL;
PFNGLDRAWELEMENTARRAYAPPLEPROC glad_glDrawElementArrayAPPLE = NULL;
PFNGLDRAWRANGEELEMENTARRAYAPPLEPROC glad_glDrawRangeElementArrayAPPLE = NULL;
PFNGLMULTIDRAWELEMENTARRAYAPPLEPROC glad_glMultiDrawElementArrayAPPLE = NULL;
PFNGLMULTIDRAWRANGEELEMENTARRAYAPPLEPROC glad_glMultiDrawRangeElementArrayAPPLE = NULL;
PFNGLGENFENCESAPPLEPROC glad_glGenFencesAPPLE = NULL;
PFNGLDELETEFENCESAPPLEPROC glad_glDeleteFencesAPPLE = NULL;
PFNGLSETFENCEAPPLEPROC glad_glSetFenceAPPLE = NULL;
PFNGLISFENCEAPPLEPROC glad_glIsFenceAPPLE = NULL;
PFNGLTESTFENCEAPPLEPROC glad_glTestFenceAPPLE = NULL;
PFNGLFINISHFENCEAPPLEPROC glad_glFinishFenceAPPLE = NULL;
PFNGLTESTOBJECTAPPLEPROC glad_glTestObjectAPPLE = NULL;
PFNGLFINISHOBJECTAPPLEPROC glad_glFinishObjectAPPLE = NULL;
PFNGLBUFFERPARAMETERIAPPLEPROC glad_glBufferParameteriAPPLE = NULL;
PFNGLFLUSHMAPPEDBUFFERRANGEAPPLEPROC glad_glFlushMappedBufferRangeAPPLE = NULL;
PFNGLOBJECTPURGEABLEAPPLEPROC glad_glObjectPurgeableAPPLE = NULL;
PFNGLOBJECTUNPURGEABLEAPPLEPROC glad_glObjectUnpurgeableAPPLE = NULL;
PFNGLGETOBJECTPARAMETERIVAPPLEPROC glad_glGetObjectParameterivAPPLE = NULL;
PFNGLTEXTURERANGEAPPLEPROC glad_glTextureRangeAPPLE = NULL;
PFNGLGETTEXPARAMETERPOINTERVAPPLEPROC glad_glGetTexParameterPointervAPPLE = NULL;
PFNGLBINDVERTEXARRAYAPPLEPROC glad_glBindVertexArrayAPPLE = NULL;
PFNGLDELETEVERTEXARRAYSAPPLEPROC glad_glDeleteVertexArraysAPPLE = NULL;
PFNGLGENVERTEXARRAYSAPPLEPROC glad_glGenVertexArraysAPPLE = NULL;
PFNGLISVERTEXARRAYAPPLEPROC glad_glIsVertexArrayAPPLE = NULL;
PFNGLVERTEXARRAYRANGEAPPLEPROC glad_glVertexArrayRangeAPPLE = NULL;
PFNGLFLUSHVERTEXARRAYRANGEAPPLEPROC glad_glFlushVertexArrayRangeAPPLE = NULL;
PFNGLVERTEXARRAYPARAMETERIAPPLEPROC glad_glVertexArrayParameteriAPPLE = NULL;
PFNGLENABLEVERTEXATTRIBAPPLEPROC glad_glEnableVertexAttribAPPLE = NULL;
PFNGLDISABLEVERTEXATTRIBAPPLEPROC glad_glDisableVertexAttribAPPLE = NULL;
PFNGLISVERTEXATTRIBENABLEDAPPLEPROC glad_glIsVertexAttribEnabledAPPLE = NULL;
PFNGLMAPVERTEXATTRIB1DAPPLEPROC glad_glMapVertexAttrib1dAPPLE = NULL;
PFNGLMAPVERTEXATTRIB1FAPPLEPROC glad_glMapVertexAttrib1fAPPLE = NULL;
PFNGLMAPVERTEXATTRIB2DAPPLEPROC glad_glMapVertexAttrib2dAPPLE = NULL;
PFNGLMAPVERTEXATTRIB2FAPPLEPROC glad_glMapVertexAttrib2fAPPLE = NULL;
PFNGLRELEASESHADERCOMPILERPROC glad_glReleaseShaderCompiler = NULL;
PFNGLSHADERBINARYPROC glad_glShaderBinary = NULL;
PFNGLGETSHADERPRECISIONFORMATPROC glad_glGetShaderPrecisionFormat = NULL;
PFNGLDEPTHRANGEFPROC glad_glDepthRangef = NULL;
PFNGLCLEARDEPTHFPROC glad_glClearDepthf = NULL;
PFNGLMEMORYBARRIERBYREGIONPROC glad_glMemoryBarrierByRegion = NULL;
PFNGLPRIMITIVEBOUNDINGBOXARBPROC glad_glPrimitiveBoundingBoxARB = NULL;
PFNGLDRAWARRAYSINSTANCEDBASEINSTANCEPROC glad_glDrawArraysInstancedBaseInstance = NULL;
PFNGLDRAWELEMENTSINSTANCEDBASEINSTANCEPROC glad_glDrawElementsInstancedBaseInstance = NULL;
PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCEPROC glad_glDrawElementsInstancedBaseVertexBaseInstance = NULL;
PFNGLGETTEXTUREHANDLEARBPROC glad_glGetTextureHandleARB = NULL;
PFNGLGETTEXTURESAMPLERHANDLEARBPROC glad_glGetTextureSamplerHandleARB = NULL;
PFNGLMAKETEXTUREHANDLERESIDENTARBPROC glad_glMakeTextureHandleResidentARB = NULL;
PFNGLMAKETEXTUREHANDLENONRESIDENTARBPROC glad_glMakeTextureHandleNonResidentARB = NULL;
PFNGLGETIMAGEHANDLEARBPROC glad_glGetImageHandleARB = NULL;
PFNGLMAKEIMAGEHANDLERESIDENTARBPROC glad_glMakeImageHandleResidentARB = NULL;
PFNGLMAKEIMAGEHANDLENONRESIDENTARBPROC glad_glMakeImageHandleNonResidentARB = NULL;
PFNGLUNIFORMHANDLEUI64ARBPROC glad_glUniformHandleui64ARB = NULL;
PFNGLUNIFORMHANDLEUI64VARBPROC glad_glUniformHandleui64vARB = NULL;
PFNGLPROGRAMUNIFORMHANDLEUI64ARBPROC glad_glProgramUniformHandleui64ARB = NULL;
PFNGLPROGRAMUNIFORMHANDLEUI64VARBPROC glad_glProgramUniformHandleui64vARB = NULL;
PFNGLISTEXTUREHANDLERESIDENTARBPROC glad_glIsTextureHandleResidentARB = NULL;
PFNGLISIMAGEHANDLERESIDENTARBPROC glad_glIsImageHandleResidentARB = NULL;
PFNGLVERTEXATTRIBL1UI64ARBPROC glad_glVertexAttribL1ui64ARB = NULL;
PFNGLVERTEXATTRIBL1UI64VARBPROC glad_glVertexAttribL1ui64vARB = NULL;
PFNGLGETVERTEXATTRIBLUI64VARBPROC glad_glGetVertexAttribLui64vARB = NULL;
PFNGLBUFFERSTORAGEPROC glad_glBufferStorage = NULL;
PFNGLCREATESYNCFROMCLEVENTARBPROC glad_glCreateSyncFromCLeventARB = NULL;
PFNGLCLEARBUFFERDATAPROC glad_glClearBufferData = NULL;
PFNGLCLEARBUFFERSUBDATAPROC glad_glClearBufferSubData = NULL;
PFNGLCLEARTEXIMAGEPROC glad_glClearTexImage = NULL;
PFNGLCLEARTEXSUBIMAGEPROC glad_glClearTexSubImage = NULL;
PFNGLCLIPCONTROLPROC glad_glClipControl = NULL;
PFNGLCLAMPCOLORARBPROC glad_glClampColorARB = NULL;
PFNGLDISPATCHCOMPUTEPROC glad_glDispatchCompute = NULL;
PFNGLDISPATCHCOMPUTEINDIRECTPROC glad_glDispatchComputeIndirect = NULL;
PFNGLDISPATCHCOMPUTEGROUPSIZEARBPROC glad_glDispatchComputeGroupSizeARB = NULL;
PFNGLCOPYIMAGESUBDATAPROC glad_glCopyImageSubData = NULL;
PFNGLDEBUGMESSAGECONTROLARBPROC glad_glDebugMessageControlARB = NULL;
PFNGLDEBUGMESSAGEINSERTARBPROC glad_glDebugMessageInsertARB = NULL;
PFNGLDEBUGMESSAGECALLBACKARBPROC glad_glDebugMessageCallbackARB = NULL;
PFNGLGETDEBUGMESSAGELOGARBPROC glad_glGetDebugMessageLogARB = NULL;
PFNGLCREATETRANSFORMFEEDBACKSPROC glad_glCreateTransformFeedbacks = NULL;
PFNGLTRANSFORMFEEDBACKBUFFERBASEPROC glad_glTransformFeedbackBufferBase = NULL;
PFNGLTRANSFORMFEEDBACKBUFFERRANGEPROC glad_glTransformFeedbackBufferRange = NULL;
PFNGLGETTRANSFORMFEEDBACKIVPROC glad_glGetTransformFeedbackiv = NULL;
PFNGLGETTRANSFORMFEEDBACKI_VPROC glad_glGetTransformFeedbacki_v = NULL;
PFNGLGETTRANSFORMFEEDBACKI64_VPROC glad_glGetTransformFeedbacki64_v = NULL;
PFNGLCREATEBUFFERSPROC glad_glCreateBuffers = NULL;
PFNGLNAMEDBUFFERSTORAGEPROC glad_glNamedBufferStorage = NULL;
PFNGLNAMEDBUFFERDATAPROC glad_glNamedBufferData = NULL;
PFNGLNAMEDBUFFERSUBDATAPROC glad_glNamedBufferSubData = NULL;
PFNGLCOPYNAMEDBUFFERSUBDATAPROC glad_glCopyNamedBufferSubData = NULL;
PFNGLCLEARNAMEDBUFFERDATAPROC glad_glClearNamedBufferData = NULL;
PFNGLCLEARNAMEDBUFFERSUBDATAPROC glad_glClearNamedBufferSubData = NULL;
PFNGLMAPNAMEDBUFFERPROC glad_glMapNamedBuffer = NULL;
PFNGLMAPNAMEDBUFFERRANGEPROC glad_glMapNamedBufferRange = NULL;
PFNGLUNMAPNAMEDBUFFERPROC glad_glUnmapNamedBuffer = NULL;
PFNGLFLUSHMAPPEDNAMEDBUFFERRANGEPROC glad_glFlushMappedNamedBufferRange = NULL;
PFNGLGETNAMEDBUFFERPARAMETERIVPROC glad_glGetNamedBufferParameteriv = NULL;
PFNGLGETNAMEDBUFFERPARAMETERI64VPROC glad_glGetNamedBufferParameteri64v = NULL;
PFNGLGETNAMEDBUFFERPOINTERVPROC glad_glGetNamedBufferPointerv = NULL;
PFNGLGETNAMEDBUFFERSUBDATAPROC glad_glGetNamedBufferSubData = NULL;
PFNGLCREATEFRAMEBUFFERSPROC glad_glCreateFramebuffers = NULL;
PFNGLNAMEDFRAMEBUFFERRENDERBUFFERPROC glad_glNamedFramebufferRenderbuffer = NULL;
PFNGLNAMEDFRAMEBUFFERPARAMETERIPROC glad_glNamedFramebufferParameteri = NULL;
PFNGLNAMEDFRAMEBUFFERTEXTUREPROC glad_glNamedFramebufferTexture = NULL;
PFNGLNAMEDFRAMEBUFFERTEXTURELAYERPROC glad_glNamedFramebufferTextureLayer = NULL;
PFNGLNAMEDFRAMEBUFFERDRAWBUFFERPROC glad_glNamedFramebufferDrawBuffer = NULL;
PFNGLNAMEDFRAMEBUFFERDRAWBUFFERSPROC glad_glNamedFramebufferDrawBuffers = NULL;
PFNGLNAMEDFRAMEBUFFERREADBUFFERPROC glad_glNamedFramebufferReadBuffer = NULL;
PFNGLINVALIDATENAMEDFRAMEBUFFERDATAPROC glad_glInvalidateNamedFramebufferData = NULL;
PFNGLINVALIDATENAMEDFRAMEBUFFERSUBDATAPROC glad_glInvalidateNamedFramebufferSubData = NULL;
PFNGLCLEARNAMEDFRAMEBUFFERIVPROC glad_glClearNamedFramebufferiv = NULL;
PFNGLCLEARNAMEDFRAMEBUFFERUIVPROC glad_glClearNamedFramebufferuiv = NULL;
PFNGLCLEARNAMEDFRAMEBUFFERFVPROC glad_glClearNamedFramebufferfv = NULL;
PFNGLCLEARNAMEDFRAMEBUFFERFIPROC glad_glClearNamedFramebufferfi = NULL;
PFNGLBLITNAMEDFRAMEBUFFERPROC glad_glBlitNamedFramebuffer = NULL;
PFNGLCHECKNAMEDFRAMEBUFFERSTATUSPROC glad_glCheckNamedFramebufferStatus = NULL;
PFNGLGETNAMEDFRAMEBUFFERPARAMETERIVPROC glad_glGetNamedFramebufferParameteriv = NULL;
PFNGLGETNAMEDFRAMEBUFFERATTACHMENTPARAMETERIVPROC glad_glGetNamedFramebufferAttachmentParameteriv = NULL;
PFNGLCREATERENDERBUFFERSPROC glad_glCreateRenderbuffers = NULL;
PFNGLNAMEDRENDERBUFFERSTORAGEPROC glad_glNamedRenderbufferStorage = NULL;
PFNGLNAMEDRENDERBUFFERSTORAGEMULTISAMPLEPROC glad_glNamedRenderbufferStorageMultisample = NULL;
PFNGLGETNAMEDRENDERBUFFERPARAMETERIVPROC glad_glGetNamedRenderbufferParameteriv = NULL;
PFNGLCREATETEXTURESPROC glad_glCreateTextures = NULL;
PFNGLTEXTUREBUFFERPROC glad_glTextureBuffer = NULL;
PFNGLTEXTUREBUFFERRANGEPROC glad_glTextureBufferRange = NULL;
PFNGLTEXTURESTORAGE1DPROC glad_glTextureStorage1D = NULL;
PFNGLTEXTURESTORAGE2DPROC glad_glTextureStorage2D = NULL;
PFNGLTEXTURESTORAGE3DPROC glad_glTextureStorage3D = NULL;
PFNGLTEXTURESTORAGE2DMULTISAMPLEPROC glad_glTextureStorage2DMultisample = NULL;
PFNGLTEXTURESTORAGE3DMULTISAMPLEPROC glad_glTextureStorage3DMultisample = NULL;
PFNGLTEXTURESUBIMAGE1DPROC glad_glTextureSubImage1D = NULL;
PFNGLTEXTURESUBIMAGE2DPROC glad_glTextureSubImage2D = NULL;
PFNGLTEXTURESUBIMAGE3DPROC glad_glTextureSubImage3D = NULL;
PFNGLCOMPRESSEDTEXTURESUBIMAGE1DPROC glad_glCompressedTextureSubImage1D = NULL;
PFNGLCOMPRESSEDTEXTURESUBIMAGE2DPROC glad_glCompressedTextureSubImage2D = NULL;
PFNGLCOMPRESSEDTEXTURESUBIMAGE3DPROC glad_glCompressedTextureSubImage3D = NULL;
PFNGLCOPYTEXTURESUBIMAGE1DPROC glad_glCopyTextureSubImage1D = NULL;
PFNGLCOPYTEXTURESUBIMAGE2DPROC glad_glCopyTextureSubImage2D = NULL;
PFNGLCOPYTEXTURESUBIMAGE3DPROC glad_glCopyTextureSubImage3D = NULL;
PFNGLTEXTUREPARAMETERFPROC glad_glTextureParameterf = NULL;
PFNGLTEXTUREPARAMETERFVPROC glad_glTextureParameterfv = NULL;
PFNGLTEXTUREPARAMETERIPROC glad_glTextureParameteri = NULL;
PFNGLTEXTUREPARAMETERIIVPROC glad_glTextureParameterIiv = NULL;
PFNGLTEXTUREPARAMETERIUIVPROC glad_glTextureParameterIuiv = NULL;
PFNGLTEXTUREPARAMETERIVPROC glad_glTextureParameteriv = NULL;
PFNGLGENERATETEXTUREMIPMAPPROC glad_glGenerateTextureMipmap = NULL;
PFNGLBINDTEXTUREUNITPROC glad_glBindTextureUnit = NULL;
PFNGLGETTEXTUREIMAGEPROC glad_glGetTextureImage = NULL;
PFNGLGETCOMPRESSEDTEXTUREIMAGEPROC glad_glGetCompressedTextureImage = NULL;
PFNGLGETTEXTURELEVELPARAMETERFVPROC glad_glGetTextureLevelParameterfv = NULL;
PFNGLGETTEXTURELEVELPARAMETERIVPROC glad_glGetTextureLevelParameteriv = NULL;
PFNGLGETTEXTUREPARAMETERFVPROC glad_glGetTextureParameterfv = NULL;
PFNGLGETTEXTUREPARAMETERIIVPROC glad_glGetTextureParameterIiv = NULL;
PFNGLGETTEXTUREPARAMETERIUIVPROC glad_glGetTextureParameterIuiv = NULL;
PFNGLGETTEXTUREPARAMETERIVPROC glad_glGetTextureParameteriv = NULL;
PFNGLCREATEVERTEXARRAYSPROC glad_glCreateVertexArrays = NULL;
PFNGLDISABLEVERTEXARRAYATTRIBPROC glad_glDisableVertexArrayAttrib = NULL;
PFNGLENABLEVERTEXARRAYATTRIBPROC glad_glEnableVertexArrayAttrib = NULL;
PFNGLVERTEXARRAYELEMENTBUFFERPROC glad_glVertexArrayElementBuffer = NULL;
PFNGLVERTEXARRAYVERTEXBUFFERPROC glad_glVertexArrayVertexBuffer = NULL;
PFNGLVERTEXARRAYVERTEXBUFFERSPROC glad_glVertexArrayVertexBuffers = NULL;
PFNGLVERTEXARRAYATTRIBBINDINGPROC glad_glVertexArrayAttribBinding = NULL;
PFNGLVERTEXARRAYATTRIBFORMATPROC glad_glVertexArrayAttribFormat = NULL;
PFNGLVERTEXARRAYATTRIBIFORMATPROC glad_glVertexArrayAttribIFormat = NULL;
PFNGLVERTEXARRAYATTRIBLFORMATPROC glad_glVertexArrayAttribLFormat = NULL;
PFNGLVERTEXARRAYBINDINGDIVISORPROC glad_glVertexArrayBindingDivisor = NULL;
PFNGLGETVERTEXARRAYIVPROC glad_glGetVertexArrayiv = NULL;
PFNGLGETVERTEXARRAYINDEXEDIVPROC glad_glGetVertexArrayIndexediv = NULL;
PFNGLGETVERTEXARRAYINDEXED64IVPROC glad_glGetVertexArrayIndexed64iv = NULL;
PFNGLCREATESAMPLERSPROC glad_glCreateSamplers = NULL;
PFNGLCREATEPROGRAMPIPELINESPROC glad_glCreateProgramPipelines = NULL;
PFNGLCREATEQUERIESPROC glad_glCreateQueries = NULL;
PFNGLGETQUERYBUFFEROBJECTI64VPROC glad_glGetQueryBufferObjecti64v = NULL;
PFNGLGETQUERYBUFFEROBJECTIVPROC glad_glGetQueryBufferObjectiv = NULL;
PFNGLGETQUERYBUFFEROBJECTUI64VPROC glad_glGetQueryBufferObjectui64v = NULL;
PFNGLGETQUERYBUFFEROBJECTUIVPROC glad_glGetQueryBufferObjectuiv = NULL;
PFNGLDRAWBUFFERSARBPROC glad_glDrawBuffersARB = NULL;
PFNGLBLENDEQUATIONIARBPROC glad_glBlendEquationiARB = NULL;
PFNGLBLENDEQUATIONSEPARATEIARBPROC glad_glBlendEquationSeparateiARB = NULL;
PFNGLBLENDFUNCIARBPROC glad_glBlendFunciARB = NULL;
PFNGLBLENDFUNCSEPARATEIARBPROC glad_glBlendFuncSeparateiARB = NULL;
PFNGLDRAWARRAYSINDIRECTPROC glad_glDrawArraysIndirect = NULL;
PFNGLDRAWELEMENTSINDIRECTPROC glad_glDrawElementsIndirect = NULL;
PFNGLDRAWARRAYSINSTANCEDARBPROC glad_glDrawArraysInstancedARB = NULL;
PFNGLDRAWELEMENTSINSTANCEDARBPROC glad_glDrawElementsInstancedARB = NULL;
PFNGLPROGRAMSTRINGARBPROC glad_glProgramStringARB = NULL;
PFNGLBINDPROGRAMARBPROC glad_glBindProgramARB = NULL;
PFNGLDELETEPROGRAMSARBPROC glad_glDeleteProgramsARB = NULL;
PFNGLGENPROGRAMSARBPROC glad_glGenProgramsARB = NULL;
PFNGLPROGRAMENVPARAMETER4DARBPROC glad_glProgramEnvParameter4dARB = NULL;
PFNGLPROGRAMENVPARAMETER4DVARBPROC glad_glProgramEnvParameter4dvARB = NULL;
PFNGLPROGRAMENVPARAMETER4FARBPROC glad_glProgramEnvParameter4fARB = NULL;
PFNGLPROGRAMENVPARAMETER4FVARBPROC glad_glProgramEnvParameter4fvARB = NULL;
PFNGLPROGRAMLOCALPARAMETER4DARBPROC glad_glProgramLocalParameter4dARB = NULL;
PFNGLPROGRAMLOCALPARAMETER4DVARBPROC glad_glProgramLocalParameter4dvARB = NULL;
PFNGLPROGRAMLOCALPARAMETER4FARBPROC glad_glProgramLocalParameter4fARB = NULL;
PFNGLPROGRAMLOCALPARAMETER4FVARBPROC glad_glProgramLocalParameter4fvARB = NULL;
PFNGLGETPROGRAMENVPARAMETERDVARBPROC glad_glGetProgramEnvParameterdvARB = NULL;
PFNGLGETPROGRAMENVPARAMETERFVARBPROC glad_glGetProgramEnvParameterfvARB = NULL;
PFNGLGETPROGRAMLOCALPARAMETERDVARBPROC glad_glGetProgramLocalParameterdvARB = NULL;
PFNGLGETPROGRAMLOCALPARAMETERFVARBPROC glad_glGetProgramLocalParameterfvARB = NULL;
PFNGLGETPROGRAMIVARBPROC glad_glGetProgramivARB = NULL;
PFNGLGETPROGRAMSTRINGARBPROC glad_glGetProgramStringARB = NULL;
PFNGLISPROGRAMARBPROC glad_glIsProgramARB = NULL;
PFNGLFRAMEBUFFERPARAMETERIPROC glad_glFramebufferParameteri = NULL;
PFNGLGETFRAMEBUFFERPARAMETERIVPROC glad_glGetFramebufferParameteriv = NULL;
PFNGLPROGRAMPARAMETERIARBPROC glad_glProgramParameteriARB = NULL;
PFNGLFRAMEBUFFERTEXTUREARBPROC glad_glFramebufferTextureARB = NULL;
PFNGLFRAMEBUFFERTEXTURELAYERARBPROC glad_glFramebufferTextureLayerARB = NULL;
PFNGLFRAMEBUFFERTEXTUREFACEARBPROC glad_glFramebufferTextureFaceARB = NULL;
PFNGLGETPROGRAMBINARYPROC glad_glGetProgramBinary = NULL;
PFNGLPROGRAMBINARYPROC glad_glProgramBinary = NULL;
PFNGLPROGRAMPARAMETERIPROC glad_glProgramParameteri = NULL;
PFNGLGETTEXTURESUBIMAGEPROC glad_glGetTextureSubImage = NULL;
PFNGLGETCOMPRESSEDTEXTURESUBIMAGEPROC glad_glGetCompressedTextureSubImage = NULL;
PFNGLSPECIALIZESHADERARBPROC glad_glSpecializeShaderARB = NULL;
PFNGLUNIFORM1DPROC glad_glUniform1d = NULL;
PFNGLUNIFORM2DPROC glad_glUniform2d = NULL;
PFNGLUNIFORM3DPROC glad_glUniform3d = NULL;
PFNGLUNIFORM4DPROC glad_glUniform4d = NULL;
PFNGLUNIFORM1DVPROC glad_glUniform1dv = NULL;
PFNGLUNIFORM2DVPROC glad_glUniform2dv = NULL;
PFNGLUNIFORM3DVPROC glad_glUniform3dv = NULL;
PFNGLUNIFORM4DVPROC glad_glUniform4dv = NULL;
PFNGLUNIFORMMATRIX2DVPROC glad_glUniformMatrix2dv = NULL;
PFNGLUNIFORMMATRIX3DVPROC glad_glUniformMatrix3dv = NULL;
PFNGLUNIFORMMATRIX4DVPROC glad_glUniformMatrix4dv = NULL;
PFNGLUNIFORMMATRIX2X3DVPROC glad_glUniformMatrix2x3dv = NULL;
PFNGLUNIFORMMATRIX2X4DVPROC glad_glUniformMatrix2x4dv = NULL;
PFNGLUNIFORMMATRIX3X2DVPROC glad_glUniformMatrix3x2dv = NULL;
PFNGLUNIFORMMATRIX3X4DVPROC glad_glUniformMatrix3x4dv = NULL;
PFNGLUNIFORMMATRIX4X2DVPROC glad_glUniformMatrix4x2dv = NULL;
PFNGLUNIFORMMATRIX4X3DVPROC glad_glUniformMatrix4x3dv = NULL;
PFNGLGETUNIFORMDVPROC glad_glGetUniformdv = NULL;
PFNGLUNIFORM1I64ARBPROC glad_glUniform1i64ARB = NULL;
PFNGLUNIFORM2I64ARBPROC glad_glUniform2i64ARB = NULL;
PFNGLUNIFORM3I64ARBPROC glad_glUniform3i64ARB = NULL;
PFNGLUNIFORM4I64ARBPROC glad_glUniform4i64ARB = NULL;
PFNGLUNIFORM1I64VARBPROC glad_glUniform1i64vARB = NULL;
PFNGLUNIFORM2I64VARBPROC glad_glUniform2i64vARB = NULL;
PFNGLUNIFORM3I64VARBPROC glad_glUniform3i64vARB = NULL;
PFNGLUNIFORM4I64VARBPROC glad_glUniform4i64vARB = NULL;
PFNGLUNIFORM1UI64ARBPROC glad_glUniform1ui64ARB = NULL;
PFNGLUNIFORM2UI64ARBPROC glad_glUniform2ui64ARB = NULL;
PFNGLUNIFORM3UI64ARBPROC glad_glUniform3ui64ARB = NULL;
PFNGLUNIFORM4UI64ARBPROC glad_glUniform4ui64ARB = NULL;
PFNGLUNIFORM1UI64VARBPROC glad_glUniform1ui64vARB = NULL;
PFNGLUNIFORM2UI64VARBPROC glad_glUniform2ui64vARB = NULL;
PFNGLUNIFORM3UI64VARBPROC glad_glUniform3ui64vARB = NULL;
PFNGLUNIFORM4UI64VARBPROC glad_glUniform4ui64vARB = NULL;
PFNGLGETUNIFORMI64VARBPROC glad_glGetUniformi64vARB = NULL;
PFNGLGETUNIFORMUI64VARBPROC glad_glGetUniformui64vARB = NULL;
PFNGLGETNUNIFORMI64VARBPROC glad_glGetnUniformi64vARB = NULL;
PFNGLGETNUNIFORMUI64VARBPROC glad_glGetnUniformui64vARB = NULL;
PFNGLPROGRAMUNIFORM1I64ARBPROC glad_glProgramUniform1i64ARB = NULL;
PFNGLPROGRAMUNIFORM2I64ARBPROC glad_glProgramUniform2i64ARB = NULL;
PFNGLPROGRAMUNIFORM3I64ARBPROC glad_glProgramUniform3i64ARB = NULL;
PFNGLPROGRAMUNIFORM4I64ARBPROC glad_glProgramUniform4i64ARB = NULL;
PFNGLPROGRAMUNIFORM1I64VARBPROC glad_glProgramUniform1i64vARB = NULL;
PFNGLPROGRAMUNIFORM2I64VARBPROC glad_glProgramUniform2i64vARB = NULL;
PFNGLPROGRAMUNIFORM3I64VARBPROC glad_glProgramUniform3i64vARB = NULL;
PFNGLPROGRAMUNIFORM4I64VARBPROC glad_glProgramUniform4i64vARB = NULL;
PFNGLPROGRAMUNIFORM1UI64ARBPROC glad_glProgramUniform1ui64ARB = NULL;
PFNGLPROGRAMUNIFORM2UI64ARBPROC glad_glProgramUniform2ui64ARB = NULL;
PFNGLPROGRAMUNIFORM3UI64ARBPROC glad_glProgramUniform3ui64ARB = NULL;
PFNGLPROGRAMUNIFORM4UI64ARBPROC glad_glProgramUniform4ui64ARB = NULL;
PFNGLPROGRAMUNIFORM1UI64VARBPROC glad_glProgramUniform1ui64vARB = NULL;
PFNGLPROGRAMUNIFORM2UI64VARBPROC glad_glProgramUniform2ui64vARB = NULL;
PFNGLPROGRAMUNIFORM3UI64VARBPROC glad_glProgramUniform3ui64vARB = NULL;
PFNGLPROGRAMUNIFORM4UI64VARBPROC glad_glProgramUniform4ui64vARB = NULL;
PFNGLCOLORTABLEPROC glad_glColorTable = NULL;
PFNGLCOLORTABLEPARAMETERFVPROC glad_glColorTableParameterfv = NULL;
PFNGLCOLORTABLEPARAMETERIVPROC glad_glColorTableParameteriv = NULL;
PFNGLCOPYCOLORTABLEPROC glad_glCopyColorTable = NULL;
PFNGLGETCOLORTABLEPROC glad_glGetColorTable = NULL;
PFNGLGETCOLORTABLEPARAMETERFVPROC glad_glGetColorTableParameterfv = NULL;
PFNGLGETCOLORTABLEPARAMETERIVPROC glad_glGetColorTableParameteriv = NULL;
PFNGLCOLORSUBTABLEPROC glad_glColorSubTable = NULL;
PFNGLCOPYCOLORSUBTABLEPROC glad_glCopyColorSubTable = NULL;
PFNGLCONVOLUTIONFILTER1DPROC glad_glConvolutionFilter1D = NULL;
PFNGLCONVOLUTIONFILTER2DPROC glad_glConvolutionFilter2D = NULL;
PFNGLCONVOLUTIONPARAMETERFPROC glad_glConvolutionParameterf = NULL;
PFNGLCONVOLUTIONPARAMETERFVPROC glad_glConvolutionParameterfv = NULL;
PFNGLCONVOLUTIONPARAMETERIPROC glad_glConvolutionParameteri = NULL;
PFNGLCONVOLUTIONPARAMETERIVPROC glad_glConvolutionParameteriv = NULL;
PFNGLCOPYCONVOLUTIONFILTER1DPROC glad_glCopyConvolutionFilter1D = NULL;
PFNGLCOPYCONVOLUTIONFILTER2DPROC glad_glCopyConvolutionFilter2D = NULL;
PFNGLGETCONVOLUTIONFILTERPROC glad_glGetConvolutionFilter = NULL;
PFNGLGETCONVOLUTIONPARAMETERFVPROC glad_glGetConvolutionParameterfv = NULL;
PFNGLGETCONVOLUTIONPARAMETERIVPROC glad_glGetConvolutionParameteriv = NULL;
PFNGLGETSEPARABLEFILTERPROC glad_glGetSeparableFilter = NULL;
PFNGLSEPARABLEFILTER2DPROC glad_glSeparableFilter2D = NULL;
PFNGLGETHISTOGRAMPROC glad_glGetHistogram = NULL;
PFNGLGETHISTOGRAMPARAMETERFVPROC glad_glGetHistogramParameterfv = NULL;
PFNGLGETHISTOGRAMPARAMETERIVPROC glad_glGetHistogramParameteriv = NULL;
PFNGLGETMINMAXPROC glad_glGetMinmax = NULL;
PFNGLGETMINMAXPARAMETERFVPROC glad_glGetMinmaxParameterfv = NULL;
PFNGLGETMINMAXPARAMETERIVPROC glad_glGetMinmaxParameteriv = NULL;
PFNGLHISTOGRAMPROC glad_glHistogram = NULL;
PFNGLMINMAXPROC glad_glMinmax = NULL;
PFNGLRESETHISTOGRAMPROC glad_glResetHistogram = NULL;
PFNGLRESETMINMAXPROC glad_glResetMinmax = NULL;
PFNGLMULTIDRAWARRAYSINDIRECTCOUNTARBPROC glad_glMultiDrawArraysIndirectCountARB = NULL;
PFNGLMULTIDRAWELEMENTSINDIRECTCOUNTARBPROC glad_glMultiDrawElementsIndirectCountARB = NULL;
PFNGLVERTEXATTRIBDIVISORARBPROC glad_glVertexAttribDivisorARB = NULL;
PFNGLGETINTERNALFORMATIVPROC glad_glGetInternalformativ = NULL;
PFNGLGETINTERNALFORMATI64VPROC glad_glGetInternalformati64v = NULL;
PFNGLINVALIDATETEXSUBIMAGEPROC glad_glInvalidateTexSubImage = NULL;
PFNGLINVALIDATETEXIMAGEPROC glad_glInvalidateTexImage = NULL;
PFNGLINVALIDATEBUFFERSUBDATAPROC glad_glInvalidateBufferSubData = NULL;
PFNGLINVALIDATEBUFFERDATAPROC glad_glInvalidateBufferData = NULL;
PFNGLINVALIDATEFRAMEBUFFERPROC glad_glInvalidateFramebuffer = NULL;
PFNGLINVALIDATESUBFRAMEBUFFERPROC glad_glInvalidateSubFramebuffer = NULL;
PFNGLCURRENTPALETTEMATRIXARBPROC glad_glCurrentPaletteMatrixARB = NULL;
PFNGLMATRIXINDEXUBVARBPROC glad_glMatrixIndexubvARB = NULL;
PFNGLMATRIXINDEXUSVARBPROC glad_glMatrixIndexusvARB = NULL;
PFNGLMATRIXINDEXUIVARBPROC glad_glMatrixIndexuivARB = NULL;
PFNGLMATRIXINDEXPOINTERARBPROC glad_glMatrixIndexPointerARB = NULL;
PFNGLBINDBUFFERSBASEPROC glad_glBindBuffersBase = NULL;
PFNGLBINDBUFFERSRANGEPROC glad_glBindBuffersRange = NULL;
PFNGLBINDTEXTURESPROC glad_glBindTextures = NULL;
PFNGLBINDSAMPLERSPROC glad_glBindSamplers = NULL;
PFNGLBINDIMAGETEXTURESPROC glad_glBindImageTextures = NULL;
PFNGLBINDVERTEXBUFFERSPROC glad_glBindVertexBuffers = NULL;
PFNGLMULTIDRAWARRAYSINDIRECTPROC glad_glMultiDrawArraysIndirect = NULL;
PFNGLMULTIDRAWELEMENTSINDIRECTPROC glad_glMultiDrawElementsIndirect = NULL;
PFNGLSAMPLECOVERAGEARBPROC glad_glSampleCoverageARB = NULL;
PFNGLACTIVETEXTUREARBPROC glad_glActiveTextureARB = NULL;
PFNGLCLIENTACTIVETEXTUREARBPROC glad_glClientActiveTextureARB = NULL;
PFNGLMULTITEXCOORD1DARBPROC glad_glMultiTexCoord1dARB = NULL;
PFNGLMULTITEXCOORD1DVARBPROC glad_glMultiTexCoord1dvARB = NULL;
PFNGLMULTITEXCOORD1FARBPROC glad_glMultiTexCoord1fARB = NULL;
PFNGLMULTITEXCOORD1FVARBPROC glad_glMultiTexCoord1fvARB = NULL;
PFNGLMULTITEXCOORD1IARBPROC glad_glMultiTexCoord1iARB = NULL;
PFNGLMULTITEXCOORD1IVARBPROC glad_glMultiTexCoord1ivARB = NULL;
PFNGLMULTITEXCOORD1SARBPROC glad_glMultiTexCoord1sARB = NULL;
PFNGLMULTITEXCOORD1SVARBPROC glad_glMultiTexCoord1svARB = NULL;
PFNGLMULTITEXCOORD2DARBPROC glad_glMultiTexCoord2dARB = NULL;
PFNGLMULTITEXCOORD2DVARBPROC glad_glMultiTexCoord2dvARB = NULL;
PFNGLMULTITEXCOORD2FARBPROC glad_glMultiTexCoord2fARB = NULL;
PFNGLMULTITEXCOORD2FVARBPROC glad_glMultiTexCoord2fvARB = NULL;
PFNGLMULTITEXCOORD2IARBPROC glad_glMultiTexCoord2iARB = NULL;
PFNGLMULTITEXCOORD2IVARBPROC glad_glMultiTexCoord2ivARB = NULL;
PFNGLMULTITEXCOORD2SARBPROC glad_glMultiTexCoord2sARB = NULL;
PFNGLMULTITEXCOORD2SVARBPROC glad_glMultiTexCoord2svARB = NULL;
PFNGLMULTITEXCOORD3DARBPROC glad_glMultiTexCoord3dARB = NULL;
PFNGLMULTITEXCOORD3DVARBPROC glad_glMultiTexCoord3dvARB = NULL;
PFNGLMULTITEXCOORD3FARBPROC glad_glMultiTexCoord3fARB = NULL;
PFNGLMULTITEXCOORD3FVARBPROC glad_glMultiTexCoord3fvARB = NULL;
PFNGLMULTITEXCOORD3IARBPROC glad_glMultiTexCoord3iARB = NULL;
PFNGLMULTITEXCOORD3IVARBPROC glad_glMultiTexCoord3ivARB = NULL;
PFNGLMULTITEXCOORD3SARBPROC glad_glMultiTexCoord3sARB = NULL;
PFNGLMULTITEXCOORD3SVARBPROC glad_glMultiTexCoord3svARB = NULL;
PFNGLMULTITEXCOORD4DARBPROC glad_glMultiTexCoord4dARB = NULL;
PFNGLMULTITEXCOORD4DVARBPROC glad_glMultiTexCoord4dvARB = NULL;
PFNGLMULTITEXCOORD4FARBPROC glad_glMultiTexCoord4fARB = NULL;
PFNGLMULTITEXCOORD4FVARBPROC glad_glMultiTexCoord4fvARB = NULL;
PFNGLMULTITEXCOORD4IARBPROC glad_glMultiTexCoord4iARB = NULL;
PFNGLMULTITEXCOORD4IVARBPROC glad_glMultiTexCoord4ivARB = NULL;
PFNGLMULTITEXCOORD4SARBPROC glad_glMultiTexCoord4sARB = NULL;
PFNGLMULTITEXCOORD4SVARBPROC glad_glMultiTexCoord4svARB = NULL;
PFNGLGENQUERIESARBPROC glad_glGenQueriesARB = NULL;
PFNGLDELETEQUERIESARBPROC glad_glDeleteQueriesARB = NULL;
PFNGLISQUERYARBPROC glad_glIsQueryARB = NULL;
PFNGLBEGINQUERYARBPROC glad_glBeginQueryARB = NULL;
PFNGLENDQUERYARBPROC glad_glEndQueryARB = NULL;
PFNGLGETQUERYIVARBPROC glad_glGetQueryivARB = NULL;
PFNGLGETQUERYOBJECTIVARBPROC glad_glGetQueryObjectivARB = NULL;
PFNGLGETQUERYOBJECTUIVARBPROC glad_glGetQueryObjectuivARB = NULL;
PFNGLMAXSHADERCOMPILERTHREADSARBPROC glad_glMaxShaderCompilerThreadsARB = NULL;
PFNGLPOINTPARAMETERFARBPROC glad_glPointParameterfARB = NULL;
PFNGLPOINTPARAMETERFVARBPROC glad_glPointParameterfvARB = NULL;
PFNGLPOLYGONOFFSETCLAMPPROC glad_glPolygonOffsetClamp = NULL;
PFNGLGETPROGRAMINTERFACEIVPROC glad_glGetProgramInterfaceiv = NULL;
PFNGLGETPROGRAMRESOURCEINDEXPROC glad_glGetProgramResourceIndex = NULL;
PFNGLGETPROGRAMRESOURCENAMEPROC glad_glGetProgramResourceName = NULL;
PFNGLGETPROGRAMRESOURCEIVPROC glad_glGetProgramResourceiv = NULL;
PFNGLGETPROGRAMRESOURCELOCATIONPROC glad_glGetProgramResourceLocation = NULL;
PFNGLGETPROGRAMRESOURCELOCATIONINDEXPROC glad_glGetProgramResourceLocationIndex = NULL;
PFNGLGETGRAPHICSRESETSTATUSARBPROC glad_glGetGraphicsResetStatusARB = NULL;
PFNGLGETNTEXIMAGEARBPROC glad_glGetnTexImageARB = NULL;
PFNGLREADNPIXELSARBPROC glad_glReadnPixelsARB = NULL;
PFNGLGETNCOMPRESSEDTEXIMAGEARBPROC glad_glGetnCompressedTexImageARB = NULL;
PFNGLGETNUNIFORMFVARBPROC glad_glGetnUniformfvARB = NULL;
PFNGLGETNUNIFORMIVARBPROC glad_glGetnUniformivARB = NULL;
PFNGLGETNUNIFORMUIVARBPROC glad_glGetnUniformuivARB = NULL;
PFNGLGETNUNIFORMDVARBPROC glad_glGetnUniformdvARB = NULL;
PFNGLGETNMAPDVARBPROC glad_glGetnMapdvARB = NULL;
PFNGLGETNMAPFVARBPROC glad_glGetnMapfvARB = NULL;
PFNGLGETNMAPIVARBPROC glad_glGetnMapivARB = NULL;
PFNGLGETNPIXELMAPFVARBPROC glad_glGetnPixelMapfvARB = NULL;
PFNGLGETNPIXELMAPUIVARBPROC glad_glGetnPixelMapuivARB = NULL;
PFNGLGETNPIXELMAPUSVARBPROC glad_glGetnPixelMapusvARB = NULL;
PFNGLGETNPOLYGONSTIPPLEARBPROC glad_glGetnPolygonStippleARB = NULL;
PFNGLGETNCOLORTABLEARBPROC glad_glGetnColorTableARB = NULL;
PFNGLGETNCONVOLUTIONFILTERARBPROC glad_glGetnConvolutionFilterARB = NULL;
PFNGLGETNSEPARABLEFILTERARBPROC glad_glGetnSeparableFilterARB = NULL;
PFNGLGETNHISTOGRAMARBPROC glad_glGetnHistogramARB = NULL;
PFNGLGETNMINMAXARBPROC glad_glGetnMinmaxARB = NULL;
PFNGLFRAMEBUFFERSAMPLELOCATIONSFVARBPROC glad_glFramebufferSampleLocationsfvARB = NULL;
PFNGLNAMEDFRAMEBUFFERSAMPLELOCATIONSFVARBPROC glad_glNamedFramebufferSampleLocationsfvARB = NULL;
PFNGLEVALUATEDEPTHVALUESARBPROC glad_glEvaluateDepthValuesARB = NULL;
PFNGLMINSAMPLESHADINGARBPROC glad_glMinSampleShadingARB = NULL;
PFNGLUSEPROGRAMSTAGESPROC glad_glUseProgramStages = NULL;
PFNGLACTIVESHADERPROGRAMPROC glad_glActiveShaderProgram = NULL;
PFNGLCREATESHADERPROGRAMVPROC glad_glCreateShaderProgramv = NULL;
PFNGLBINDPROGRAMPIPELINEPROC glad_glBindProgramPipeline = NULL;
PFNGLDELETEPROGRAMPIPELINESPROC glad_glDeleteProgramPipelines = NULL;
PFNGLGENPROGRAMPIPELINESPROC glad_glGenProgramPipelines = NULL;
PFNGLISPROGRAMPIPELINEPROC glad_glIsProgramPipeline = NULL;
PFNGLGETPROGRAMPIPELINEIVPROC glad_glGetProgramPipelineiv = NULL;
PFNGLPROGRAMUNIFORM1IPROC glad_glProgramUniform1i = NULL;
PFNGLPROGRAMUNIFORM1IVPROC glad_glProgramUniform1iv = NULL;
PFNGLPROGRAMUNIFORM1FPROC glad_glProgramUniform1f = NULL;
PFNGLPROGRAMUNIFORM1FVPROC glad_glProgramUniform1fv = NULL;
PFNGLPROGRAMUNIFORM1DPROC glad_glProgramUniform1d = NULL;
PFNGLPROGRAMUNIFORM1DVPROC glad_glProgramUniform1dv = NULL;
PFNGLPROGRAMUNIFORM1UIPROC glad_glProgramUniform1ui = NULL;
PFNGLPROGRAMUNIFORM1UIVPROC glad_glProgramUniform1uiv = NULL;
PFNGLPROGRAMUNIFORM2IPROC glad_glProgramUniform2i = NULL;
PFNGLPROGRAMUNIFORM2IVPROC glad_glProgramUniform2iv = NULL;
PFNGLPROGRAMUNIFORM2FPROC glad_glProgramUniform2f = NULL;
PFNGLPROGRAMUNIFORM2FVPROC glad_glProgramUniform2fv = NULL;
PFNGLPROGRAMUNIFORM2DPROC glad_glProgramUniform2d = NULL;
PFNGLPROGRAMUNIFORM2DVPROC glad_glProgramUniform2dv = NULL;
PFNGLPROGRAMUNIFORM2UIPROC glad_glProgramUniform2ui = NULL;
PFNGLPROGRAMUNIFORM2UIVPROC glad_glProgramUniform2uiv = NULL;
PFNGLPROGRAMUNIFORM3IPROC glad_glProgramUniform3i = NULL;
PFNGLPROGRAMUNIFORM3IVPROC glad_glProgramUniform3iv = NULL;
PFNGLPROGRAMUNIFORM3FPROC glad_glProgramUniform3f = NULL;
PFNGLPROGRAMUNIFORM3FVPROC glad_glProgramUniform3fv = NULL;
PFNGLPROGRAMUNIFORM3DPROC glad_glProgramUniform3d = NULL;
PFNGLPROGRAMUNIFORM3DVPROC glad_glProgramUniform3dv = NULL;
PFNGLPROGRAMUNIFORM3UIPROC glad_glProgramUniform3ui = NULL;
PFNGLPROGRAMUNIFORM3UIVPROC glad_glProgramUniform3uiv = NULL;
PFNGLPROGRAMUNIFORM4IPROC glad_glProgramUniform4i = NULL;
PFNGLPROGRAMUNIFORM4IVPROC glad_glProgramUniform4iv = NULL;
PFNGLPROGRAMUNIFORM4FPROC glad_glProgramUniform4f = NULL;
PFNGLPROGRAMUNIFORM4FVPROC glad_glProgramUniform4fv = NULL;
PFNGLPROGRAMUNIFORM4DPROC glad_glProgramUniform4d = NULL;
PFNGLPROGRAMUNIFORM4DVPROC glad_glProgramUniform4dv = NULL;
PFNGLPROGRAMUNIFORM4UIPROC glad_glProgramUniform4ui = NULL;
PFNGLPROGRAMUNIFORM4UIVPROC glad_glProgramUniform4uiv = NULL;
PFNGLPROGRAMUNIFORMMATRIX2FVPROC glad_glProgramUniformMatrix2fv = NULL;
PFNGLPROGRAMUNIFORMMATRIX3FVPROC glad_glProgramUniformMatrix3fv = NULL;
PFNGLPROGRAMUNIFORMMATRIX4FVPROC glad_glProgramUniformMatrix4fv = NULL;
PFNGLPROGRAMUNIFORMMATRIX2DVPROC glad_glProgramUniformMatrix2dv = NULL;
PFNGLPROGRAMUNIFORMMATRIX3DVPROC glad_glProgramUniformMatrix3dv = NULL;
PFNGLPROGRAMUNIFORMMATRIX4DVPROC glad_glProgramUniformMatrix4dv = NULL;
PFNGLPROGRAMUNIFORMMATRIX2X3FVPROC glad_glProgramUniformMatrix2x3fv = NULL;
PFNGLPROGRAMUNIFORMMATRIX3X2FVPROC glad_glProgramUniformMatrix3x2fv = NULL;
PFNGLPROGRAMUNIFORMMATRIX2X4FVPROC glad_glProgramUniformMatrix2x4fv = NULL;
PFNGLPROGRAMUNIFORMMATRIX4X2FVPROC glad_glProgramUniformMatrix4x2fv = NULL;
PFNGLPROGRAMUNIFORMMATRIX3X4FVPROC glad_glProgramUniformMatrix3x4fv = NULL;
PFNGLPROGRAMUNIFORMMATRIX4X3FVPROC glad_glProgramUniformMatrix4x3fv = NULL;
PFNGLPROGRAMUNIFORMMATRIX2X3DVPROC glad_glProgramUniformMatrix2x3dv = NULL;
PFNGLPROGRAMUNIFORMMATRIX3X2DVPROC glad_glProgramUniformMatrix3x2dv = NULL;
PFNGLPROGRAMUNIFORMMATRIX2X4DVPROC glad_glProgramUniformMatrix2x4dv = NULL;
PFNGLPROGRAMUNIFORMMATRIX4X2DVPROC glad_glProgramUniformMatrix4x2dv = NULL;
PFNGLPROGRAMUNIFORMMATRIX3X4DVPROC glad_glProgramUniformMatrix3x4dv = NULL;
PFNGLPROGRAMUNIFORMMATRIX4X3DVPROC glad_glProgramUniformMatrix4x3dv = NULL;
PFNGLVALIDATEPROGRAMPIPELINEPROC glad_glValidateProgramPipeline = NULL;
PFNGLGETPROGRAMPIPELINEINFOLOGPROC glad_glGetProgramPipelineInfoLog = NULL;
PFNGLGETACTIVEATOMICCOUNTERBUFFERIVPROC glad_glGetActiveAtomicCounterBufferiv = NULL;
PFNGLBINDIMAGETEXTUREPROC glad_glBindImageTexture = NULL;
PFNGLMEMORYBARRIERPROC glad_glMemoryBarrier = NULL;
PFNGLDELETEOBJECTARBPROC glad_glDeleteObjectARB = NULL;
PFNGLGETHANDLEARBPROC glad_glGetHandleARB = NULL;
PFNGLDETACHOBJECTARBPROC glad_glDetachObjectARB = NULL;
PFNGLCREATESHADEROBJECTARBPROC glad_glCreateShaderObjectARB = NULL;
PFNGLSHADERSOURCEARBPROC glad_glShaderSourceARB = NULL;
PFNGLCOMPILESHADERARBPROC glad_glCompileShaderARB = NULL;
PFNGLCREATEPROGRAMOBJECTARBPROC glad_glCreateProgramObjectARB = NULL;
PFNGLATTACHOBJECTARBPROC glad_glAttachObjectARB = NULL;
PFNGLLINKPROGRAMARBPROC glad_glLinkProgramARB = NULL;
PFNGLUSEPROGRAMOBJECTARBPROC glad_glUseProgramObjectARB = NULL;
PFNGLVALIDATEPROGRAMARBPROC glad_glValidateProgramARB = NULL;
PFNGLUNIFORM1FARBPROC glad_glUniform1fARB = NULL;
PFNGLUNIFORM2FARBPROC glad_glUniform2fARB = NULL;
PFNGLUNIFORM3FARBPROC glad_glUniform3fARB = NULL;
PFNGLUNIFORM4FARBPROC glad_glUniform4fARB = NULL;
PFNGLUNIFORM1IARBPROC glad_glUniform1iARB = NULL;
PFNGLUNIFORM2IARBPROC glad_glUniform2iARB = NULL;
PFNGLUNIFORM3IARBPROC glad_glUniform3iARB = NULL;
PFNGLUNIFORM4IARBPROC glad_glUniform4iARB = NULL;
PFNGLUNIFORM1FVARBPROC glad_glUniform1fvARB = NULL;
PFNGLUNIFORM2FVARBPROC glad_glUniform2fvARB = NULL;
PFNGLUNIFORM3FVARBPROC glad_glUniform3fvARB = NULL;
PFNGLUNIFORM4FVARBPROC glad_glUniform4fvARB = NULL;
PFNGLUNIFORM1IVARBPROC glad_glUniform1ivARB = NULL;
PFNGLUNIFORM2IVARBPROC glad_glUniform2ivARB = NULL;
PFNGLUNIFORM3IVARBPROC glad_glUniform3ivARB = NULL;
PFNGLUNIFORM4IVARBPROC glad_glUniform4ivARB = NULL;
PFNGLUNIFORMMATRIX2FVARBPROC glad_glUniformMatrix2fvARB = NULL;
PFNGLUNIFORMMATRIX3FVARBPROC glad_glUniformMatrix3fvARB = NULL;
PFNGLUNIFORMMATRIX4FVARBPROC glad_glUniformMatrix4fvARB = NULL;
PFNGLGETOBJECTPARAMETERFVARBPROC glad_glGetObjectParameterfvARB = NULL;
PFNGLGETOBJECTPARAMETERIVARBPROC glad_glGetObjectParameterivARB = NULL;
PFNGLGETINFOLOGARBPROC glad_glGetInfoLogARB = NULL;
PFNGLGETATTACHEDOBJECTSARBPROC glad_glGetAttachedObjectsARB = NULL;
PFNGLGETUNIFORMLOCATIONARBPROC glad_glGetUniformLocationARB = NULL;
PFNGLGETACTIVEUNIFORMARBPROC glad_glGetActiveUniformARB = NULL;
PFNGLGETUNIFORMFVARBPROC glad_glGetUniformfvARB = NULL;
PFNGLGETUNIFORMIVARBPROC glad_glGetUniformivARB = NULL;
PFNGLGETSHADERSOURCEARBPROC glad_glGetShaderSourceARB = NULL;
PFNGLSHADERSTORAGEBLOCKBINDINGPROC glad_glShaderStorageBlockBinding = NULL;
PFNGLGETSUBROUTINEUNIFORMLOCATIONPROC glad_glGetSubroutineUniformLocation = NULL;
PFNGLGETSUBROUTINEINDEXPROC glad_glGetSubroutineIndex = NULL;
PFNGLGETACTIVESUBROUTINEUNIFORMIVPROC glad_glGetActiveSubroutineUniformiv = NULL;
PFNGLGETACTIVESUBROUTINEUNIFORMNAMEPROC glad_glGetActiveSubroutineUniformName = NULL;
PFNGLGETACTIVESUBROUTINENAMEPROC glad_glGetActiveSubroutineName = NULL;
PFNGLUNIFORMSUBROUTINESUIVPROC glad_glUniformSubroutinesuiv = NULL;
PFNGLGETUNIFORMSUBROUTINEUIVPROC glad_glGetUniformSubroutineuiv = NULL;
PFNGLGETPROGRAMSTAGEIVPROC glad_glGetProgramStageiv = NULL;
PFNGLNAMEDSTRINGARBPROC glad_glNamedStringARB = NULL;
PFNGLDELETENAMEDSTRINGARBPROC glad_glDeleteNamedStringARB = NULL;
PFNGLCOMPILESHADERINCLUDEARBPROC glad_glCompileShaderIncludeARB = NULL;
PFNGLISNAMEDSTRINGARBPROC glad_glIsNamedStringARB = NULL;
PFNGLGETNAMEDSTRINGARBPROC glad_glGetNamedStringARB = NULL;
PFNGLGETNAMEDSTRINGIVARBPROC glad_glGetNamedStringivARB = NULL;
PFNGLBUFFERPAGECOMMITMENTARBPROC glad_glBufferPageCommitmentARB = NULL;
PFNGLNAMEDBUFFERPAGECOMMITMENTEXTPROC glad_glNamedBufferPageCommitmentEXT = NULL;
PFNGLNAMEDBUFFERPAGECOMMITMENTARBPROC glad_glNamedBufferPageCommitmentARB = NULL;
PFNGLTEXPAGECOMMITMENTARBPROC glad_glTexPageCommitmentARB = NULL;
PFNGLPATCHPARAMETERIPROC glad_glPatchParameteri = NULL;
PFNGLPATCHPARAMETERFVPROC glad_glPatchParameterfv = NULL;
PFNGLTEXTUREBARRIERPROC glad_glTextureBarrier = NULL;
PFNGLTEXBUFFERARBPROC glad_glTexBufferARB = NULL;
PFNGLTEXBUFFERRANGEPROC glad_glTexBufferRange = NULL;
PFNGLCOMPRESSEDTEXIMAGE3DARBPROC glad_glCompressedTexImage3DARB = NULL;
PFNGLCOMPRESSEDTEXIMAGE2DARBPROC glad_glCompressedTexImage2DARB = NULL;
PFNGLCOMPRESSEDTEXIMAGE1DARBPROC glad_glCompressedTexImage1DARB = NULL;
PFNGLCOMPRESSEDTEXSUBIMAGE3DARBPROC glad_glCompressedTexSubImage3DARB = NULL;
PFNGLCOMPRESSEDTEXSUBIMAGE2DARBPROC glad_glCompressedTexSubImage2DARB = NULL;
PFNGLCOMPRESSEDTEXSUBIMAGE1DARBPROC glad_glCompressedTexSubImage1DARB = NULL;
PFNGLGETCOMPRESSEDTEXIMAGEARBPROC glad_glGetCompressedTexImageARB = NULL;
PFNGLTEXSTORAGE1DPROC glad_glTexStorage1D = NULL;
PFNGLTEXSTORAGE2DPROC glad_glTexStorage2D = NULL;
PFNGLTEXSTORAGE3DPROC glad_glTexStorage3D = NULL;
PFNGLTEXSTORAGE2DMULTISAMPLEPROC glad_glTexStorage2DMultisample = NULL;
PFNGLTEXSTORAGE3DMULTISAMPLEPROC glad_glTexStorage3DMultisample = NULL;
PFNGLTEXTUREVIEWPROC glad_glTextureView = NULL;
PFNGLBINDTRANSFORMFEEDBACKPROC glad_glBindTransformFeedback = NULL;
PFNGLDELETETRANSFORMFEEDBACKSPROC glad_glDeleteTransformFeedbacks = NULL;
PFNGLGENTRANSFORMFEEDBACKSPROC glad_glGenTransformFeedbacks = NULL;
PFNGLISTRANSFORMFEEDBACKPROC glad_glIsTransformFeedback = NULL;
PFNGLPAUSETRANSFORMFEEDBACKPROC glad_glPauseTransformFeedback = NULL;
PFNGLRESUMETRANSFORMFEEDBACKPROC glad_glResumeTransformFeedback = NULL;
PFNGLDRAWTRANSFORMFEEDBACKPROC glad_glDrawTransformFeedback = NULL;
PFNGLDRAWTRANSFORMFEEDBACKSTREAMPROC glad_glDrawTransformFeedbackStream = NULL;
PFNGLBEGINQUERYINDEXEDPROC glad_glBeginQueryIndexed = NULL;
PFNGLENDQUERYINDEXEDPROC glad_glEndQueryIndexed = NULL;
PFNGLGETQUERYINDEXEDIVPROC glad_glGetQueryIndexediv = NULL;
PFNGLDRAWTRANSFORMFEEDBACKINSTANCEDPROC glad_glDrawTransformFeedbackInstanced = NULL;
PFNGLDRAWTRANSFORMFEEDBACKSTREAMINSTANCEDPROC glad_glDrawTransformFeedbackStreamInstanced = NULL;
PFNGLLOADTRANSPOSEMATRIXFARBPROC glad_glLoadTransposeMatrixfARB = NULL;
PFNGLLOADTRANSPOSEMATRIXDARBPROC glad_glLoadTransposeMatrixdARB = NULL;
PFNGLMULTTRANSPOSEMATRIXFARBPROC glad_glMultTransposeMatrixfARB = NULL;
PFNGLMULTTRANSPOSEMATRIXDARBPROC glad_glMultTransposeMatrixdARB = NULL;
PFNGLVERTEXATTRIBL1DPROC glad_glVertexAttribL1d = NULL;
PFNGLVERTEXATTRIBL2DPROC glad_glVertexAttribL2d = NULL;
PFNGLVERTEXATTRIBL3DPROC glad_glVertexAttribL3d = NULL;
PFNGLVERTEXATTRIBL4DPROC glad_glVertexAttribL4d = NULL;
PFNGLVERTEXATTRIBL1DVPROC glad_glVertexAttribL1dv = NULL;
PFNGLVERTEXATTRIBL2DVPROC glad_glVertexAttribL2dv = NULL;
PFNGLVERTEXATTRIBL3DVPROC glad_glVertexAttribL3dv = NULL;
PFNGLVERTEXATTRIBL4DVPROC glad_glVertexAttribL4dv = NULL;
PFNGLVERTEXATTRIBLPOINTERPROC glad_glVertexAttribLPointer = NULL;
PFNGLGETVERTEXATTRIBLDVPROC glad_glGetVertexAttribLdv = NULL;
PFNGLBINDVERTEXBUFFERPROC glad_glBindVertexBuffer = NULL;
PFNGLVERTEXATTRIBFORMATPROC glad_glVertexAttribFormat = NULL;
PFNGLVERTEXATTRIBIFORMATPROC glad_glVertexAttribIFormat = NULL;
PFNGLVERTEXATTRIBLFORMATPROC glad_glVertexAttribLFormat = NULL;
PFNGLVERTEXATTRIBBINDINGPROC glad_glVertexAttribBinding = NULL;
PFNGLVERTEXBINDINGDIVISORPROC glad_glVertexBindingDivisor = NULL;
PFNGLWEIGHTBVARBPROC glad_glWeightbvARB = NULL;
PFNGLWEIGHTSVARBPROC glad_glWeightsvARB = NULL;
PFNGLWEIGHTIVARBPROC glad_glWeightivARB = NULL;
PFNGLWEIGHTFVARBPROC glad_glWeightfvARB = NULL;
PFNGLWEIGHTDVARBPROC glad_glWeightdvARB = NULL;
PFNGLWEIGHTUBVARBPROC glad_glWeightubvARB = NULL;
PFNGLWEIGHTUSVARBPROC glad_glWeightusvARB = NULL;
PFNGLWEIGHTUIVARBPROC glad_glWeightuivARB = NULL;
PFNGLWEIGHTPOINTERARBPROC glad_glWeightPointerARB = NULL;
PFNGLVERTEXBLENDARBPROC glad_glVertexBlendARB = NULL;
PFNGLBINDBUFFERARBPROC glad_glBindBufferARB = NULL;
PFNGLDELETEBUFFERSARBPROC glad_glDeleteBuffersARB = NULL;
PFNGLGENBUFFERSARBPROC glad_glGenBuffersARB = NULL;
PFNGLISBUFFERARBPROC glad_glIsBufferARB = NULL;
PFNGLBUFFERDATAARBPROC glad_glBufferDataARB = NULL;
PFNGLBUFFERSUBDATAARBPROC glad_glBufferSubDataARB = NULL;
PFNGLGETBUFFERSUBDATAARBPROC glad_glGetBufferSubDataARB = NULL;
PFNGLMAPBUFFERARBPROC glad_glMapBufferARB = NULL;
PFNGLUNMAPBUFFERARBPROC glad_glUnmapBufferARB = NULL;
PFNGLGETBUFFERPARAMETERIVARBPROC glad_glGetBufferParameterivARB = NULL;
PFNGLGETBUFFERPOINTERVARBPROC glad_glGetBufferPointervARB = NULL;
PFNGLVERTEXATTRIB1DARBPROC glad_glVertexAttrib1dARB = NULL;
PFNGLVERTEXATTRIB1DVARBPROC glad_glVertexAttrib1dvARB = NULL;
PFNGLVERTEXATTRIB1FARBPROC glad_glVertexAttrib1fARB = NULL;
PFNGLVERTEXATTRIB1FVARBPROC glad_glVertexAttrib1fvARB = NULL;
PFNGLVERTEXATTRIB1SARBPROC glad_glVertexAttrib1sARB = NULL;
PFNGLVERTEXATTRIB1SVARBPROC glad_glVertexAttrib1svARB = NULL;
PFNGLVERTEXATTRIB2DARBPROC glad_glVertexAttrib2dARB = NULL;
PFNGLVERTEXATTRIB2DVARBPROC glad_glVertexAttrib2dvARB = NULL;
PFNGLVERTEXATTRIB2FARBPROC glad_glVertexAttrib2fARB = NULL;
PFNGLVERTEXATTRIB2FVARBPROC glad_glVertexAttrib2fvARB = NULL;
PFNGLVERTEXATTRIB2SARBPROC glad_glVertexAttrib2sARB = NULL;
PFNGLVERTEXATTRIB2SVARBPROC glad_glVertexAttrib2svARB = NULL;
PFNGLVERTEXATTRIB3DARBPROC glad_glVertexAttrib3dARB = NULL;
PFNGLVERTEXATTRIB3DVARBPROC glad_glVertexAttrib3dvARB = NULL;
PFNGLVERTEXATTRIB3FARBPROC glad_glVertexAttrib3fARB = NULL;
PFNGLVERTEXATTRIB3FVARBPROC glad_glVertexAttrib3fvARB = NULL;
PFNGLVERTEXATTRIB3SARBPROC glad_glVertexAttrib3sARB = NULL;
PFNGLVERTEXATTRIB3SVARBPROC glad_glVertexAttrib3svARB = NULL;
PFNGLVERTEXATTRIB4NBVARBPROC glad_glVertexAttrib4NbvARB = NULL;
PFNGLVERTEXATTRIB4NIVARBPROC glad_glVertexAttrib4NivARB = NULL;
PFNGLVERTEXATTRIB4NSVARBPROC glad_glVertexAttrib4NsvARB = NULL;
PFNGLVERTEXATTRIB4NUBARBPROC glad_glVertexAttrib4NubARB = NULL;
PFNGLVERTEXATTRIB4NUBVARBPROC glad_glVertexAttrib4NubvARB = NULL;
PFNGLVERTEXATTRIB4NUIVARBPROC glad_glVertexAttrib4NuivARB = NULL;
PFNGLVERTEXATTRIB4NUSVARBPROC glad_glVertexAttrib4NusvARB = NULL;
PFNGLVERTEXATTRIB4BVARBPROC glad_glVertexAttrib4bvARB = NULL;
PFNGLVERTEXATTRIB4DARBPROC glad_glVertexAttrib4dARB = NULL;
PFNGLVERTEXATTRIB4DVARBPROC glad_glVertexAttrib4dvARB = NULL;
PFNGLVERTEXATTRIB4FARBPROC glad_glVertexAttrib4fARB = NULL;
PFNGLVERTEXATTRIB4FVARBPROC glad_glVertexAttrib4fvARB = NULL;
PFNGLVERTEXATTRIB4IVARBPROC glad_glVertexAttrib4ivARB = NULL;
PFNGLVERTEXATTRIB4SARBPROC glad_glVertexAttrib4sARB = NULL;
PFNGLVERTEXATTRIB4SVARBPROC glad_glVertexAttrib4svARB = NULL;
PFNGLVERTEXATTRIB4UBVARBPROC glad_glVertexAttrib4ubvARB = NULL;
PFNGLVERTEXATTRIB4UIVARBPROC glad_glVertexAttrib4uivARB = NULL;
PFNGLVERTEXATTRIB4USVARBPROC glad_glVertexAttrib4usvARB = NULL;
PFNGLVERTEXATTRIBPOINTERARBPROC glad_glVertexAttribPointerARB = NULL;
PFNGLENABLEVERTEXATTRIBARRAYARBPROC glad_glEnableVertexAttribArrayARB = NULL;
PFNGLDISABLEVERTEXATTRIBARRAYARBPROC glad_glDisableVertexAttribArrayARB = NULL;
PFNGLGETVERTEXATTRIBDVARBPROC glad_glGetVertexAttribdvARB = NULL;
PFNGLGETVERTEXATTRIBFVARBPROC glad_glGetVertexAttribfvARB = NULL;
PFNGLGETVERTEXATTRIBIVARBPROC glad_glGetVertexAttribivARB = NULL;
PFNGLGETVERTEXATTRIBPOINTERVARBPROC glad_glGetVertexAttribPointervARB = NULL;
PFNGLBINDATTRIBLOCATIONARBPROC glad_glBindAttribLocationARB = NULL;
PFNGLGETACTIVEATTRIBARBPROC glad_glGetActiveAttribARB = NULL;
PFNGLGETATTRIBLOCATIONARBPROC glad_glGetAttribLocationARB = NULL;
PFNGLVIEWPORTARRAYVPROC glad_glViewportArrayv = NULL;
PFNGLVIEWPORTINDEXEDFPROC glad_glViewportIndexedf = NULL;
PFNGLVIEWPORTINDEXEDFVPROC glad_glViewportIndexedfv = NULL;
PFNGLSCISSORARRAYVPROC glad_glScissorArrayv = NULL;
PFNGLSCISSORINDEXEDPROC glad_glScissorIndexed = NULL;
PFNGLSCISSORINDEXEDVPROC glad_glScissorIndexedv = NULL;
PFNGLDEPTHRANGEARRAYVPROC glad_glDepthRangeArrayv = NULL;
PFNGLDEPTHRANGEINDEXEDPROC glad_glDepthRangeIndexed = NULL;
PFNGLGETFLOATI_VPROC glad_glGetFloati_v = NULL;
PFNGLGETDOUBLEI_VPROC glad_glGetDoublei_v = NULL;
PFNGLDEPTHRANGEARRAYDVNVPROC glad_glDepthRangeArraydvNV = NULL;
PFNGLDEPTHRANGEINDEXEDDNVPROC glad_glDepthRangeIndexeddNV = NULL;
PFNGLWINDOWPOS2DARBPROC glad_glWindowPos2dARB = NULL;
PFNGLWINDOWPOS2DVARBPROC glad_glWindowPos2dvARB = NULL;
PFNGLWINDOWPOS2FARBPROC glad_glWindowPos2fARB = NULL;
PFNGLWINDOWPOS2FVARBPROC glad_glWindowPos2fvARB = NULL;
PFNGLWINDOWPOS2IARBPROC glad_glWindowPos2iARB = NULL;
PFNGLWINDOWPOS2IVARBPROC glad_glWindowPos2ivARB = NULL;
PFNGLWINDOWPOS2SARBPROC glad_glWindowPos2sARB = NULL;
PFNGLWINDOWPOS2SVARBPROC glad_glWindowPos2svARB = NULL;
PFNGLWINDOWPOS3DARBPROC glad_glWindowPos3dARB = NULL;
PFNGLWINDOWPOS3DVARBPROC glad_glWindowPos3dvARB = NULL;
PFNGLWINDOWPOS3FARBPROC glad_glWindowPos3fARB = NULL;
PFNGLWINDOWPOS3FVARBPROC glad_glWindowPos3fvARB = NULL;
PFNGLWINDOWPOS3IARBPROC glad_glWindowPos3iARB = NULL;
PFNGLWINDOWPOS3IVARBPROC glad_glWindowPos3ivARB = NULL;
PFNGLWINDOWPOS3SARBPROC glad_glWindowPos3sARB = NULL;
PFNGLWINDOWPOS3SVARBPROC glad_glWindowPos3svARB = NULL;
PFNGLDRAWBUFFERSATIPROC glad_glDrawBuffersATI = NULL;
PFNGLELEMENTPOINTERATIPROC glad_glElementPointerATI = NULL;
PFNGLDRAWELEMENTARRAYATIPROC glad_glDrawElementArrayATI = NULL;
PFNGLDRAWRANGEELEMENTARRAYATIPROC glad_glDrawRangeElementArrayATI = NULL;
PFNGLTEXBUMPPARAMETERIVATIPROC glad_glTexBumpParameterivATI = NULL;
PFNGLTEXBUMPPARAMETERFVATIPROC glad_glTexBumpParameterfvATI = NULL;
PFNGLGETTEXBUMPPARAMETERIVATIPROC glad_glGetTexBumpParameterivATI = NULL;
PFNGLGETTEXBUMPPARAMETERFVATIPROC glad_glGetTexBumpParameterfvATI = NULL;
PFNGLGENFRAGMENTSHADERSATIPROC glad_glGenFragmentShadersATI = NULL;
PFNGLBINDFRAGMENTSHADERATIPROC glad_glBindFragmentShaderATI = NULL;
PFNGLDELETEFRAGMENTSHADERATIPROC glad_glDeleteFragmentShaderATI = NULL;
PFNGLBEGINFRAGMENTSHADERATIPROC glad_glBeginFragmentShaderATI = NULL;
PFNGLENDFRAGMENTSHADERATIPROC glad_glEndFragmentShaderATI = NULL;
PFNGLPASSTEXCOORDATIPROC glad_glPassTexCoordATI = NULL;
PFNGLSAMPLEMAPATIPROC glad_glSampleMapATI = NULL;
PFNGLCOLORFRAGMENTOP1ATIPROC glad_glColorFragmentOp1ATI = NULL;
PFNGLCOLORFRAGMENTOP2ATIPROC glad_glColorFragmentOp2ATI = NULL;
PFNGLCOLORFRAGMENTOP3ATIPROC glad_glColorFragmentOp3ATI = NULL;
PFNGLALPHAFRAGMENTOP1ATIPROC glad_glAlphaFragmentOp1ATI = NULL;
PFNGLALPHAFRAGMENTOP2ATIPROC glad_glAlphaFragmentOp2ATI = NULL;
PFNGLALPHAFRAGMENTOP3ATIPROC glad_glAlphaFragmentOp3ATI = NULL;
PFNGLSETFRAGMENTSHADERCONSTANTATIPROC glad_glSetFragmentShaderConstantATI = NULL;
PFNGLMAPOBJECTBUFFERATIPROC glad_glMapObjectBufferATI = NULL;
PFNGLUNMAPOBJECTBUFFERATIPROC glad_glUnmapObjectBufferATI = NULL;
PFNGLPNTRIANGLESIATIPROC glad_glPNTrianglesiATI = NULL;
PFNGLPNTRIANGLESFATIPROC glad_glPNTrianglesfATI = NULL;
PFNGLSTENCILOPSEPARATEATIPROC glad_glStencilOpSeparateATI = NULL;
PFNGLSTENCILFUNCSEPARATEATIPROC glad_glStencilFuncSeparateATI = NULL;
PFNGLNEWOBJECTBUFFERATIPROC glad_glNewObjectBufferATI = NULL;
PFNGLISOBJECTBUFFERATIPROC glad_glIsObjectBufferATI = NULL;
PFNGLUPDATEOBJECTBUFFERATIPROC glad_glUpdateObjectBufferATI = NULL;
PFNGLGETOBJECTBUFFERFVATIPROC glad_glGetObjectBufferfvATI = NULL;
PFNGLGETOBJECTBUFFERIVATIPROC glad_glGetObjectBufferivATI = NULL;
PFNGLFREEOBJECTBUFFERATIPROC glad_glFreeObjectBufferATI = NULL;
PFNGLARRAYOBJECTATIPROC glad_glArrayObjectATI = NULL;
PFNGLGETARRAYOBJECTFVATIPROC glad_glGetArrayObjectfvATI = NULL;
PFNGLGETARRAYOBJECTIVATIPROC glad_glGetArrayObjectivATI = NULL;
PFNGLVARIANTARRAYOBJECTATIPROC glad_glVariantArrayObjectATI = NULL;
PFNGLGETVARIANTARRAYOBJECTFVATIPROC glad_glGetVariantArrayObjectfvATI = NULL;
PFNGLGETVARIANTARRAYOBJECTIVATIPROC glad_glGetVariantArrayObjectivATI = NULL;
PFNGLVERTEXATTRIBARRAYOBJECTATIPROC glad_glVertexAttribArrayObjectATI = NULL;
PFNGLGETVERTEXATTRIBARRAYOBJECTFVATIPROC glad_glGetVertexAttribArrayObjectfvATI = NULL;
PFNGLGETVERTEXATTRIBARRAYOBJECTIVATIPROC glad_glGetVertexAttribArrayObjectivATI = NULL;
PFNGLVERTEXSTREAM1SATIPROC glad_glVertexStream1sATI = NULL;
PFNGLVERTEXSTREAM1SVATIPROC glad_glVertexStream1svATI = NULL;
PFNGLVERTEXSTREAM1IATIPROC glad_glVertexStream1iATI = NULL;
PFNGLVERTEXSTREAM1IVATIPROC glad_glVertexStream1ivATI = NULL;
PFNGLVERTEXSTREAM1FATIPROC glad_glVertexStream1fATI = NULL;
PFNGLVERTEXSTREAM1FVATIPROC glad_glVertexStream1fvATI = NULL;
PFNGLVERTEXSTREAM1DATIPROC glad_glVertexStream1dATI = NULL;
PFNGLVERTEXSTREAM1DVATIPROC glad_glVertexStream1dvATI = NULL;
PFNGLVERTEXSTREAM2SATIPROC glad_glVertexStream2sATI = NULL;
PFNGLVERTEXSTREAM2SVATIPROC glad_glVertexStream2svATI = NULL;
PFNGLVERTEXSTREAM2IATIPROC glad_glVertexStream2iATI = NULL;
PFNGLVERTEXSTREAM2IVATIPROC glad_glVertexStream2ivATI = NULL;
PFNGLVERTEXSTREAM2FATIPROC glad_glVertexStream2fATI = NULL;
PFNGLVERTEXSTREAM2FVATIPROC glad_glVertexStream2fvATI = NULL;
PFNGLVERTEXSTREAM2DATIPROC glad_glVertexStream2dATI = NULL;
PFNGLVERTEXSTREAM2DVATIPROC glad_glVertexStream2dvATI = NULL;
PFNGLVERTEXSTREAM3SATIPROC glad_glVertexStream3sATI = NULL;
PFNGLVERTEXSTREAM3SVATIPROC glad_glVertexStream3svATI = NULL;
PFNGLVERTEXSTREAM3IATIPROC glad_glVertexStream3iATI = NULL;
PFNGLVERTEXSTREAM3IVATIPROC glad_glVertexStream3ivATI = NULL;
PFNGLVERTEXSTREAM3FATIPROC glad_glVertexStream3fATI = NULL;
PFNGLVERTEXSTREAM3FVATIPROC glad_glVertexStream3fvATI = NULL;
PFNGLVERTEXSTREAM3DATIPROC glad_glVertexStream3dATI = NULL;
PFNGLVERTEXSTREAM3DVATIPROC glad_glVertexStream3dvATI = NULL;
PFNGLVERTEXSTREAM4SATIPROC glad_glVertexStream4sATI = NULL;
PFNGLVERTEXSTREAM4SVATIPROC glad_glVertexStream4svATI = NULL;
PFNGLVERTEXSTREAM4IATIPROC glad_glVertexStream4iATI = NULL;
PFNGLVERTEXSTREAM4IVATIPROC glad_glVertexStream4ivATI = NULL;
PFNGLVERTEXSTREAM4FATIPROC glad_glVertexStream4fATI = NULL;
PFNGLVERTEXSTREAM4FVATIPROC glad_glVertexStream4fvATI = NULL;
PFNGLVERTEXSTREAM4DATIPROC glad_glVertexStream4dATI = NULL;
PFNGLVERTEXSTREAM4DVATIPROC glad_glVertexStream4dvATI = NULL;
PFNGLNORMALSTREAM3BATIPROC glad_glNormalStream3bATI = NULL;
PFNGLNORMALSTREAM3BVATIPROC glad_glNormalStream3bvATI = NULL;
PFNGLNORMALSTREAM3SATIPROC glad_glNormalStream3sATI = NULL;
PFNGLNORMALSTREAM3SVATIPROC glad_glNormalStream3svATI = NULL;
PFNGLNORMALSTREAM3IATIPROC glad_glNormalStream3iATI = NULL;
PFNGLNORMALSTREAM3IVATIPROC glad_glNormalStream3ivATI = NULL;
PFNGLNORMALSTREAM3FATIPROC glad_glNormalStream3fATI = NULL;
PFNGLNORMALSTREAM3FVATIPROC glad_glNormalStream3fvATI = NULL;
PFNGLNORMALSTREAM3DATIPROC glad_glNormalStream3dATI = NULL;
PFNGLNORMALSTREAM3DVATIPROC glad_glNormalStream3dvATI = NULL;
PFNGLCLIENTACTIVEVERTEXSTREAMATIPROC glad_glClientActiveVertexStreamATI = NULL;
PFNGLVERTEXBLENDENVIATIPROC glad_glVertexBlendEnviATI = NULL;
PFNGLVERTEXBLENDENVFATIPROC glad_glVertexBlendEnvfATI = NULL;
PFNGLEGLIMAGETARGETTEXSTORAGEEXTPROC glad_glEGLImageTargetTexStorageEXT = NULL;
PFNGLEGLIMAGETARGETTEXTURESTORAGEEXTPROC glad_glEGLImageTargetTextureStorageEXT = NULL;
PFNGLUNIFORMBUFFEREXTPROC glad_glUniformBufferEXT = NULL;
PFNGLGETUNIFORMBUFFERSIZEEXTPROC glad_glGetUniformBufferSizeEXT = NULL;
PFNGLGETUNIFORMOFFSETEXTPROC glad_glGetUniformOffsetEXT = NULL;
PFNGLBLENDCOLOREXTPROC glad_glBlendColorEXT = NULL;
PFNGLBLENDEQUATIONSEPARATEEXTPROC glad_glBlendEquationSeparateEXT = NULL;
PFNGLBLENDFUNCSEPARATEEXTPROC glad_glBlendFuncSeparateEXT = NULL;
PFNGLBLENDEQUATIONEXTPROC glad_glBlendEquationEXT = NULL;
PFNGLCOLORSUBTABLEEXTPROC glad_glColorSubTableEXT = NULL;
PFNGLCOPYCOLORSUBTABLEEXTPROC glad_glCopyColorSubTableEXT = NULL;
PFNGLLOCKARRAYSEXTPROC glad_glLockArraysEXT = NULL;
PFNGLUNLOCKARRAYSEXTPROC glad_glUnlockArraysEXT = NULL;
PFNGLCONVOLUTIONFILTER1DEXTPROC glad_glConvolutionFilter1DEXT = NULL;
PFNGLCONVOLUTIONFILTER2DEXTPROC glad_glConvolutionFilter2DEXT = NULL;
PFNGLCONVOLUTIONPARAMETERFEXTPROC glad_glConvolutionParameterfEXT = NULL;
PFNGLCONVOLUTIONPARAMETERFVEXTPROC glad_glConvolutionParameterfvEXT = NULL;
PFNGLCONVOLUTIONPARAMETERIEXTPROC glad_glConvolutionParameteriEXT = NULL;
PFNGLCONVOLUTIONPARAMETERIVEXTPROC glad_glConvolutionParameterivEXT = NULL;
PFNGLCOPYCONVOLUTIONFILTER1DEXTPROC glad_glCopyConvolutionFilter1DEXT = NULL;
PFNGLCOPYCONVOLUTIONFILTER2DEXTPROC glad_glCopyConvolutionFilter2DEXT = NULL;
PFNGLGETCONVOLUTIONFILTEREXTPROC glad_glGetConvolutionFilterEXT = NULL;
PFNGLGETCONVOLUTIONPARAMETERFVEXTPROC glad_glGetConvolutionParameterfvEXT = NULL;
PFNGLGETCONVOLUTIONPARAMETERIVEXTPROC glad_glGetConvolutionParameterivEXT = NULL;
PFNGLGETSEPARABLEFILTEREXTPROC glad_glGetSeparableFilterEXT = NULL;
PFNGLSEPARABLEFILTER2DEXTPROC glad_glSeparableFilter2DEXT = NULL;
PFNGLTANGENT3BEXTPROC glad_glTangent3bEXT = NULL;
PFNGLTANGENT3BVEXTPROC glad_glTangent3bvEXT = NULL;
PFNGLTANGENT3DEXTPROC glad_glTangent3dEXT = NULL;
PFNGLTANGENT3DVEXTPROC glad_glTangent3dvEXT = NULL;
PFNGLTANGENT3FEXTPROC glad_glTangent3fEXT = NULL;
PFNGLTANGENT3FVEXTPROC glad_glTangent3fvEXT = NULL;
PFNGLTANGENT3IEXTPROC glad_glTangent3iEXT = NULL;
PFNGLTANGENT3IVEXTPROC glad_glTangent3ivEXT = NULL;
PFNGLTANGENT3SEXTPROC glad_glTangent3sEXT = NULL;
PFNGLTANGENT3SVEXTPROC glad_glTangent3svEXT = NULL;
PFNGLBINORMAL3BEXTPROC glad_glBinormal3bEXT = NULL;
PFNGLBINORMAL3BVEXTPROC glad_glBinormal3bvEXT = NULL;
PFNGLBINORMAL3DEXTPROC glad_glBinormal3dEXT = NULL;
PFNGLBINORMAL3DVEXTPROC glad_glBinormal3dvEXT = NULL;
PFNGLBINORMAL3FEXTPROC glad_glBinormal3fEXT = NULL;
PFNGLBINORMAL3FVEXTPROC glad_glBinormal3fvEXT = NULL;
PFNGLBINORMAL3IEXTPROC glad_glBinormal3iEXT = NULL;
PFNGLBINORMAL3IVEXTPROC glad_glBinormal3ivEXT = NULL;
PFNGLBINORMAL3SEXTPROC glad_glBinormal3sEXT = NULL;
PFNGLBINORMAL3SVEXTPROC glad_glBinormal3svEXT = NULL;
PFNGLTANGENTPOINTEREXTPROC glad_glTangentPointerEXT = NULL;
PFNGLBINORMALPOINTEREXTPROC glad_glBinormalPointerEXT = NULL;
PFNGLCOPYTEXIMAGE1DEXTPROC glad_glCopyTexImage1DEXT = NULL;
PFNGLCOPYTEXIMAGE2DEXTPROC glad_glCopyTexImage2DEXT = NULL;
PFNGLCOPYTEXSUBIMAGE1DEXTPROC glad_glCopyTexSubImage1DEXT = NULL;
PFNGLCOPYTEXSUBIMAGE2DEXTPROC glad_glCopyTexSubImage2DEXT = NULL;
PFNGLCOPYTEXSUBIMAGE3DEXTPROC glad_glCopyTexSubImage3DEXT = NULL;
PFNGLCULLPARAMETERDVEXTPROC glad_glCullParameterdvEXT = NULL;
PFNGLCULLPARAMETERFVEXTPROC glad_glCullParameterfvEXT = NULL;
PFNGLLABELOBJECTEXTPROC glad_glLabelObjectEXT = NULL;
PFNGLGETOBJECTLABELEXTPROC glad_glGetObjectLabelEXT = NULL;
PFNGLINSERTEVENTMARKEREXTPROC glad_glInsertEventMarkerEXT = NULL;
PFNGLPUSHGROUPMARKEREXTPROC glad_glPushGroupMarkerEXT = NULL;
PFNGLPOPGROUPMARKEREXTPROC glad_glPopGroupMarkerEXT = NULL;
PFNGLDEPTHBOUNDSEXTPROC glad_glDepthBoundsEXT = NULL;
PFNGLMATRIXLOADFEXTPROC glad_glMatrixLoadfEXT = NULL;
PFNGLMATRIXLOADDEXTPROC glad_glMatrixLoaddEXT = NULL;
PFNGLMATRIXMULTFEXTPROC glad_glMatrixMultfEXT = NULL;
PFNGLMATRIXMULTDEXTPROC glad_glMatrixMultdEXT = NULL;
PFNGLMATRIXLOADIDENTITYEXTPROC glad_glMatrixLoadIdentityEXT = NULL;
PFNGLMATRIXROTATEFEXTPROC glad_glMatrixRotatefEXT = NULL;
PFNGLMATRIXROTATEDEXTPROC glad_glMatrixRotatedEXT = NULL;
PFNGLMATRIXSCALEFEXTPROC glad_glMatrixScalefEXT = NULL;
PFNGLMATRIXSCALEDEXTPROC glad_glMatrixScaledEXT = NULL;
PFNGLMATRIXTRANSLATEFEXTPROC glad_glMatrixTranslatefEXT = NULL;
PFNGLMATRIXTRANSLATEDEXTPROC glad_glMatrixTranslatedEXT = NULL;
PFNGLMATRIXFRUSTUMEXTPROC glad_glMatrixFrustumEXT = NULL;
PFNGLMATRIXORTHOEXTPROC glad_glMatrixOrthoEXT = NULL;
PFNGLMATRIXPOPEXTPROC glad_glMatrixPopEXT = NULL;
PFNGLMATRIXPUSHEXTPROC glad_glMatrixPushEXT = NULL;
PFNGLCLIENTATTRIBDEFAULTEXTPROC glad_glClientAttribDefaultEXT = NULL;
PFNGLPUSHCLIENTATTRIBDEFAULTEXTPROC glad_glPushClientAttribDefaultEXT = NULL;
PFNGLTEXTUREPARAMETERFEXTPROC glad_glTextureParameterfEXT = NULL;
PFNGLTEXTUREPARAMETERFVEXTPROC glad_glTextureParameterfvEXT = NULL;
PFNGLTEXTUREPARAMETERIEXTPROC glad_glTextureParameteriEXT = NULL;
PFNGLTEXTUREPARAMETERIVEXTPROC glad_glTextureParameterivEXT = NULL;
PFNGLTEXTUREIMAGE1DEXTPROC glad_glTextureImage1DEXT = NULL;
PFNGLTEXTUREIMAGE2DEXTPROC glad_glTextureImage2DEXT = NULL;
PFNGLTEXTURESUBIMAGE1DEXTPROC glad_glTextureSubImage1DEXT = NULL;
PFNGLTEXTURESUBIMAGE2DEXTPROC glad_glTextureSubImage2DEXT = NULL;
PFNGLCOPYTEXTUREIMAGE1DEXTPROC glad_glCopyTextureImage1DEXT = NULL;
PFNGLCOPYTEXTUREIMAGE2DEXTPROC glad_glCopyTextureImage2DEXT = NULL;
PFNGLCOPYTEXTURESUBIMAGE1DEXTPROC glad_glCopyTextureSubImage1DEXT = NULL;
PFNGLCOPYTEXTURESUBIMAGE2DEXTPROC glad_glCopyTextureSubImage2DEXT = NULL;
PFNGLGETTEXTUREIMAGEEXTPROC glad_glGetTextureImageEXT = NULL;
PFNGLGETTEXTUREPARAMETERFVEXTPROC glad_glGetTextureParameterfvEXT = NULL;
PFNGLGETTEXTUREPARAMETERIVEXTPROC glad_glGetTextureParameterivEXT = NULL;
PFNGLGETTEXTURELEVELPARAMETERFVEXTPROC glad_glGetTextureLevelParameterfvEXT = NULL;
PFNGLGETTEXTURELEVELPARAMETERIVEXTPROC glad_glGetTextureLevelParameterivEXT = NULL;
PFNGLTEXTUREIMAGE3DEXTPROC glad_glTextureImage3DEXT = NULL;
PFNGLTEXTURESUBIMAGE3DEXTPROC glad_glTextureSubImage3DEXT = NULL;
PFNGLCOPYTEXTURESUBIMAGE3DEXTPROC glad_glCopyTextureSubImage3DEXT = NULL;
PFNGLBINDMULTITEXTUREEXTPROC glad_glBindMultiTextureEXT = NULL;
PFNGLMULTITEXCOORDPOINTEREXTPROC glad_glMultiTexCoordPointerEXT = NULL;
PFNGLMULTITEXENVFEXTPROC glad_glMultiTexEnvfEXT = NULL;
PFNGLMULTITEXENVFVEXTPROC glad_glMultiTexEnvfvEXT = NULL;
PFNGLMULTITEXENVIEXTPROC glad_glMultiTexEnviEXT = NULL;
PFNGLMULTITEXENVIVEXTPROC glad_glMultiTexEnvivEXT = NULL;
PFNGLMULTITEXGENDEXTPROC glad_glMultiTexGendEXT = NULL;
PFNGLMULTITEXGENDVEXTPROC glad_glMultiTexGendvEXT = NULL;
PFNGLMULTITEXGENFEXTPROC glad_glMultiTexGenfEXT = NULL;
PFNGLMULTITEXGENFVEXTPROC glad_glMultiTexGenfvEXT = NULL;
PFNGLMULTITEXGENIEXTPROC glad_glMultiTexGeniEXT = NULL;
PFNGLMULTITEXGENIVEXTPROC glad_glMultiTexGenivEXT = NULL;
PFNGLGETMULTITEXENVFVEXTPROC glad_glGetMultiTexEnvfvEXT = NULL;
PFNGLGETMULTITEXENVIVEXTPROC glad_glGetMultiTexEnvivEXT = NULL;
PFNGLGETMULTITEXGENDVEXTPROC glad_glGetMultiTexGendvEXT = NULL;
PFNGLGETMULTITEXGENFVEXTPROC glad_glGetMultiTexGenfvEXT = NULL;
PFNGLGETMULTITEXGENIVEXTPROC glad_glGetMultiTexGenivEXT = NULL;
PFNGLMULTITEXPARAMETERIEXTPROC glad_glMultiTexParameteriEXT = NULL;
PFNGLMULTITEXPARAMETERIVEXTPROC glad_glMultiTexParameterivEXT = NULL;
PFNGLMULTITEXPARAMETERFEXTPROC glad_glMultiTexParameterfEXT = NULL;
PFNGLMULTITEXPARAMETERFVEXTPROC glad_glMultiTexParameterfvEXT = NULL;
PFNGLMULTITEXIMAGE1DEXTPROC glad_glMultiTexImage1DEXT = NULL;
PFNGLMULTITEXIMAGE2DEXTPROC glad_glMultiTexImage2DEXT = NULL;
PFNGLMULTITEXSUBIMAGE1DEXTPROC glad_glMultiTexSubImage1DEXT = NULL;
PFNGLMULTITEXSUBIMAGE2DEXTPROC glad_glMultiTexSubImage2DEXT = NULL;
PFNGLCOPYMULTITEXIMAGE1DEXTPROC glad_glCopyMultiTexImage1DEXT = NULL;
PFNGLCOPYMULTITEXIMAGE2DEXTPROC glad_glCopyMultiTexImage2DEXT = NULL;
PFNGLCOPYMULTITEXSUBIMAGE1DEXTPROC glad_glCopyMultiTexSubImage1DEXT = NULL;
PFNGLCOPYMULTITEXSUBIMAGE2DEXTPROC glad_glCopyMultiTexSubImage2DEXT = NULL;
PFNGLGETMULTITEXIMAGEEXTPROC glad_glGetMultiTexImageEXT = NULL;
PFNGLGETMULTITEXPARAMETERFVEXTPROC glad_glGetMultiTexParameterfvEXT = NULL;
PFNGLGETMULTITEXPARAMETERIVEXTPROC glad_glGetMultiTexParameterivEXT = NULL;
PFNGLGETMULTITEXLEVELPARAMETERFVEXTPROC glad_glGetMultiTexLevelParameterfvEXT = NULL;
PFNGLGETMULTITEXLEVELPARAMETERIVEXTPROC glad_glGetMultiTexLevelParameterivEXT = NULL;
PFNGLMULTITEXIMAGE3DEXTPROC glad_glMultiTexImage3DEXT = NULL;
PFNGLMULTITEXSUBIMAGE3DEXTPROC glad_glMultiTexSubImage3DEXT = NULL;
PFNGLCOPYMULTITEXSUBIMAGE3DEXTPROC glad_glCopyMultiTexSubImage3DEXT = NULL;
PFNGLENABLECLIENTSTATEINDEXEDEXTPROC glad_glEnableClientStateIndexedEXT = NULL;
PFNGLDISABLECLIENTSTATEINDEXEDEXTPROC glad_glDisableClientStateIndexedEXT = NULL;
PFNGLGETFLOATINDEXEDVEXTPROC glad_glGetFloatIndexedvEXT = NULL;
PFNGLGETDOUBLEINDEXEDVEXTPROC glad_glGetDoubleIndexedvEXT = NULL;
PFNGLGETPOINTERINDEXEDVEXTPROC glad_glGetPointerIndexedvEXT = NULL;
PFNGLENABLEINDEXEDEXTPROC glad_glEnableIndexedEXT = NULL;
PFNGLDISABLEINDEXEDEXTPROC glad_glDisableIndexedEXT = NULL;
PFNGLISENABLEDINDEXEDEXTPROC glad_glIsEnabledIndexedEXT = NULL;
PFNGLGETINTEGERINDEXEDVEXTPROC glad_glGetIntegerIndexedvEXT = NULL;
PFNGLGETBOOLEANINDEXEDVEXTPROC glad_glGetBooleanIndexedvEXT = NULL;
PFNGLCOMPRESSEDTEXTUREIMAGE3DEXTPROC glad_glCompressedTextureImage3DEXT = NULL;
PFNGLCOMPRESSEDTEXTUREIMAGE2DEXTPROC glad_glCompressedTextureImage2DEXT = NULL;
PFNGLCOMPRESSEDTEXTUREIMAGE1DEXTPROC glad_glCompressedTextureImage1DEXT = NULL;
PFNGLCOMPRESSEDTEXTURESUBIMAGE3DEXTPROC glad_glCompressedTextureSubImage3DEXT = NULL;
PFNGLCOMPRESSEDTEXTURESUBIMAGE2DEXTPROC glad_glCompressedTextureSubImage2DEXT = NULL;
PFNGLCOMPRESSEDTEXTURESUBIMAGE1DEXTPROC glad_glCompressedTextureSubImage1DEXT = NULL;
PFNGLGETCOMPRESSEDTEXTUREIMAGEEXTPROC glad_glGetCompressedTextureImageEXT = NULL;
PFNGLCOMPRESSEDMULTITEXIMAGE3DEXTPROC glad_glCompressedMultiTexImage3DEXT = NULL;
PFNGLCOMPRESSEDMULTITEXIMAGE2DEXTPROC glad_glCompressedMultiTexImage2DEXT = NULL;
PFNGLCOMPRESSEDMULTITEXIMAGE1DEXTPROC glad_glCompressedMultiTexImage1DEXT = NULL;
PFNGLCOMPRESSEDMULTITEXSUBIMAGE3DEXTPROC glad_glCompressedMultiTexSubImage3DEXT = NULL;
PFNGLCOMPRESSEDMULTITEXSUBIMAGE2DEXTPROC glad_glCompressedMultiTexSubImage2DEXT = NULL;
PFNGLCOMPRESSEDMULTITEXSUBIMAGE1DEXTPROC glad_glCompressedMultiTexSubImage1DEXT = NULL;
PFNGLGETCOMPRESSEDMULTITEXIMAGEEXTPROC glad_glGetCompressedMultiTexImageEXT = NULL;
PFNGLMATRIXLOADTRANSPOSEFEXTPROC glad_glMatrixLoadTransposefEXT = NULL;
PFNGLMATRIXLOADTRANSPOSEDEXTPROC glad_glMatrixLoadTransposedEXT = NULL;
PFNGLMATRIXMULTTRANSPOSEFEXTPROC glad_glMatrixMultTransposefEXT = NULL;
PFNGLMATRIXMULTTRANSPOSEDEXTPROC glad_glMatrixMultTransposedEXT = NULL;
PFNGLNAMEDBUFFERDATAEXTPROC glad_glNamedBufferDataEXT = NULL;
PFNGLNAMEDBUFFERSUBDATAEXTPROC glad_glNamedBufferSubDataEXT = NULL;
PFNGLMAPNAMEDBUFFEREXTPROC glad_glMapNamedBufferEXT = NULL;
PFNGLUNMAPNAMEDBUFFEREXTPROC glad_glUnmapNamedBufferEXT = NULL;
PFNGLGETNAMEDBUFFERPARAMETERIVEXTPROC glad_glGetNamedBufferParameterivEXT = NULL;
PFNGLGETNAMEDBUFFERPOINTERVEXTPROC glad_glGetNamedBufferPointervEXT = NULL;
PFNGLGETNAMEDBUFFERSUBDATAEXTPROC glad_glGetNamedBufferSubDataEXT = NULL;
PFNGLPROGRAMUNIFORM1FEXTPROC glad_glProgramUniform1fEXT = NULL;
PFNGLPROGRAMUNIFORM2FEXTPROC glad_glProgramUniform2fEXT = NULL;
PFNGLPROGRAMUNIFORM3FEXTPROC glad_glProgramUniform3fEXT = NULL;
PFNGLPROGRAMUNIFORM4FEXTPROC glad_glProgramUniform4fEXT = NULL;
PFNGLPROGRAMUNIFORM1IEXTPROC glad_glProgramUniform1iEXT = NULL;
PFNGLPROGRAMUNIFORM2IEXTPROC glad_glProgramUniform2iEXT = NULL;
PFNGLPROGRAMUNIFORM3IEXTPROC glad_glProgramUniform3iEXT = NULL;
PFNGLPROGRAMUNIFORM4IEXTPROC glad_glProgramUniform4iEXT = NULL;
PFNGLPROGRAMUNIFORM1FVEXTPROC glad_glProgramUniform1fvEXT = NULL;
PFNGLPROGRAMUNIFORM2FVEXTPROC glad_glProgramUniform2fvEXT = NULL;
PFNGLPROGRAMUNIFORM3FVEXTPROC glad_glProgramUniform3fvEXT = NULL;
PFNGLPROGRAMUNIFORM4FVEXTPROC glad_glProgramUniform4fvEXT = NULL;
PFNGLPROGRAMUNIFORM1IVEXTPROC glad_glProgramUniform1ivEXT = NULL;
PFNGLPROGRAMUNIFORM2IVEXTPROC glad_glProgramUniform2ivEXT = NULL;
PFNGLPROGRAMUNIFORM3IVEXTPROC glad_glProgramUniform3ivEXT = NULL;
PFNGLPROGRAMUNIFORM4IVEXTPROC glad_glProgramUniform4ivEXT = NULL;
PFNGLPROGRAMUNIFORMMATRIX2FVEXTPROC glad_glProgramUniformMatrix2fvEXT = NULL;
PFNGLPROGRAMUNIFORMMATRIX3FVEXTPROC glad_glProgramUniformMatrix3fvEXT = NULL;
PFNGLPROGRAMUNIFORMMATRIX4FVEXTPROC glad_glProgramUniformMatrix4fvEXT = NULL;
PFNGLPROGRAMUNIFORMMATRIX2X3FVEXTPROC glad_glProgramUniformMatrix2x3fvEXT = NULL;
PFNGLPROGRAMUNIFORMMATRIX3X2FVEXTPROC glad_glProgramUniformMatrix3x2fvEXT = NULL;
PFNGLPROGRAMUNIFORMMATRIX2X4FVEXTPROC glad_glProgramUniformMatrix2x4fvEXT = NULL;
PFNGLPROGRAMUNIFORMMATRIX4X2FVEXTPROC glad_glProgramUniformMatrix4x2fvEXT = NULL;
PFNGLPROGRAMUNIFORMMATRIX3X4FVEXTPROC glad_glProgramUniformMatrix3x4fvEXT = NULL;
PFNGLPROGRAMUNIFORMMATRIX4X3FVEXTPROC glad_glProgramUniformMatrix4x3fvEXT = NULL;
PFNGLTEXTUREBUFFEREXTPROC glad_glTextureBufferEXT = NULL;
PFNGLMULTITEXBUFFEREXTPROC glad_glMultiTexBufferEXT = NULL;
PFNGLTEXTUREPARAMETERIIVEXTPROC glad_glTextureParameterIivEXT = NULL;
PFNGLTEXTUREPARAMETERIUIVEXTPROC glad_glTextureParameterIuivEXT = NULL;
PFNGLGETTEXTUREPARAMETERIIVEXTPROC glad_glGetTextureParameterIivEXT = NULL;
PFNGLGETTEXTUREPARAMETERIUIVEXTPROC glad_glGetTextureParameterIuivEXT = NULL;
PFNGLMULTITEXPARAMETERIIVEXTPROC glad_glMultiTexParameterIivEXT = NULL;
PFNGLMULTITEXPARAMETERIUIVEXTPROC glad_glMultiTexParameterIuivEXT = NULL;
PFNGLGETMULTITEXPARAMETERIIVEXTPROC glad_glGetMultiTexParameterIivEXT = NULL;
PFNGLGETMULTITEXPARAMETERIUIVEXTPROC glad_glGetMultiTexParameterIuivEXT = NULL;
PFNGLPROGRAMUNIFORM1UIEXTPROC glad_glProgramUniform1uiEXT = NULL;
PFNGLPROGRAMUNIFORM2UIEXTPROC glad_glProgramUniform2uiEXT = NULL;
PFNGLPROGRAMUNIFORM3UIEXTPROC glad_glProgramUniform3uiEXT = NULL;
PFNGLPROGRAMUNIFORM4UIEXTPROC glad_glProgramUniform4uiEXT = NULL;
PFNGLPROGRAMUNIFORM1UIVEXTPROC glad_glProgramUniform1uivEXT = NULL;
PFNGLPROGRAMUNIFORM2UIVEXTPROC glad_glProgramUniform2uivEXT = NULL;
PFNGLPROGRAMUNIFORM3UIVEXTPROC glad_glProgramUniform3uivEXT = NULL;
PFNGLPROGRAMUNIFORM4UIVEXTPROC glad_glProgramUniform4uivEXT = NULL;
PFNGLNAMEDPROGRAMLOCALPARAMETERS4FVEXTPROC glad_glNamedProgramLocalParameters4fvEXT = NULL;
PFNGLNAMEDPROGRAMLOCALPARAMETERI4IEXTPROC glad_glNamedProgramLocalParameterI4iEXT = NULL;
PFNGLNAMEDPROGRAMLOCALPARAMETERI4IVEXTPROC glad_glNamedProgramLocalParameterI4ivEXT = NULL;
PFNGLNAMEDPROGRAMLOCALPARAMETERSI4IVEXTPROC glad_glNamedProgramLocalParametersI4ivEXT = NULL;
PFNGLNAMEDPROGRAMLOCALPARAMETERI4UIEXTPROC glad_glNamedProgramLocalParameterI4uiEXT = NULL;
PFNGLNAMEDPROGRAMLOCALPARAMETERI4UIVEXTPROC glad_glNamedProgramLocalParameterI4uivEXT = NULL;
PFNGLNAMEDPROGRAMLOCALPARAMETERSI4UIVEXTPROC glad_glNamedProgramLocalParametersI4uivEXT = NULL;
PFNGLGETNAMEDPROGRAMLOCALPARAMETERIIVEXTPROC glad_glGetNamedProgramLocalParameterIivEXT = NULL;
PFNGLGETNAMEDPROGRAMLOCALPARAMETERIUIVEXTPROC glad_glGetNamedProgramLocalParameterIuivEXT = NULL;
PFNGLENABLECLIENTSTATEIEXTPROC glad_glEnableClientStateiEXT = NULL;
PFNGLDISABLECLIENTSTATEIEXTPROC glad_glDisableClientStateiEXT = NULL;
PFNGLGETFLOATI_VEXTPROC glad_glGetFloati_vEXT = NULL;
PFNGLGETDOUBLEI_VEXTPROC glad_glGetDoublei_vEXT = NULL;
PFNGLGETPOINTERI_VEXTPROC glad_glGetPointeri_vEXT = NULL;
PFNGLNAMEDPROGRAMSTRINGEXTPROC glad_glNamedProgramStringEXT = NULL;
PFNGLNAMEDPROGRAMLOCALPARAMETER4DEXTPROC glad_glNamedProgramLocalParameter4dEXT = NULL;
PFNGLNAMEDPROGRAMLOCALPARAMETER4DVEXTPROC glad_glNamedProgramLocalParameter4dvEXT = NULL;
PFNGLNAMEDPROGRAMLOCALPARAMETER4FEXTPROC glad_glNamedProgramLocalParameter4fEXT = NULL;
PFNGLNAMEDPROGRAMLOCALPARAMETER4FVEXTPROC glad_glNamedProgramLocalParameter4fvEXT = NULL;
PFNGLGETNAMEDPROGRAMLOCALPARAMETERDVEXTPROC glad_glGetNamedProgramLocalParameterdvEXT = NULL;
PFNGLGETNAMEDPROGRAMLOCALPARAMETERFVEXTPROC glad_glGetNamedProgramLocalParameterfvEXT = NULL;
PFNGLGETNAMEDPROGRAMIVEXTPROC glad_glGetNamedProgramivEXT = NULL;
PFNGLGETNAMEDPROGRAMSTRINGEXTPROC glad_glGetNamedProgramStringEXT = NULL;
PFNGLNAMEDRENDERBUFFERSTORAGEEXTPROC glad_glNamedRenderbufferStorageEXT = NULL;
PFNGLGETNAMEDRENDERBUFFERPARAMETERIVEXTPROC glad_glGetNamedRenderbufferParameterivEXT = NULL;
PFNGLNAMEDRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC glad_glNamedRenderbufferStorageMultisampleEXT = NULL;
PFNGLNAMEDRENDERBUFFERSTORAGEMULTISAMPLECOVERAGEEXTPROC glad_glNamedRenderbufferStorageMultisampleCoverageEXT = NULL;
PFNGLCHECKNAMEDFRAMEBUFFERSTATUSEXTPROC glad_glCheckNamedFramebufferStatusEXT = NULL;
PFNGLNAMEDFRAMEBUFFERTEXTURE1DEXTPROC glad_glNamedFramebufferTexture1DEXT = NULL;
PFNGLNAMEDFRAMEBUFFERTEXTURE2DEXTPROC glad_glNamedFramebufferTexture2DEXT = NULL;
PFNGLNAMEDFRAMEBUFFERTEXTURE3DEXTPROC glad_glNamedFramebufferTexture3DEXT = NULL;
PFNGLNAMEDFRAMEBUFFERRENDERBUFFEREXTPROC glad_glNamedFramebufferRenderbufferEXT = NULL;
PFNGLGETNAMEDFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC glad_glGetNamedFramebufferAttachmentParameterivEXT = NULL;
PFNGLGENERATETEXTUREMIPMAPEXTPROC glad_glGenerateTextureMipmapEXT = NULL;
PFNGLGENERATEMULTITEXMIPMAPEXTPROC glad_glGenerateMultiTexMipmapEXT = NULL;
PFNGLFRAMEBUFFERDRAWBUFFEREXTPROC glad_glFramebufferDrawBufferEXT = NULL;
PFNGLFRAMEBUFFERDRAWBUFFERSEXTPROC glad_glFramebufferDrawBuffersEXT = NULL;
PFNGLFRAMEBUFFERREADBUFFEREXTPROC glad_glFramebufferReadBufferEXT = NULL;
PFNGLGETFRAMEBUFFERPARAMETERIVEXTPROC glad_glGetFramebufferParameterivEXT = NULL;
PFNGLNAMEDCOPYBUFFERSUBDATAEXTPROC glad_glNamedCopyBufferSubDataEXT = NULL;
PFNGLNAMEDFRAMEBUFFERTEXTUREEXTPROC glad_glNamedFramebufferTextureEXT = NULL;
PFNGLNAMEDFRAMEBUFFERTEXTURELAYEREXTPROC glad_glNamedFramebufferTextureLayerEXT = NULL;
PFNGLNAMEDFRAMEBUFFERTEXTUREFACEEXTPROC glad_glNamedFramebufferTextureFaceEXT = NULL;
PFNGLTEXTURERENDERBUFFEREXTPROC glad_glTextureRenderbufferEXT = NULL;
PFNGLMULTITEXRENDERBUFFEREXTPROC glad_glMultiTexRenderbufferEXT = NULL;
PFNGLVERTEXARRAYVERTEXOFFSETEXTPROC glad_glVertexArrayVertexOffsetEXT = NULL;
PFNGLVERTEXARRAYCOLOROFFSETEXTPROC glad_glVertexArrayColorOffsetEXT = NULL;
PFNGLVERTEXARRAYEDGEFLAGOFFSETEXTPROC glad_glVertexArrayEdgeFlagOffsetEXT = NULL;
PFNGLVERTEXARRAYINDEXOFFSETEXTPROC glad_glVertexArrayIndexOffsetEXT = NULL;
PFNGLVERTEXARRAYNORMALOFFSETEXTPROC glad_glVertexArrayNormalOffsetEXT = NULL;
PFNGLVERTEXARRAYTEXCOORDOFFSETEXTPROC glad_glVertexArrayTexCoordOffsetEXT = NULL;
PFNGLVERTEXARRAYMULTITEXCOORDOFFSETEXTPROC glad_glVertexArrayMultiTexCoordOffsetEXT = NULL;
PFNGLVERTEXARRAYFOGCOORDOFFSETEXTPROC glad_glVertexArrayFogCoordOffsetEXT = NULL;
PFNGLVERTEXARRAYSECONDARYCOLOROFFSETEXTPROC glad_glVertexArraySecondaryColorOffsetEXT = NULL;
PFNGLVERTEXARRAYVERTEXATTRIBOFFSETEXTPROC glad_glVertexArrayVertexAttribOffsetEXT = NULL;
PFNGLVERTEXARRAYVERTEXATTRIBIOFFSETEXTPROC glad_glVertexArrayVertexAttribIOffsetEXT = NULL;
PFNGLENABLEVERTEXARRAYEXTPROC glad_glEnableVertexArrayEXT = NULL;
PFNGLDISABLEVERTEXARRAYEXTPROC glad_glDisableVertexArrayEXT = NULL;
PFNGLENABLEVERTEXARRAYATTRIBEXTPROC glad_glEnableVertexArrayAttribEXT = NULL;
PFNGLDISABLEVERTEXARRAYATTRIBEXTPROC glad_glDisableVertexArrayAttribEXT = NULL;
PFNGLGETVERTEXARRAYINTEGERVEXTPROC glad_glGetVertexArrayIntegervEXT = NULL;
PFNGLGETVERTEXARRAYPOINTERVEXTPROC glad_glGetVertexArrayPointervEXT = NULL;
PFNGLGETVERTEXARRAYINTEGERI_VEXTPROC glad_glGetVertexArrayIntegeri_vEXT = NULL;
PFNGLGETVERTEXARRAYPOINTERI_VEXTPROC glad_glGetVertexArrayPointeri_vEXT = NULL;
PFNGLMAPNAMEDBUFFERRANGEEXTPROC glad_glMapNamedBufferRangeEXT = NULL;
PFNGLFLUSHMAPPEDNAMEDBUFFERRANGEEXTPROC glad_glFlushMappedNamedBufferRangeEXT = NULL;
PFNGLNAMEDBUFFERSTORAGEEXTPROC glad_glNamedBufferStorageEXT = NULL;
PFNGLCLEARNAMEDBUFFERDATAEXTPROC glad_glClearNamedBufferDataEXT = NULL;
PFNGLCLEARNAMEDBUFFERSUBDATAEXTPROC glad_glClearNamedBufferSubDataEXT = NULL;
PFNGLNAMEDFRAMEBUFFERPARAMETERIEXTPROC glad_glNamedFramebufferParameteriEXT = NULL;
PFNGLGETNAMEDFRAMEBUFFERPARAMETERIVEXTPROC glad_glGetNamedFramebufferParameterivEXT = NULL;
PFNGLPROGRAMUNIFORM1DEXTPROC glad_glProgramUniform1dEXT = NULL;
PFNGLPROGRAMUNIFORM2DEXTPROC glad_glProgramUniform2dEXT = NULL;
PFNGLPROGRAMUNIFORM3DEXTPROC glad_glProgramUniform3dEXT = NULL;
PFNGLPROGRAMUNIFORM4DEXTPROC glad_glProgramUniform4dEXT = NULL;
PFNGLPROGRAMUNIFORM1DVEXTPROC glad_glProgramUniform1dvEXT = NULL;
PFNGLPROGRAMUNIFORM2DVEXTPROC glad_glProgramUniform2dvEXT = NULL;
PFNGLPROGRAMUNIFORM3DVEXTPROC glad_glProgramUniform3dvEXT = NULL;
PFNGLPROGRAMUNIFORM4DVEXTPROC glad_glProgramUniform4dvEXT = NULL;
PFNGLPROGRAMUNIFORMMATRIX2DVEXTPROC glad_glProgramUniformMatrix2dvEXT = NULL;
PFNGLPROGRAMUNIFORMMATRIX3DVEXTPROC glad_glProgramUniformMatrix3dvEXT = NULL;
PFNGLPROGRAMUNIFORMMATRIX4DVEXTPROC glad_glProgramUniformMatrix4dvEXT = NULL;
PFNGLPROGRAMUNIFORMMATRIX2X3DVEXTPROC glad_glProgramUniformMatrix2x3dvEXT = NULL;
PFNGLPROGRAMUNIFORMMATRIX2X4DVEXTPROC glad_glProgramUniformMatrix2x4dvEXT = NULL;
PFNGLPROGRAMUNIFORMMATRIX3X2DVEXTPROC glad_glProgramUniformMatrix3x2dvEXT = NULL;
PFNGLPROGRAMUNIFORMMATRIX3X4DVEXTPROC glad_glProgramUniformMatrix3x4dvEXT = NULL;
PFNGLPROGRAMUNIFORMMATRIX4X2DVEXTPROC glad_glProgramUniformMatrix4x2dvEXT = NULL;
PFNGLPROGRAMUNIFORMMATRIX4X3DVEXTPROC glad_glProgramUniformMatrix4x3dvEXT = NULL;
PFNGLTEXTUREBUFFERRANGEEXTPROC glad_glTextureBufferRangeEXT = NULL;
PFNGLTEXTURESTORAGE1DEXTPROC glad_glTextureStorage1DEXT = NULL;
PFNGLTEXTURESTORAGE2DEXTPROC glad_glTextureStorage2DEXT = NULL;
PFNGLTEXTURESTORAGE3DEXTPROC glad_glTextureStorage3DEXT = NULL;
PFNGLTEXTURESTORAGE2DMULTISAMPLEEXTPROC glad_glTextureStorage2DMultisampleEXT = NULL;
PFNGLTEXTURESTORAGE3DMULTISAMPLEEXTPROC glad_glTextureStorage3DMultisampleEXT = NULL;
PFNGLVERTEXARRAYBINDVERTEXBUFFEREXTPROC glad_glVertexArrayBindVertexBufferEXT = NULL;
PFNGLVERTEXARRAYVERTEXATTRIBFORMATEXTPROC glad_glVertexArrayVertexAttribFormatEXT = NULL;
PFNGLVERTEXARRAYVERTEXATTRIBIFORMATEXTPROC glad_glVertexArrayVertexAttribIFormatEXT = NULL;
PFNGLVERTEXARRAYVERTEXATTRIBLFORMATEXTPROC glad_glVertexArrayVertexAttribLFormatEXT = NULL;
PFNGLVERTEXARRAYVERTEXATTRIBBINDINGEXTPROC glad_glVertexArrayVertexAttribBindingEXT = NULL;
PFNGLVERTEXARRAYVERTEXBINDINGDIVISOREXTPROC glad_glVertexArrayVertexBindingDivisorEXT = NULL;
PFNGLVERTEXARRAYVERTEXATTRIBLOFFSETEXTPROC glad_glVertexArrayVertexAttribLOffsetEXT = NULL;
PFNGLTEXTUREPAGECOMMITMENTEXTPROC glad_glTexturePageCommitmentEXT = NULL;
PFNGLVERTEXARRAYVERTEXATTRIBDIVISOREXTPROC glad_glVertexArrayVertexAttribDivisorEXT = NULL;
PFNGLCOLORMASKINDEXEDEXTPROC glad_glColorMaskIndexedEXT = NULL;
PFNGLDRAWARRAYSINSTANCEDEXTPROC glad_glDrawArraysInstancedEXT = NULL;
PFNGLDRAWELEMENTSINSTANCEDEXTPROC glad_glDrawElementsInstancedEXT = NULL;
PFNGLDRAWRANGEELEMENTSEXTPROC glad_glDrawRangeElementsEXT = NULL;
PFNGLBUFFERSTORAGEEXTERNALEXTPROC glad_glBufferStorageExternalEXT = NULL;
PFNGLNAMEDBUFFERSTORAGEEXTERNALEXTPROC glad_glNamedBufferStorageExternalEXT = NULL;
PFNGLFOGCOORDFEXTPROC glad_glFogCoordfEXT = NULL;
PFNGLFOGCOORDFVEXTPROC glad_glFogCoordfvEXT = NULL;
PFNGLFOGCOORDDEXTPROC glad_glFogCoorddEXT = NULL;
PFNGLFOGCOORDDVEXTPROC glad_glFogCoorddvEXT = NULL;
PFNGLFOGCOORDPOINTEREXTPROC glad_glFogCoordPointerEXT = NULL;
PFNGLBLITFRAMEBUFFEREXTPROC glad_glBlitFramebufferEXT = NULL;
PFNGLBLITFRAMEBUFFERLAYERSEXTPROC glad_glBlitFramebufferLayersEXT = NULL;
PFNGLBLITFRAMEBUFFERLAYEREXTPROC glad_glBlitFramebufferLayerEXT = NULL;
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC glad_glRenderbufferStorageMultisampleEXT = NULL;
PFNGLISRENDERBUFFEREXTPROC glad_glIsRenderbufferEXT = NULL;
PFNGLBINDRENDERBUFFEREXTPROC glad_glBindRenderbufferEXT = NULL;
PFNGLDELETERENDERBUFFERSEXTPROC glad_glDeleteRenderbuffersEXT = NULL;
PFNGLGENRENDERBUFFERSEXTPROC glad_glGenRenderbuffersEXT = NULL;
PFNGLRENDERBUFFERSTORAGEEXTPROC glad_glRenderbufferStorageEXT = NULL;
PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC glad_glGetRenderbufferParameterivEXT = NULL;
PFNGLISFRAMEBUFFEREXTPROC glad_glIsFramebufferEXT = NULL;
PFNGLBINDFRAMEBUFFEREXTPROC glad_glBindFramebufferEXT = NULL;
PFNGLDELETEFRAMEBUFFERSEXTPROC glad_glDeleteFramebuffersEXT = NULL;
PFNGLGENFRAMEBUFFERSEXTPROC glad_glGenFramebuffersEXT = NULL;
PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glad_glCheckFramebufferStatusEXT = NULL;
PFNGLFRAMEBUFFERTEXTURE1DEXTPROC glad_glFramebufferTexture1DEXT = NULL;
PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glad_glFramebufferTexture2DEXT = NULL;
PFNGLFRAMEBUFFERTEXTURE3DEXTPROC glad_glFramebufferTexture3DEXT = NULL;
PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC glad_glFramebufferRenderbufferEXT = NULL;
PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC glad_glGetFramebufferAttachmentParameterivEXT = NULL;
PFNGLGENERATEMIPMAPEXTPROC glad_glGenerateMipmapEXT = NULL;
PFNGLPROGRAMPARAMETERIEXTPROC glad_glProgramParameteriEXT = NULL;
PFNGLPROGRAMENVPARAMETERS4FVEXTPROC glad_glProgramEnvParameters4fvEXT = NULL;
PFNGLPROGRAMLOCALPARAMETERS4FVEXTPROC glad_glProgramLocalParameters4fvEXT = NULL;
PFNGLGETUNIFORMUIVEXTPROC glad_glGetUniformuivEXT = NULL;
PFNGLBINDFRAGDATALOCATIONEXTPROC glad_glBindFragDataLocationEXT = NULL;
PFNGLGETFRAGDATALOCATIONEXTPROC glad_glGetFragDataLocationEXT = NULL;
PFNGLUNIFORM1UIEXTPROC glad_glUniform1uiEXT = NULL;
PFNGLUNIFORM2UIEXTPROC glad_glUniform2uiEXT = NULL;
PFNGLUNIFORM3UIEXTPROC glad_glUniform3uiEXT = NULL;
PFNGLUNIFORM4UIEXTPROC glad_glUniform4uiEXT = NULL;
PFNGLUNIFORM1UIVEXTPROC glad_glUniform1uivEXT = NULL;
PFNGLUNIFORM2UIVEXTPROC glad_glUniform2uivEXT = NULL;
PFNGLUNIFORM3UIVEXTPROC glad_glUniform3uivEXT = NULL;
PFNGLUNIFORM4UIVEXTPROC glad_glUniform4uivEXT = NULL;
PFNGLVERTEXATTRIBI1IEXTPROC glad_glVertexAttribI1iEXT = NULL;
PFNGLVERTEXATTRIBI2IEXTPROC glad_glVertexAttribI2iEXT = NULL;
PFNGLVERTEXATTRIBI3IEXTPROC glad_glVertexAttribI3iEXT = NULL;
PFNGLVERTEXATTRIBI4IEXTPROC glad_glVertexAttribI4iEXT = NULL;
PFNGLVERTEXATTRIBI1UIEXTPROC glad_glVertexAttribI1uiEXT = NULL;
PFNGLVERTEXATTRIBI2UIEXTPROC glad_glVertexAttribI2uiEXT = NULL;
PFNGLVERTEXATTRIBI3UIEXTPROC glad_glVertexAttribI3uiEXT = NULL;
PFNGLVERTEXATTRIBI4UIEXTPROC glad_glVertexAttribI4uiEXT = NULL;
PFNGLVERTEXATTRIBI1IVEXTPROC glad_glVertexAttribI1ivEXT = NULL;
PFNGLVERTEXATTRIBI2IVEXTPROC glad_glVertexAttribI2ivEXT = NULL;
PFNGLVERTEXATTRIBI3IVEXTPROC glad_glVertexAttribI3ivEXT = NULL;
PFNGLVERTEXATTRIBI4IVEXTPROC glad_glVertexAttribI4ivEXT = NULL;
PFNGLVERTEXATTRIBI1UIVEXTPROC glad_glVertexAttribI1uivEXT = NULL;
PFNGLVERTEXATTRIBI2UIVEXTPROC glad_glVertexAttribI2uivEXT = NULL;
PFNGLVERTEXATTRIBI3UIVEXTPROC glad_glVertexAttribI3uivEXT = NULL;
PFNGLVERTEXATTRIBI4UIVEXTPROC glad_glVertexAttribI4uivEXT = NULL;
PFNGLVERTEXATTRIBI4BVEXTPROC glad_glVertexAttribI4bvEXT = NULL;
PFNGLVERTEXATTRIBI4SVEXTPROC glad_glVertexAttribI4svEXT = NULL;
PFNGLVERTEXATTRIBI4UBVEXTPROC glad_glVertexAttribI4ubvEXT = NULL;
PFNGLVERTEXATTRIBI4USVEXTPROC glad_glVertexAttribI4usvEXT = NULL;
PFNGLVERTEXATTRIBIPOINTEREXTPROC glad_glVertexAttribIPointerEXT = NULL;
PFNGLGETVERTEXATTRIBIIVEXTPROC glad_glGetVertexAttribIivEXT = NULL;
PFNGLGETVERTEXATTRIBIUIVEXTPROC glad_glGetVertexAttribIuivEXT = NULL;
PFNGLGETHISTOGRAMEXTPROC glad_glGetHistogramEXT = NULL;
PFNGLGETHISTOGRAMPARAMETERFVEXTPROC glad_glGetHistogramParameterfvEXT = NULL;
PFNGLGETHISTOGRAMPARAMETERIVEXTPROC glad_glGetHistogramParameterivEXT = NULL;
PFNGLGETMINMAXEXTPROC glad_glGetMinmaxEXT = NULL;
PFNGLGETMINMAXPARAMETERFVEXTPROC glad_glGetMinmaxParameterfvEXT = NULL;
PFNGLGETMINMAXPARAMETERIVEXTPROC glad_glGetMinmaxParameterivEXT = NULL;
PFNGLHISTOGRAMEXTPROC glad_glHistogramEXT = NULL;
PFNGLMINMAXEXTPROC glad_glMinmaxEXT = NULL;
PFNGLRESETHISTOGRAMEXTPROC glad_glResetHistogramEXT = NULL;
PFNGLRESETMINMAXEXTPROC glad_glResetMinmaxEXT = NULL;
PFNGLINDEXFUNCEXTPROC glad_glIndexFuncEXT = NULL;
PFNGLINDEXMATERIALEXTPROC glad_glIndexMaterialEXT = NULL;
PFNGLAPPLYTEXTUREEXTPROC glad_glApplyTextureEXT = NULL;
PFNGLTEXTURELIGHTEXTPROC glad_glTextureLightEXT = NULL;
PFNGLTEXTUREMATERIALEXTPROC glad_glTextureMaterialEXT = NULL;
PFNGLGETUNSIGNEDBYTEVEXTPROC glad_glGetUnsignedBytevEXT = NULL;
PFNGLGETUNSIGNEDBYTEI_VEXTPROC glad_glGetUnsignedBytei_vEXT = NULL;
PFNGLDELETEMEMORYOBJECTSEXTPROC glad_glDeleteMemoryObjectsEXT = NULL;
PFNGLISMEMORYOBJECTEXTPROC glad_glIsMemoryObjectEXT = NULL;
PFNGLCREATEMEMORYOBJECTSEXTPROC glad_glCreateMemoryObjectsEXT = NULL;
PFNGLMEMORYOBJECTPARAMETERIVEXTPROC glad_glMemoryObjectParameterivEXT = NULL;
PFNGLGETMEMORYOBJECTPARAMETERIVEXTPROC glad_glGetMemoryObjectParameterivEXT = NULL;
PFNGLTEXSTORAGEMEM2DEXTPROC glad_glTexStorageMem2DEXT = NULL;
PFNGLTEXSTORAGEMEM2DMULTISAMPLEEXTPROC glad_glTexStorageMem2DMultisampleEXT = NULL;
PFNGLTEXSTORAGEMEM3DEXTPROC glad_glTexStorageMem3DEXT = NULL;
PFNGLTEXSTORAGEMEM3DMULTISAMPLEEXTPROC glad_glTexStorageMem3DMultisampleEXT = NULL;
PFNGLBUFFERSTORAGEMEMEXTPROC glad_glBufferStorageMemEXT = NULL;
PFNGLTEXTURESTORAGEMEM2DEXTPROC glad_glTextureStorageMem2DEXT = NULL;
PFNGLTEXTURESTORAGEMEM2DMULTISAMPLEEXTPROC glad_glTextureStorageMem2DMultisampleEXT = NULL;
PFNGLTEXTURESTORAGEMEM3DEXTPROC glad_glTextureStorageMem3DEXT = NULL;
PFNGLTEXTURESTORAGEMEM3DMULTISAMPLEEXTPROC glad_glTextureStorageMem3DMultisampleEXT = NULL;
PFNGLNAMEDBUFFERSTORAGEMEMEXTPROC glad_glNamedBufferStorageMemEXT = NULL;
PFNGLTEXSTORAGEMEM1DEXTPROC glad_glTexStorageMem1DEXT = NULL;
PFNGLTEXTURESTORAGEMEM1DEXTPROC glad_glTextureStorageMem1DEXT = NULL;
PFNGLIMPORTMEMORYFDEXTPROC glad_glImportMemoryFdEXT = NULL;
PFNGLIMPORTMEMORYWIN32HANDLEEXTPROC glad_glImportMemoryWin32HandleEXT = NULL;
PFNGLIMPORTMEMORYWIN32NAMEEXTPROC glad_glImportMemoryWin32NameEXT = NULL;
PFNGLMULTIDRAWARRAYSEXTPROC glad_glMultiDrawArraysEXT = NULL;
PFNGLMULTIDRAWELEMENTSEXTPROC glad_glMultiDrawElementsEXT = NULL;
PFNGLSAMPLEMASKEXTPROC glad_glSampleMaskEXT = NULL;
PFNGLSAMPLEPATTERNEXTPROC glad_glSamplePatternEXT = NULL;
PFNGLCOLORTABLEEXTPROC glad_glColorTableEXT = NULL;
PFNGLGETCOLORTABLEEXTPROC glad_glGetColorTableEXT = NULL;
PFNGLGETCOLORTABLEPARAMETERIVEXTPROC glad_glGetColorTableParameterivEXT = NULL;
PFNGLGETCOLORTABLEPARAMETERFVEXTPROC glad_glGetColorTableParameterfvEXT = NULL;
PFNGLPIXELTRANSFORMPARAMETERIEXTPROC glad_glPixelTransformParameteriEXT = NULL;
PFNGLPIXELTRANSFORMPARAMETERFEXTPROC glad_glPixelTransformParameterfEXT = NULL;
PFNGLPIXELTRANSFORMPARAMETERIVEXTPROC glad_glPixelTransformParameterivEXT = NULL;
PFNGLPIXELTRANSFORMPARAMETERFVEXTPROC glad_glPixelTransformParameterfvEXT = NULL;
PFNGLGETPIXELTRANSFORMPARAMETERIVEXTPROC glad_glGetPixelTransformParameterivEXT = NULL;
PFNGLGETPIXELTRANSFORMPARAMETERFVEXTPROC glad_glGetPixelTransformParameterfvEXT = NULL;
PFNGLPOINTPARAMETERFEXTPROC glad_glPointParameterfEXT = NULL;
PFNGLPOINTPARAMETERFVEXTPROC glad_glPointParameterfvEXT = NULL;
PFNGLPOLYGONOFFSETEXTPROC glad_glPolygonOffsetEXT = NULL;
PFNGLPOLYGONOFFSETCLAMPEXTPROC glad_glPolygonOffsetClampEXT = NULL;
PFNGLPROVOKINGVERTEXEXTPROC glad_glProvokingVertexEXT = NULL;
PFNGLRASTERSAMPLESEXTPROC glad_glRasterSamplesEXT = NULL;
PFNGLSECONDARYCOLOR3BEXTPROC glad_glSecondaryColor3bEXT = NULL;
PFNGLSECONDARYCOLOR3BVEXTPROC glad_glSecondaryColor3bvEXT = NULL;
PFNGLSECONDARYCOLOR3DEXTPROC glad_glSecondaryColor3dEXT = NULL;
PFNGLSECONDARYCOLOR3DVEXTPROC glad_glSecondaryColor3dvEXT = NULL;
PFNGLSECONDARYCOLOR3FEXTPROC glad_glSecondaryColor3fEXT = NULL;
PFNGLSECONDARYCOLOR3FVEXTPROC glad_glSecondaryColor3fvEXT = NULL;
PFNGLSECONDARYCOLOR3IEXTPROC glad_glSecondaryColor3iEXT = NULL;
PFNGLSECONDARYCOLOR3IVEXTPROC glad_glSecondaryColor3ivEXT = NULL;
PFNGLSECONDARYCOLOR3SEXTPROC glad_glSecondaryColor3sEXT = NULL;
PFNGLSECONDARYCOLOR3SVEXTPROC glad_glSecondaryColor3svEXT = NULL;
PFNGLSECONDARYCOLOR3UBEXTPROC glad_glSecondaryColor3ubEXT = NULL;
PFNGLSECONDARYCOLOR3UBVEXTPROC glad_glSecondaryColor3ubvEXT = NULL;
PFNGLSECONDARYCOLOR3UIEXTPROC glad_glSecondaryColor3uiEXT = NULL;
PFNGLSECONDARYCOLOR3UIVEXTPROC glad_glSecondaryColor3uivEXT = NULL;
PFNGLSECONDARYCOLOR3USEXTPROC glad_glSecondaryColor3usEXT = NULL;
PFNGLSECONDARYCOLOR3USVEXTPROC glad_glSecondaryColor3usvEXT = NULL;
PFNGLSECONDARYCOLORPOINTEREXTPROC glad_glSecondaryColorPointerEXT = NULL;
PFNGLGENSEMAPHORESEXTPROC glad_glGenSemaphoresEXT = NULL;
PFNGLDELETESEMAPHORESEXTPROC glad_glDeleteSemaphoresEXT = NULL;
PFNGLISSEMAPHOREEXTPROC glad_glIsSemaphoreEXT = NULL;
PFNGLSEMAPHOREPARAMETERUI64VEXTPROC glad_glSemaphoreParameterui64vEXT = NULL;
PFNGLGETSEMAPHOREPARAMETERUI64VEXTPROC glad_glGetSemaphoreParameterui64vEXT = NULL;
PFNGLWAITSEMAPHOREEXTPROC glad_glWaitSemaphoreEXT = NULL;
PFNGLSIGNALSEMAPHOREEXTPROC glad_glSignalSemaphoreEXT = NULL;
PFNGLIMPORTSEMAPHOREFDEXTPROC glad_glImportSemaphoreFdEXT = NULL;
PFNGLIMPORTSEMAPHOREWIN32HANDLEEXTPROC glad_glImportSemaphoreWin32HandleEXT = NULL;
PFNGLIMPORTSEMAPHOREWIN32NAMEEXTPROC glad_glImportSemaphoreWin32NameEXT = NULL;
PFNGLUSESHADERPROGRAMEXTPROC glad_glUseShaderProgramEXT = NULL;
PFNGLACTIVEPROGRAMEXTPROC glad_glActiveProgramEXT = NULL;
PFNGLCREATESHADERPROGRAMEXTPROC glad_glCreateShaderProgramEXT = NULL;
PFNGLACTIVESHADERPROGRAMEXTPROC glad_glActiveShaderProgramEXT = NULL;
PFNGLBINDPROGRAMPIPELINEEXTPROC glad_glBindProgramPipelineEXT = NULL;
PFNGLCREATESHADERPROGRAMVEXTPROC glad_glCreateShaderProgramvEXT = NULL;
PFNGLDELETEPROGRAMPIPELINESEXTPROC glad_glDeleteProgramPipelinesEXT = NULL;
PFNGLGENPROGRAMPIPELINESEXTPROC glad_glGenProgramPipelinesEXT = NULL;
PFNGLGETPROGRAMPIPELINEINFOLOGEXTPROC glad_glGetProgramPipelineInfoLogEXT = NULL;
PFNGLGETPROGRAMPIPELINEIVEXTPROC glad_glGetProgramPipelineivEXT = NULL;
PFNGLISPROGRAMPIPELINEEXTPROC glad_glIsProgramPipelineEXT = NULL;
PFNGLUSEPROGRAMSTAGESEXTPROC glad_glUseProgramStagesEXT = NULL;
PFNGLVALIDATEPROGRAMPIPELINEEXTPROC glad_glValidateProgramPipelineEXT = NULL;
PFNGLFRAMEBUFFERFETCHBARRIEREXTPROC glad_glFramebufferFetchBarrierEXT = NULL;
PFNGLBINDIMAGETEXTUREEXTPROC glad_glBindImageTextureEXT = NULL;
PFNGLMEMORYBARRIEREXTPROC glad_glMemoryBarrierEXT = NULL;
PFNGLSTENCILCLEARTAGEXTPROC glad_glStencilClearTagEXT = NULL;
PFNGLACTIVESTENCILFACEEXTPROC glad_glActiveStencilFaceEXT = NULL;
PFNGLTEXSUBIMAGE1DEXTPROC glad_glTexSubImage1DEXT = NULL;
PFNGLTEXSUBIMAGE2DEXTPROC glad_glTexSubImage2DEXT = NULL;
PFNGLTEXIMAGE3DEXTPROC glad_glTexImage3DEXT = NULL;
PFNGLTEXSUBIMAGE3DEXTPROC glad_glTexSubImage3DEXT = NULL;
PFNGLFRAMEBUFFERTEXTURELAYEREXTPROC glad_glFramebufferTextureLayerEXT = NULL;
PFNGLTEXBUFFEREXTPROC glad_glTexBufferEXT = NULL;
PFNGLTEXPARAMETERIIVEXTPROC glad_glTexParameterIivEXT = NULL;
PFNGLTEXPARAMETERIUIVEXTPROC glad_glTexParameterIuivEXT = NULL;
PFNGLGETTEXPARAMETERIIVEXTPROC glad_glGetTexParameterIivEXT = NULL;
PFNGLGETTEXPARAMETERIUIVEXTPROC glad_glGetTexParameterIuivEXT = NULL;
PFNGLCLEARCOLORIIEXTPROC glad_glClearColorIiEXT = NULL;
PFNGLCLEARCOLORIUIEXTPROC glad_glClearColorIuiEXT = NULL;
PFNGLARETEXTURESRESIDENTEXTPROC glad_glAreTexturesResidentEXT = NULL;
PFNGLBINDTEXTUREEXTPROC glad_glBindTextureEXT = NULL;
PFNGLDELETETEXTURESEXTPROC glad_glDeleteTexturesEXT = NULL;
PFNGLGENTEXTURESEXTPROC glad_glGenTexturesEXT = NULL;
PFNGLISTEXTUREEXTPROC glad_glIsTextureEXT = NULL;
PFNGLPRIORITIZETEXTURESEXTPROC glad_glPrioritizeTexturesEXT = NULL;
PFNGLTEXTURENORMALEXTPROC glad_glTextureNormalEXT = NULL;
PFNGLTEXSTORAGE1DEXTPROC glad_glTexStorage1DEXT = NULL;
PFNGLTEXSTORAGE2DEXTPROC glad_glTexStorage2DEXT = NULL;
PFNGLTEXSTORAGE3DEXTPROC glad_glTexStorage3DEXT = NULL;
PFNGLGETQUERYOBJECTI64VEXTPROC glad_glGetQueryObjecti64vEXT = NULL;
PFNGLGETQUERYOBJECTUI64VEXTPROC glad_glGetQueryObjectui64vEXT = NULL;
PFNGLBEGINTRANSFORMFEEDBACKEXTPROC glad_glBeginTransformFeedbackEXT = NULL;
PFNGLENDTRANSFORMFEEDBACKEXTPROC glad_glEndTransformFeedbackEXT = NULL;
PFNGLBINDBUFFERRANGEEXTPROC glad_glBindBufferRangeEXT = NULL;
PFNGLBINDBUFFEROFFSETEXTPROC glad_glBindBufferOffsetEXT = NULL;
PFNGLBINDBUFFERBASEEXTPROC glad_glBindBufferBaseEXT = NULL;
PFNGLTRANSFORMFEEDBACKVARYINGSEXTPROC glad_glTransformFeedbackVaryingsEXT = NULL;
PFNGLGETTRANSFORMFEEDBACKVARYINGEXTPROC glad_glGetTransformFeedbackVaryingEXT = NULL;
PFNGLARRAYELEMENTEXTPROC glad_glArrayElementEXT = NULL;
PFNGLCOLORPOINTEREXTPROC glad_glColorPointerEXT = NULL;
PFNGLDRAWARRAYSEXTPROC glad_glDrawArraysEXT = NULL;
PFNGLEDGEFLAGPOINTEREXTPROC glad_glEdgeFlagPointerEXT = NULL;
PFNGLGETPOINTERVEXTPROC glad_glGetPointervEXT = NULL;
PFNGLINDEXPOINTEREXTPROC glad_glIndexPointerEXT = NULL;
PFNGLNORMALPOINTEREXTPROC glad_glNormalPointerEXT = NULL;
PFNGLTEXCOORDPOINTEREXTPROC glad_glTexCoordPointerEXT = NULL;
PFNGLVERTEXPOINTEREXTPROC glad_glVertexPointerEXT = NULL;
PFNGLVERTEXATTRIBL1DEXTPROC glad_glVertexAttribL1dEXT = NULL;
PFNGLVERTEXATTRIBL2DEXTPROC glad_glVertexAttribL2dEXT = NULL;
PFNGLVERTEXATTRIBL3DEXTPROC glad_glVertexAttribL3dEXT = NULL;
PFNGLVERTEXATTRIBL4DEXTPROC glad_glVertexAttribL4dEXT = NULL;
PFNGLVERTEXATTRIBL1DVEXTPROC glad_glVertexAttribL1dvEXT = NULL;
PFNGLVERTEXATTRIBL2DVEXTPROC glad_glVertexAttribL2dvEXT = NULL;
PFNGLVERTEXATTRIBL3DVEXTPROC glad_glVertexAttribL3dvEXT = NULL;
PFNGLVERTEXATTRIBL4DVEXTPROC glad_glVertexAttribL4dvEXT = NULL;
PFNGLVERTEXATTRIBLPOINTEREXTPROC glad_glVertexAttribLPointerEXT = NULL;
PFNGLGETVERTEXATTRIBLDVEXTPROC glad_glGetVertexAttribLdvEXT = NULL;
PFNGLBEGINVERTEXSHADEREXTPROC glad_glBeginVertexShaderEXT = NULL;
PFNGLENDVERTEXSHADEREXTPROC glad_glEndVertexShaderEXT = NULL;
PFNGLBINDVERTEXSHADEREXTPROC glad_glBindVertexShaderEXT = NULL;
PFNGLGENVERTEXSHADERSEXTPROC glad_glGenVertexShadersEXT = NULL;
PFNGLDELETEVERTEXSHADEREXTPROC glad_glDeleteVertexShaderEXT = NULL;
PFNGLSHADEROP1EXTPROC glad_glShaderOp1EXT = NULL;
PFNGLSHADEROP2EXTPROC glad_glShaderOp2EXT = NULL;
PFNGLSHADEROP3EXTPROC glad_glShaderOp3EXT = NULL;
PFNGLSWIZZLEEXTPROC glad_glSwizzleEXT = NULL;
PFNGLWRITEMASKEXTPROC glad_glWriteMaskEXT = NULL;
PFNGLINSERTCOMPONENTEXTPROC glad_glInsertComponentEXT = NULL;
PFNGLEXTRACTCOMPONENTEXTPROC glad_glExtractComponentEXT = NULL;
PFNGLGENSYMBOLSEXTPROC glad_glGenSymbolsEXT = NULL;
PFNGLSETINVARIANTEXTPROC glad_glSetInvariantEXT = NULL;
PFNGLSETLOCALCONSTANTEXTPROC glad_glSetLocalConstantEXT = NULL;
PFNGLVARIANTBVEXTPROC glad_glVariantbvEXT = NULL;
PFNGLVARIANTSVEXTPROC glad_glVariantsvEXT = NULL;
PFNGLVARIANTIVEXTPROC glad_glVariantivEXT = NULL;
PFNGLVARIANTFVEXTPROC glad_glVariantfvEXT = NULL;
PFNGLVARIANTDVEXTPROC glad_glVariantdvEXT = NULL;
PFNGLVARIANTUBVEXTPROC glad_glVariantubvEXT = NULL;
PFNGLVARIANTUSVEXTPROC glad_glVariantusvEXT = NULL;
PFNGLVARIANTUIVEXTPROC glad_glVariantuivEXT = NULL;
PFNGLVARIANTPOINTEREXTPROC glad_glVariantPointerEXT = NULL;
PFNGLENABLEVARIANTCLIENTSTATEEXTPROC glad_glEnableVariantClientStateEXT = NULL;
PFNGLDISABLEVARIANTCLIENTSTATEEXTPROC glad_glDisableVariantClientStateEXT = NULL;
PFNGLBINDLIGHTPARAMETEREXTPROC glad_glBindLightParameterEXT = NULL;
PFNGLBINDMATERIALPARAMETEREXTPROC glad_glBindMaterialParameterEXT = NULL;
PFNGLBINDTEXGENPARAMETEREXTPROC glad_glBindTexGenParameterEXT = NULL;
PFNGLBINDTEXTUREUNITPARAMETEREXTPROC glad_glBindTextureUnitParameterEXT = NULL;
PFNGLBINDPARAMETEREXTPROC glad_glBindParameterEXT = NULL;
PFNGLISVARIANTENABLEDEXTPROC glad_glIsVariantEnabledEXT = NULL;
PFNGLGETVARIANTBOOLEANVEXTPROC glad_glGetVariantBooleanvEXT = NULL;
PFNGLGETVARIANTINTEGERVEXTPROC glad_glGetVariantIntegervEXT = NULL;
PFNGLGETVARIANTFLOATVEXTPROC glad_glGetVariantFloatvEXT = NULL;
PFNGLGETVARIANTPOINTERVEXTPROC glad_glGetVariantPointervEXT = NULL;
PFNGLGETINVARIANTBOOLEANVEXTPROC glad_glGetInvariantBooleanvEXT = NULL;
PFNGLGETINVARIANTINTEGERVEXTPROC glad_glGetInvariantIntegervEXT = NULL;
PFNGLGETINVARIANTFLOATVEXTPROC glad_glGetInvariantFloatvEXT = NULL;
PFNGLGETLOCALCONSTANTBOOLEANVEXTPROC glad_glGetLocalConstantBooleanvEXT = NULL;
PFNGLGETLOCALCONSTANTINTEGERVEXTPROC glad_glGetLocalConstantIntegervEXT = NULL;
PFNGLGETLOCALCONSTANTFLOATVEXTPROC glad_glGetLocalConstantFloatvEXT = NULL;
PFNGLVERTEXWEIGHTFEXTPROC glad_glVertexWeightfEXT = NULL;
PFNGLVERTEXWEIGHTFVEXTPROC glad_glVertexWeightfvEXT = NULL;
PFNGLVERTEXWEIGHTPOINTEREXTPROC glad_glVertexWeightPointerEXT = NULL;
PFNGLACQUIREKEYEDMUTEXWIN32EXTPROC glad_glAcquireKeyedMutexWin32EXT = NULL;
PFNGLRELEASEKEYEDMUTEXWIN32EXTPROC glad_glReleaseKeyedMutexWin32EXT = NULL;
PFNGLWINDOWRECTANGLESEXTPROC glad_glWindowRectanglesEXT = NULL;
PFNGLIMPORTSYNCEXTPROC glad_glImportSyncEXT = NULL;
PFNGLFRAMETERMINATORGREMEDYPROC glad_glFrameTerminatorGREMEDY = NULL;
PFNGLSTRINGMARKERGREMEDYPROC glad_glStringMarkerGREMEDY = NULL;
PFNGLIMAGETRANSFORMPARAMETERIHPPROC glad_glImageTransformParameteriHP = NULL;
PFNGLIMAGETRANSFORMPARAMETERFHPPROC glad_glImageTransformParameterfHP = NULL;
PFNGLIMAGETRANSFORMPARAMETERIVHPPROC glad_glImageTransformParameterivHP = NULL;
PFNGLIMAGETRANSFORMPARAMETERFVHPPROC glad_glImageTransformParameterfvHP = NULL;
PFNGLGETIMAGETRANSFORMPARAMETERIVHPPROC glad_glGetImageTransformParameterivHP = NULL;
PFNGLGETIMAGETRANSFORMPARAMETERFVHPPROC glad_glGetImageTransformParameterfvHP = NULL;
PFNGLMULTIMODEDRAWARRAYSIBMPROC glad_glMultiModeDrawArraysIBM = NULL;
PFNGLMULTIMODEDRAWELEMENTSIBMPROC glad_glMultiModeDrawElementsIBM = NULL;
PFNGLFLUSHSTATICDATAIBMPROC glad_glFlushStaticDataIBM = NULL;
PFNGLCOLORPOINTERLISTIBMPROC glad_glColorPointerListIBM = NULL;
PFNGLSECONDARYCOLORPOINTERLISTIBMPROC glad_glSecondaryColorPointerListIBM = NULL;
PFNGLEDGEFLAGPOINTERLISTIBMPROC glad_glEdgeFlagPointerListIBM = NULL;
PFNGLFOGCOORDPOINTERLISTIBMPROC glad_glFogCoordPointerListIBM = NULL;
PFNGLINDEXPOINTERLISTIBMPROC glad_glIndexPointerListIBM = NULL;
PFNGLNORMALPOINTERLISTIBMPROC glad_glNormalPointerListIBM = NULL;
PFNGLTEXCOORDPOINTERLISTIBMPROC glad_glTexCoordPointerListIBM = NULL;
PFNGLVERTEXPOINTERLISTIBMPROC glad_glVertexPointerListIBM = NULL;
PFNGLBLENDFUNCSEPARATEINGRPROC glad_glBlendFuncSeparateINGR = NULL;
PFNGLAPPLYFRAMEBUFFERATTACHMENTCMAAINTELPROC glad_glApplyFramebufferAttachmentCMAAINTEL = NULL;
PFNGLSYNCTEXTUREINTELPROC glad_glSyncTextureINTEL = NULL;
PFNGLUNMAPTEXTURE2DINTELPROC glad_glUnmapTexture2DINTEL = NULL;
PFNGLMAPTEXTURE2DINTELPROC glad_glMapTexture2DINTEL = NULL;
PFNGLVERTEXPOINTERVINTELPROC glad_glVertexPointervINTEL = NULL;
PFNGLNORMALPOINTERVINTELPROC glad_glNormalPointervINTEL = NULL;
PFNGLCOLORPOINTERVINTELPROC glad_glColorPointervINTEL = NULL;
PFNGLTEXCOORDPOINTERVINTELPROC glad_glTexCoordPointervINTEL = NULL;
PFNGLBEGINPERFQUERYINTELPROC glad_glBeginPerfQueryINTEL = NULL;
PFNGLCREATEPERFQUERYINTELPROC glad_glCreatePerfQueryINTEL = NULL;
PFNGLDELETEPERFQUERYINTELPROC glad_glDeletePerfQueryINTEL = NULL;
PFNGLENDPERFQUERYINTELPROC glad_glEndPerfQueryINTEL = NULL;
PFNGLGETFIRSTPERFQUERYIDINTELPROC glad_glGetFirstPerfQueryIdINTEL = NULL;
PFNGLGETNEXTPERFQUERYIDINTELPROC glad_glGetNextPerfQueryIdINTEL = NULL;
PFNGLGETPERFCOUNTERINFOINTELPROC glad_glGetPerfCounterInfoINTEL = NULL;
PFNGLGETPERFQUERYDATAINTELPROC glad_glGetPerfQueryDataINTEL = NULL;
PFNGLGETPERFQUERYIDBYNAMEINTELPROC glad_glGetPerfQueryIdByNameINTEL = NULL;
PFNGLGETPERFQUERYINFOINTELPROC glad_glGetPerfQueryInfoINTEL = NULL;
PFNGLBLENDBARRIERKHRPROC glad_glBlendBarrierKHR = NULL;
PFNGLDEBUGMESSAGECONTROLPROC glad_glDebugMessageControl = NULL;
PFNGLDEBUGMESSAGEINSERTPROC glad_glDebugMessageInsert = NULL;
PFNGLDEBUGMESSAGECALLBACKPROC glad_glDebugMessageCallback = NULL;
PFNGLGETDEBUGMESSAGELOGPROC glad_glGetDebugMessageLog = NULL;
PFNGLPUSHDEBUGGROUPPROC glad_glPushDebugGroup = NULL;
PFNGLPOPDEBUGGROUPPROC glad_glPopDebugGroup = NULL;
PFNGLOBJECTLABELPROC glad_glObjectLabel = NULL;
PFNGLGETOBJECTLABELPROC glad_glGetObjectLabel = NULL;
PFNGLOBJECTPTRLABELPROC glad_glObjectPtrLabel = NULL;
PFNGLGETOBJECTPTRLABELPROC glad_glGetObjectPtrLabel = NULL;
PFNGLGETPOINTERVPROC glad_glGetPointerv = NULL;
PFNGLDEBUGMESSAGECONTROLKHRPROC glad_glDebugMessageControlKHR = NULL;
PFNGLDEBUGMESSAGEINSERTKHRPROC glad_glDebugMessageInsertKHR = NULL;
PFNGLDEBUGMESSAGECALLBACKKHRPROC glad_glDebugMessageCallbackKHR = NULL;
PFNGLGETDEBUGMESSAGELOGKHRPROC glad_glGetDebugMessageLogKHR = NULL;
PFNGLPUSHDEBUGGROUPKHRPROC glad_glPushDebugGroupKHR = NULL;
PFNGLPOPDEBUGGROUPKHRPROC glad_glPopDebugGroupKHR = NULL;
PFNGLOBJECTLABELKHRPROC glad_glObjectLabelKHR = NULL;
PFNGLGETOBJECTLABELKHRPROC glad_glGetObjectLabelKHR = NULL;
PFNGLOBJECTPTRLABELKHRPROC glad_glObjectPtrLabelKHR = NULL;
PFNGLGETOBJECTPTRLABELKHRPROC glad_glGetObjectPtrLabelKHR = NULL;
PFNGLGETPOINTERVKHRPROC glad_glGetPointervKHR = NULL;
PFNGLMAXSHADERCOMPILERTHREADSKHRPROC glad_glMaxShaderCompilerThreadsKHR = NULL;
PFNGLGETGRAPHICSRESETSTATUSPROC glad_glGetGraphicsResetStatus = NULL;
PFNGLREADNPIXELSPROC glad_glReadnPixels = NULL;
PFNGLGETNUNIFORMFVPROC glad_glGetnUniformfv = NULL;
PFNGLGETNUNIFORMIVPROC glad_glGetnUniformiv = NULL;
PFNGLGETNUNIFORMUIVPROC glad_glGetnUniformuiv = NULL;
PFNGLGETGRAPHICSRESETSTATUSKHRPROC glad_glGetGraphicsResetStatusKHR = NULL;
PFNGLREADNPIXELSKHRPROC glad_glReadnPixelsKHR = NULL;
PFNGLGETNUNIFORMFVKHRPROC glad_glGetnUniformfvKHR = NULL;
PFNGLGETNUNIFORMIVKHRPROC glad_glGetnUniformivKHR = NULL;
PFNGLGETNUNIFORMUIVKHRPROC glad_glGetnUniformuivKHR = NULL;
PFNGLFRAMEBUFFERPARAMETERIMESAPROC glad_glFramebufferParameteriMESA = NULL;
PFNGLGETFRAMEBUFFERPARAMETERIVMESAPROC glad_glGetFramebufferParameterivMESA = NULL;
PFNGLRESIZEBUFFERSMESAPROC glad_glResizeBuffersMESA = NULL;
PFNGLWINDOWPOS2DMESAPROC glad_glWindowPos2dMESA = NULL;
PFNGLWINDOWPOS2DVMESAPROC glad_glWindowPos2dvMESA = NULL;
PFNGLWINDOWPOS2FMESAPROC glad_glWindowPos2fMESA = NULL;
PFNGLWINDOWPOS2FVMESAPROC glad_glWindowPos2fvMESA = NULL;
PFNGLWINDOWPOS2IMESAPROC glad_glWindowPos2iMESA = NULL;
PFNGLWINDOWPOS2IVMESAPROC glad_glWindowPos2ivMESA = NULL;
PFNGLWINDOWPOS2SMESAPROC glad_glWindowPos2sMESA = NULL;
PFNGLWINDOWPOS2SVMESAPROC glad_glWindowPos2svMESA = NULL;
PFNGLWINDOWPOS3DMESAPROC glad_glWindowPos3dMESA = NULL;
PFNGLWINDOWPOS3DVMESAPROC glad_glWindowPos3dvMESA = NULL;
PFNGLWINDOWPOS3FMESAPROC glad_glWindowPos3fMESA = NULL;
PFNGLWINDOWPOS3FVMESAPROC glad_glWindowPos3fvMESA = NULL;
PFNGLWINDOWPOS3IMESAPROC glad_glWindowPos3iMESA = NULL;
PFNGLWINDOWPOS3IVMESAPROC glad_glWindowPos3ivMESA = NULL;
PFNGLWINDOWPOS3SMESAPROC glad_glWindowPos3sMESA = NULL;
PFNGLWINDOWPOS3SVMESAPROC glad_glWindowPos3svMESA = NULL;
PFNGLWINDOWPOS4DMESAPROC glad_glWindowPos4dMESA = NULL;
PFNGLWINDOWPOS4DVMESAPROC glad_glWindowPos4dvMESA = NULL;
PFNGLWINDOWPOS4FMESAPROC glad_glWindowPos4fMESA = NULL;
PFNGLWINDOWPOS4FVMESAPROC glad_glWindowPos4fvMESA = NULL;
PFNGLWINDOWPOS4IMESAPROC glad_glWindowPos4iMESA = NULL;
PFNGLWINDOWPOS4IVMESAPROC glad_glWindowPos4ivMESA = NULL;
PFNGLWINDOWPOS4SMESAPROC glad_glWindowPos4sMESA = NULL;
PFNGLWINDOWPOS4SVMESAPROC glad_glWindowPos4svMESA = NULL;
PFNGLBEGINCONDITIONALRENDERNVXPROC glad_glBeginConditionalRenderNVX = NULL;
PFNGLENDCONDITIONALRENDERNVXPROC glad_glEndConditionalRenderNVX = NULL;
PFNGLUPLOADGPUMASKNVXPROC glad_glUploadGpuMaskNVX = NULL;
PFNGLMULTICASTVIEWPORTARRAYVNVXPROC glad_glMulticastViewportArrayvNVX = NULL;
PFNGLMULTICASTVIEWPORTPOSITIONWSCALENVXPROC glad_glMulticastViewportPositionWScaleNVX = NULL;
PFNGLMULTICASTSCISSORARRAYVNVXPROC glad_glMulticastScissorArrayvNVX = NULL;
PFNGLASYNCCOPYBUFFERSUBDATANVXPROC glad_glAsyncCopyBufferSubDataNVX = NULL;
PFNGLASYNCCOPYIMAGESUBDATANVXPROC glad_glAsyncCopyImageSubDataNVX = NULL;
PFNGLLGPUNAMEDBUFFERSUBDATANVXPROC glad_glLGPUNamedBufferSubDataNVX = NULL;
PFNGLLGPUCOPYIMAGESUBDATANVXPROC glad_glLGPUCopyImageSubDataNVX = NULL;
PFNGLLGPUINTERLOCKNVXPROC glad_glLGPUInterlockNVX = NULL;
PFNGLCREATEPROGRESSFENCENVXPROC glad_glCreateProgressFenceNVX = NULL;
PFNGLSIGNALSEMAPHOREUI64NVXPROC glad_glSignalSemaphoreui64NVX = NULL;
PFNGLWAITSEMAPHOREUI64NVXPROC glad_glWaitSemaphoreui64NVX = NULL;
PFNGLCLIENTWAITSEMAPHOREUI64NVXPROC glad_glClientWaitSemaphoreui64NVX = NULL;
PFNGLALPHATOCOVERAGEDITHERCONTROLNVPROC glad_glAlphaToCoverageDitherControlNV = NULL;
PFNGLMULTIDRAWARRAYSINDIRECTBINDLESSNVPROC glad_glMultiDrawArraysIndirectBindlessNV = NULL;
PFNGLMULTIDRAWELEMENTSINDIRECTBINDLESSNVPROC glad_glMultiDrawElementsIndirectBindlessNV = NULL;
PFNGLMULTIDRAWARRAYSINDIRECTBINDLESSCOUNTNVPROC glad_glMultiDrawArraysIndirectBindlessCountNV = NULL;
PFNGLMULTIDRAWELEMENTSINDIRECTBINDLESSCOUNTNVPROC glad_glMultiDrawElementsIndirectBindlessCountNV = NULL;
PFNGLGETTEXTUREHANDLENVPROC glad_glGetTextureHandleNV = NULL;
PFNGLGETTEXTURESAMPLERHANDLENVPROC glad_glGetTextureSamplerHandleNV = NULL;
PFNGLMAKETEXTUREHANDLERESIDENTNVPROC glad_glMakeTextureHandleResidentNV = NULL;
PFNGLMAKETEXTUREHANDLENONRESIDENTNVPROC glad_glMakeTextureHandleNonResidentNV = NULL;
PFNGLGETIMAGEHANDLENVPROC glad_glGetImageHandleNV = NULL;
PFNGLMAKEIMAGEHANDLERESIDENTNVPROC glad_glMakeImageHandleResidentNV = NULL;
PFNGLMAKEIMAGEHANDLENONRESIDENTNVPROC glad_glMakeImageHandleNonResidentNV = NULL;
PFNGLUNIFORMHANDLEUI64NVPROC glad_glUniformHandleui64NV = NULL;
PFNGLUNIFORMHANDLEUI64VNVPROC glad_glUniformHandleui64vNV = NULL;
PFNGLPROGRAMUNIFORMHANDLEUI64NVPROC glad_glProgramUniformHandleui64NV = NULL;
PFNGLPROGRAMUNIFORMHANDLEUI64VNVPROC glad_glProgramUniformHandleui64vNV = NULL;
PFNGLISTEXTUREHANDLERESIDENTNVPROC glad_glIsTextureHandleResidentNV = NULL;
PFNGLISIMAGEHANDLERESIDENTNVPROC glad_glIsImageHandleResidentNV = NULL;
PFNGLBLENDPARAMETERINVPROC glad_glBlendParameteriNV = NULL;
PFNGLBLENDBARRIERNVPROC glad_glBlendBarrierNV = NULL;
PFNGLVIEWPORTPOSITIONWSCALENVPROC glad_glViewportPositionWScaleNV = NULL;
PFNGLCREATESTATESNVPROC glad_glCreateStatesNV = NULL;
PFNGLDELETESTATESNVPROC glad_glDeleteStatesNV = NULL;
PFNGLISSTATENVPROC glad_glIsStateNV = NULL;
PFNGLSTATECAPTURENVPROC glad_glStateCaptureNV = NULL;
PFNGLGETCOMMANDHEADERNVPROC glad_glGetCommandHeaderNV = NULL;
PFNGLGETSTAGEINDEXNVPROC glad_glGetStageIndexNV = NULL;
PFNGLDRAWCOMMANDSNVPROC glad_glDrawCommandsNV = NULL;
PFNGLDRAWCOMMANDSADDRESSNVPROC glad_glDrawCommandsAddressNV = NULL;
PFNGLDRAWCOMMANDSSTATESNVPROC glad_glDrawCommandsStatesNV = NULL;
PFNGLDRAWCOMMANDSSTATESADDRESSNVPROC glad_glDrawCommandsStatesAddressNV = NULL;
PFNGLCREATECOMMANDLISTSNVPROC glad_glCreateCommandListsNV = NULL;
PFNGLDELETECOMMANDLISTSNVPROC glad_glDeleteCommandListsNV = NULL;
PFNGLISCOMMANDLISTNVPROC glad_glIsCommandListNV = NULL;
PFNGLLISTDRAWCOMMANDSSTATESCLIENTNVPROC glad_glListDrawCommandsStatesClientNV = NULL;
PFNGLCOMMANDLISTSEGMENTSNVPROC glad_glCommandListSegmentsNV = NULL;
PFNGLCOMPILECOMMANDLISTNVPROC glad_glCompileCommandListNV = NULL;
PFNGLCALLCOMMANDLISTNVPROC glad_glCallCommandListNV = NULL;
PFNGLBEGINCONDITIONALRENDERNVPROC glad_glBeginConditionalRenderNV = NULL;
PFNGLENDCONDITIONALRENDERNVPROC glad_glEndConditionalRenderNV = NULL;
PFNGLSUBPIXELPRECISIONBIASNVPROC glad_glSubpixelPrecisionBiasNV = NULL;
PFNGLCONSERVATIVERASTERPARAMETERFNVPROC glad_glConservativeRasterParameterfNV = NULL;
PFNGLCONSERVATIVERASTERPARAMETERINVPROC glad_glConservativeRasterParameteriNV = NULL;
PFNGLCOPYIMAGESUBDATANVPROC glad_glCopyImageSubDataNV = NULL;
PFNGLDEPTHRANGEDNVPROC glad_glDepthRangedNV = NULL;
PFNGLCLEARDEPTHDNVPROC glad_glClearDepthdNV = NULL;
PFNGLDEPTHBOUNDSDNVPROC glad_glDepthBoundsdNV = NULL;
PFNGLDRAWTEXTURENVPROC glad_glDrawTextureNV = NULL;
PFNGLDRAWVKIMAGENVPROC glad_glDrawVkImageNV = NULL;
PFNGLGETVKPROCADDRNVPROC glad_glGetVkProcAddrNV = NULL;
PFNGLWAITVKSEMAPHORENVPROC glad_glWaitVkSemaphoreNV = NULL;
PFNGLSIGNALVKSEMAPHORENVPROC glad_glSignalVkSemaphoreNV = NULL;
PFNGLSIGNALVKFENCENVPROC glad_glSignalVkFenceNV = NULL;
PFNGLMAPCONTROLPOINTSNVPROC glad_glMapControlPointsNV = NULL;
PFNGLMAPPARAMETERIVNVPROC glad_glMapParameterivNV = NULL;
PFNGLMAPPARAMETERFVNVPROC glad_glMapParameterfvNV = NULL;
PFNGLGETMAPCONTROLPOINTSNVPROC glad_glGetMapControlPointsNV = NULL;
PFNGLGETMAPPARAMETERIVNVPROC glad_glGetMapParameterivNV = NULL;
PFNGLGETMAPPARAMETERFVNVPROC glad_glGetMapParameterfvNV = NULL;
PFNGLGETMAPATTRIBPARAMETERIVNVPROC glad_glGetMapAttribParameterivNV = NULL;
PFNGLGETMAPATTRIBPARAMETERFVNVPROC glad_glGetMapAttribParameterfvNV = NULL;
PFNGLEVALMAPSNVPROC glad_glEvalMapsNV = NULL;
PFNGLGETMULTISAMPLEFVNVPROC glad_glGetMultisamplefvNV = NULL;
PFNGLSAMPLEMASKINDEXEDNVPROC glad_glSampleMaskIndexedNV = NULL;
PFNGLTEXRENDERBUFFERNVPROC glad_glTexRenderbufferNV = NULL;
PFNGLDELETEFENCESNVPROC glad_glDeleteFencesNV = NULL;
PFNGLGENFENCESNVPROC glad_glGenFencesNV = NULL;
PFNGLISFENCENVPROC glad_glIsFenceNV = NULL;
PFNGLTESTFENCENVPROC glad_glTestFenceNV = NULL;
PFNGLGETFENCEIVNVPROC glad_glGetFenceivNV = NULL;
PFNGLFINISHFENCENVPROC glad_glFinishFenceNV = NULL;
PFNGLSETFENCENVPROC glad_glSetFenceNV = NULL;
PFNGLFRAGMENTCOVERAGECOLORNVPROC glad_glFragmentCoverageColorNV = NULL;
PFNGLPROGRAMNAMEDPARAMETER4FNVPROC glad_glProgramNamedParameter4fNV = NULL;
PFNGLPROGRAMNAMEDPARAMETER4FVNVPROC glad_glProgramNamedParameter4fvNV = NULL;
PFNGLPROGRAMNAMEDPARAMETER4DNVPROC glad_glProgramNamedParameter4dNV = NULL;
PFNGLPROGRAMNAMEDPARAMETER4DVNVPROC glad_glProgramNamedParameter4dvNV = NULL;
PFNGLGETPROGRAMNAMEDPARAMETERFVNVPROC glad_glGetProgramNamedParameterfvNV = NULL;
PFNGLGETPROGRAMNAMEDPARAMETERDVNVPROC glad_glGetProgramNamedParameterdvNV = NULL;
PFNGLCOVERAGEMODULATIONTABLENVPROC glad_glCoverageModulationTableNV = NULL;
PFNGLGETCOVERAGEMODULATIONTABLENVPROC glad_glGetCoverageModulationTableNV = NULL;
PFNGLCOVERAGEMODULATIONNVPROC glad_glCoverageModulationNV = NULL;
PFNGLRENDERBUFFERSTORAGEMULTISAMPLECOVERAGENVPROC glad_glRenderbufferStorageMultisampleCoverageNV = NULL;
PFNGLPROGRAMVERTEXLIMITNVPROC glad_glProgramVertexLimitNV = NULL;
PFNGLFRAMEBUFFERTEXTUREEXTPROC glad_glFramebufferTextureEXT = NULL;
PFNGLFRAMEBUFFERTEXTUREFACEEXTPROC glad_glFramebufferTextureFaceEXT = NULL;
PFNGLRENDERGPUMASKNVPROC glad_glRenderGpuMaskNV = NULL;
PFNGLMULTICASTBUFFERSUBDATANVPROC glad_glMulticastBufferSubDataNV = NULL;
PFNGLMULTICASTCOPYBUFFERSUBDATANVPROC glad_glMulticastCopyBufferSubDataNV = NULL;
PFNGLMULTICASTCOPYIMAGESUBDATANVPROC glad_glMulticastCopyImageSubDataNV = NULL;
PFNGLMULTICASTBLITFRAMEBUFFERNVPROC glad_glMulticastBlitFramebufferNV = NULL;
PFNGLMULTICASTFRAMEBUFFERSAMPLELOCATIONSFVNVPROC glad_glMulticastFramebufferSampleLocationsfvNV = NULL;
PFNGLMULTICASTBARRIERNVPROC glad_glMulticastBarrierNV = NULL;
PFNGLMULTICASTWAITSYNCNVPROC glad_glMulticastWaitSyncNV = NULL;
PFNGLMULTICASTGETQUERYOBJECTIVNVPROC glad_glMulticastGetQueryObjectivNV = NULL;
PFNGLMULTICASTGETQUERYOBJECTUIVNVPROC glad_glMulticastGetQueryObjectuivNV = NULL;
PFNGLMULTICASTGETQUERYOBJECTI64VNVPROC glad_glMulticastGetQueryObjecti64vNV = NULL;
PFNGLMULTICASTGETQUERYOBJECTUI64VNVPROC glad_glMulticastGetQueryObjectui64vNV = NULL;
PFNGLPROGRAMLOCALPARAMETERI4INVPROC glad_glProgramLocalParameterI4iNV = NULL;
PFNGLPROGRAMLOCALPARAMETERI4IVNVPROC glad_glProgramLocalParameterI4ivNV = NULL;
PFNGLPROGRAMLOCALPARAMETERSI4IVNVPROC glad_glProgramLocalParametersI4ivNV = NULL;
PFNGLPROGRAMLOCALPARAMETERI4UINVPROC glad_glProgramLocalParameterI4uiNV = NULL;
PFNGLPROGRAMLOCALPARAMETERI4UIVNVPROC glad_glProgramLocalParameterI4uivNV = NULL;
PFNGLPROGRAMLOCALPARAMETERSI4UIVNVPROC glad_glProgramLocalParametersI4uivNV = NULL;
PFNGLPROGRAMENVPARAMETERI4INVPROC glad_glProgramEnvParameterI4iNV = NULL;
PFNGLPROGRAMENVPARAMETERI4IVNVPROC glad_glProgramEnvParameterI4ivNV = NULL;
PFNGLPROGRAMENVPARAMETERSI4IVNVPROC glad_glProgramEnvParametersI4ivNV = NULL;
PFNGLPROGRAMENVPARAMETERI4UINVPROC glad_glProgramEnvParameterI4uiNV = NULL;
PFNGLPROGRAMENVPARAMETERI4UIVNVPROC glad_glProgramEnvParameterI4uivNV = NULL;
PFNGLPROGRAMENVPARAMETERSI4UIVNVPROC glad_glProgramEnvParametersI4uivNV = NULL;
PFNGLGETPROGRAMLOCALPARAMETERIIVNVPROC glad_glGetProgramLocalParameterIivNV = NULL;
PFNGLGETPROGRAMLOCALPARAMETERIUIVNVPROC glad_glGetProgramLocalParameterIuivNV = NULL;
PFNGLGETPROGRAMENVPARAMETERIIVNVPROC glad_glGetProgramEnvParameterIivNV = NULL;
PFNGLGETPROGRAMENVPARAMETERIUIVNVPROC glad_glGetProgramEnvParameterIuivNV = NULL;
PFNGLPROGRAMSUBROUTINEPARAMETERSUIVNVPROC glad_glProgramSubroutineParametersuivNV = NULL;
PFNGLGETPROGRAMSUBROUTINEPARAMETERUIVNVPROC glad_glGetProgramSubroutineParameteruivNV = NULL;
PFNGLVERTEX2HNVPROC glad_glVertex2hNV = NULL;
PFNGLVERTEX2HVNVPROC glad_glVertex2hvNV = NULL;
PFNGLVERTEX3HNVPROC glad_glVertex3hNV = NULL;
PFNGLVERTEX3HVNVPROC glad_glVertex3hvNV = NULL;
PFNGLVERTEX4HNVPROC glad_glVertex4hNV = NULL;
PFNGLVERTEX4HVNVPROC glad_glVertex4hvNV = NULL;
PFNGLNORMAL3HNVPROC glad_glNormal3hNV = NULL;
PFNGLNORMAL3HVNVPROC glad_glNormal3hvNV = NULL;
PFNGLCOLOR3HNVPROC glad_glColor3hNV = NULL;
PFNGLCOLOR3HVNVPROC glad_glColor3hvNV = NULL;
PFNGLCOLOR4HNVPROC glad_glColor4hNV = NULL;
PFNGLCOLOR4HVNVPROC glad_glColor4hvNV = NULL;
PFNGLTEXCOORD1HNVPROC glad_glTexCoord1hNV = NULL;
PFNGLTEXCOORD1HVNVPROC glad_glTexCoord1hvNV = NULL;
PFNGLTEXCOORD2HNVPROC glad_glTexCoord2hNV = NULL;
PFNGLTEXCOORD2HVNVPROC glad_glTexCoord2hvNV = NULL;
PFNGLTEXCOORD3HNVPROC glad_glTexCoord3hNV = NULL;
PFNGLTEXCOORD3HVNVPROC glad_glTexCoord3hvNV = NULL;
PFNGLTEXCOORD4HNVPROC glad_glTexCoord4hNV = NULL;
PFNGLTEXCOORD4HVNVPROC glad_glTexCoord4hvNV = NULL;
PFNGLMULTITEXCOORD1HNVPROC glad_glMultiTexCoord1hNV = NULL;
PFNGLMULTITEXCOORD1HVNVPROC glad_glMultiTexCoord1hvNV = NULL;
PFNGLMULTITEXCOORD2HNVPROC glad_glMultiTexCoord2hNV = NULL;
PFNGLMULTITEXCOORD2HVNVPROC glad_glMultiTexCoord2hvNV = NULL;
PFNGLMULTITEXCOORD3HNVPROC glad_glMultiTexCoord3hNV = NULL;
PFNGLMULTITEXCOORD3HVNVPROC glad_glMultiTexCoord3hvNV = NULL;
PFNGLMULTITEXCOORD4HNVPROC glad_glMultiTexCoord4hNV = NULL;
PFNGLMULTITEXCOORD4HVNVPROC glad_glMultiTexCoord4hvNV = NULL;
PFNGLFOGCOORDHNVPROC glad_glFogCoordhNV = NULL;
PFNGLFOGCOORDHVNVPROC glad_glFogCoordhvNV = NULL;
PFNGLSECONDARYCOLOR3HNVPROC glad_glSecondaryColor3hNV = NULL;
PFNGLSECONDARYCOLOR3HVNVPROC glad_glSecondaryColor3hvNV = NULL;
PFNGLVERTEXWEIGHTHNVPROC glad_glVertexWeighthNV = NULL;
PFNGLVERTEXWEIGHTHVNVPROC glad_glVertexWeighthvNV = NULL;
PFNGLVERTEXATTRIB1HNVPROC glad_glVertexAttrib1hNV = NULL;
PFNGLVERTEXATTRIB1HVNVPROC glad_glVertexAttrib1hvNV = NULL;
PFNGLVERTEXATTRIB2HNVPROC glad_glVertexAttrib2hNV = NULL;
PFNGLVERTEXATTRIB2HVNVPROC glad_glVertexAttrib2hvNV = NULL;
PFNGLVERTEXATTRIB3HNVPROC glad_glVertexAttrib3hNV = NULL;
PFNGLVERTEXATTRIB3HVNVPROC glad_glVertexAttrib3hvNV = NULL;
PFNGLVERTEXATTRIB4HNVPROC glad_glVertexAttrib4hNV = NULL;
PFNGLVERTEXATTRIB4HVNVPROC glad_glVertexAttrib4hvNV = NULL;
PFNGLVERTEXATTRIBS1HVNVPROC glad_glVertexAttribs1hvNV = NULL;
PFNGLVERTEXATTRIBS2HVNVPROC glad_glVertexAttribs2hvNV = NULL;
PFNGLVERTEXATTRIBS3HVNVPROC glad_glVertexAttribs3hvNV = NULL;
PFNGLVERTEXATTRIBS4HVNVPROC glad_glVertexAttribs4hvNV = NULL;
PFNGLGETINTERNALFORMATSAMPLEIVNVPROC glad_glGetInternalformatSampleivNV = NULL;
PFNGLGETMEMORYOBJECTDETACHEDRESOURCESUIVNVPROC glad_glGetMemoryObjectDetachedResourcesuivNV = NULL;
PFNGLRESETMEMORYOBJECTPARAMETERNVPROC glad_glResetMemoryObjectParameterNV = NULL;
PFNGLTEXATTACHMEMORYNVPROC glad_glTexAttachMemoryNV = NULL;
PFNGLBUFFERATTACHMEMORYNVPROC glad_glBufferAttachMemoryNV = NULL;
PFNGLTEXTUREATTACHMEMORYNVPROC glad_glTextureAttachMemoryNV = NULL;
PFNGLNAMEDBUFFERATTACHMEMORYNVPROC glad_glNamedBufferAttachMemoryNV = NULL;
PFNGLBUFFERPAGECOMMITMENTMEMNVPROC glad_glBufferPageCommitmentMemNV = NULL;
PFNGLTEXPAGECOMMITMENTMEMNVPROC glad_glTexPageCommitmentMemNV = NULL;
PFNGLNAMEDBUFFERPAGECOMMITMENTMEMNVPROC glad_glNamedBufferPageCommitmentMemNV = NULL;
PFNGLTEXTUREPAGECOMMITMENTMEMNVPROC glad_glTexturePageCommitmentMemNV = NULL;
PFNGLDRAWMESHTASKSNVPROC glad_glDrawMeshTasksNV = NULL;
PFNGLDRAWMESHTASKSINDIRECTNVPROC glad_glDrawMeshTasksIndirectNV = NULL;
PFNGLMULTIDRAWMESHTASKSINDIRECTNVPROC glad_glMultiDrawMeshTasksIndirectNV = NULL;
PFNGLMULTIDRAWMESHTASKSINDIRECTCOUNTNVPROC glad_glMultiDrawMeshTasksIndirectCountNV = NULL;
PFNGLGENOCCLUSIONQUERIESNVPROC glad_glGenOcclusionQueriesNV = NULL;
PFNGLDELETEOCCLUSIONQUERIESNVPROC glad_glDeleteOcclusionQueriesNV = NULL;
PFNGLISOCCLUSIONQUERYNVPROC glad_glIsOcclusionQueryNV = NULL;
PFNGLBEGINOCCLUSIONQUERYNVPROC glad_glBeginOcclusionQueryNV = NULL;
PFNGLENDOCCLUSIONQUERYNVPROC glad_glEndOcclusionQueryNV = NULL;
PFNGLGETOCCLUSIONQUERYIVNVPROC glad_glGetOcclusionQueryivNV = NULL;
PFNGLGETOCCLUSIONQUERYUIVNVPROC glad_glGetOcclusionQueryuivNV = NULL;
PFNGLPROGRAMBUFFERPARAMETERSFVNVPROC glad_glProgramBufferParametersfvNV = NULL;
PFNGLPROGRAMBUFFERPARAMETERSIIVNVPROC glad_glProgramBufferParametersIivNV = NULL;
PFNGLPROGRAMBUFFERPARAMETERSIUIVNVPROC glad_glProgramBufferParametersIuivNV = NULL;
PFNGLGENPATHSNVPROC glad_glGenPathsNV = NULL;
PFNGLDELETEPATHSNVPROC glad_glDeletePathsNV = NULL;
PFNGLISPATHNVPROC glad_glIsPathNV = NULL;
PFNGLPATHCOMMANDSNVPROC glad_glPathCommandsNV = NULL;
PFNGLPATHCOORDSNVPROC glad_glPathCoordsNV = NULL;
PFNGLPATHSUBCOMMANDSNVPROC glad_glPathSubCommandsNV = NULL;
PFNGLPATHSUBCOORDSNVPROC glad_glPathSubCoordsNV = NULL;
PFNGLPATHSTRINGNVPROC glad_glPathStringNV = NULL;
PFNGLPATHGLYPHSNVPROC glad_glPathGlyphsNV = NULL;
PFNGLPATHGLYPHRANGENVPROC glad_glPathGlyphRangeNV = NULL;
PFNGLWEIGHTPATHSNVPROC glad_glWeightPathsNV = NULL;
PFNGLCOPYPATHNVPROC glad_glCopyPathNV = NULL;
PFNGLINTERPOLATEPATHSNVPROC glad_glInterpolatePathsNV = NULL;
PFNGLTRANSFORMPATHNVPROC glad_glTransformPathNV = NULL;
PFNGLPATHPARAMETERIVNVPROC glad_glPathParameterivNV = NULL;
PFNGLPATHPARAMETERINVPROC glad_glPathParameteriNV = NULL;
PFNGLPATHPARAMETERFVNVPROC glad_glPathParameterfvNV = NULL;
PFNGLPATHPARAMETERFNVPROC glad_glPathParameterfNV = NULL;
PFNGLPATHDASHARRAYNVPROC glad_glPathDashArrayNV = NULL;
PFNGLPATHSTENCILFUNCNVPROC glad_glPathStencilFuncNV = NULL;
PFNGLPATHSTENCILDEPTHOFFSETNVPROC glad_glPathStencilDepthOffsetNV = NULL;
PFNGLSTENCILFILLPATHNVPROC glad_glStencilFillPathNV = NULL;
PFNGLSTENCILSTROKEPATHNVPROC glad_glStencilStrokePathNV = NULL;
PFNGLSTENCILFILLPATHINSTANCEDNVPROC glad_glStencilFillPathInstancedNV = NULL;
PFNGLSTENCILSTROKEPATHINSTANCEDNVPROC glad_glStencilStrokePathInstancedNV = NULL;
PFNGLPATHCOVERDEPTHFUNCNVPROC glad_glPathCoverDepthFuncNV = NULL;
PFNGLCOVERFILLPATHNVPROC glad_glCoverFillPathNV = NULL;
PFNGLCOVERSTROKEPATHNVPROC glad_glCoverStrokePathNV = NULL;
PFNGLCOVERFILLPATHINSTANCEDNVPROC glad_glCoverFillPathInstancedNV = NULL;
PFNGLCOVERSTROKEPATHINSTANCEDNVPROC glad_glCoverStrokePathInstancedNV = NULL;
PFNGLGETPATHPARAMETERIVNVPROC glad_glGetPathParameterivNV = NULL;
PFNGLGETPATHPARAMETERFVNVPROC glad_glGetPathParameterfvNV = NULL;
PFNGLGETPATHCOMMANDSNVPROC glad_glGetPathCommandsNV = NULL;
PFNGLGETPATHCOORDSNVPROC glad_glGetPathCoordsNV = NULL;
PFNGLGETPATHDASHARRAYNVPROC glad_glGetPathDashArrayNV = NULL;
PFNGLGETPATHMETRICSNVPROC glad_glGetPathMetricsNV = NULL;
PFNGLGETPATHMETRICRANGENVPROC glad_glGetPathMetricRangeNV = NULL;
PFNGLGETPATHSPACINGNVPROC glad_glGetPathSpacingNV = NULL;
PFNGLISPOINTINFILLPATHNVPROC glad_glIsPointInFillPathNV = NULL;
PFNGLISPOINTINSTROKEPATHNVPROC glad_glIsPointInStrokePathNV = NULL;
PFNGLGETPATHLENGTHNVPROC glad_glGetPathLengthNV = NULL;
PFNGLPOINTALONGPATHNVPROC glad_glPointAlongPathNV = NULL;
PFNGLMATRIXLOAD3X2FNVPROC glad_glMatrixLoad3x2fNV = NULL;
PFNGLMATRIXLOAD3X3FNVPROC glad_glMatrixLoad3x3fNV = NULL;
PFNGLMATRIXLOADTRANSPOSE3X3FNVPROC glad_glMatrixLoadTranspose3x3fNV = NULL;
PFNGLMATRIXMULT3X2FNVPROC glad_glMatrixMult3x2fNV = NULL;
PFNGLMATRIXMULT3X3FNVPROC glad_glMatrixMult3x3fNV = NULL;
PFNGLMATRIXMULTTRANSPOSE3X3FNVPROC glad_glMatrixMultTranspose3x3fNV = NULL;
PFNGLSTENCILTHENCOVERFILLPATHNVPROC glad_glStencilThenCoverFillPathNV = NULL;
PFNGLSTENCILTHENCOVERSTROKEPATHNVPROC glad_glStencilThenCoverStrokePathNV = NULL;
PFNGLSTENCILTHENCOVERFILLPATHINSTANCEDNVPROC glad_glStencilThenCoverFillPathInstancedNV = NULL;
PFNGLSTENCILTHENCOVERSTROKEPATHINSTANCEDNVPROC glad_glStencilThenCoverStrokePathInstancedNV = NULL;
PFNGLPATHGLYPHINDEXRANGENVPROC glad_glPathGlyphIndexRangeNV = NULL;
PFNGLPATHGLYPHINDEXARRAYNVPROC glad_glPathGlyphIndexArrayNV = NULL;
PFNGLPATHMEMORYGLYPHINDEXARRAYNVPROC glad_glPathMemoryGlyphIndexArrayNV = NULL;
PFNGLPROGRAMPATHFRAGMENTINPUTGENNVPROC glad_glProgramPathFragmentInputGenNV = NULL;
PFNGLGETPROGRAMRESOURCEFVNVPROC glad_glGetProgramResourcefvNV = NULL;
PFNGLPATHCOLORGENNVPROC glad_glPathColorGenNV = NULL;
PFNGLPATHTEXGENNVPROC glad_glPathTexGenNV = NULL;
PFNGLPATHFOGGENNVPROC glad_glPathFogGenNV = NULL;
PFNGLGETPATHCOLORGENIVNVPROC glad_glGetPathColorGenivNV = NULL;
PFNGLGETPATHCOLORGENFVNVPROC glad_glGetPathColorGenfvNV = NULL;
PFNGLGETPATHTEXGENIVNVPROC glad_glGetPathTexGenivNV = NULL;
PFNGLGETPATHTEXGENFVNVPROC glad_glGetPathTexGenfvNV = NULL;
PFNGLPIXELDATARANGENVPROC glad_glPixelDataRangeNV = NULL;
PFNGLFLUSHPIXELDATARANGENVPROC glad_glFlushPixelDataRangeNV = NULL;
PFNGLPOINTPARAMETERINVPROC glad_glPointParameteriNV = NULL;
PFNGLPOINTPARAMETERIVNVPROC glad_glPointParameterivNV = NULL;
PFNGLPRESENTFRAMEKEYEDNVPROC glad_glPresentFrameKeyedNV = NULL;
PFNGLPRESENTFRAMEDUALFILLNVPROC glad_glPresentFrameDualFillNV = NULL;
PFNGLGETVIDEOIVNVPROC glad_glGetVideoivNV = NULL;
PFNGLGETVIDEOUIVNVPROC glad_glGetVideouivNV = NULL;
PFNGLGETVIDEOI64VNVPROC glad_glGetVideoi64vNV = NULL;
PFNGLGETVIDEOUI64VNVPROC glad_glGetVideoui64vNV = NULL;
PFNGLPRIMITIVERESTARTNVPROC glad_glPrimitiveRestartNV = NULL;
PFNGLPRIMITIVERESTARTINDEXNVPROC glad_glPrimitiveRestartIndexNV = NULL;
PFNGLQUERYRESOURCENVPROC glad_glQueryResourceNV = NULL;
PFNGLGENQUERYRESOURCETAGNVPROC glad_glGenQueryResourceTagNV = NULL;
PFNGLDELETEQUERYRESOURCETAGNVPROC glad_glDeleteQueryResourceTagNV = NULL;
PFNGLQUERYRESOURCETAGNVPROC glad_glQueryResourceTagNV = NULL;
PFNGLCOMBINERPARAMETERFVNVPROC glad_glCombinerParameterfvNV = NULL;
PFNGLCOMBINERPARAMETERFNVPROC glad_glCombinerParameterfNV = NULL;
PFNGLCOMBINERPARAMETERIVNVPROC glad_glCombinerParameterivNV = NULL;
PFNGLCOMBINERPARAMETERINVPROC glad_glCombinerParameteriNV = NULL;
PFNGLCOMBINERINPUTNVPROC glad_glCombinerInputNV = NULL;
PFNGLCOMBINEROUTPUTNVPROC glad_glCombinerOutputNV = NULL;
PFNGLFINALCOMBINERINPUTNVPROC glad_glFinalCombinerInputNV = NULL;
PFNGLGETCOMBINERINPUTPARAMETERFVNVPROC glad_glGetCombinerInputParameterfvNV = NULL;
PFNGLGETCOMBINERINPUTPARAMETERIVNVPROC glad_glGetCombinerInputParameterivNV = NULL;
PFNGLGETCOMBINEROUTPUTPARAMETERFVNVPROC glad_glGetCombinerOutputParameterfvNV = NULL;
PFNGLGETCOMBINEROUTPUTPARAMETERIVNVPROC glad_glGetCombinerOutputParameterivNV = NULL;
PFNGLGETFINALCOMBINERINPUTPARAMETERFVNVPROC glad_glGetFinalCombinerInputParameterfvNV = NULL;
PFNGLGETFINALCOMBINERINPUTPARAMETERIVNVPROC glad_glGetFinalCombinerInputParameterivNV = NULL;
PFNGLCOMBINERSTAGEPARAMETERFVNVPROC glad_glCombinerStageParameterfvNV = NULL;
PFNGLGETCOMBINERSTAGEPARAMETERFVNVPROC glad_glGetCombinerStageParameterfvNV = NULL;
PFNGLFRAMEBUFFERSAMPLELOCATIONSFVNVPROC glad_glFramebufferSampleLocationsfvNV = NULL;
PFNGLNAMEDFRAMEBUFFERSAMPLELOCATIONSFVNVPROC glad_glNamedFramebufferSampleLocationsfvNV = NULL;
PFNGLRESOLVEDEPTHVALUESNVPROC glad_glResolveDepthValuesNV = NULL;
PFNGLSCISSOREXCLUSIVENVPROC glad_glScissorExclusiveNV = NULL;
PFNGLSCISSOREXCLUSIVEARRAYVNVPROC glad_glScissorExclusiveArrayvNV = NULL;
PFNGLMAKEBUFFERRESIDENTNVPROC glad_glMakeBufferResidentNV = NULL;
PFNGLMAKEBUFFERNONRESIDENTNVPROC glad_glMakeBufferNonResidentNV = NULL;
PFNGLISBUFFERRESIDENTNVPROC glad_glIsBufferResidentNV = NULL;
PFNGLMAKENAMEDBUFFERRESIDENTNVPROC glad_glMakeNamedBufferResidentNV = NULL;
PFNGLMAKENAMEDBUFFERNONRESIDENTNVPROC glad_glMakeNamedBufferNonResidentNV = NULL;
PFNGLISNAMEDBUFFERRESIDENTNVPROC glad_glIsNamedBufferResidentNV = NULL;
PFNGLGETBUFFERPARAMETERUI64VNVPROC glad_glGetBufferParameterui64vNV = NULL;
PFNGLGETNAMEDBUFFERPARAMETERUI64VNVPROC glad_glGetNamedBufferParameterui64vNV = NULL;
PFNGLGETINTEGERUI64VNVPROC glad_glGetIntegerui64vNV = NULL;
PFNGLUNIFORMUI64NVPROC glad_glUniformui64NV = NULL;
PFNGLUNIFORMUI64VNVPROC glad_glUniformui64vNV = NULL;
PFNGLPROGRAMUNIFORMUI64NVPROC glad_glProgramUniformui64NV = NULL;
PFNGLPROGRAMUNIFORMUI64VNVPROC glad_glProgramUniformui64vNV = NULL;
PFNGLBINDSHADINGRATEIMAGENVPROC glad_glBindShadingRateImageNV = NULL;
PFNGLGETSHADINGRATEIMAGEPALETTENVPROC glad_glGetShadingRateImagePaletteNV = NULL;
PFNGLGETSHADINGRATESAMPLELOCATIONIVNVPROC glad_glGetShadingRateSampleLocationivNV = NULL;
PFNGLSHADINGRATEIMAGEBARRIERNVPROC glad_glShadingRateImageBarrierNV = NULL;
PFNGLSHADINGRATEIMAGEPALETTENVPROC glad_glShadingRateImagePaletteNV = NULL;
PFNGLSHADINGRATESAMPLEORDERNVPROC glad_glShadingRateSampleOrderNV = NULL;
PFNGLSHADINGRATESAMPLEORDERCUSTOMNVPROC glad_glShadingRateSampleOrderCustomNV = NULL;
PFNGLTEXTUREBARRIERNVPROC glad_glTextureBarrierNV = NULL;
PFNGLTEXIMAGE2DMULTISAMPLECOVERAGENVPROC glad_glTexImage2DMultisampleCoverageNV = NULL;
PFNGLTEXIMAGE3DMULTISAMPLECOVERAGENVPROC glad_glTexImage3DMultisampleCoverageNV = NULL;
PFNGLTEXTUREIMAGE2DMULTISAMPLENVPROC glad_glTextureImage2DMultisampleNV = NULL;
PFNGLTEXTUREIMAGE3DMULTISAMPLENVPROC glad_glTextureImage3DMultisampleNV = NULL;
PFNGLTEXTUREIMAGE2DMULTISAMPLECOVERAGENVPROC glad_glTextureImage2DMultisampleCoverageNV = NULL;
PFNGLTEXTUREIMAGE3DMULTISAMPLECOVERAGENVPROC glad_glTextureImage3DMultisampleCoverageNV = NULL;
PFNGLCREATESEMAPHORESNVPROC glad_glCreateSemaphoresNV = NULL;
PFNGLSEMAPHOREPARAMETERIVNVPROC glad_glSemaphoreParameterivNV = NULL;
PFNGLGETSEMAPHOREPARAMETERIVNVPROC glad_glGetSemaphoreParameterivNV = NULL;
PFNGLBEGINTRANSFORMFEEDBACKNVPROC glad_glBeginTransformFeedbackNV = NULL;
PFNGLENDTRANSFORMFEEDBACKNVPROC glad_glEndTransformFeedbackNV = NULL;
PFNGLTRANSFORMFEEDBACKATTRIBSNVPROC glad_glTransformFeedbackAttribsNV = NULL;
PFNGLBINDBUFFERRANGENVPROC glad_glBindBufferRangeNV = NULL;
PFNGLBINDBUFFEROFFSETNVPROC glad_glBindBufferOffsetNV = NULL;
PFNGLBINDBUFFERBASENVPROC glad_glBindBufferBaseNV = NULL;
PFNGLTRANSFORMFEEDBACKVARYINGSNVPROC glad_glTransformFeedbackVaryingsNV = NULL;
PFNGLACTIVEVARYINGNVPROC glad_glActiveVaryingNV = NULL;
PFNGLGETVARYINGLOCATIONNVPROC glad_glGetVaryingLocationNV = NULL;
PFNGLGETACTIVEVARYINGNVPROC glad_glGetActiveVaryingNV = NULL;
PFNGLGETTRANSFORMFEEDBACKVARYINGNVPROC glad_glGetTransformFeedbackVaryingNV = NULL;
PFNGLTRANSFORMFEEDBACKSTREAMATTRIBSNVPROC glad_glTransformFeedbackStreamAttribsNV = NULL;
PFNGLBINDTRANSFORMFEEDBACKNVPROC glad_glBindTransformFeedbackNV = NULL;
PFNGLDELETETRANSFORMFEEDBACKSNVPROC glad_glDeleteTransformFeedbacksNV = NULL;
PFNGLGENTRANSFORMFEEDBACKSNVPROC glad_glGenTransformFeedbacksNV = NULL;
PFNGLISTRANSFORMFEEDBACKNVPROC glad_glIsTransformFeedbackNV = NULL;
PFNGLPAUSETRANSFORMFEEDBACKNVPROC glad_glPauseTransformFeedbackNV = NULL;
PFNGLRESUMETRANSFORMFEEDBACKNVPROC glad_glResumeTransformFeedbackNV = NULL;
PFNGLDRAWTRANSFORMFEEDBACKNVPROC glad_glDrawTransformFeedbackNV = NULL;
PFNGLVDPAUINITNVPROC glad_glVDPAUInitNV = NULL;
PFNGLVDPAUFININVPROC glad_glVDPAUFiniNV = NULL;
PFNGLVDPAUREGISTERVIDEOSURFACENVPROC glad_glVDPAURegisterVideoSurfaceNV = NULL;
PFNGLVDPAUREGISTEROUTPUTSURFACENVPROC glad_glVDPAURegisterOutputSurfaceNV = NULL;
PFNGLVDPAUISSURFACENVPROC glad_glVDPAUIsSurfaceNV = NULL;
PFNGLVDPAUUNREGISTERSURFACENVPROC glad_glVDPAUUnregisterSurfaceNV = NULL;
PFNGLVDPAUGETSURFACEIVNVPROC glad_glVDPAUGetSurfaceivNV = NULL;
PFNGLVDPAUSURFACEACCESSNVPROC glad_glVDPAUSurfaceAccessNV = NULL;
PFNGLVDPAUMAPSURFACESNVPROC glad_glVDPAUMapSurfacesNV = NULL;
PFNGLVDPAUUNMAPSURFACESNVPROC glad_glVDPAUUnmapSurfacesNV = NULL;
PFNGLVDPAUREGISTERVIDEOSURFACEWITHPICTURESTRUCTURENVPROC glad_glVDPAURegisterVideoSurfaceWithPictureStructureNV = NULL;
PFNGLFLUSHVERTEXARRAYRANGENVPROC glad_glFlushVertexArrayRangeNV = NULL;
PFNGLVERTEXARRAYRANGENVPROC glad_glVertexArrayRangeNV = NULL;
PFNGLVERTEXATTRIBL1I64NVPROC glad_glVertexAttribL1i64NV = NULL;
PFNGLVERTEXATTRIBL2I64NVPROC glad_glVertexAttribL2i64NV = NULL;
PFNGLVERTEXATTRIBL3I64NVPROC glad_glVertexAttribL3i64NV = NULL;
PFNGLVERTEXATTRIBL4I64NVPROC glad_glVertexAttribL4i64NV = NULL;
PFNGLVERTEXATTRIBL1I64VNVPROC glad_glVertexAttribL1i64vNV = NULL;
PFNGLVERTEXATTRIBL2I64VNVPROC glad_glVertexAttribL2i64vNV = NULL;
PFNGLVERTEXATTRIBL3I64VNVPROC glad_glVertexAttribL3i64vNV = NULL;
PFNGLVERTEXATTRIBL4I64VNVPROC glad_glVertexAttribL4i64vNV = NULL;
PFNGLVERTEXATTRIBL1UI64NVPROC glad_glVertexAttribL1ui64NV = NULL;
PFNGLVERTEXATTRIBL2UI64NVPROC glad_glVertexAttribL2ui64NV = NULL;
PFNGLVERTEXATTRIBL3UI64NVPROC glad_glVertexAttribL3ui64NV = NULL;
PFNGLVERTEXATTRIBL4UI64NVPROC glad_glVertexAttribL4ui64NV = NULL;
PFNGLVERTEXATTRIBL1UI64VNVPROC glad_glVertexAttribL1ui64vNV = NULL;
PFNGLVERTEXATTRIBL2UI64VNVPROC glad_glVertexAttribL2ui64vNV = NULL;
PFNGLVERTEXATTRIBL3UI64VNVPROC glad_glVertexAttribL3ui64vNV = NULL;
PFNGLVERTEXATTRIBL4UI64VNVPROC glad_glVertexAttribL4ui64vNV = NULL;
PFNGLGETVERTEXATTRIBLI64VNVPROC glad_glGetVertexAttribLi64vNV = NULL;
PFNGLGETVERTEXATTRIBLUI64VNVPROC glad_glGetVertexAttribLui64vNV = NULL;
PFNGLVERTEXATTRIBLFORMATNVPROC glad_glVertexAttribLFormatNV = NULL;
PFNGLBUFFERADDRESSRANGENVPROC glad_glBufferAddressRangeNV = NULL;
PFNGLVERTEXFORMATNVPROC glad_glVertexFormatNV = NULL;
PFNGLNORMALFORMATNVPROC glad_glNormalFormatNV = NULL;
PFNGLCOLORFORMATNVPROC glad_glColorFormatNV = NULL;
PFNGLINDEXFORMATNVPROC glad_glIndexFormatNV = NULL;
PFNGLTEXCOORDFORMATNVPROC glad_glTexCoordFormatNV = NULL;
PFNGLEDGEFLAGFORMATNVPROC glad_glEdgeFlagFormatNV = NULL;
PFNGLSECONDARYCOLORFORMATNVPROC glad_glSecondaryColorFormatNV = NULL;
PFNGLFOGCOORDFORMATNVPROC glad_glFogCoordFormatNV = NULL;
PFNGLVERTEXATTRIBFORMATNVPROC glad_glVertexAttribFormatNV = NULL;
PFNGLVERTEXATTRIBIFORMATNVPROC glad_glVertexAttribIFormatNV = NULL;
PFNGLGETINTEGERUI64I_VNVPROC glad_glGetIntegerui64i_vNV = NULL;
PFNGLAREPROGRAMSRESIDENTNVPROC glad_glAreProgramsResidentNV = NULL;
PFNGLBINDPROGRAMNVPROC glad_glBindProgramNV = NULL;
PFNGLDELETEPROGRAMSNVPROC glad_glDeleteProgramsNV = NULL;
PFNGLEXECUTEPROGRAMNVPROC glad_glExecuteProgramNV = NULL;
PFNGLGENPROGRAMSNVPROC glad_glGenProgramsNV = NULL;
PFNGLGETPROGRAMPARAMETERDVNVPROC glad_glGetProgramParameterdvNV = NULL;
PFNGLGETPROGRAMPARAMETERFVNVPROC glad_glGetProgramParameterfvNV = NULL;
PFNGLGETPROGRAMIVNVPROC glad_glGetProgramivNV = NULL;
PFNGLGETPROGRAMSTRINGNVPROC glad_glGetProgramStringNV = NULL;
PFNGLGETTRACKMATRIXIVNVPROC glad_glGetTrackMatrixivNV = NULL;
PFNGLGETVERTEXATTRIBDVNVPROC glad_glGetVertexAttribdvNV = NULL;
PFNGLGETVERTEXATTRIBFVNVPROC glad_glGetVertexAttribfvNV = NULL;
PFNGLGETVERTEXATTRIBIVNVPROC glad_glGetVertexAttribivNV = NULL;
PFNGLGETVERTEXATTRIBPOINTERVNVPROC glad_glGetVertexAttribPointervNV = NULL;
PFNGLISPROGRAMNVPROC glad_glIsProgramNV = NULL;
PFNGLLOADPROGRAMNVPROC glad_glLoadProgramNV = NULL;
PFNGLPROGRAMPARAMETER4DNVPROC glad_glProgramParameter4dNV = NULL;
PFNGLPROGRAMPARAMETER4DVNVPROC glad_glProgramParameter4dvNV = NULL;
PFNGLPROGRAMPARAMETER4FNVPROC glad_glProgramParameter4fNV = NULL;
PFNGLPROGRAMPARAMETER4FVNVPROC glad_glProgramParameter4fvNV = NULL;
PFNGLPROGRAMPARAMETERS4DVNVPROC glad_glProgramParameters4dvNV = NULL;
PFNGLPROGRAMPARAMETERS4FVNVPROC glad_glProgramParameters4fvNV = NULL;
PFNGLREQUESTRESIDENTPROGRAMSNVPROC glad_glRequestResidentProgramsNV = NULL;
PFNGLTRACKMATRIXNVPROC glad_glTrackMatrixNV = NULL;
PFNGLVERTEXATTRIBPOINTERNVPROC glad_glVertexAttribPointerNV = NULL;
PFNGLVERTEXATTRIB1DNVPROC glad_glVertexAttrib1dNV = NULL;
PFNGLVERTEXATTRIB1DVNVPROC glad_glVertexAttrib1dvNV = NULL;
PFNGLVERTEXATTRIB1FNVPROC glad_glVertexAttrib1fNV = NULL;
PFNGLVERTEXATTRIB1FVNVPROC glad_glVertexAttrib1fvNV = NULL;
PFNGLVERTEXATTRIB1SNVPROC glad_glVertexAttrib1sNV = NULL;
PFNGLVERTEXATTRIB1SVNVPROC glad_glVertexAttrib1svNV = NULL;
PFNGLVERTEXATTRIB2DNVPROC glad_glVertexAttrib2dNV = NULL;
PFNGLVERTEXATTRIB2DVNVPROC glad_glVertexAttrib2dvNV = NULL;
PFNGLVERTEXATTRIB2FNVPROC glad_glVertexAttrib2fNV = NULL;
PFNGLVERTEXATTRIB2FVNVPROC glad_glVertexAttrib2fvNV = NULL;
PFNGLVERTEXATTRIB2SNVPROC glad_glVertexAttrib2sNV = NULL;
PFNGLVERTEXATTRIB2SVNVPROC glad_glVertexAttrib2svNV = NULL;
PFNGLVERTEXATTRIB3DNVPROC glad_glVertexAttrib3dNV = NULL;
PFNGLVERTEXATTRIB3DVNVPROC glad_glVertexAttrib3dvNV = NULL;
PFNGLVERTEXATTRIB3FNVPROC glad_glVertexAttrib3fNV = NULL;
PFNGLVERTEXATTRIB3FVNVPROC glad_glVertexAttrib3fvNV = NULL;
PFNGLVERTEXATTRIB3SNVPROC glad_glVertexAttrib3sNV = NULL;
PFNGLVERTEXATTRIB3SVNVPROC glad_glVertexAttrib3svNV = NULL;
PFNGLVERTEXATTRIB4DNVPROC glad_glVertexAttrib4dNV = NULL;
PFNGLVERTEXATTRIB4DVNVPROC glad_glVertexAttrib4dvNV = NULL;
PFNGLVERTEXATTRIB4FNVPROC glad_glVertexAttrib4fNV = NULL;
PFNGLVERTEXATTRIB4FVNVPROC glad_glVertexAttrib4fvNV = NULL;
PFNGLVERTEXATTRIB4SNVPROC glad_glVertexAttrib4sNV = NULL;
PFNGLVERTEXATTRIB4SVNVPROC glad_glVertexAttrib4svNV = NULL;
PFNGLVERTEXATTRIB4UBNVPROC glad_glVertexAttrib4ubNV = NULL;
PFNGLVERTEXATTRIB4UBVNVPROC glad_glVertexAttrib4ubvNV = NULL;
PFNGLVERTEXATTRIBS1DVNVPROC glad_glVertexAttribs1dvNV = NULL;
PFNGLVERTEXATTRIBS1FVNVPROC glad_glVertexAttribs1fvNV = NULL;
PFNGLVERTEXATTRIBS1SVNVPROC glad_glVertexAttribs1svNV = NULL;
PFNGLVERTEXATTRIBS2DVNVPROC glad_glVertexAttribs2dvNV = NULL;
PFNGLVERTEXATTRIBS2FVNVPROC glad_glVertexAttribs2fvNV = NULL;
PFNGLVERTEXATTRIBS2SVNVPROC glad_glVertexAttribs2svNV = NULL;
PFNGLVERTEXATTRIBS3DVNVPROC glad_glVertexAttribs3dvNV = NULL;
PFNGLVERTEXATTRIBS3FVNVPROC glad_glVertexAttribs3fvNV = NULL;
PFNGLVERTEXATTRIBS3SVNVPROC glad_glVertexAttribs3svNV = NULL;
PFNGLVERTEXATTRIBS4DVNVPROC glad_glVertexAttribs4dvNV = NULL;
PFNGLVERTEXATTRIBS4FVNVPROC glad_glVertexAttribs4fvNV = NULL;
PFNGLVERTEXATTRIBS4SVNVPROC glad_glVertexAttribs4svNV = NULL;
PFNGLVERTEXATTRIBS4UBVNVPROC glad_glVertexAttribs4ubvNV = NULL;
PFNGLBEGINVIDEOCAPTURENVPROC glad_glBeginVideoCaptureNV = NULL;
PFNGLBINDVIDEOCAPTURESTREAMBUFFERNVPROC glad_glBindVideoCaptureStreamBufferNV = NULL;
PFNGLBINDVIDEOCAPTURESTREAMTEXTURENVPROC glad_glBindVideoCaptureStreamTextureNV = NULL;
PFNGLENDVIDEOCAPTURENVPROC glad_glEndVideoCaptureNV = NULL;
PFNGLGETVIDEOCAPTUREIVNVPROC glad_glGetVideoCaptureivNV = NULL;
PFNGLGETVIDEOCAPTURESTREAMIVNVPROC glad_glGetVideoCaptureStreamivNV = NULL;
PFNGLGETVIDEOCAPTURESTREAMFVNVPROC glad_glGetVideoCaptureStreamfvNV = NULL;
PFNGLGETVIDEOCAPTURESTREAMDVNVPROC glad_glGetVideoCaptureStreamdvNV = NULL;
PFNGLVIDEOCAPTURENVPROC glad_glVideoCaptureNV = NULL;
PFNGLVIDEOCAPTURESTREAMPARAMETERIVNVPROC glad_glVideoCaptureStreamParameterivNV = NULL;
PFNGLVIDEOCAPTURESTREAMPARAMETERFVNVPROC glad_glVideoCaptureStreamParameterfvNV = NULL;
PFNGLVIDEOCAPTURESTREAMPARAMETERDVNVPROC glad_glVideoCaptureStreamParameterdvNV = NULL;
PFNGLVIEWPORTSWIZZLENVPROC glad_glViewportSwizzleNV = NULL;
PFNGLMULTITEXCOORD1BOESPROC glad_glMultiTexCoord1bOES = NULL;
PFNGLMULTITEXCOORD1BVOESPROC glad_glMultiTexCoord1bvOES = NULL;
PFNGLMULTITEXCOORD2BOESPROC glad_glMultiTexCoord2bOES = NULL;
PFNGLMULTITEXCOORD2BVOESPROC glad_glMultiTexCoord2bvOES = NULL;
PFNGLMULTITEXCOORD3BOESPROC glad_glMultiTexCoord3bOES = NULL;
PFNGLMULTITEXCOORD3BVOESPROC glad_glMultiTexCoord3bvOES = NULL;
PFNGLMULTITEXCOORD4BOESPROC glad_glMultiTexCoord4bOES = NULL;
PFNGLMULTITEXCOORD4BVOESPROC glad_glMultiTexCoord4bvOES = NULL;
PFNGLTEXCOORD1BOESPROC glad_glTexCoord1bOES = NULL;
PFNGLTEXCOORD1BVOESPROC glad_glTexCoord1bvOES = NULL;
PFNGLTEXCOORD2BOESPROC glad_glTexCoord2bOES = NULL;
PFNGLTEXCOORD2BVOESPROC glad_glTexCoord2bvOES = NULL;
PFNGLTEXCOORD3BOESPROC glad_glTexCoord3bOES = NULL;
PFNGLTEXCOORD3BVOESPROC glad_glTexCoord3bvOES = NULL;
PFNGLTEXCOORD4BOESPROC glad_glTexCoord4bOES = NULL;
PFNGLTEXCOORD4BVOESPROC glad_glTexCoord4bvOES = NULL;
PFNGLVERTEX2BOESPROC glad_glVertex2bOES = NULL;
PFNGLVERTEX2BVOESPROC glad_glVertex2bvOES = NULL;
PFNGLVERTEX3BOESPROC glad_glVertex3bOES = NULL;
PFNGLVERTEX3BVOESPROC glad_glVertex3bvOES = NULL;
PFNGLVERTEX4BOESPROC glad_glVertex4bOES = NULL;
PFNGLVERTEX4BVOESPROC glad_glVertex4bvOES = NULL;
PFNGLALPHAFUNCXOESPROC glad_glAlphaFuncxOES = NULL;
PFNGLCLEARCOLORXOESPROC glad_glClearColorxOES = NULL;
PFNGLCLEARDEPTHXOESPROC glad_glClearDepthxOES = NULL;
PFNGLCLIPPLANEXOESPROC glad_glClipPlanexOES = NULL;
PFNGLCOLOR4XOESPROC glad_glColor4xOES = NULL;
PFNGLDEPTHRANGEXOESPROC glad_glDepthRangexOES = NULL;
PFNGLFOGXOESPROC glad_glFogxOES = NULL;
PFNGLFOGXVOESPROC glad_glFogxvOES = NULL;
PFNGLFRUSTUMXOESPROC glad_glFrustumxOES = NULL;
PFNGLGETCLIPPLANEXOESPROC glad_glGetClipPlanexOES = NULL;
PFNGLGETFIXEDVOESPROC glad_glGetFixedvOES = NULL;
PFNGLGETTEXENVXVOESPROC glad_glGetTexEnvxvOES = NULL;
PFNGLGETTEXPARAMETERXVOESPROC glad_glGetTexParameterxvOES = NULL;
PFNGLLIGHTMODELXOESPROC glad_glLightModelxOES = NULL;
PFNGLLIGHTMODELXVOESPROC glad_glLightModelxvOES = NULL;
PFNGLLIGHTXOESPROC glad_glLightxOES = NULL;
PFNGLLIGHTXVOESPROC glad_glLightxvOES = NULL;
PFNGLLINEWIDTHXOESPROC glad_glLineWidthxOES = NULL;
PFNGLLOADMATRIXXOESPROC glad_glLoadMatrixxOES = NULL;
PFNGLMATERIALXOESPROC glad_glMaterialxOES = NULL;
PFNGLMATERIALXVOESPROC glad_glMaterialxvOES = NULL;
PFNGLMULTMATRIXXOESPROC glad_glMultMatrixxOES = NULL;
PFNGLMULTITEXCOORD4XOESPROC glad_glMultiTexCoord4xOES = NULL;
PFNGLNORMAL3XOESPROC glad_glNormal3xOES = NULL;
PFNGLORTHOXOESPROC glad_glOrthoxOES = NULL;
PFNGLPOINTPARAMETERXVOESPROC glad_glPointParameterxvOES = NULL;
PFNGLPOINTSIZEXOESPROC glad_glPointSizexOES = NULL;
PFNGLPOLYGONOFFSETXOESPROC glad_glPolygonOffsetxOES = NULL;
PFNGLROTATEXOESPROC glad_glRotatexOES = NULL;
PFNGLSCALEXOESPROC glad_glScalexOES = NULL;
PFNGLTEXENVXOESPROC glad_glTexEnvxOES = NULL;
PFNGLTEXENVXVOESPROC glad_glTexEnvxvOES = NULL;
PFNGLTEXPARAMETERXOESPROC glad_glTexParameterxOES = NULL;
PFNGLTEXPARAMETERXVOESPROC glad_glTexParameterxvOES = NULL;
PFNGLTRANSLATEXOESPROC glad_glTranslatexOES = NULL;
PFNGLGETLIGHTXVOESPROC glad_glGetLightxvOES = NULL;
PFNGLGETMATERIALXVOESPROC glad_glGetMaterialxvOES = NULL;
PFNGLPOINTPARAMETERXOESPROC glad_glPointParameterxOES = NULL;
PFNGLSAMPLECOVERAGEXOESPROC glad_glSampleCoveragexOES = NULL;
PFNGLACCUMXOESPROC glad_glAccumxOES = NULL;
PFNGLBITMAPXOESPROC glad_glBitmapxOES = NULL;
PFNGLBLENDCOLORXOESPROC glad_glBlendColorxOES = NULL;
PFNGLCLEARACCUMXOESPROC glad_glClearAccumxOES = NULL;
PFNGLCOLOR3XOESPROC glad_glColor3xOES = NULL;
PFNGLCOLOR3XVOESPROC glad_glColor3xvOES = NULL;
PFNGLCOLOR4XVOESPROC glad_glColor4xvOES = NULL;
PFNGLCONVOLUTIONPARAMETERXOESPROC glad_glConvolutionParameterxOES = NULL;
PFNGLCONVOLUTIONPARAMETERXVOESPROC glad_glConvolutionParameterxvOES = NULL;
PFNGLEVALCOORD1XOESPROC glad_glEvalCoord1xOES = NULL;
PFNGLEVALCOORD1XVOESPROC glad_glEvalCoord1xvOES = NULL;
PFNGLEVALCOORD2XOESPROC glad_glEvalCoord2xOES = NULL;
PFNGLEVALCOORD2XVOESPROC glad_glEvalCoord2xvOES = NULL;
PFNGLFEEDBACKBUFFERXOESPROC glad_glFeedbackBufferxOES = NULL;
PFNGLGETCONVOLUTIONPARAMETERXVOESPROC glad_glGetConvolutionParameterxvOES = NULL;
PFNGLGETHISTOGRAMPARAMETERXVOESPROC glad_glGetHistogramParameterxvOES = NULL;
PFNGLGETLIGHTXOESPROC glad_glGetLightxOES = NULL;
PFNGLGETMAPXVOESPROC glad_glGetMapxvOES = NULL;
PFNGLGETMATERIALXOESPROC glad_glGetMaterialxOES = NULL;
PFNGLGETPIXELMAPXVPROC glad_glGetPixelMapxv = NULL;
PFNGLGETTEXGENXVOESPROC glad_glGetTexGenxvOES = NULL;
PFNGLGETTEXLEVELPARAMETERXVOESPROC glad_glGetTexLevelParameterxvOES = NULL;
PFNGLINDEXXOESPROC glad_glIndexxOES = NULL;
PFNGLINDEXXVOESPROC glad_glIndexxvOES = NULL;
PFNGLLOADTRANSPOSEMATRIXXOESPROC glad_glLoadTransposeMatrixxOES = NULL;
PFNGLMAP1XOESPROC glad_glMap1xOES = NULL;
PFNGLMAP2XOESPROC glad_glMap2xOES = NULL;
PFNGLMAPGRID1XOESPROC glad_glMapGrid1xOES = NULL;
PFNGLMAPGRID2XOESPROC glad_glMapGrid2xOES = NULL;
PFNGLMULTTRANSPOSEMATRIXXOESPROC glad_glMultTransposeMatrixxOES = NULL;
PFNGLMULTITEXCOORD1XOESPROC glad_glMultiTexCoord1xOES = NULL;
PFNGLMULTITEXCOORD1XVOESPROC glad_glMultiTexCoord1xvOES = NULL;
PFNGLMULTITEXCOORD2XOESPROC glad_glMultiTexCoord2xOES = NULL;
PFNGLMULTITEXCOORD2XVOESPROC glad_glMultiTexCoord2xvOES = NULL;
PFNGLMULTITEXCOORD3XOESPROC glad_glMultiTexCoord3xOES = NULL;
PFNGLMULTITEXCOORD3XVOESPROC glad_glMultiTexCoord3xvOES = NULL;
PFNGLMULTITEXCOORD4XVOESPROC glad_glMultiTexCoord4xvOES = NULL;
PFNGLNORMAL3XVOESPROC glad_glNormal3xvOES = NULL;
PFNGLPASSTHROUGHXOESPROC glad_glPassThroughxOES = NULL;
PFNGLPIXELMAPXPROC glad_glPixelMapx = NULL;
PFNGLPIXELSTOREXPROC glad_glPixelStorex = NULL;
PFNGLPIXELTRANSFERXOESPROC glad_glPixelTransferxOES = NULL;
PFNGLPIXELZOOMXOESPROC glad_glPixelZoomxOES = NULL;
PFNGLPRIORITIZETEXTURESXOESPROC glad_glPrioritizeTexturesxOES = NULL;
PFNGLRASTERPOS2XOESPROC glad_glRasterPos2xOES = NULL;
PFNGLRASTERPOS2XVOESPROC glad_glRasterPos2xvOES = NULL;
PFNGLRASTERPOS3XOESPROC glad_glRasterPos3xOES = NULL;
PFNGLRASTERPOS3XVOESPROC glad_glRasterPos3xvOES = NULL;
PFNGLRASTERPOS4XOESPROC glad_glRasterPos4xOES = NULL;
PFNGLRASTERPOS4XVOESPROC glad_glRasterPos4xvOES = NULL;
PFNGLRECTXOESPROC glad_glRectxOES = NULL;
PFNGLRECTXVOESPROC glad_glRectxvOES = NULL;
PFNGLTEXCOORD1XOESPROC glad_glTexCoord1xOES = NULL;
PFNGLTEXCOORD1XVOESPROC glad_glTexCoord1xvOES = NULL;
PFNGLTEXCOORD2XOESPROC glad_glTexCoord2xOES = NULL;
PFNGLTEXCOORD2XVOESPROC glad_glTexCoord2xvOES = NULL;
PFNGLTEXCOORD3XOESPROC glad_glTexCoord3xOES = NULL;
PFNGLTEXCOORD3XVOESPROC glad_glTexCoord3xvOES = NULL;
PFNGLTEXCOORD4XOESPROC glad_glTexCoord4xOES = NULL;
PFNGLTEXCOORD4XVOESPROC glad_glTexCoord4xvOES = NULL;
PFNGLTEXGENXOESPROC glad_glTexGenxOES = NULL;
PFNGLTEXGENXVOESPROC glad_glTexGenxvOES = NULL;
PFNGLVERTEX2XOESPROC glad_glVertex2xOES = NULL;
PFNGLVERTEX2XVOESPROC glad_glVertex2xvOES = NULL;
PFNGLVERTEX3XOESPROC glad_glVertex3xOES = NULL;
PFNGLVERTEX3XVOESPROC glad_glVertex3xvOES = NULL;
PFNGLVERTEX4XOESPROC glad_glVertex4xOES = NULL;
PFNGLVERTEX4XVOESPROC glad_glVertex4xvOES = NULL;
PFNGLQUERYMATRIXXOESPROC glad_glQueryMatrixxOES = NULL;
PFNGLCLEARDEPTHFOESPROC glad_glClearDepthfOES = NULL;
PFNGLCLIPPLANEFOESPROC glad_glClipPlanefOES = NULL;
PFNGLDEPTHRANGEFOESPROC glad_glDepthRangefOES = NULL;
PFNGLFRUSTUMFOESPROC glad_glFrustumfOES = NULL;
PFNGLGETCLIPPLANEFOESPROC glad_glGetClipPlanefOES = NULL;
PFNGLORTHOFOESPROC glad_glOrthofOES = NULL;
PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC glad_glFramebufferTextureMultiviewOVR = NULL;
PFNGLHINTPGIPROC glad_glHintPGI = NULL;
PFNGLDETAILTEXFUNCSGISPROC glad_glDetailTexFuncSGIS = NULL;
PFNGLGETDETAILTEXFUNCSGISPROC glad_glGetDetailTexFuncSGIS = NULL;
PFNGLFOGFUNCSGISPROC glad_glFogFuncSGIS = NULL;
PFNGLGETFOGFUNCSGISPROC glad_glGetFogFuncSGIS = NULL;
PFNGLSAMPLEMASKSGISPROC glad_glSampleMaskSGIS = NULL;
PFNGLSAMPLEPATTERNSGISPROC glad_glSamplePatternSGIS = NULL;
PFNGLPIXELTEXGENPARAMETERISGISPROC glad_glPixelTexGenParameteriSGIS = NULL;
PFNGLPIXELTEXGENPARAMETERIVSGISPROC glad_glPixelTexGenParameterivSGIS = NULL;
PFNGLPIXELTEXGENPARAMETERFSGISPROC glad_glPixelTexGenParameterfSGIS = NULL;
PFNGLPIXELTEXGENPARAMETERFVSGISPROC glad_glPixelTexGenParameterfvSGIS = NULL;
PFNGLGETPIXELTEXGENPARAMETERIVSGISPROC glad_glGetPixelTexGenParameterivSGIS = NULL;
PFNGLGETPIXELTEXGENPARAMETERFVSGISPROC glad_glGetPixelTexGenParameterfvSGIS = NULL;
PFNGLPOINTPARAMETERFSGISPROC glad_glPointParameterfSGIS = NULL;
PFNGLPOINTPARAMETERFVSGISPROC glad_glPointParameterfvSGIS = NULL;
PFNGLSHARPENTEXFUNCSGISPROC glad_glSharpenTexFuncSGIS = NULL;
PFNGLGETSHARPENTEXFUNCSGISPROC glad_glGetSharpenTexFuncSGIS = NULL;
PFNGLTEXIMAGE4DSGISPROC glad_glTexImage4DSGIS = NULL;
PFNGLTEXSUBIMAGE4DSGISPROC glad_glTexSubImage4DSGIS = NULL;
PFNGLTEXTURECOLORMASKSGISPROC glad_glTextureColorMaskSGIS = NULL;
PFNGLGETTEXFILTERFUNCSGISPROC glad_glGetTexFilterFuncSGIS = NULL;
PFNGLTEXFILTERFUNCSGISPROC glad_glTexFilterFuncSGIS = NULL;
PFNGLASYNCMARKERSGIXPROC glad_glAsyncMarkerSGIX = NULL;
PFNGLFINISHASYNCSGIXPROC glad_glFinishAsyncSGIX = NULL;
PFNGLPOLLASYNCSGIXPROC glad_glPollAsyncSGIX = NULL;
PFNGLGENASYNCMARKERSSGIXPROC glad_glGenAsyncMarkersSGIX = NULL;
PFNGLDELETEASYNCMARKERSSGIXPROC glad_glDeleteAsyncMarkersSGIX = NULL;
PFNGLISASYNCMARKERSGIXPROC glad_glIsAsyncMarkerSGIX = NULL;
PFNGLFLUSHRASTERSGIXPROC glad_glFlushRasterSGIX = NULL;
PFNGLFRAGMENTCOLORMATERIALSGIXPROC glad_glFragmentColorMaterialSGIX = NULL;
PFNGLFRAGMENTLIGHTFSGIXPROC glad_glFragmentLightfSGIX = NULL;
PFNGLFRAGMENTLIGHTFVSGIXPROC glad_glFragmentLightfvSGIX = NULL;
PFNGLFRAGMENTLIGHTISGIXPROC glad_glFragmentLightiSGIX = NULL;
PFNGLFRAGMENTLIGHTIVSGIXPROC glad_glFragmentLightivSGIX = NULL;
PFNGLFRAGMENTLIGHTMODELFSGIXPROC glad_glFragmentLightModelfSGIX = NULL;
PFNGLFRAGMENTLIGHTMODELFVSGIXPROC glad_glFragmentLightModelfvSGIX = NULL;
PFNGLFRAGMENTLIGHTMODELISGIXPROC glad_glFragmentLightModeliSGIX = NULL;
PFNGLFRAGMENTLIGHTMODELIVSGIXPROC glad_glFragmentLightModelivSGIX = NULL;
PFNGLFRAGMENTMATERIALFSGIXPROC glad_glFragmentMaterialfSGIX = NULL;
PFNGLFRAGMENTMATERIALFVSGIXPROC glad_glFragmentMaterialfvSGIX = NULL;
PFNGLFRAGMENTMATERIALISGIXPROC glad_glFragmentMaterialiSGIX = NULL;
PFNGLFRAGMENTMATERIALIVSGIXPROC glad_glFragmentMaterialivSGIX = NULL;
PFNGLGETFRAGMENTLIGHTFVSGIXPROC glad_glGetFragmentLightfvSGIX = NULL;
PFNGLGETFRAGMENTLIGHTIVSGIXPROC glad_glGetFragmentLightivSGIX = NULL;
PFNGLGETFRAGMENTMATERIALFVSGIXPROC glad_glGetFragmentMaterialfvSGIX = NULL;
PFNGLGETFRAGMENTMATERIALIVSGIXPROC glad_glGetFragmentMaterialivSGIX = NULL;
PFNGLLIGHTENVISGIXPROC glad_glLightEnviSGIX = NULL;
PFNGLFRAMEZOOMSGIXPROC glad_glFrameZoomSGIX = NULL;
PFNGLIGLOOINTERFACESGIXPROC glad_glIglooInterfaceSGIX = NULL;
PFNGLGETINSTRUMENTSSGIXPROC glad_glGetInstrumentsSGIX = NULL;
PFNGLINSTRUMENTSBUFFERSGIXPROC glad_glInstrumentsBufferSGIX = NULL;
PFNGLPOLLINSTRUMENTSSGIXPROC glad_glPollInstrumentsSGIX = NULL;
PFNGLREADINSTRUMENTSSGIXPROC glad_glReadInstrumentsSGIX = NULL;
PFNGLSTARTINSTRUMENTSSGIXPROC glad_glStartInstrumentsSGIX = NULL;
PFNGLSTOPINSTRUMENTSSGIXPROC glad_glStopInstrumentsSGIX = NULL;
PFNGLGETLISTPARAMETERFVSGIXPROC glad_glGetListParameterfvSGIX = NULL;
PFNGLGETLISTPARAMETERIVSGIXPROC glad_glGetListParameterivSGIX = NULL;
PFNGLLISTPARAMETERFSGIXPROC glad_glListParameterfSGIX = NULL;
PFNGLLISTPARAMETERFVSGIXPROC glad_glListParameterfvSGIX = NULL;
PFNGLLISTPARAMETERISGIXPROC glad_glListParameteriSGIX = NULL;
PFNGLLISTPARAMETERIVSGIXPROC glad_glListParameterivSGIX = NULL;
PFNGLPIXELTEXGENSGIXPROC glad_glPixelTexGenSGIX = NULL;
PFNGLDEFORMATIONMAP3DSGIXPROC glad_glDeformationMap3dSGIX = NULL;
PFNGLDEFORMATIONMAP3FSGIXPROC glad_glDeformationMap3fSGIX = NULL;
PFNGLDEFORMSGIXPROC glad_glDeformSGIX = NULL;
PFNGLLOADIDENTITYDEFORMATIONMAPSGIXPROC glad_glLoadIdentityDeformationMapSGIX = NULL;
PFNGLREFERENCEPLANESGIXPROC glad_glReferencePlaneSGIX = NULL;
PFNGLSPRITEPARAMETERFSGIXPROC glad_glSpriteParameterfSGIX = NULL;
PFNGLSPRITEPARAMETERFVSGIXPROC glad_glSpriteParameterfvSGIX = NULL;
PFNGLSPRITEPARAMETERISGIXPROC glad_glSpriteParameteriSGIX = NULL;
PFNGLSPRITEPARAMETERIVSGIXPROC glad_glSpriteParameterivSGIX = NULL;
PFNGLTAGSAMPLEBUFFERSGIXPROC glad_glTagSampleBufferSGIX = NULL;
PFNGLCOLORTABLESGIPROC glad_glColorTableSGI = NULL;
PFNGLCOLORTABLEPARAMETERFVSGIPROC glad_glColorTableParameterfvSGI = NULL;
PFNGLCOLORTABLEPARAMETERIVSGIPROC glad_glColorTableParameterivSGI = NULL;
PFNGLCOPYCOLORTABLESGIPROC glad_glCopyColorTableSGI = NULL;
PFNGLGETCOLORTABLESGIPROC glad_glGetColorTableSGI = NULL;
PFNGLGETCOLORTABLEPARAMETERFVSGIPROC glad_glGetColorTableParameterfvSGI = NULL;
PFNGLGETCOLORTABLEPARAMETERIVSGIPROC glad_glGetColorTableParameterivSGI = NULL;
PFNGLFINISHTEXTURESUNXPROC glad_glFinishTextureSUNX = NULL;
PFNGLGLOBALALPHAFACTORBSUNPROC glad_glGlobalAlphaFactorbSUN = NULL;
PFNGLGLOBALALPHAFACTORSSUNPROC glad_glGlobalAlphaFactorsSUN = NULL;
PFNGLGLOBALALPHAFACTORISUNPROC glad_glGlobalAlphaFactoriSUN = NULL;
PFNGLGLOBALALPHAFACTORFSUNPROC glad_glGlobalAlphaFactorfSUN = NULL;
PFNGLGLOBALALPHAFACTORDSUNPROC glad_glGlobalAlphaFactordSUN = NULL;
PFNGLGLOBALALPHAFACTORUBSUNPROC glad_glGlobalAlphaFactorubSUN = NULL;
PFNGLGLOBALALPHAFACTORUSSUNPROC glad_glGlobalAlphaFactorusSUN = NULL;
PFNGLGLOBALALPHAFACTORUISUNPROC glad_glGlobalAlphaFactoruiSUN = NULL;
PFNGLDRAWMESHARRAYSSUNPROC glad_glDrawMeshArraysSUN = NULL;
PFNGLREPLACEMENTCODEUISUNPROC glad_glReplacementCodeuiSUN = NULL;
PFNGLREPLACEMENTCODEUSSUNPROC glad_glReplacementCodeusSUN = NULL;
PFNGLREPLACEMENTCODEUBSUNPROC glad_glReplacementCodeubSUN = NULL;
PFNGLREPLACEMENTCODEUIVSUNPROC glad_glReplacementCodeuivSUN = NULL;
PFNGLREPLACEMENTCODEUSVSUNPROC glad_glReplacementCodeusvSUN = NULL;
PFNGLREPLACEMENTCODEUBVSUNPROC glad_glReplacementCodeubvSUN = NULL;
PFNGLREPLACEMENTCODEPOINTERSUNPROC glad_glReplacementCodePointerSUN = NULL;
PFNGLCOLOR4UBVERTEX2FSUNPROC glad_glColor4ubVertex2fSUN = NULL;
PFNGLCOLOR4UBVERTEX2FVSUNPROC glad_glColor4ubVertex2fvSUN = NULL;
PFNGLCOLOR4UBVERTEX3FSUNPROC glad_glColor4ubVertex3fSUN = NULL;
PFNGLCOLOR4UBVERTEX3FVSUNPROC glad_glColor4ubVertex3fvSUN = NULL;
PFNGLCOLOR3FVERTEX3FSUNPROC glad_glColor3fVertex3fSUN = NULL;
PFNGLCOLOR3FVERTEX3FVSUNPROC glad_glColor3fVertex3fvSUN = NULL;
PFNGLNORMAL3FVERTEX3FSUNPROC glad_glNormal3fVertex3fSUN = NULL;
PFNGLNORMAL3FVERTEX3FVSUNPROC glad_glNormal3fVertex3fvSUN = NULL;
PFNGLCOLOR4FNORMAL3FVERTEX3FSUNPROC glad_glColor4fNormal3fVertex3fSUN = NULL;
PFNGLCOLOR4FNORMAL3FVERTEX3FVSUNPROC glad_glColor4fNormal3fVertex3fvSUN = NULL;
PFNGLTEXCOORD2FVERTEX3FSUNPROC glad_glTexCoord2fVertex3fSUN = NULL;
PFNGLTEXCOORD2FVERTEX3FVSUNPROC glad_glTexCoord2fVertex3fvSUN = NULL;
PFNGLTEXCOORD4FVERTEX4FSUNPROC glad_glTexCoord4fVertex4fSUN = NULL;
PFNGLTEXCOORD4FVERTEX4FVSUNPROC glad_glTexCoord4fVertex4fvSUN = NULL;
PFNGLTEXCOORD2FCOLOR4UBVERTEX3FSUNPROC glad_glTexCoord2fColor4ubVertex3fSUN = NULL;
PFNGLTEXCOORD2FCOLOR4UBVERTEX3FVSUNPROC glad_glTexCoord2fColor4ubVertex3fvSUN = NULL;
PFNGLTEXCOORD2FCOLOR3FVERTEX3FSUNPROC glad_glTexCoord2fColor3fVertex3fSUN = NULL;
PFNGLTEXCOORD2FCOLOR3FVERTEX3FVSUNPROC glad_glTexCoord2fColor3fVertex3fvSUN = NULL;
PFNGLTEXCOORD2FNORMAL3FVERTEX3FSUNPROC glad_glTexCoord2fNormal3fVertex3fSUN = NULL;
PFNGLTEXCOORD2FNORMAL3FVERTEX3FVSUNPROC glad_glTexCoord2fNormal3fVertex3fvSUN = NULL;
PFNGLTEXCOORD2FCOLOR4FNORMAL3FVERTEX3FSUNPROC glad_glTexCoord2fColor4fNormal3fVertex3fSUN = NULL;
PFNGLTEXCOORD2FCOLOR4FNORMAL3FVERTEX3FVSUNPROC glad_glTexCoord2fColor4fNormal3fVertex3fvSUN = NULL;
PFNGLTEXCOORD4FCOLOR4FNORMAL3FVERTEX4FSUNPROC glad_glTexCoord4fColor4fNormal3fVertex4fSUN = NULL;
PFNGLTEXCOORD4FCOLOR4FNORMAL3FVERTEX4FVSUNPROC glad_glTexCoord4fColor4fNormal3fVertex4fvSUN = NULL;
PFNGLREPLACEMENTCODEUIVERTEX3FSUNPROC glad_glReplacementCodeuiVertex3fSUN = NULL;
PFNGLREPLACEMENTCODEUIVERTEX3FVSUNPROC glad_glReplacementCodeuiVertex3fvSUN = NULL;
PFNGLREPLACEMENTCODEUICOLOR4UBVERTEX3FSUNPROC glad_glReplacementCodeuiColor4ubVertex3fSUN = NULL;
PFNGLREPLACEMENTCODEUICOLOR4UBVERTEX3FVSUNPROC glad_glReplacementCodeuiColor4ubVertex3fvSUN = NULL;
PFNGLREPLACEMENTCODEUICOLOR3FVERTEX3FSUNPROC glad_glReplacementCodeuiColor3fVertex3fSUN = NULL;
PFNGLREPLACEMENTCODEUICOLOR3FVERTEX3FVSUNPROC glad_glReplacementCodeuiColor3fVertex3fvSUN = NULL;
PFNGLREPLACEMENTCODEUINORMAL3FVERTEX3FSUNPROC glad_glReplacementCodeuiNormal3fVertex3fSUN = NULL;
PFNGLREPLACEMENTCODEUINORMAL3FVERTEX3FVSUNPROC glad_glReplacementCodeuiNormal3fVertex3fvSUN = NULL;
PFNGLREPLACEMENTCODEUICOLOR4FNORMAL3FVERTEX3FSUNPROC glad_glReplacementCodeuiColor4fNormal3fVertex3fSUN = NULL;
PFNGLREPLACEMENTCODEUICOLOR4FNORMAL3FVERTEX3FVSUNPROC glad_glReplacementCodeuiColor4fNormal3fVertex3fvSUN = NULL;
PFNGLREPLACEMENTCODEUITEXCOORD2FVERTEX3FSUNPROC glad_glReplacementCodeuiTexCoord2fVertex3fSUN = NULL;
PFNGLREPLACEMENTCODEUITEXCOORD2FVERTEX3FVSUNPROC glad_glReplacementCodeuiTexCoord2fVertex3fvSUN = NULL;
PFNGLREPLACEMENTCODEUITEXCOORD2FNORMAL3FVERTEX3FSUNPROC glad_glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN = NULL;
PFNGLREPLACEMENTCODEUITEXCOORD2FNORMAL3FVERTEX3FVSUNPROC glad_glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN = NULL;
PFNGLREPLACEMENTCODEUITEXCOORD2FCOLOR4FNORMAL3FVERTEX3FSUNPROC glad_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN = NULL;
PFNGLREPLACEMENTCODEUITEXCOORD2FCOLOR4FNORMAL3FVERTEX3FVSUNPROC glad_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN = NULL;
static void load_GL_VERSION_1_0(GLADloadproc load) {
	if(!GLAD_GL_VERSION_1_0) return;
	glad_glCullFace = (PFNGLCULLFACEPROC)load("glCullFace");
	glad_glFrontFace = (PFNGLFRONTFACEPROC)load("glFrontFace");
	glad_glHint = (PFNGLHINTPROC)load("glHint");
	glad_glLineWidth = (PFNGLLINEWIDTHPROC)load("glLineWidth");
	glad_glPointSize = (PFNGLPOINTSIZEPROC)load("glPointSize");
	glad_glPolygonMode = (PFNGLPOLYGONMODEPROC)load("glPolygonMode");
	glad_glScissor = (PFNGLSCISSORPROC)load("glScissor");
	glad_glTexParameterf = (PFNGLTEXPARAMETERFPROC)load("glTexParameterf");
	glad_glTexParameterfv = (PFNGLTEXPARAMETERFVPROC)load("glTexParameterfv");
	glad_glTexParameteri = (PFNGLTEXPARAMETERIPROC)load("glTexParameteri");
	glad_glTexParameteriv = (PFNGLTEXPARAMETERIVPROC)load("glTexParameteriv");
	glad_glTexImage1D = (PFNGLTEXIMAGE1DPROC)load("glTexImage1D");
	glad_glTexImage2D = (PFNGLTEXIMAGE2DPROC)load("glTexImage2D");
	glad_glDrawBuffer = (PFNGLDRAWBUFFERPROC)load("glDrawBuffer");
	glad_glClear = (PFNGLCLEARPROC)load("glClear");
	glad_glClearColor = (PFNGLCLEARCOLORPROC)load("glClearColor");
	glad_glClearStencil = (PFNGLCLEARSTENCILPROC)load("glClearStencil");
	glad_glClearDepth = (PFNGLCLEARDEPTHPROC)load("glClearDepth");
	glad_glStencilMask = (PFNGLSTENCILMASKPROC)load("glStencilMask");
	glad_glColorMask = (PFNGLCOLORMASKPROC)load("glColorMask");
	glad_glDepthMask = (PFNGLDEPTHMASKPROC)load("glDepthMask");
	glad_glDisable = (PFNGLDISABLEPROC)load("glDisable");
	glad_glEnable = (PFNGLENABLEPROC)load("glEnable");
	glad_glFinish = (PFNGLFINISHPROC)load("glFinish");
	glad_glFlush = (PFNGLFLUSHPROC)load("glFlush");
	glad_glBlendFunc = (PFNGLBLENDFUNCPROC)load("glBlendFunc");
	glad_glLogicOp = (PFNGLLOGICOPPROC)load("glLogicOp");
	glad_glStencilFunc = (PFNGLSTENCILFUNCPROC)load("glStencilFunc");
	glad_glStencilOp = (PFNGLSTENCILOPPROC)load("glStencilOp");
	glad_glDepthFunc = (PFNGLDEPTHFUNCPROC)load("glDepthFunc");
	glad_glPixelStoref = (PFNGLPIXELSTOREFPROC)load("glPixelStoref");
	glad_glPixelStorei = (PFNGLPIXELSTOREIPROC)load("glPixelStorei");
	glad_glReadBuffer = (PFNGLREADBUFFERPROC)load("glReadBuffer");
	glad_glReadPixels = (PFNGLREADPIXELSPROC)load("glReadPixels");
	glad_glGetBooleanv = (PFNGLGETBOOLEANVPROC)load("glGetBooleanv");
	glad_glGetDoublev = (PFNGLGETDOUBLEVPROC)load("glGetDoublev");
	glad_glGetError = (PFNGLGETERRORPROC)load("glGetError");
	glad_glGetFloatv = (PFNGLGETFLOATVPROC)load("glGetFloatv");
	glad_glGetIntegerv = (PFNGLGETINTEGERVPROC)load("glGetIntegerv");
	glad_glGetString = (PFNGLGETSTRINGPROC)load("glGetString");
	glad_glGetTexImage = (PFNGLGETTEXIMAGEPROC)load("glGetTexImage");
	glad_glGetTexParameterfv = (PFNGLGETTEXPARAMETERFVPROC)load("glGetTexParameterfv");
	glad_glGetTexParameteriv = (PFNGLGETTEXPARAMETERIVPROC)load("glGetTexParameteriv");
	glad_glGetTexLevelParameterfv = (PFNGLGETTEXLEVELPARAMETERFVPROC)load("glGetTexLevelParameterfv");
	glad_glGetTexLevelParameteriv = (PFNGLGETTEXLEVELPARAMETERIVPROC)load("glGetTexLevelParameteriv");
	glad_glIsEnabled = (PFNGLISENABLEDPROC)load("glIsEnabled");
	glad_glDepthRange = (PFNGLDEPTHRANGEPROC)load("glDepthRange");
	glad_glViewport = (PFNGLVIEWPORTPROC)load("glViewport");
}
static void load_GL_VERSION_1_1(GLADloadproc load) {
	if(!GLAD_GL_VERSION_1_1) return;
	glad_glDrawArrays = (PFNGLDRAWARRAYSPROC)load("glDrawArrays");
	glad_glDrawElements = (PFNGLDRAWELEMENTSPROC)load("glDrawElements");
	glad_glPolygonOffset = (PFNGLPOLYGONOFFSETPROC)load("glPolygonOffset");
	glad_glCopyTexImage1D = (PFNGLCOPYTEXIMAGE1DPROC)load("glCopyTexImage1D");
	glad_glCopyTexImage2D = (PFNGLCOPYTEXIMAGE2DPROC)load("glCopyTexImage2D");
	glad_glCopyTexSubImage1D = (PFNGLCOPYTEXSUBIMAGE1DPROC)load("glCopyTexSubImage1D");
	glad_glCopyTexSubImage2D = (PFNGLCOPYTEXSUBIMAGE2DPROC)load("glCopyTexSubImage2D");
	glad_glTexSubImage1D = (PFNGLTEXSUBIMAGE1DPROC)load("glTexSubImage1D");
	glad_glTexSubImage2D = (PFNGLTEXSUBIMAGE2DPROC)load("glTexSubImage2D");
	glad_glBindTexture = (PFNGLBINDTEXTUREPROC)load("glBindTexture");
	glad_glDeleteTextures = (PFNGLDELETETEXTURESPROC)load("glDeleteTextures");
	glad_glGenTextures = (PFNGLGENTEXTURESPROC)load("glGenTextures");
	glad_glIsTexture = (PFNGLISTEXTUREPROC)load("glIsTexture");
}
static void load_GL_VERSION_1_2(GLADloadproc load) {
	if(!GLAD_GL_VERSION_1_2) return;
	glad_glDrawRangeElements = (PFNGLDRAWRANGEELEMENTSPROC)load("glDrawRangeElements");
	glad_glTexImage3D = (PFNGLTEXIMAGE3DPROC)load("glTexImage3D");
	glad_glTexSubImage3D = (PFNGLTEXSUBIMAGE3DPROC)load("glTexSubImage3D");
	glad_glCopyTexSubImage3D = (PFNGLCOPYTEXSUBIMAGE3DPROC)load("glCopyTexSubImage3D");
}
static void load_GL_VERSION_1_3(GLADloadproc load) {
	if(!GLAD_GL_VERSION_1_3) return;
	glad_glActiveTexture = (PFNGLACTIVETEXTUREPROC)load("glActiveTexture");
	glad_glSampleCoverage = (PFNGLSAMPLECOVERAGEPROC)load("glSampleCoverage");
	glad_glCompressedTexImage3D = (PFNGLCOMPRESSEDTEXIMAGE3DPROC)load("glCompressedTexImage3D");
	glad_glCompressedTexImage2D = (PFNGLCOMPRESSEDTEXIMAGE2DPROC)load("glCompressedTexImage2D");
	glad_glCompressedTexImage1D = (PFNGLCOMPRESSEDTEXIMAGE1DPROC)load("glCompressedTexImage1D");
	glad_glCompressedTexSubImage3D = (PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC)load("glCompressedTexSubImage3D");
	glad_glCompressedTexSubImage2D = (PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC)load("glCompressedTexSubImage2D");
	glad_glCompressedTexSubImage1D = (PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC)load("glCompressedTexSubImage1D");
	glad_glGetCompressedTexImage = (PFNGLGETCOMPRESSEDTEXIMAGEPROC)load("glGetCompressedTexImage");
}
static void load_GL_VERSION_1_4(GLADloadproc load) {
	if(!GLAD_GL_VERSION_1_4) return;
	glad_glBlendFuncSeparate = (PFNGLBLENDFUNCSEPARATEPROC)load("glBlendFuncSeparate");
	glad_glMultiDrawArrays = (PFNGLMULTIDRAWARRAYSPROC)load("glMultiDrawArrays");
	glad_glMultiDrawElements = (PFNGLMULTIDRAWELEMENTSPROC)load("glMultiDrawElements");
	glad_glPointParameterf = (PFNGLPOINTPARAMETERFPROC)load("glPointParameterf");
	glad_glPointParameterfv = (PFNGLPOINTPARAMETERFVPROC)load("glPointParameterfv");
	glad_glPointParameteri = (PFNGLPOINTPARAMETERIPROC)load("glPointParameteri");
	glad_glPointParameteriv = (PFNGLPOINTPARAMETERIVPROC)load("glPointParameteriv");
	glad_glBlendColor = (PFNGLBLENDCOLORPROC)load("glBlendColor");
	glad_glBlendEquation = (PFNGLBLENDEQUATIONPROC)load("glBlendEquation");
}
static void load_GL_VERSION_1_5(GLADloadproc load) {
	if(!GLAD_GL_VERSION_1_5) return;
	glad_glGenQueries = (PFNGLGENQUERIESPROC)load("glGenQueries");
	glad_glDeleteQueries = (PFNGLDELETEQUERIESPROC)load("glDeleteQueries");
	glad_glIsQuery = (PFNGLISQUERYPROC)load("glIsQuery");
	glad_glBeginQuery = (PFNGLBEGINQUERYPROC)load("glBeginQuery");
	glad_glEndQuery = (PFNGLENDQUERYPROC)load("glEndQuery");
	glad_glGetQueryiv = (PFNGLGETQUERYIVPROC)load("glGetQueryiv");
	glad_glGetQueryObjectiv = (PFNGLGETQUERYOBJECTIVPROC)load("glGetQueryObjectiv");
	glad_glGetQueryObjectuiv = (PFNGLGETQUERYOBJECTUIVPROC)load("glGetQueryObjectuiv");
	glad_glBindBuffer = (PFNGLBINDBUFFERPROC)load("glBindBuffer");
	glad_glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)load("glDeleteBuffers");
	glad_glGenBuffers = (PFNGLGENBUFFERSPROC)load("glGenBuffers");
	glad_glIsBuffer = (PFNGLISBUFFERPROC)load("glIsBuffer");
	glad_glBufferData = (PFNGLBUFFERDATAPROC)load("glBufferData");
	glad_glBufferSubData = (PFNGLBUFFERSUBDATAPROC)load("glBufferSubData");
	glad_glGetBufferSubData = (PFNGLGETBUFFERSUBDATAPROC)load("glGetBufferSubData");
	glad_glMapBuffer = (PFNGLMAPBUFFERPROC)load("glMapBuffer");
	glad_glUnmapBuffer = (PFNGLUNMAPBUFFERPROC)load("glUnmapBuffer");
	glad_glGetBufferParameteriv = (PFNGLGETBUFFERPARAMETERIVPROC)load("glGetBufferParameteriv");
	glad_glGetBufferPointerv = (PFNGLGETBUFFERPOINTERVPROC)load("glGetBufferPointerv");
}
static void load_GL_VERSION_2_0(GLADloadproc load) {
	if(!GLAD_GL_VERSION_2_0) return;
	glad_glBlendEquationSeparate = (PFNGLBLENDEQUATIONSEPARATEPROC)load("glBlendEquationSeparate");
	glad_glDrawBuffers = (PFNGLDRAWBUFFERSPROC)load("glDrawBuffers");
	glad_glStencilOpSeparate = (PFNGLSTENCILOPSEPARATEPROC)load("glStencilOpSeparate");
	glad_glStencilFuncSeparate = (PFNGLSTENCILFUNCSEPARATEPROC)load("glStencilFuncSeparate");
	glad_glStencilMaskSeparate = (PFNGLSTENCILMASKSEPARATEPROC)load("glStencilMaskSeparate");
	glad_glAttachShader = (PFNGLATTACHSHADERPROC)load("glAttachShader");
	glad_glBindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC)load("glBindAttribLocation");
	glad_glCompileShader = (PFNGLCOMPILESHADERPROC)load("glCompileShader");
	glad_glCreateProgram = (PFNGLCREATEPROGRAMPROC)load("glCreateProgram");
	glad_glCreateShader = (PFNGLCREATESHADERPROC)load("glCreateShader");
	glad_glDeleteProgram = (PFNGLDELETEPROGRAMPROC)load("glDeleteProgram");
	glad_glDeleteShader = (PFNGLDELETESHADERPROC)load("glDeleteShader");
	glad_glDetachShader = (PFNGLDETACHSHADERPROC)load("glDetachShader");
	glad_glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)load("glDisableVertexAttribArray");
	glad_glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)load("glEnableVertexAttribArray");
	glad_glGetActiveAttrib = (PFNGLGETACTIVEATTRIBPROC)load("glGetActiveAttrib");
	glad_glGetActiveUniform = (PFNGLGETACTIVEUNIFORMPROC)load("glGetActiveUniform");
	glad_glGetAttachedShaders = (PFNGLGETATTACHEDSHADERSPROC)load("glGetAttachedShaders");
	glad_glGetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC)load("glGetAttribLocation");
	glad_glGetProgramiv = (PFNGLGETPROGRAMIVPROC)load("glGetProgramiv");
	glad_glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)load("glGetProgramInfoLog");
	glad_glGetShaderiv = (PFNGLGETSHADERIVPROC)load("glGetShaderiv");
	glad_glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)load("glGetShaderInfoLog");
	glad_glGetShaderSource = (PFNGLGETSHADERSOURCEPROC)load("glGetShaderSource");
	glad_glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)load("glGetUniformLocation");
	glad_glGetUniformfv = (PFNGLGETUNIFORMFVPROC)load("glGetUniformfv");
	glad_glGetUniformiv = (PFNGLGETUNIFORMIVPROC)load("glGetUniformiv");
	glad_glGetVertexAttribdv = (PFNGLGETVERTEXATTRIBDVPROC)load("glGetVertexAttribdv");
	glad_glGetVertexAttribfv = (PFNGLGETVERTEXATTRIBFVPROC)load("glGetVertexAttribfv");
	glad_glGetVertexAttribiv = (PFNGLGETVERTEXATTRIBIVPROC)load("glGetVertexAttribiv");
	glad_glGetVertexAttribPointerv = (PFNGLGETVERTEXATTRIBPOINTERVPROC)load("glGetVertexAttribPointerv");
	glad_glIsProgram = (PFNGLISPROGRAMPROC)load("glIsProgram");
	glad_glIsShader = (PFNGLISSHADERPROC)load("glIsShader");
	glad_glLinkProgram = (PFNGLLINKPROGRAMPROC)load("glLinkProgram");
	glad_glShaderSource = (PFNGLSHADERSOURCEPROC)load("glShaderSource");
	glad_glUseProgram = (PFNGLUSEPROGRAMPROC)load("glUseProgram");
	glad_glUniform1f = (PFNGLUNIFORM1FPROC)load("glUniform1f");
	glad_glUniform2f = (PFNGLUNIFORM2FPROC)load("glUniform2f");
	glad_glUniform3f = (PFNGLUNIFORM3FPROC)load("glUniform3f");
	glad_glUniform4f = (PFNGLUNIFORM4FPROC)load("glUniform4f");
	glad_glUniform1i = (PFNGLUNIFORM1IPROC)load("glUniform1i");
	glad_glUniform2i = (PFNGLUNIFORM2IPROC)load("glUniform2i");
	glad_glUniform3i = (PFNGLUNIFORM3IPROC)load("glUniform3i");
	glad_glUniform4i = (PFNGLUNIFORM4IPROC)load("glUniform4i");
	glad_glUniform1fv = (PFNGLUNIFORM1FVPROC)load("glUniform1fv");
	glad_glUniform2fv = (PFNGLUNIFORM2FVPROC)load("glUniform2fv");
	glad_glUniform3fv = (PFNGLUNIFORM3FVPROC)load("glUniform3fv");
	glad_glUniform4fv = (PFNGLUNIFORM4FVPROC)load("glUniform4fv");
	glad_glUniform1iv = (PFNGLUNIFORM1IVPROC)load("glUniform1iv");
	glad_glUniform2iv = (PFNGLUNIFORM2IVPROC)load("glUniform2iv");
	glad_glUniform3iv = (PFNGLUNIFORM3IVPROC)load("glUniform3iv");
	glad_glUniform4iv = (PFNGLUNIFORM4IVPROC)load("glUniform4iv");
	glad_glUniformMatrix2fv = (PFNGLUNIFORMMATRIX2FVPROC)load("glUniformMatrix2fv");
	glad_glUniformMatrix3fv = (PFNGLUNIFORMMATRIX3FVPROC)load("glUniformMatrix3fv");
	glad_glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)load("glUniformMatrix4fv");
	glad_glValidateProgram = (PFNGLVALIDATEPROGRAMPROC)load("glValidateProgram");
	glad_glVertexAttrib1d = (PFNGLVERTEXATTRIB1DPROC)load("glVertexAttrib1d");
	glad_glVertexAttrib1dv = (PFNGLVERTEXATTRIB1DVPROC)load("glVertexAttrib1dv");
	glad_glVertexAttrib1f = (PFNGLVERTEXATTRIB1FPROC)load("glVertexAttrib1f");
	glad_glVertexAttrib1fv = (PFNGLVERTEXATTRIB1FVPROC)load("glVertexAttrib1fv");
	glad_glVertexAttrib1s = (PFNGLVERTEXATTRIB1SPROC)load("glVertexAttrib1s");
	glad_glVertexAttrib1sv = (PFNGLVERTEXATTRIB1SVPROC)load("glVertexAttrib1sv");
	glad_glVertexAttrib2d = (PFNGLVERTEXATTRIB2DPROC)load("glVertexAttrib2d");
	glad_glVertexAttrib2dv = (PFNGLVERTEXATTRIB2DVPROC)load("glVertexAttrib2dv");
	glad_glVertexAttrib2f = (PFNGLVERTEXATTRIB2FPROC)load("glVertexAttrib2f");
	glad_glVertexAttrib2fv = (PFNGLVERTEXATTRIB2FVPROC)load("glVertexAttrib2fv");
	glad_glVertexAttrib2s = (PFNGLVERTEXATTRIB2SPROC)load("glVertexAttrib2s");
	glad_glVertexAttrib2sv = (PFNGLVERTEXATTRIB2SVPROC)load("glVertexAttrib2sv");
	glad_glVertexAttrib3d = (PFNGLVERTEXATTRIB3DPROC)load("glVertexAttrib3d");
	glad_glVertexAttrib3dv = (PFNGLVERTEXATTRIB3DVPROC)load("glVertexAttrib3dv");
	glad_glVertexAttrib3f = (PFNGLVERTEXATTRIB3FPROC)load("glVertexAttrib3f");
	glad_glVertexAttrib3fv = (PFNGLVERTEXATTRIB3FVPROC)load("glVertexAttrib3fv");
	glad_glVertexAttrib3s = (PFNGLVERTEXATTRIB3SPROC)load("glVertexAttrib3s");
	glad_glVertexAttrib3sv = (PFNGLVERTEXATTRIB3SVPROC)load("glVertexAttrib3sv");
	glad_glVertexAttrib4Nbv = (PFNGLVERTEXATTRIB4NBVPROC)load("glVertexAttrib4Nbv");
	glad_glVertexAttrib4Niv = (PFNGLVERTEXATTRIB4NIVPROC)load("glVertexAttrib4Niv");
	glad_glVertexAttrib4Nsv = (PFNGLVERTEXATTRIB4NSVPROC)load("glVertexAttrib4Nsv");
	glad_glVertexAttrib4Nub = (PFNGLVERTEXATTRIB4NUBPROC)load("glVertexAttrib4Nub");
	glad_glVertexAttrib4Nubv = (PFNGLVERTEXATTRIB4NUBVPROC)load("glVertexAttrib4Nubv");
	glad_glVertexAttrib4Nuiv = (PFNGLVERTEXATTRIB4NUIVPROC)load("glVertexAttrib4Nuiv");
	glad_glVertexAttrib4Nusv = (PFNGLVERTEXATTRIB4NUSVPROC)load("glVertexAttrib4Nusv");
	glad_glVertexAttrib4bv = (PFNGLVERTEXATTRIB4BVPROC)load("glVertexAttrib4bv");
	glad_glVertexAttrib4d = (PFNGLVERTEXATTRIB4DPROC)load("glVertexAttrib4d");
	glad_glVertexAttrib4dv = (PFNGLVERTEXATTRIB4DVPROC)load("glVertexAttrib4dv");
	glad_glVertexAttrib4f = (PFNGLVERTEXATTRIB4FPROC)load("glVertexAttrib4f");
	glad_glVertexAttrib4fv = (PFNGLVERTEXATTRIB4FVPROC)load("glVertexAttrib4fv");
	glad_glVertexAttrib4iv = (PFNGLVERTEXATTRIB4IVPROC)load("glVertexAttrib4iv");
	glad_glVertexAttrib4s = (PFNGLVERTEXATTRIB4SPROC)load("glVertexAttrib4s");
	glad_glVertexAttrib4sv = (PFNGLVERTEXATTRIB4SVPROC)load("glVertexAttrib4sv");
	glad_glVertexAttrib4ubv = (PFNGLVERTEXATTRIB4UBVPROC)load("glVertexAttrib4ubv");
	glad_glVertexAttrib4uiv = (PFNGLVERTEXATTRIB4UIVPROC)load("glVertexAttrib4uiv");
	glad_glVertexAttrib4usv = (PFNGLVERTEXATTRIB4USVPROC)load("glVertexAttrib4usv");
	glad_glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)load("glVertexAttribPointer");
}
static void load_GL_VERSION_2_1(GLADloadproc load) {
	if(!GLAD_GL_VERSION_2_1) return;
	glad_glUniformMatrix2x3fv = (PFNGLUNIFORMMATRIX2X3FVPROC)load("glUniformMatrix2x3fv");
	glad_glUniformMatrix3x2fv = (PFNGLUNIFORMMATRIX3X2FVPROC)load("glUniformMatrix3x2fv");
	glad_glUniformMatrix2x4fv = (PFNGLUNIFORMMATRIX2X4FVPROC)load("glUniformMatrix2x4fv");
	glad_glUniformMatrix4x2fv = (PFNGLUNIFORMMATRIX4X2FVPROC)load("glUniformMatrix4x2fv");
	glad_glUniformMatrix3x4fv = (PFNGLUNIFORMMATRIX3X4FVPROC)load("glUniformMatrix3x4fv");
	glad_glUniformMatrix4x3fv = (PFNGLUNIFORMMATRIX4X3FVPROC)load("glUniformMatrix4x3fv");
}
static void load_GL_VERSION_3_0(GLADloadproc load) {
	if(!GLAD_GL_VERSION_3_0) return;
	glad_glColorMaski = (PFNGLCOLORMASKIPROC)load("glColorMaski");
	glad_glGetBooleani_v = (PFNGLGETBOOLEANI_VPROC)load("glGetBooleani_v");
	glad_glGetIntegeri_v = (PFNGLGETINTEGERI_VPROC)load("glGetIntegeri_v");
	glad_glEnablei = (PFNGLENABLEIPROC)load("glEnablei");
	glad_glDisablei = (PFNGLDISABLEIPROC)load("glDisablei");
	glad_glIsEnabledi = (PFNGLISENABLEDIPROC)load("glIsEnabledi");
	glad_glBeginTransformFeedback = (PFNGLBEGINTRANSFORMFEEDBACKPROC)load("glBeginTransformFeedback");
	glad_glEndTransformFeedback = (PFNGLENDTRANSFORMFEEDBACKPROC)load("glEndTransformFeedback");
	glad_glBindBufferRange = (PFNGLBINDBUFFERRANGEPROC)load("glBindBufferRange");
	glad_glBindBufferBase = (PFNGLBINDBUFFERBASEPROC)load("glBindBufferBase");
	glad_glTransformFeedbackVaryings = (PFNGLTRANSFORMFEEDBACKVARYINGSPROC)load("glTransformFeedbackVaryings");
	glad_glGetTransformFeedbackVarying = (PFNGLGETTRANSFORMFEEDBACKVARYINGPROC)load("glGetTransformFeedbackVarying");
	glad_glClampColor = (PFNGLCLAMPCOLORPROC)load("glClampColor");
	glad_glBeginConditionalRender = (PFNGLBEGINCONDITIONALRENDERPROC)load("glBeginConditionalRender");
	glad_glEndConditionalRender = (PFNGLENDCONDITIONALRENDERPROC)load("glEndConditionalRender");
	glad_glVertexAttribIPointer = (PFNGLVERTEXATTRIBIPOINTERPROC)load("glVertexAttribIPointer");
	glad_glGetVertexAttribIiv = (PFNGLGETVERTEXATTRIBIIVPROC)load("glGetVertexAttribIiv");
	glad_glGetVertexAttribIuiv = (PFNGLGETVERTEXATTRIBIUIVPROC)load("glGetVertexAttribIuiv");
	glad_glVertexAttribI1i = (PFNGLVERTEXATTRIBI1IPROC)load("glVertexAttribI1i");
	glad_glVertexAttribI2i = (PFNGLVERTEXATTRIBI2IPROC)load("glVertexAttribI2i");
	glad_glVertexAttribI3i = (PFNGLVERTEXATTRIBI3IPROC)load("glVertexAttribI3i");
	glad_glVertexAttribI4i = (PFNGLVERTEXATTRIBI4IPROC)load("glVertexAttribI4i");
	glad_glVertexAttribI1ui = (PFNGLVERTEXATTRIBI1UIPROC)load("glVertexAttribI1ui");
	glad_glVertexAttribI2ui = (PFNGLVERTEXATTRIBI2UIPROC)load("glVertexAttribI2ui");
	glad_glVertexAttribI3ui = (PFNGLVERTEXATTRIBI3UIPROC)load("glVertexAttribI3ui");
	glad_glVertexAttribI4ui = (PFNGLVERTEXATTRIBI4UIPROC)load("glVertexAttribI4ui");
	glad_glVertexAttribI1iv = (PFNGLVERTEXATTRIBI1IVPROC)load("glVertexAttribI1iv");
	glad_glVertexAttribI2iv = (PFNGLVERTEXATTRIBI2IVPROC)load("glVertexAttribI2iv");
	glad_glVertexAttribI3iv = (PFNGLVERTEXATTRIBI3IVPROC)load("glVertexAttribI3iv");
	glad_glVertexAttribI4iv = (PFNGLVERTEXATTRIBI4IVPROC)load("glVertexAttribI4iv");
	glad_glVertexAttribI1uiv = (PFNGLVERTEXATTRIBI1UIVPROC)load("glVertexAttribI1uiv");
	glad_glVertexAttribI2uiv = (PFNGLVERTEXATTRIBI2UIVPROC)load("glVertexAttribI2uiv");
	glad_glVertexAttribI3uiv = (PFNGLVERTEXATTRIBI3UIVPROC)load("glVertexAttribI3uiv");
	glad_glVertexAttribI4uiv = (PFNGLVERTEXATTRIBI4UIVPROC)load("glVertexAttribI4uiv");
	glad_glVertexAttribI4bv = (PFNGLVERTEXATTRIBI4BVPROC)load("glVertexAttribI4bv");
	glad_glVertexAttribI4sv = (PFNGLVERTEXATTRIBI4SVPROC)load("glVertexAttribI4sv");
	glad_glVertexAttribI4ubv = (PFNGLVERTEXATTRIBI4UBVPROC)load("glVertexAttribI4ubv");
	glad_glVertexAttribI4usv = (PFNGLVERTEXATTRIBI4USVPROC)load("glVertexAttribI4usv");
	glad_glGetUniformuiv = (PFNGLGETUNIFORMUIVPROC)load("glGetUniformuiv");
	glad_glBindFragDataLocation = (PFNGLBINDFRAGDATALOCATIONPROC)load("glBindFragDataLocation");
	glad_glGetFragDataLocation = (PFNGLGETFRAGDATALOCATIONPROC)load("glGetFragDataLocation");
	glad_glUniform1ui = (PFNGLUNIFORM1UIPROC)load("glUniform1ui");
	glad_glUniform2ui = (PFNGLUNIFORM2UIPROC)load("glUniform2ui");
	glad_glUniform3ui = (PFNGLUNIFORM3UIPROC)load("glUniform3ui");
	glad_glUniform4ui = (PFNGLUNIFORM4UIPROC)load("glUniform4ui");
	glad_glUniform1uiv = (PFNGLUNIFORM1UIVPROC)load("glUniform1uiv");
	glad_glUniform2uiv = (PFNGLUNIFORM2UIVPROC)load("glUniform2uiv");
	glad_glUniform3uiv = (PFNGLUNIFORM3UIVPROC)load("glUniform3uiv");
	glad_glUniform4uiv = (PFNGLUNIFORM4UIVPROC)load("glUniform4uiv");
	glad_glTexParameterIiv = (PFNGLTEXPARAMETERIIVPROC)load("glTexParameterIiv");
	glad_glTexParameterIuiv = (PFNGLTEXPARAMETERIUIVPROC)load("glTexParameterIuiv");
	glad_glGetTexParameterIiv = (PFNGLGETTEXPARAMETERIIVPROC)load("glGetTexParameterIiv");
	glad_glGetTexParameterIuiv = (PFNGLGETTEXPARAMETERIUIVPROC)load("glGetTexParameterIuiv");
	glad_glClearBufferiv = (PFNGLCLEARBUFFERIVPROC)load("glClearBufferiv");
	glad_glClearBufferuiv = (PFNGLCLEARBUFFERUIVPROC)load("glClearBufferuiv");
	glad_glClearBufferfv = (PFNGLCLEARBUFFERFVPROC)load("glClearBufferfv");
	glad_glClearBufferfi = (PFNGLCLEARBUFFERFIPROC)load("glClearBufferfi");
	glad_glGetStringi = (PFNGLGETSTRINGIPROC)load("glGetStringi");
	glad_glIsRenderbuffer = (PFNGLISRENDERBUFFERPROC)load("glIsRenderbuffer");
	glad_glBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC)load("glBindRenderbuffer");
	glad_glDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC)load("glDeleteRenderbuffers");
	glad_glGenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC)load("glGenRenderbuffers");
	glad_glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC)load("glRenderbufferStorage");
	glad_glGetRenderbufferParameteriv = (PFNGLGETRENDERBUFFERPARAMETERIVPROC)load("glGetRenderbufferParameteriv");
	glad_glIsFramebuffer = (PFNGLISFRAMEBUFFERPROC)load("glIsFramebuffer");
	glad_glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)load("glBindFramebuffer");
	glad_glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)load("glDeleteFramebuffers");
	glad_glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)load("glGenFramebuffers");
	glad_glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)load("glCheckFramebufferStatus");
	glad_glFramebufferTexture1D = (PFNGLFRAMEBUFFERTEXTURE1DPROC)load("glFramebufferTexture1D");
	glad_glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)load("glFramebufferTexture2D");
	glad_glFramebufferTexture3D = (PFNGLFRAMEBUFFERTEXTURE3DPROC)load("glFramebufferTexture3D");
	glad_glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)load("glFramebufferRenderbuffer");
	glad_glGetFramebufferAttachmentParameteriv = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC)load("glGetFramebufferAttachmentParameteriv");
	glad_glGenerateMipmap = (PFNGLGENERATEMIPMAPPROC)load("glGenerateMipmap");
	glad_glBlitFramebuffer = (PFNGLBLITFRAMEBUFFERPROC)load("glBlitFramebuffer");
	glad_glRenderbufferStorageMultisample = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC)load("glRenderbufferStorageMultisample");
	glad_glFramebufferTextureLayer = (PFNGLFRAMEBUFFERTEXTURELAYERPROC)load("glFramebufferTextureLayer");
	glad_glMapBufferRange = (PFNGLMAPBUFFERRANGEPROC)load("glMapBufferRange");
	glad_glFlushMappedBufferRange = (PFNGLFLUSHMAPPEDBUFFERRANGEPROC)load("glFlushMappedBufferRange");
	glad_glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)load("glBindVertexArray");
	glad_glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)load("glDeleteVertexArrays");
	glad_glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)load("glGenVertexArrays");
	glad_glIsVertexArray = (PFNGLISVERTEXARRAYPROC)load("glIsVertexArray");
}
static void load_GL_VERSION_3_1(GLADloadproc load) {
	if(!GLAD_GL_VERSION_3_1) return;
	glad_glDrawArraysInstanced = (PFNGLDRAWARRAYSINSTANCEDPROC)load("glDrawArraysInstanced");
	glad_glDrawElementsInstanced = (PFNGLDRAWELEMENTSINSTANCEDPROC)load("glDrawElementsInstanced");
	glad_glTexBuffer = (PFNGLTEXBUFFERPROC)load("glTexBuffer");
	glad_glPrimitiveRestartIndex = (PFNGLPRIMITIVERESTARTINDEXPROC)load("glPrimitiveRestartIndex");
	glad_glCopyBufferSubData = (PFNGLCOPYBUFFERSUBDATAPROC)load("glCopyBufferSubData");
	glad_glGetUniformIndices = (PFNGLGETUNIFORMINDICESPROC)load("glGetUniformIndices");
	glad_glGetActiveUniformsiv = (PFNGLGETACTIVEUNIFORMSIVPROC)load("glGetActiveUniformsiv");
	glad_glGetActiveUniformName = (PFNGLGETACTIVEUNIFORMNAMEPROC)load("glGetActiveUniformName");
	glad_glGetUniformBlockIndex = (PFNGLGETUNIFORMBLOCKINDEXPROC)load("glGetUniformBlockIndex");
	glad_glGetActiveUniformBlockiv = (PFNGLGETACTIVEUNIFORMBLOCKIVPROC)load("glGetActiveUniformBlockiv");
	glad_glGetActiveUniformBlockName = (PFNGLGETACTIVEUNIFORMBLOCKNAMEPROC)load("glGetActiveUniformBlockName");
	glad_glUniformBlockBinding = (PFNGLUNIFORMBLOCKBINDINGPROC)load("glUniformBlockBinding");
	glad_glBindBufferRange = (PFNGLBINDBUFFERRANGEPROC)load("glBindBufferRange");
	glad_glBindBufferBase = (PFNGLBINDBUFFERBASEPROC)load("glBindBufferBase");
	glad_glGetIntegeri_v = (PFNGLGETINTEGERI_VPROC)load("glGetIntegeri_v");
}
static void load_GL_VERSION_3_2(GLADloadproc load) {
	if(!GLAD_GL_VERSION_3_2) return;
	glad_glDrawElementsBaseVertex = (PFNGLDRAWELEMENTSBASEVERTEXPROC)load("glDrawElementsBaseVertex");
	glad_glDrawRangeElementsBaseVertex = (PFNGLDRAWRANGEELEMENTSBASEVERTEXPROC)load("glDrawRangeElementsBaseVertex");
	glad_glDrawElementsInstancedBaseVertex = (PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC)load("glDrawElementsInstancedBaseVertex");
	glad_glMultiDrawElementsBaseVertex = (PFNGLMULTIDRAWELEMENTSBASEVERTEXPROC)load("glMultiDrawElementsBaseVertex");
	glad_glProvokingVertex = (PFNGLPROVOKINGVERTEXPROC)load("glProvokingVertex");
	glad_glFenceSync = (PFNGLFENCESYNCPROC)load("glFenceSync");
	glad_glIsSync = (PFNGLISSYNCPROC)load("glIsSync");
	glad_glDeleteSync = (PFNGLDELETESYNCPROC)load("glDeleteSync");
	glad_glClientWaitSync = (PFNGLCLIENTWAITSYNCPROC)load("glClientWaitSync");
	glad_glWaitSync = (PFNGLWAITSYNCPROC)load("glWaitSync");
	glad_glGetInteger64v = (PFNGLGETINTEGER64VPROC)load("glGetInteger64v");
	glad_glGetSynciv = (PFNGLGETSYNCIVPROC)load("glGetSynciv");
	glad_glGetInteger64i_v = (PFNGLGETINTEGER64I_VPROC)load("glGetInteger64i_v");
	glad_glGetBufferParameteri64v = (PFNGLGETBUFFERPARAMETERI64VPROC)load("glGetBufferParameteri64v");
	glad_glFramebufferTexture = (PFNGLFRAMEBUFFERTEXTUREPROC)load("glFramebufferTexture");
	glad_glTexImage2DMultisample = (PFNGLTEXIMAGE2DMULTISAMPLEPROC)load("glTexImage2DMultisample");
	glad_glTexImage3DMultisample = (PFNGLTEXIMAGE3DMULTISAMPLEPROC)load("glTexImage3DMultisample");
	glad_glGetMultisamplefv = (PFNGLGETMULTISAMPLEFVPROC)load("glGetMultisamplefv");
	glad_glSampleMaski = (PFNGLSAMPLEMASKIPROC)load("glSampleMaski");
}
static void load_GL_VERSION_3_3(GLADloadproc load) {
	if(!GLAD_GL_VERSION_3_3) return;
	glad_glBindFragDataLocationIndexed = (PFNGLBINDFRAGDATALOCATIONINDEXEDPROC)load("glBindFragDataLocationIndexed");
	glad_glGetFragDataIndex = (PFNGLGETFRAGDATAINDEXPROC)load("glGetFragDataIndex");
	glad_glGenSamplers = (PFNGLGENSAMPLERSPROC)load("glGenSamplers");
	glad_glDeleteSamplers = (PFNGLDELETESAMPLERSPROC)load("glDeleteSamplers");
	glad_glIsSampler = (PFNGLISSAMPLERPROC)load("glIsSampler");
	glad_glBindSampler = (PFNGLBINDSAMPLERPROC)load("glBindSampler");
	glad_glSamplerParameteri = (PFNGLSAMPLERPARAMETERIPROC)load("glSamplerParameteri");
	glad_glSamplerParameteriv = (PFNGLSAMPLERPARAMETERIVPROC)load("glSamplerParameteriv");
	glad_glSamplerParameterf = (PFNGLSAMPLERPARAMETERFPROC)load("glSamplerParameterf");
	glad_glSamplerParameterfv = (PFNGLSAMPLERPARAMETERFVPROC)load("glSamplerParameterfv");
	glad_glSamplerParameterIiv = (PFNGLSAMPLERPARAMETERIIVPROC)load("glSamplerParameterIiv");
	glad_glSamplerParameterIuiv = (PFNGLSAMPLERPARAMETERIUIVPROC)load("glSamplerParameterIuiv");
	glad_glGetSamplerParameteriv = (PFNGLGETSAMPLERPARAMETERIVPROC)load("glGetSamplerParameteriv");
	glad_glGetSamplerParameterIiv = (PFNGLGETSAMPLERPARAMETERIIVPROC)load("glGetSamplerParameterIiv");
	glad_glGetSamplerParameterfv = (PFNGLGETSAMPLERPARAMETERFVPROC)load("glGetSamplerParameterfv");
	glad_glGetSamplerParameterIuiv = (PFNGLGETSAMPLERPARAMETERIUIVPROC)load("glGetSamplerParameterIuiv");
	glad_glQueryCounter = (PFNGLQUERYCOUNTERPROC)load("glQueryCounter");
	glad_glGetQueryObjecti64v = (PFNGLGETQUERYOBJECTI64VPROC)load("glGetQueryObjecti64v");
	glad_glGetQueryObjectui64v = (PFNGLGETQUERYOBJECTUI64VPROC)load("glGetQueryObjectui64v");
	glad_glVertexAttribDivisor = (PFNGLVERTEXATTRIBDIVISORPROC)load("glVertexAttribDivisor");
	glad_glVertexAttribP1ui = (PFNGLVERTEXATTRIBP1UIPROC)load("glVertexAttribP1ui");
	glad_glVertexAttribP1uiv = (PFNGLVERTEXATTRIBP1UIVPROC)load("glVertexAttribP1uiv");
	glad_glVertexAttribP2ui = (PFNGLVERTEXATTRIBP2UIPROC)load("glVertexAttribP2ui");
	glad_glVertexAttribP2uiv = (PFNGLVERTEXATTRIBP2UIVPROC)load("glVertexAttribP2uiv");
	glad_glVertexAttribP3ui = (PFNGLVERTEXATTRIBP3UIPROC)load("glVertexAttribP3ui");
	glad_glVertexAttribP3uiv = (PFNGLVERTEXATTRIBP3UIVPROC)load("glVertexAttribP3uiv");
	glad_glVertexAttribP4ui = (PFNGLVERTEXATTRIBP4UIPROC)load("glVertexAttribP4ui");
	glad_glVertexAttribP4uiv = (PFNGLVERTEXATTRIBP4UIVPROC)load("glVertexAttribP4uiv");
	glad_glVertexP2ui = (PFNGLVERTEXP2UIPROC)load("glVertexP2ui");
	glad_glVertexP2uiv = (PFNGLVERTEXP2UIVPROC)load("glVertexP2uiv");
	glad_glVertexP3ui = (PFNGLVERTEXP3UIPROC)load("glVertexP3ui");
	glad_glVertexP3uiv = (PFNGLVERTEXP3UIVPROC)load("glVertexP3uiv");
	glad_glVertexP4ui = (PFNGLVERTEXP4UIPROC)load("glVertexP4ui");
	glad_glVertexP4uiv = (PFNGLVERTEXP4UIVPROC)load("glVertexP4uiv");
	glad_glTexCoordP1ui = (PFNGLTEXCOORDP1UIPROC)load("glTexCoordP1ui");
	glad_glTexCoordP1uiv = (PFNGLTEXCOORDP1UIVPROC)load("glTexCoordP1uiv");
	glad_glTexCoordP2ui = (PFNGLTEXCOORDP2UIPROC)load("glTexCoordP2ui");
	glad_glTexCoordP2uiv = (PFNGLTEXCOORDP2UIVPROC)load("glTexCoordP2uiv");
	glad_glTexCoordP3ui = (PFNGLTEXCOORDP3UIPROC)load("glTexCoordP3ui");
	glad_glTexCoordP3uiv = (PFNGLTEXCOORDP3UIVPROC)load("glTexCoordP3uiv");
	glad_glTexCoordP4ui = (PFNGLTEXCOORDP4UIPROC)load("glTexCoordP4ui");
	glad_glTexCoordP4uiv = (PFNGLTEXCOORDP4UIVPROC)load("glTexCoordP4uiv");
	glad_glMultiTexCoordP1ui = (PFNGLMULTITEXCOORDP1UIPROC)load("glMultiTexCoordP1ui");
	glad_glMultiTexCoordP1uiv = (PFNGLMULTITEXCOORDP1UIVPROC)load("glMultiTexCoordP1uiv");
	glad_glMultiTexCoordP2ui = (PFNGLMULTITEXCOORDP2UIPROC)load("glMultiTexCoordP2ui");
	glad_glMultiTexCoordP2uiv = (PFNGLMULTITEXCOORDP2UIVPROC)load("glMultiTexCoordP2uiv");
	glad_glMultiTexCoordP3ui = (PFNGLMULTITEXCOORDP3UIPROC)load("glMultiTexCoordP3ui");
	glad_glMultiTexCoordP3uiv = (PFNGLMULTITEXCOORDP3UIVPROC)load("glMultiTexCoordP3uiv");
	glad_glMultiTexCoordP4ui = (PFNGLMULTITEXCOORDP4UIPROC)load("glMultiTexCoordP4ui");
	glad_glMultiTexCoordP4uiv = (PFNGLMULTITEXCOORDP4UIVPROC)load("glMultiTexCoordP4uiv");
	glad_glNormalP3ui = (PFNGLNORMALP3UIPROC)load("glNormalP3ui");
	glad_glNormalP3uiv = (PFNGLNORMALP3UIVPROC)load("glNormalP3uiv");
	glad_glColorP3ui = (PFNGLCOLORP3UIPROC)load("glColorP3ui");
	glad_glColorP3uiv = (PFNGLCOLORP3UIVPROC)load("glColorP3uiv");
	glad_glColorP4ui = (PFNGLCOLORP4UIPROC)load("glColorP4ui");
	glad_glColorP4uiv = (PFNGLCOLORP4UIVPROC)load("glColorP4uiv");
	glad_glSecondaryColorP3ui = (PFNGLSECONDARYCOLORP3UIPROC)load("glSecondaryColorP3ui");
	glad_glSecondaryColorP3uiv = (PFNGLSECONDARYCOLORP3UIVPROC)load("glSecondaryColorP3uiv");
}
static void load_GL_3DFX_tbuffer(GLADloadproc load) {
	if(!GLAD_GL_3DFX_tbuffer) return;
	glad_glTbufferMask3DFX = (PFNGLTBUFFERMASK3DFXPROC)load("glTbufferMask3DFX");
}
static void load_GL_AMD_debug_output(GLADloadproc load) {
	if(!GLAD_GL_AMD_debug_output) return;
	glad_glDebugMessageEnableAMD = (PFNGLDEBUGMESSAGEENABLEAMDPROC)load("glDebugMessageEnableAMD");
	glad_glDebugMessageInsertAMD = (PFNGLDEBUGMESSAGEINSERTAMDPROC)load("glDebugMessageInsertAMD");
	glad_glDebugMessageCallbackAMD = (PFNGLDEBUGMESSAGECALLBACKAMDPROC)load("glDebugMessageCallbackAMD");
	glad_glGetDebugMessageLogAMD = (PFNGLGETDEBUGMESSAGELOGAMDPROC)load("glGetDebugMessageLogAMD");
}
static void load_GL_AMD_draw_buffers_blend(GLADloadproc load) {
	if(!GLAD_GL_AMD_draw_buffers_blend) return;
	glad_glBlendFuncIndexedAMD = (PFNGLBLENDFUNCINDEXEDAMDPROC)load("glBlendFuncIndexedAMD");
	glad_glBlendFuncSeparateIndexedAMD = (PFNGLBLENDFUNCSEPARATEINDEXEDAMDPROC)load("glBlendFuncSeparateIndexedAMD");
	glad_glBlendEquationIndexedAMD = (PFNGLBLENDEQUATIONINDEXEDAMDPROC)load("glBlendEquationIndexedAMD");
	glad_glBlendEquationSeparateIndexedAMD = (PFNGLBLENDEQUATIONSEPARATEINDEXEDAMDPROC)load("glBlendEquationSeparateIndexedAMD");
}
static void load_GL_AMD_framebuffer_multisample_advanced(GLADloadproc load) {
	if(!GLAD_GL_AMD_framebuffer_multisample_advanced) return;
	glad_glRenderbufferStorageMultisampleAdvancedAMD = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEADVANCEDAMDPROC)load("glRenderbufferStorageMultisampleAdvancedAMD");
	glad_glNamedRenderbufferStorageMultisampleAdvancedAMD = (PFNGLNAMEDRENDERBUFFERSTORAGEMULTISAMPLEADVANCEDAMDPROC)load("glNamedRenderbufferStorageMultisampleAdvancedAMD");
}
static void load_GL_AMD_framebuffer_sample_positions(GLADloadproc load) {
	if(!GLAD_GL_AMD_framebuffer_sample_positions) return;
	glad_glFramebufferSamplePositionsfvAMD = (PFNGLFRAMEBUFFERSAMPLEPOSITIONSFVAMDPROC)load("glFramebufferSamplePositionsfvAMD");
	glad_glNamedFramebufferSamplePositionsfvAMD = (PFNGLNAMEDFRAMEBUFFERSAMPLEPOSITIONSFVAMDPROC)load("glNamedFramebufferSamplePositionsfvAMD");
	glad_glGetFramebufferParameterfvAMD = (PFNGLGETFRAMEBUFFERPARAMETERFVAMDPROC)load("glGetFramebufferParameterfvAMD");
	glad_glGetNamedFramebufferParameterfvAMD = (PFNGLGETNAMEDFRAMEBUFFERPARAMETERFVAMDPROC)load("glGetNamedFramebufferParameterfvAMD");
}
static void load_GL_AMD_gpu_shader_int64(GLADloadproc load) {
	if(!GLAD_GL_AMD_gpu_shader_int64) return;
	glad_glUniform1i64NV = (PFNGLUNIFORM1I64NVPROC)load("glUniform1i64NV");
	glad_glUniform2i64NV = (PFNGLUNIFORM2I64NVPROC)load("glUniform2i64NV");
	glad_glUniform3i64NV = (PFNGLUNIFORM3I64NVPROC)load("glUniform3i64NV");
	glad_glUniform4i64NV = (PFNGLUNIFORM4I64NVPROC)load("glUniform4i64NV");
	glad_glUniform1i64vNV = (PFNGLUNIFORM1I64VNVPROC)load("glUniform1i64vNV");
	glad_glUniform2i64vNV = (PFNGLUNIFORM2I64VNVPROC)load("glUniform2i64vNV");
	glad_glUniform3i64vNV = (PFNGLUNIFORM3I64VNVPROC)load("glUniform3i64vNV");
	glad_glUniform4i64vNV = (PFNGLUNIFORM4I64VNVPROC)load("glUniform4i64vNV");
	glad_glUniform1ui64NV = (PFNGLUNIFORM1UI64NVPROC)load("glUniform1ui64NV");
	glad_glUniform2ui64NV = (PFNGLUNIFORM2UI64NVPROC)load("glUniform2ui64NV");
	glad_glUniform3ui64NV = (PFNGLUNIFORM3UI64NVPROC)load("glUniform3ui64NV");
	glad_glUniform4ui64NV = (PFNGLUNIFORM4UI64NVPROC)load("glUniform4ui64NV");
	glad_glUniform1ui64vNV = (PFNGLUNIFORM1UI64VNVPROC)load("glUniform1ui64vNV");
	glad_glUniform2ui64vNV = (PFNGLUNIFORM2UI64VNVPROC)load("glUniform2ui64vNV");
	glad_glUniform3ui64vNV = (PFNGLUNIFORM3UI64VNVPROC)load("glUniform3ui64vNV");
	glad_glUniform4ui64vNV = (PFNGLUNIFORM4UI64VNVPROC)load("glUniform4ui64vNV");
	glad_glGetUniformi64vNV = (PFNGLGETUNIFORMI64VNVPROC)load("glGetUniformi64vNV");
	glad_glGetUniformui64vNV = (PFNGLGETUNIFORMUI64VNVPROC)load("glGetUniformui64vNV");
	glad_glProgramUniform1i64NV = (PFNGLPROGRAMUNIFORM1I64NVPROC)load("glProgramUniform1i64NV");
	glad_glProgramUniform2i64NV = (PFNGLPROGRAMUNIFORM2I64NVPROC)load("glProgramUniform2i64NV");
	glad_glProgramUniform3i64NV = (PFNGLPROGRAMUNIFORM3I64NVPROC)load("glProgramUniform3i64NV");
	glad_glProgramUniform4i64NV = (PFNGLPROGRAMUNIFORM4I64NVPROC)load("glProgramUniform4i64NV");
	glad_glProgramUniform1i64vNV = (PFNGLPROGRAMUNIFORM1I64VNVPROC)load("glProgramUniform1i64vNV");
	glad_glProgramUniform2i64vNV = (PFNGLPROGRAMUNIFORM2I64VNVPROC)load("glProgramUniform2i64vNV");
	glad_glProgramUniform3i64vNV = (PFNGLPROGRAMUNIFORM3I64VNVPROC)load("glProgramUniform3i64vNV");
	glad_glProgramUniform4i64vNV = (PFNGLPROGRAMUNIFORM4I64VNVPROC)load("glProgramUniform4i64vNV");
	glad_glProgramUniform1ui64NV = (PFNGLPROGRAMUNIFORM1UI64NVPROC)load("glProgramUniform1ui64NV");
	glad_glProgramUniform2ui64NV = (PFNGLPROGRAMUNIFORM2UI64NVPROC)load("glProgramUniform2ui64NV");
	glad_glProgramUniform3ui64NV = (PFNGLPROGRAMUNIFORM3UI64NVPROC)load("glProgramUniform3ui64NV");
	glad_glProgramUniform4ui64NV = (PFNGLPROGRAMUNIFORM4UI64NVPROC)load("glProgramUniform4ui64NV");
	glad_glProgramUniform1ui64vNV = (PFNGLPROGRAMUNIFORM1UI64VNVPROC)load("glProgramUniform1ui64vNV");
	glad_glProgramUniform2ui64vNV = (PFNGLPROGRAMUNIFORM2UI64VNVPROC)load("glProgramUniform2ui64vNV");
	glad_glProgramUniform3ui64vNV = (PFNGLPROGRAMUNIFORM3UI64VNVPROC)load("glProgramUniform3ui64vNV");
	glad_glProgramUniform4ui64vNV = (PFNGLPROGRAMUNIFORM4UI64VNVPROC)load("glProgramUniform4ui64vNV");
}
static void load_GL_AMD_interleaved_elements(GLADloadproc load) {
	if(!GLAD_GL_AMD_interleaved_elements) return;
	glad_glVertexAttribParameteriAMD = (PFNGLVERTEXATTRIBPARAMETERIAMDPROC)load("glVertexAttribParameteriAMD");
}
static void load_GL_AMD_multi_draw_indirect(GLADloadproc load) {
	if(!GLAD_GL_AMD_multi_draw_indirect) return;
	glad_glMultiDrawArraysIndirectAMD = (PFNGLMULTIDRAWARRAYSINDIRECTAMDPROC)load("glMultiDrawArraysIndirectAMD");
	glad_glMultiDrawElementsIndirectAMD = (PFNGLMULTIDRAWELEMENTSINDIRECTAMDPROC)load("glMultiDrawElementsIndirectAMD");
}
static void load_GL_AMD_name_gen_delete(GLADloadproc load) {
	if(!GLAD_GL_AMD_name_gen_delete) return;
	glad_glGenNamesAMD = (PFNGLGENNAMESAMDPROC)load("glGenNamesAMD");
	glad_glDeleteNamesAMD = (PFNGLDELETENAMESAMDPROC)load("glDeleteNamesAMD");
	glad_glIsNameAMD = (PFNGLISNAMEAMDPROC)load("glIsNameAMD");
}
static void load_GL_AMD_occlusion_query_event(GLADloadproc load) {
	if(!GLAD_GL_AMD_occlusion_query_event) return;
	glad_glQueryObjectParameteruiAMD = (PFNGLQUERYOBJECTPARAMETERUIAMDPROC)load("glQueryObjectParameteruiAMD");
}
static void load_GL_AMD_performance_monitor(GLADloadproc load) {
	if(!GLAD_GL_AMD_performance_monitor) return;
	glad_glGetPerfMonitorGroupsAMD = (PFNGLGETPERFMONITORGROUPSAMDPROC)load("glGetPerfMonitorGroupsAMD");
	glad_glGetPerfMonitorCountersAMD = (PFNGLGETPERFMONITORCOUNTERSAMDPROC)load("glGetPerfMonitorCountersAMD");
	glad_glGetPerfMonitorGroupStringAMD = (PFNGLGETPERFMONITORGROUPSTRINGAMDPROC)load("glGetPerfMonitorGroupStringAMD");
	glad_glGetPerfMonitorCounterStringAMD = (PFNGLGETPERFMONITORCOUNTERSTRINGAMDPROC)load("glGetPerfMonitorCounterStringAMD");
	glad_glGetPerfMonitorCounterInfoAMD = (PFNGLGETPERFMONITORCOUNTERINFOAMDPROC)load("glGetPerfMonitorCounterInfoAMD");
	glad_glGenPerfMonitorsAMD = (PFNGLGENPERFMONITORSAMDPROC)load("glGenPerfMonitorsAMD");
	glad_glDeletePerfMonitorsAMD = (PFNGLDELETEPERFMONITORSAMDPROC)load("glDeletePerfMonitorsAMD");
	glad_glSelectPerfMonitorCountersAMD = (PFNGLSELECTPERFMONITORCOUNTERSAMDPROC)load("glSelectPerfMonitorCountersAMD");
	glad_glBeginPerfMonitorAMD = (PFNGLBEGINPERFMONITORAMDPROC)load("glBeginPerfMonitorAMD");
	glad_glEndPerfMonitorAMD = (PFNGLENDPERFMONITORAMDPROC)load("glEndPerfMonitorAMD");
	glad_glGetPerfMonitorCounterDataAMD = (PFNGLGETPERFMONITORCOUNTERDATAAMDPROC)load("glGetPerfMonitorCounterDataAMD");
}
static void load_GL_AMD_sample_positions(GLADloadproc load) {
	if(!GLAD_GL_AMD_sample_positions) return;
	glad_glSetMultisamplefvAMD = (PFNGLSETMULTISAMPLEFVAMDPROC)load("glSetMultisamplefvAMD");
}
static void load_GL_AMD_sparse_texture(GLADloadproc load) {
	if(!GLAD_GL_AMD_sparse_texture) return;
	glad_glTexStorageSparseAMD = (PFNGLTEXSTORAGESPARSEAMDPROC)load("glTexStorageSparseAMD");
	glad_glTextureStorageSparseAMD = (PFNGLTEXTURESTORAGESPARSEAMDPROC)load("glTextureStorageSparseAMD");
}
static void load_GL_AMD_stencil_operation_extended(GLADloadproc load) {
	if(!GLAD_GL_AMD_stencil_operation_extended) return;
	glad_glStencilOpValueAMD = (PFNGLSTENCILOPVALUEAMDPROC)load("glStencilOpValueAMD");
}
static void load_GL_AMD_vertex_shader_tessellator(GLADloadproc load) {
	if(!GLAD_GL_AMD_vertex_shader_tessellator) return;
	glad_glTessellationFactorAMD = (PFNGLTESSELLATIONFACTORAMDPROC)load("glTessellationFactorAMD");
	glad_glTessellationModeAMD = (PFNGLTESSELLATIONMODEAMDPROC)load("glTessellationModeAMD");
}
static void load_GL_APPLE_element_array(GLADloadproc load) {
	if(!GLAD_GL_APPLE_element_array) return;
	glad_glElementPointerAPPLE = (PFNGLELEMENTPOINTERAPPLEPROC)load("glElementPointerAPPLE");
	glad_glDrawElementArrayAPPLE = (PFNGLDRAWELEMENTARRAYAPPLEPROC)load("glDrawElementArrayAPPLE");
	glad_glDrawRangeElementArrayAPPLE = (PFNGLDRAWRANGEELEMENTARRAYAPPLEPROC)load("glDrawRangeElementArrayAPPLE");
	glad_glMultiDrawElementArrayAPPLE = (PFNGLMULTIDRAWELEMENTARRAYAPPLEPROC)load("glMultiDrawElementArrayAPPLE");
	glad_glMultiDrawRangeElementArrayAPPLE = (PFNGLMULTIDRAWRANGEELEMENTARRAYAPPLEPROC)load("glMultiDrawRangeElementArrayAPPLE");
}
static void load_GL_APPLE_fence(GLADloadproc load) {
	if(!GLAD_GL_APPLE_fence) return;
	glad_glGenFencesAPPLE = (PFNGLGENFENCESAPPLEPROC)load("glGenFencesAPPLE");
	glad_glDeleteFencesAPPLE = (PFNGLDELETEFENCESAPPLEPROC)load("glDeleteFencesAPPLE");
	glad_glSetFenceAPPLE = (PFNGLSETFENCEAPPLEPROC)load("glSetFenceAPPLE");
	glad_glIsFenceAPPLE = (PFNGLISFENCEAPPLEPROC)load("glIsFenceAPPLE");
	glad_glTestFenceAPPLE = (PFNGLTESTFENCEAPPLEPROC)load("glTestFenceAPPLE");
	glad_glFinishFenceAPPLE = (PFNGLFINISHFENCEAPPLEPROC)load("glFinishFenceAPPLE");
	glad_glTestObjectAPPLE = (PFNGLTESTOBJECTAPPLEPROC)load("glTestObjectAPPLE");
	glad_glFinishObjectAPPLE = (PFNGLFINISHOBJECTAPPLEPROC)load("glFinishObjectAPPLE");
}
static void load_GL_APPLE_flush_buffer_range(GLADloadproc load) {
	if(!GLAD_GL_APPLE_flush_buffer_range) return;
	glad_glBufferParameteriAPPLE = (PFNGLBUFFERPARAMETERIAPPLEPROC)load("glBufferParameteriAPPLE");
	glad_glFlushMappedBufferRangeAPPLE = (PFNGLFLUSHMAPPEDBUFFERRANGEAPPLEPROC)load("glFlushMappedBufferRangeAPPLE");
}
static void load_GL_APPLE_object_purgeable(GLADloadproc load) {
	if(!GLAD_GL_APPLE_object_purgeable) return;
	glad_glObjectPurgeableAPPLE = (PFNGLOBJECTPURGEABLEAPPLEPROC)load("glObjectPurgeableAPPLE");
	glad_glObjectUnpurgeableAPPLE = (PFNGLOBJECTUNPURGEABLEAPPLEPROC)load("glObjectUnpurgeableAPPLE");
	glad_glGetObjectParameterivAPPLE = (PFNGLGETOBJECTPARAMETERIVAPPLEPROC)load("glGetObjectParameterivAPPLE");
}
static void load_GL_APPLE_texture_range(GLADloadproc load) {
	if(!GLAD_GL_APPLE_texture_range) return;
	glad_glTextureRangeAPPLE = (PFNGLTEXTURERANGEAPPLEPROC)load("glTextureRangeAPPLE");
	glad_glGetTexParameterPointervAPPLE = (PFNGLGETTEXPARAMETERPOINTERVAPPLEPROC)load("glGetTexParameterPointervAPPLE");
}
static void load_GL_APPLE_vertex_array_object(GLADloadproc load) {
	if(!GLAD_GL_APPLE_vertex_array_object) return;
	glad_glBindVertexArrayAPPLE = (PFNGLBINDVERTEXARRAYAPPLEPROC)load("glBindVertexArrayAPPLE");
	glad_glDeleteVertexArraysAPPLE = (PFNGLDELETEVERTEXARRAYSAPPLEPROC)load("glDeleteVertexArraysAPPLE");
	glad_glGenVertexArraysAPPLE = (PFNGLGENVERTEXARRAYSAPPLEPROC)load("glGenVertexArraysAPPLE");
	glad_glIsVertexArrayAPPLE = (PFNGLISVERTEXARRAYAPPLEPROC)load("glIsVertexArrayAPPLE");
}
static void load_GL_APPLE_vertex_array_range(GLADloadproc load) {
	if(!GLAD_GL_APPLE_vertex_array_range) return;
	glad_glVertexArrayRangeAPPLE = (PFNGLVERTEXARRAYRANGEAPPLEPROC)load("glVertexArrayRangeAPPLE");
	glad_glFlushVertexArrayRangeAPPLE = (PFNGLFLUSHVERTEXARRAYRANGEAPPLEPROC)load("glFlushVertexArrayRangeAPPLE");
	glad_glVertexArrayParameteriAPPLE = (PFNGLVERTEXARRAYPARAMETERIAPPLEPROC)load("glVertexArrayParameteriAPPLE");
}
static void load_GL_APPLE_vertex_program_evaluators(GLADloadproc load) {
	if(!GLAD_GL_APPLE_vertex_program_evaluators) return;
	glad_glEnableVertexAttribAPPLE = (PFNGLENABLEVERTEXATTRIBAPPLEPROC)load("glEnableVertexAttribAPPLE");
	glad_glDisableVertexAttribAPPLE = (PFNGLDISABLEVERTEXATTRIBAPPLEPROC)load("glDisableVertexAttribAPPLE");
	glad_glIsVertexAttribEnabledAPPLE = (PFNGLISVERTEXATTRIBENABLEDAPPLEPROC)load("glIsVertexAttribEnabledAPPLE");
	glad_glMapVertexAttrib1dAPPLE = (PFNGLMAPVERTEXATTRIB1DAPPLEPROC)load("glMapVertexAttrib1dAPPLE");
	glad_glMapVertexAttrib1fAPPLE = (PFNGLMAPVERTEXATTRIB1FAPPLEPROC)load("glMapVertexAttrib1fAPPLE");
	glad_glMapVertexAttrib2dAPPLE = (PFNGLMAPVERTEXATTRIB2DAPPLEPROC)load("glMapVertexAttrib2dAPPLE");
	glad_glMapVertexAttrib2fAPPLE = (PFNGLMAPVERTEXATTRIB2FAPPLEPROC)load("glMapVertexAttrib2fAPPLE");
}
static void load_GL_ARB_ES2_compatibility(GLADloadproc load) {
	if(!GLAD_GL_ARB_ES2_compatibility) return;
	glad_glReleaseShaderCompiler = (PFNGLRELEASESHADERCOMPILERPROC)load("glReleaseShaderCompiler");
	glad_glShaderBinary = (PFNGLSHADERBINARYPROC)load("glShaderBinary");
	glad_glGetShaderPrecisionFormat = (PFNGLGETSHADERPRECISIONFORMATPROC)load("glGetShaderPrecisionFormat");
	glad_glDepthRangef = (PFNGLDEPTHRANGEFPROC)load("glDepthRangef");
	glad_glClearDepthf = (PFNGLCLEARDEPTHFPROC)load("glClearDepthf");
}
static void load_GL_ARB_ES3_1_compatibility(GLADloadproc load) {
	if(!GLAD_GL_ARB_ES3_1_compatibility) return;
	glad_glMemoryBarrierByRegion = (PFNGLMEMORYBARRIERBYREGIONPROC)load("glMemoryBarrierByRegion");
}
static void load_GL_ARB_ES3_2_compatibility(GLADloadproc load) {
	if(!GLAD_GL_ARB_ES3_2_compatibility) return;
	glad_glPrimitiveBoundingBoxARB = (PFNGLPRIMITIVEBOUNDINGBOXARBPROC)load("glPrimitiveBoundingBoxARB");
}
static void load_GL_ARB_base_instance(GLADloadproc load) {
	if(!GLAD_GL_ARB_base_instance) return;
	glad_glDrawArraysInstancedBaseInstance = (PFNGLDRAWARRAYSINSTANCEDBASEINSTANCEPROC)load("glDrawArraysInstancedBaseInstance");
	glad_glDrawElementsInstancedBaseInstance = (PFNGLDRAWELEMENTSINSTANCEDBASEINSTANCEPROC)load("glDrawElementsInstancedBaseInstance");
	glad_glDrawElementsInstancedBaseVertexBaseInstance = (PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCEPROC)load("glDrawElementsInstancedBaseVertexBaseInstance");
}
static void load_GL_ARB_bindless_texture(GLADloadproc load) {
	if(!GLAD_GL_ARB_bindless_texture) return;
	glad_glGetTextureHandleARB = (PFNGLGETTEXTUREHANDLEARBPROC)load("glGetTextureHandleARB");
	glad_glGetTextureSamplerHandleARB = (PFNGLGETTEXTURESAMPLERHANDLEARBPROC)load("glGetTextureSamplerHandleARB");
	glad_glMakeTextureHandleResidentARB = (PFNGLMAKETEXTUREHANDLERESIDENTARBPROC)load("glMakeTextureHandleResidentARB");
	glad_glMakeTextureHandleNonResidentARB = (PFNGLMAKETEXTUREHANDLENONRESIDENTARBPROC)load("glMakeTextureHandleNonResidentARB");
	glad_glGetImageHandleARB = (PFNGLGETIMAGEHANDLEARBPROC)load("glGetImageHandleARB");
	glad_glMakeImageHandleResidentARB = (PFNGLMAKEIMAGEHANDLERESIDENTARBPROC)load("glMakeImageHandleResidentARB");
	glad_glMakeImageHandleNonResidentARB = (PFNGLMAKEIMAGEHANDLENONRESIDENTARBPROC)load("glMakeImageHandleNonResidentARB");
	glad_glUniformHandleui64ARB = (PFNGLUNIFORMHANDLEUI64ARBPROC)load("glUniformHandleui64ARB");
	glad_glUniformHandleui64vARB = (PFNGLUNIFORMHANDLEUI64VARBPROC)load("glUniformHandleui64vARB");
	glad_glProgramUniformHandleui64ARB = (PFNGLPROGRAMUNIFORMHANDLEUI64ARBPROC)load("glProgramUniformHandleui64ARB");
	glad_glProgramUniformHandleui64vARB = (PFNGLPROGRAMUNIFORMHANDLEUI64VARBPROC)load("glProgramUniformHandleui64vARB");
	glad_glIsTextureHandleResidentARB = (PFNGLISTEXTUREHANDLERESIDENTARBPROC)load("glIsTextureHandleResidentARB");
	glad_glIsImageHandleResidentARB = (PFNGLISIMAGEHANDLERESIDENTARBPROC)load("glIsImageHandleResidentARB");
	glad_glVertexAttribL1ui64ARB = (PFNGLVERTEXATTRIBL1UI64ARBPROC)load("glVertexAttribL1ui64ARB");
	glad_glVertexAttribL1ui64vARB = (PFNGLVERTEXATTRIBL1UI64VARBPROC)load("glVertexAttribL1ui64vARB");
	glad_glGetVertexAttribLui64vARB = (PFNGLGETVERTEXATTRIBLUI64VARBPROC)load("glGetVertexAttribLui64vARB");
}
static void load_GL_ARB_blend_func_extended(GLADloadproc load) {
	if(!GLAD_GL_ARB_blend_func_extended) return;
	glad_glBindFragDataLocationIndexed = (PFNGLBINDFRAGDATALOCATIONINDEXEDPROC)load("glBindFragDataLocationIndexed");
	glad_glGetFragDataIndex = (PFNGLGETFRAGDATAINDEXPROC)load("glGetFragDataIndex");
}
static void load_GL_ARB_buffer_storage(GLADloadproc load) {
	if(!GLAD_GL_ARB_buffer_storage) return;
	glad_glBufferStorage = (PFNGLBUFFERSTORAGEPROC)load("glBufferStorage");
}
static void load_GL_ARB_cl_event(GLADloadproc load) {
	if(!GLAD_GL_ARB_cl_event) return;
	glad_glCreateSyncFromCLeventARB = (PFNGLCREATESYNCFROMCLEVENTARBPROC)load("glCreateSyncFromCLeventARB");
}
static void load_GL_ARB_clear_buffer_object(GLADloadproc load) {
	if(!GLAD_GL_ARB_clear_buffer_object) return;
	glad_glClearBufferData = (PFNGLCLEARBUFFERDATAPROC)load("glClearBufferData");
	glad_glClearBufferSubData = (PFNGLCLEARBUFFERSUBDATAPROC)load("glClearBufferSubData");
}
static void load_GL_ARB_clear_texture(GLADloadproc load) {
	if(!GLAD_GL_ARB_clear_texture) return;
	glad_glClearTexImage = (PFNGLCLEARTEXIMAGEPROC)load("glClearTexImage");
	glad_glClearTexSubImage = (PFNGLCLEARTEXSUBIMAGEPROC)load("glClearTexSubImage");
}
static void load_GL_ARB_clip_control(GLADloadproc load) {
	if(!GLAD_GL_ARB_clip_control) return;
	glad_glClipControl = (PFNGLCLIPCONTROLPROC)load("glClipControl");
}
static void load_GL_ARB_color_buffer_float(GLADloadproc load) {
	if(!GLAD_GL_ARB_color_buffer_float) return;
	glad_glClampColorARB = (PFNGLCLAMPCOLORARBPROC)load("glClampColorARB");
}
static void load_GL_ARB_compute_shader(GLADloadproc load) {
	if(!GLAD_GL_ARB_compute_shader) return;
	glad_glDispatchCompute = (PFNGLDISPATCHCOMPUTEPROC)load("glDispatchCompute");
	glad_glDispatchComputeIndirect = (PFNGLDISPATCHCOMPUTEINDIRECTPROC)load("glDispatchComputeIndirect");
}
static void load_GL_ARB_compute_variable_group_size(GLADloadproc load) {
	if(!GLAD_GL_ARB_compute_variable_group_size) return;
	glad_glDispatchComputeGroupSizeARB = (PFNGLDISPATCHCOMPUTEGROUPSIZEARBPROC)load("glDispatchComputeGroupSizeARB");
}
static void load_GL_ARB_copy_buffer(GLADloadproc load) {
	if(!GLAD_GL_ARB_copy_buffer) return;
	glad_glCopyBufferSubData = (PFNGLCOPYBUFFERSUBDATAPROC)load("glCopyBufferSubData");
}
static void load_GL_ARB_copy_image(GLADloadproc load) {
	if(!GLAD_GL_ARB_copy_image) return;
	glad_glCopyImageSubData = (PFNGLCOPYIMAGESUBDATAPROC)load("glCopyImageSubData");
}
static void load_GL_ARB_debug_output(GLADloadproc load) {
	if(!GLAD_GL_ARB_debug_output) return;
	glad_glDebugMessageControlARB = (PFNGLDEBUGMESSAGECONTROLARBPROC)load("glDebugMessageControlARB");
	glad_glDebugMessageInsertARB = (PFNGLDEBUGMESSAGEINSERTARBPROC)load("glDebugMessageInsertARB");
	glad_glDebugMessageCallbackARB = (PFNGLDEBUGMESSAGECALLBACKARBPROC)load("glDebugMessageCallbackARB");
	glad_glGetDebugMessageLogARB = (PFNGLGETDEBUGMESSAGELOGARBPROC)load("glGetDebugMessageLogARB");
}
static void load_GL_ARB_direct_state_access(GLADloadproc load) {
	if(!GLAD_GL_ARB_direct_state_access) return;
	glad_glCreateTransformFeedbacks = (PFNGLCREATETRANSFORMFEEDBACKSPROC)load("glCreateTransformFeedbacks");
	glad_glTransformFeedbackBufferBase = (PFNGLTRANSFORMFEEDBACKBUFFERBASEPROC)load("glTransformFeedbackBufferBase");
	glad_glTransformFeedbackBufferRange = (PFNGLTRANSFORMFEEDBACKBUFFERRANGEPROC)load("glTransformFeedbackBufferRange");
	glad_glGetTransformFeedbackiv = (PFNGLGETTRANSFORMFEEDBACKIVPROC)load("glGetTransformFeedbackiv");
	glad_glGetTransformFeedbacki_v = (PFNGLGETTRANSFORMFEEDBACKI_VPROC)load("glGetTransformFeedbacki_v");
	glad_glGetTransformFeedbacki64_v = (PFNGLGETTRANSFORMFEEDBACKI64_VPROC)load("glGetTransformFeedbacki64_v");
	glad_glCreateBuffers = (PFNGLCREATEBUFFERSPROC)load("glCreateBuffers");
	glad_glNamedBufferStorage = (PFNGLNAMEDBUFFERSTORAGEPROC)load("glNamedBufferStorage");
	glad_glNamedBufferData = (PFNGLNAMEDBUFFERDATAPROC)load("glNamedBufferData");
	glad_glNamedBufferSubData = (PFNGLNAMEDBUFFERSUBDATAPROC)load("glNamedBufferSubData");
	glad_glCopyNamedBufferSubData = (PFNGLCOPYNAMEDBUFFERSUBDATAPROC)load("glCopyNamedBufferSubData");
	glad_glClearNamedBufferData = (PFNGLCLEARNAMEDBUFFERDATAPROC)load("glClearNamedBufferData");
	glad_glClearNamedBufferSubData = (PFNGLCLEARNAMEDBUFFERSUBDATAPROC)load("glClearNamedBufferSubData");
	glad_glMapNamedBuffer = (PFNGLMAPNAMEDBUFFERPROC)load("glMapNamedBuffer");
	glad_glMapNamedBufferRange = (PFNGLMAPNAMEDBUFFERRANGEPROC)load("glMapNamedBufferRange");
	glad_glUnmapNamedBuffer = (PFNGLUNMAPNAMEDBUFFERPROC)load("glUnmapNamedBuffer");
	glad_glFlushMappedNamedBufferRange = (PFNGLFLUSHMAPPEDNAMEDBUFFERRANGEPROC)load("glFlushMappedNamedBufferRange");
	glad_glGetNamedBufferParameteriv = (PFNGLGETNAMEDBUFFERPARAMETERIVPROC)load("glGetNamedBufferParameteriv");
	glad_glGetNamedBufferParameteri64v = (PFNGLGETNAMEDBUFFERPARAMETERI64VPROC)load("glGetNamedBufferParameteri64v");
	glad_glGetNamedBufferPointerv = (PFNGLGETNAMEDBUFFERPOINTERVPROC)load("glGetNamedBufferPointerv");
	glad_glGetNamedBufferSubData = (PFNGLGETNAMEDBUFFERSUBDATAPROC)load("glGetNamedBufferSubData");
	glad_glCreateFramebuffers = (PFNGLCREATEFRAMEBUFFERSPROC)load("glCreateFramebuffers");
	glad_glNamedFramebufferRenderbuffer = (PFNGLNAMEDFRAMEBUFFERRENDERBUFFERPROC)load("glNamedFramebufferRenderbuffer");
	glad_glNamedFramebufferParameteri = (PFNGLNAMEDFRAMEBUFFERPARAMETERIPROC)load("glNamedFramebufferParameteri");
	glad_glNamedFramebufferTexture = (PFNGLNAMEDFRAMEBUFFERTEXTUREPROC)load("glNamedFramebufferTexture");
	glad_glNamedFramebufferTextureLayer = (PFNGLNAMEDFRAMEBUFFERTEXTURELAYERPROC)load("glNamedFramebufferTextureLayer");
	glad_glNamedFramebufferDrawBuffer = (PFNGLNAMEDFRAMEBUFFERDRAWBUFFERPROC)load("glNamedFramebufferDrawBuffer");
	glad_glNamedFramebufferDrawBuffers = (PFNGLNAMEDFRAMEBUFFERDRAWBUFFERSPROC)load("glNamedFramebufferDrawBuffers");
	glad_glNamedFramebufferReadBuffer = (PFNGLNAMEDFRAMEBUFFERREADBUFFERPROC)load("glNamedFramebufferReadBuffer");
	glad_glInvalidateNamedFramebufferData = (PFNGLINVALIDATENAMEDFRAMEBUFFERDATAPROC)load("glInvalidateNamedFramebufferData");
	glad_glInvalidateNamedFramebufferSubData = (PFNGLINVALIDATENAMEDFRAMEBUFFERSUBDATAPROC)load("glInvalidateNamedFramebufferSubData");
	glad_glClearNamedFramebufferiv = (PFNGLCLEARNAMEDFRAMEBUFFERIVPROC)load("glClearNamedFramebufferiv");
	glad_glClearNamedFramebufferuiv = (PFNGLCLEARNAMEDFRAMEBUFFERUIVPROC)load("glClearNamedFramebufferuiv");
	glad_glClearNamedFramebufferfv = (PFNGLCLEARNAMEDFRAMEBUFFERFVPROC)load("glClearNamedFramebufferfv");
	glad_glClearNamedFramebufferfi = (PFNGLCLEARNAMEDFRAMEBUFFERFIPROC)load("glClearNamedFramebufferfi");
	glad_glBlitNamedFramebuffer = (PFNGLBLITNAMEDFRAMEBUFFERPROC)load("glBlitNamedFramebuffer");
	glad_glCheckNamedFramebufferStatus = (PFNGLCHECKNAMEDFRAMEBUFFERSTATUSPROC)load("glCheckNamedFramebufferStatus");
	glad_glGetNamedFramebufferParameteriv = (PFNGLGETNAMEDFRAMEBUFFERPARAMETERIVPROC)load("glGetNamedFramebufferParameteriv");
	glad_glGetNamedFramebufferAttachmentParameteriv = (PFNGLGETNAMEDFRAMEBUFFERATTACHMENTPARAMETERIVPROC)load("glGetNamedFramebufferAttachmentParameteriv");
	glad_glCreateRenderbuffers = (PFNGLCREATERENDERBUFFERSPROC)load("glCreateRenderbuffers");
	glad_glNamedRenderbufferStorage = (PFNGLNAMEDRENDERBUFFERSTORAGEPROC)load("glNamedRenderbufferStorage");
	glad_glNamedRenderbufferStorageMultisample = (PFNGLNAMEDRENDERBUFFERSTORAGEMULTISAMPLEPROC)load("glNamedRenderbufferStorageMultisample");
	glad_glGetNamedRenderbufferParameteriv = (PFNGLGETNAMEDRENDERBUFFERPARAMETERIVPROC)load("glGetNamedRenderbufferParameteriv");
	glad_glCreateTextures = (PFNGLCREATETEXTURESPROC)load("glCreateTextures");
	glad_glTextureBuffer = (PFNGLTEXTUREBUFFERPROC)load("glTextureBuffer");
	glad_glTextureBufferRange = (PFNGLTEXTUREBUFFERRANGEPROC)load("glTextureBufferRange");
	glad_glTextureStorage1D = (PFNGLTEXTURESTORAGE1DPROC)load("glTextureStorage1D");
	glad_glTextureStorage2D = (PFNGLTEXTURESTORAGE2DPROC)load("glTextureStorage2D");
	glad_glTextureStorage3D = (PFNGLTEXTURESTORAGE3DPROC)load("glTextureStorage3D");
	glad_glTextureStorage2DMultisample = (PFNGLTEXTURESTORAGE2DMULTISAMPLEPROC)load("glTextureStorage2DMultisample");
	glad_glTextureStorage3DMultisample = (PFNGLTEXTURESTORAGE3DMULTISAMPLEPROC)load("glTextureStorage3DMultisample");
	glad_glTextureSubImage1D = (PFNGLTEXTURESUBIMAGE1DPROC)load("glTextureSubImage1D");
	glad_glTextureSubImage2D = (PFNGLTEXTURESUBIMAGE2DPROC)load("glTextureSubImage2D");
	glad_glTextureSubImage3D = (PFNGLTEXTURESUBIMAGE3DPROC)load("glTextureSubImage3D");
	glad_glCompressedTextureSubImage1D = (PFNGLCOMPRESSEDTEXTURESUBIMAGE1DPROC)load("glCompressedTextureSubImage1D");
	glad_glCompressedTextureSubImage2D = (PFNGLCOMPRESSEDTEXTURESUBIMAGE2DPROC)load("glCompressedTextureSubImage2D");
	glad_glCompressedTextureSubImage3D = (PFNGLCOMPRESSEDTEXTURESUBIMAGE3DPROC)load("glCompressedTextureSubImage3D");
	glad_glCopyTextureSubImage1D = (PFNGLCOPYTEXTURESUBIMAGE1DPROC)load("glCopyTextureSubImage1D");
	glad_glCopyTextureSubImage2D = (PFNGLCOPYTEXTURESUBIMAGE2DPROC)load("glCopyTextureSubImage2D");
	glad_glCopyTextureSubImage3D = (PFNGLCOPYTEXTURESUBIMAGE3DPROC)load("glCopyTextureSubImage3D");
	glad_glTextureParameterf = (PFNGLTEXTUREPARAMETERFPROC)load("glTextureParameterf");
	glad_glTextureParameterfv = (PFNGLTEXTUREPARAMETERFVPROC)load("glTextureParameterfv");
	glad_glTextureParameteri = (PFNGLTEXTUREPARAMETERIPROC)load("glTextureParameteri");
	glad_glTextureParameterIiv = (PFNGLTEXTUREPARAMETERIIVPROC)load("glTextureParameterIiv");
	glad_glTextureParameterIuiv = (PFNGLTEXTUREPARAMETERIUIVPROC)load("glTextureParameterIuiv");
	glad_glTextureParameteriv = (PFNGLTEXTUREPARAMETERIVPROC)load("glTextureParameteriv");
	glad_glGenerateTextureMipmap = (PFNGLGENERATETEXTUREMIPMAPPROC)load("glGenerateTextureMipmap");
	glad_glBindTextureUnit = (PFNGLBINDTEXTUREUNITPROC)load("glBindTextureUnit");
	glad_glGetTextureImage = (PFNGLGETTEXTUREIMAGEPROC)load("glGetTextureImage");
	glad_glGetCompressedTextureImage = (PFNGLGETCOMPRESSEDTEXTUREIMAGEPROC)load("glGetCompressedTextureImage");
	glad_glGetTextureLevelParameterfv = (PFNGLGETTEXTURELEVELPARAMETERFVPROC)load("glGetTextureLevelParameterfv");
	glad_glGetTextureLevelParameteriv = (PFNGLGETTEXTURELEVELPARAMETERIVPROC)load("glGetTextureLevelParameteriv");
	glad_glGetTextureParameterfv = (PFNGLGETTEXTUREPARAMETERFVPROC)load("glGetTextureParameterfv");
	glad_glGetTextureParameterIiv = (PFNGLGETTEXTUREPARAMETERIIVPROC)load("glGetTextureParameterIiv");
	glad_glGetTextureParameterIuiv = (PFNGLGETTEXTUREPARAMETERIUIVPROC)load("glGetTextureParameterIuiv");
	glad_glGetTextureParameteriv = (PFNGLGETTEXTUREPARAMETERIVPROC)load("glGetTextureParameteriv");
	glad_glCreateVertexArrays = (PFNGLCREATEVERTEXARRAYSPROC)load("glCreateVertexArrays");
	glad_glDisableVertexArrayAttrib = (PFNGLDISABLEVERTEXARRAYATTRIBPROC)load("glDisableVertexArrayAttrib");
	glad_glEnableVertexArrayAttrib = (PFNGLENABLEVERTEXARRAYATTRIBPROC)load("glEnableVertexArrayAttrib");
	glad_glVertexArrayElementBuffer = (PFNGLVERTEXARRAYELEMENTBUFFERPROC)load("glVertexArrayElementBuffer");
	glad_glVertexArrayVertexBuffer = (PFNGLVERTEXARRAYVERTEXBUFFERPROC)load("glVertexArrayVertexBuffer");
	glad_glVertexArrayVertexBuffers = (PFNGLVERTEXARRAYVERTEXBUFFERSPROC)load("glVertexArrayVertexBuffers");
	glad_glVertexArrayAttribBinding = (PFNGLVERTEXARRAYATTRIBBINDINGPROC)load("glVertexArrayAttribBinding");
	glad_glVertexArrayAttribFormat = (PFNGLVERTEXARRAYATTRIBFORMATPROC)load("glVertexArrayAttribFormat");
	glad_glVertexArrayAttribIFormat = (PFNGLVERTEXARRAYATTRIBIFORMATPROC)load("glVertexArrayAttribIFormat");
	glad_glVertexArrayAttribLFormat = (PFNGLVERTEXARRAYATTRIBLFORMATPROC)load("glVertexArrayAttribLFormat");
	glad_glVertexArrayBindingDivisor = (PFNGLVERTEXARRAYBINDINGDIVISORPROC)load("glVertexArrayBindingDivisor");
	glad_glGetVertexArrayiv = (PFNGLGETVERTEXARRAYIVPROC)load("glGetVertexArrayiv");
	glad_glGetVertexArrayIndexediv = (PFNGLGETVERTEXARRAYINDEXEDIVPROC)load("glGetVertexArrayIndexediv");
	glad_glGetVertexArrayIndexed64iv = (PFNGLGETVERTEXARRAYINDEXED64IVPROC)load("glGetVertexArrayIndexed64iv");
	glad_glCreateSamplers = (PFNGLCREATESAMPLERSPROC)load("glCreateSamplers");
	glad_glCreateProgramPipelines = (PFNGLCREATEPROGRAMPIPELINESPROC)load("glCreateProgramPipelines");
	glad_glCreateQueries = (PFNGLCREATEQUERIESPROC)load("glCreateQueries");
	glad_glGetQueryBufferObjecti64v = (PFNGLGETQUERYBUFFEROBJECTI64VPROC)load("glGetQueryBufferObjecti64v");
	glad_glGetQueryBufferObjectiv = (PFNGLGETQUERYBUFFEROBJECTIVPROC)load("glGetQueryBufferObjectiv");
	glad_glGetQueryBufferObjectui64v = (PFNGLGETQUERYBUFFEROBJECTUI64VPROC)load("glGetQueryBufferObjectui64v");
	glad_glGetQueryBufferObjectuiv = (PFNGLGETQUERYBUFFEROBJECTUIVPROC)load("glGetQueryBufferObjectuiv");
}
static void load_GL_ARB_draw_buffers(GLADloadproc load) {
	if(!GLAD_GL_ARB_draw_buffers) return;
	glad_glDrawBuffersARB = (PFNGLDRAWBUFFERSARBPROC)load("glDrawBuffersARB");
}
static void load_GL_ARB_draw_buffers_blend(GLADloadproc load) {
	if(!GLAD_GL_ARB_draw_buffers_blend) return;
	glad_glBlendEquationiARB = (PFNGLBLENDEQUATIONIARBPROC)load("glBlendEquationiARB");
	glad_glBlendEquationSeparateiARB = (PFNGLBLENDEQUATIONSEPARATEIARBPROC)load("glBlendEquationSeparateiARB");
	glad_glBlendFunciARB = (PFNGLBLENDFUNCIARBPROC)load("glBlendFunciARB");
	glad_glBlendFuncSeparateiARB = (PFNGLBLENDFUNCSEPARATEIARBPROC)load("glBlendFuncSeparateiARB");
}
static void load_GL_ARB_draw_elements_base_vertex(GLADloadproc load) {
	if(!GLAD_GL_ARB_draw_elements_base_vertex) return;
	glad_glDrawElementsBaseVertex = (PFNGLDRAWELEMENTSBASEVERTEXPROC)load("glDrawElementsBaseVertex");
	glad_glDrawRangeElementsBaseVertex = (PFNGLDRAWRANGEELEMENTSBASEVERTEXPROC)load("glDrawRangeElementsBaseVertex");
	glad_glDrawElementsInstancedBaseVertex = (PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC)load("glDrawElementsInstancedBaseVertex");
	glad_glMultiDrawElementsBaseVertex = (PFNGLMULTIDRAWELEMENTSBASEVERTEXPROC)load("glMultiDrawElementsBaseVertex");
}
static void load_GL_ARB_draw_indirect(GLADloadproc load) {
	if(!GLAD_GL_ARB_draw_indirect) return;
	glad_glDrawArraysIndirect = (PFNGLDRAWARRAYSINDIRECTPROC)load("glDrawArraysIndirect");
	glad_glDrawElementsIndirect = (PFNGLDRAWELEMENTSINDIRECTPROC)load("glDrawElementsIndirect");
}
static void load_GL_ARB_draw_instanced(GLADloadproc load) {
	if(!GLAD_GL_ARB_draw_instanced) return;
	glad_glDrawArraysInstancedARB = (PFNGLDRAWARRAYSINSTANCEDARBPROC)load("glDrawArraysInstancedARB");
	glad_glDrawElementsInstancedARB = (PFNGLDRAWELEMENTSINSTANCEDARBPROC)load("glDrawElementsInstancedARB");
}
static void load_GL_ARB_fragment_program(GLADloadproc load) {
	if(!GLAD_GL_ARB_fragment_program) return;
	glad_glProgramStringARB = (PFNGLPROGRAMSTRINGARBPROC)load("glProgramStringARB");
	glad_glBindProgramARB = (PFNGLBINDPROGRAMARBPROC)load("glBindProgramARB");
	glad_glDeleteProgramsARB = (PFNGLDELETEPROGRAMSARBPROC)load("glDeleteProgramsARB");
	glad_glGenProgramsARB = (PFNGLGENPROGRAMSARBPROC)load("glGenProgramsARB");
	glad_glProgramEnvParameter4dARB = (PFNGLPROGRAMENVPARAMETER4DARBPROC)load("glProgramEnvParameter4dARB");
	glad_glProgramEnvParameter4dvARB = (PFNGLPROGRAMENVPARAMETER4DVARBPROC)load("glProgramEnvParameter4dvARB");
	glad_glProgramEnvParameter4fARB = (PFNGLPROGRAMENVPARAMETER4FARBPROC)load("glProgramEnvParameter4fARB");
	glad_glProgramEnvParameter4fvARB = (PFNGLPROGRAMENVPARAMETER4FVARBPROC)load("glProgramEnvParameter4fvARB");
	glad_glProgramLocalParameter4dARB = (PFNGLPROGRAMLOCALPARAMETER4DARBPROC)load("glProgramLocalParameter4dARB");
	glad_glProgramLocalParameter4dvARB = (PFNGLPROGRAMLOCALPARAMETER4DVARBPROC)load("glProgramLocalParameter4dvARB");
	glad_glProgramLocalParameter4fARB = (PFNGLPROGRAMLOCALPARAMETER4FARBPROC)load("glProgramLocalParameter4fARB");
	glad_glProgramLocalParameter4fvARB = (PFNGLPROGRAMLOCALPARAMETER4FVARBPROC)load("glProgramLocalParameter4fvARB");
	glad_glGetProgramEnvParameterdvARB = (PFNGLGETPROGRAMENVPARAMETERDVARBPROC)load("glGetProgramEnvParameterdvARB");
	glad_glGetProgramEnvParameterfvARB = (PFNGLGETPROGRAMENVPARAMETERFVARBPROC)load("glGetProgramEnvParameterfvARB");
	glad_glGetProgramLocalParameterdvARB = (PFNGLGETPROGRAMLOCALPARAMETERDVARBPROC)load("glGetProgramLocalParameterdvARB");
	glad_glGetProgramLocalParameterfvARB = (PFNGLGETPROGRAMLOCALPARAMETERFVARBPROC)load("glGetProgramLocalParameterfvARB");
	glad_glGetProgramivARB = (PFNGLGETPROGRAMIVARBPROC)load("glGetProgramivARB");
	glad_glGetProgramStringARB = (PFNGLGETPROGRAMSTRINGARBPROC)load("glGetProgramStringARB");
	glad_glIsProgramARB = (PFNGLISPROGRAMARBPROC)load("glIsProgramARB");
}
static void load_GL_ARB_framebuffer_no_attachments(GLADloadproc load) {
	if(!GLAD_GL_ARB_framebuffer_no_attachments) return;
	glad_glFramebufferParameteri = (PFNGLFRAMEBUFFERPARAMETERIPROC)load("glFramebufferParameteri");
	glad_glGetFramebufferParameteriv = (PFNGLGETFRAMEBUFFERPARAMETERIVPROC)load("glGetFramebufferParameteriv");
}
static void load_GL_ARB_framebuffer_object(GLADloadproc load) {
	if(!GLAD_GL_ARB_framebuffer_object) return;
	glad_glIsRenderbuffer = (PFNGLISRENDERBUFFERPROC)load("glIsRenderbuffer");
	glad_glBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC)load("glBindRenderbuffer");
	glad_glDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC)load("glDeleteRenderbuffers");
	glad_glGenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC)load("glGenRenderbuffers");
	glad_glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC)load("glRenderbufferStorage");
	glad_glGetRenderbufferParameteriv = (PFNGLGETRENDERBUFFERPARAMETERIVPROC)load("glGetRenderbufferParameteriv");
	glad_glIsFramebuffer = (PFNGLISFRAMEBUFFERPROC)load("glIsFramebuffer");
	glad_glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)load("glBindFramebuffer");
	glad_glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)load("glDeleteFramebuffers");
	glad_glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)load("glGenFramebuffers");
	glad_glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)load("glCheckFramebufferStatus");
	glad_glFramebufferTexture1D = (PFNGLFRAMEBUFFERTEXTURE1DPROC)load("glFramebufferTexture1D");
	glad_glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)load("glFramebufferTexture2D");
	glad_glFramebufferTexture3D = (PFNGLFRAMEBUFFERTEXTURE3DPROC)load("glFramebufferTexture3D");
	glad_glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)load("glFramebufferRenderbuffer");
	glad_glGetFramebufferAttachmentParameteriv = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC)load("glGetFramebufferAttachmentParameteriv");
	glad_glGenerateMipmap = (PFNGLGENERATEMIPMAPPROC)load("glGenerateMipmap");
	glad_glBlitFramebuffer = (PFNGLBLITFRAMEBUFFERPROC)load("glBlitFramebuffer");
	glad_glRenderbufferStorageMultisample = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC)load("glRenderbufferStorageMultisample");
	glad_glFramebufferTextureLayer = (PFNGLFRAMEBUFFERTEXTURELAYERPROC)load("glFramebufferTextureLayer");
}
static void load_GL_ARB_geometry_shader4(GLADloadproc load) {
	if(!GLAD_GL_ARB_geometry_shader4) return;
	glad_glProgramParameteriARB = (PFNGLPROGRAMPARAMETERIARBPROC)load("glProgramParameteriARB");
	glad_glFramebufferTextureARB = (PFNGLFRAMEBUFFERTEXTUREARBPROC)load("glFramebufferTextureARB");
	glad_glFramebufferTextureLayerARB = (PFNGLFRAMEBUFFERTEXTURELAYERARBPROC)load("glFramebufferTextureLayerARB");
	glad_glFramebufferTextureFaceARB = (PFNGLFRAMEBUFFERTEXTUREFACEARBPROC)load("glFramebufferTextureFaceARB");
}
static void load_GL_ARB_get_program_binary(GLADloadproc load) {
	if(!GLAD_GL_ARB_get_program_binary) return;
	glad_glGetProgramBinary = (PFNGLGETPROGRAMBINARYPROC)load("glGetProgramBinary");
	glad_glProgramBinary = (PFNGLPROGRAMBINARYPROC)load("glProgramBinary");
	glad_glProgramParameteri = (PFNGLPROGRAMPARAMETERIPROC)load("glProgramParameteri");
}
static void load_GL_ARB_get_texture_sub_image(GLADloadproc load) {
	if(!GLAD_GL_ARB_get_texture_sub_image) return;
	glad_glGetTextureSubImage = (PFNGLGETTEXTURESUBIMAGEPROC)load("glGetTextureSubImage");
	glad_glGetCompressedTextureSubImage = (PFNGLGETCOMPRESSEDTEXTURESUBIMAGEPROC)load("glGetCompressedTextureSubImage");
}
static void load_GL_ARB_gl_spirv(GLADloadproc load) {
	if(!GLAD_GL_ARB_gl_spirv) return;
	glad_glSpecializeShaderARB = (PFNGLSPECIALIZESHADERARBPROC)load("glSpecializeShaderARB");
}
static void load_GL_ARB_gpu_shader_fp64(GLADloadproc load) {
	if(!GLAD_GL_ARB_gpu_shader_fp64) return;
	glad_glUniform1d = (PFNGLUNIFORM1DPROC)load("glUniform1d");
	glad_glUniform2d = (PFNGLUNIFORM2DPROC)load("glUniform2d");
	glad_glUniform3d = (PFNGLUNIFORM3DPROC)load("glUniform3d");
	glad_glUniform4d = (PFNGLUNIFORM4DPROC)load("glUniform4d");
	glad_glUniform1dv = (PFNGLUNIFORM1DVPROC)load("glUniform1dv");
	glad_glUniform2dv = (PFNGLUNIFORM2DVPROC)load("glUniform2dv");
	glad_glUniform3dv = (PFNGLUNIFORM3DVPROC)load("glUniform3dv");
	glad_glUniform4dv = (PFNGLUNIFORM4DVPROC)load("glUniform4dv");
	glad_glUniformMatrix2dv = (PFNGLUNIFORMMATRIX2DVPROC)load("glUniformMatrix2dv");
	glad_glUniformMatrix3dv = (PFNGLUNIFORMMATRIX3DVPROC)load("glUniformMatrix3dv");
	glad_glUniformMatrix4dv = (PFNGLUNIFORMMATRIX4DVPROC)load("glUniformMatrix4dv");
	glad_glUniformMatrix2x3dv = (PFNGLUNIFORMMATRIX2X3DVPROC)load("glUniformMatrix2x3dv");
	glad_glUniformMatrix2x4dv = (PFNGLUNIFORMMATRIX2X4DVPROC)load("glUniformMatrix2x4dv");
	glad_glUniformMatrix3x2dv = (PFNGLUNIFORMMATRIX3X2DVPROC)load("glUniformMatrix3x2dv");
	glad_glUniformMatrix3x4dv = (PFNGLUNIFORMMATRIX3X4DVPROC)load("glUniformMatrix3x4dv");
	glad_glUniformMatrix4x2dv = (PFNGLUNIFORMMATRIX4X2DVPROC)load("glUniformMatrix4x2dv");
	glad_glUniformMatrix4x3dv = (PFNGLUNIFORMMATRIX4X3DVPROC)load("glUniformMatrix4x3dv");
	glad_glGetUniformdv = (PFNGLGETUNIFORMDVPROC)load("glGetUniformdv");
}
static void load_GL_ARB_gpu_shader_int64(GLADloadproc load) {
	if(!GLAD_GL_ARB_gpu_shader_int64) return;
	glad_glUniform1i64ARB = (PFNGLUNIFORM1I64ARBPROC)load("glUniform1i64ARB");
	glad_glUniform2i64ARB = (PFNGLUNIFORM2I64ARBPROC)load("glUniform2i64ARB");
	glad_glUniform3i64ARB = (PFNGLUNIFORM3I64ARBPROC)load("glUniform3i64ARB");
	glad_glUniform4i64ARB = (PFNGLUNIFORM4I64ARBPROC)load("glUniform4i64ARB");
	glad_glUniform1i64vARB = (PFNGLUNIFORM1I64VARBPROC)load("glUniform1i64vARB");
	glad_glUniform2i64vARB = (PFNGLUNIFORM2I64VARBPROC)load("glUniform2i64vARB");
	glad_glUniform3i64vARB = (PFNGLUNIFORM3I64VARBPROC)load("glUniform3i64vARB");
	glad_glUniform4i64vARB = (PFNGLUNIFORM4I64VARBPROC)load("glUniform4i64vARB");
	glad_glUniform1ui64ARB = (PFNGLUNIFORM1UI64ARBPROC)load("glUniform1ui64ARB");
	glad_glUniform2ui64ARB = (PFNGLUNIFORM2UI64ARBPROC)load("glUniform2ui64ARB");
	glad_glUniform3ui64ARB = (PFNGLUNIFORM3UI64ARBPROC)load("glUniform3ui64ARB");
	glad_glUniform4ui64ARB = (PFNGLUNIFORM4UI64ARBPROC)load("glUniform4ui64ARB");
	glad_glUniform1ui64vARB = (PFNGLUNIFORM1UI64VARBPROC)load("glUniform1ui64vARB");
	glad_glUniform2ui64vARB = (PFNGLUNIFORM2UI64VARBPROC)load("glUniform2ui64vARB");
	glad_glUniform3ui64vARB = (PFNGLUNIFORM3UI64VARBPROC)load("glUniform3ui64vARB");
	glad_glUniform4ui64vARB = (PFNGLUNIFORM4UI64VARBPROC)load("glUniform4ui64vARB");
	glad_glGetUniformi64vARB = (PFNGLGETUNIFORMI64VARBPROC)load("glGetUniformi64vARB");
	glad_glGetUniformui64vARB = (PFNGLGETUNIFORMUI64VARBPROC)load("glGetUniformui64vARB");
	glad_glGetnUniformi64vARB = (PFNGLGETNUNIFORMI64VARBPROC)load("glGetnUniformi64vARB");
	glad_glGetnUniformui64vARB = (PFNGLGETNUNIFORMUI64VARBPROC)load("glGetnUniformui64vARB");
	glad_glProgramUniform1i64ARB = (PFNGLPROGRAMUNIFORM1I64ARBPROC)load("glProgramUniform1i64ARB");
	glad_glProgramUniform2i64ARB = (PFNGLPROGRAMUNIFORM2I64ARBPROC)load("glProgramUniform2i64ARB");
	glad_glProgramUniform3i64ARB = (PFNGLPROGRAMUNIFORM3I64ARBPROC)load("glProgramUniform3i64ARB");
	glad_glProgramUniform4i64ARB = (PFNGLPROGRAMUNIFORM4I64ARBPROC)load("glProgramUniform4i64ARB");
	glad_glProgramUniform1i64vARB = (PFNGLPROGRAMUNIFORM1I64VARBPROC)load("glProgramUniform1i64vARB");
	glad_glProgramUniform2i64vARB = (PFNGLPROGRAMUNIFORM2I64VARBPROC)load("glProgramUniform2i64vARB");
	glad_glProgramUniform3i64vARB = (PFNGLPROGRAMUNIFORM3I64VARBPROC)load("glProgramUniform3i64vARB");
	glad_glProgramUniform4i64vARB = (PFNGLPROGRAMUNIFORM4I64VARBPROC)load("glProgramUniform4i64vARB");
	glad_glProgramUniform1ui64ARB = (PFNGLPROGRAMUNIFORM1UI64ARBPROC)load("glProgramUniform1ui64ARB");
	glad_glProgramUniform2ui64ARB = (PFNGLPROGRAMUNIFORM2UI64ARBPROC)load("glProgramUniform2ui64ARB");
	glad_glProgramUniform3ui64ARB = (PFNGLPROGRAMUNIFORM3UI64ARBPROC)load("glProgramUniform3ui64ARB");
	glad_glProgramUniform4ui64ARB = (PFNGLPROGRAMUNIFORM4UI64ARBPROC)load("glProgramUniform4ui64ARB");
	glad_glProgramUniform1ui64vARB = (PFNGLPROGRAMUNIFORM1UI64VARBPROC)load("glProgramUniform1ui64vARB");
	glad_glProgramUniform2ui64vARB = (PFNGLPROGRAMUNIFORM2UI64VARBPROC)load("glProgramUniform2ui64vARB");
	glad_glProgramUniform3ui64vARB = (PFNGLPROGRAMUNIFORM3UI64VARBPROC)load("glProgramUniform3ui64vARB");
	glad_glProgramUniform4ui64vARB = (PFNGLPROGRAMUNIFORM4UI64VARBPROC)load("glProgramUniform4ui64vARB");
}
static void load_GL_ARB_imaging(GLADloadproc load) {
	if(!GLAD_GL_ARB_imaging) return;
	glad_glBlendColor = (PFNGLBLENDCOLORPROC)load("glBlendColor");
	glad_glBlendEquation = (PFNGLBLENDEQUATIONPROC)load("glBlendEquation");
	glad_glColorTable = (PFNGLCOLORTABLEPROC)load("glColorTable");
	glad_glColorTableParameterfv = (PFNGLCOLORTABLEPARAMETERFVPROC)load("glColorTableParameterfv");
	glad_glColorTableParameteriv = (PFNGLCOLORTABLEPARAMETERIVPROC)load("glColorTableParameteriv");
	glad_glCopyColorTable = (PFNGLCOPYCOLORTABLEPROC)load("glCopyColorTable");
	glad_glGetColorTable = (PFNGLGETCOLORTABLEPROC)load("glGetColorTable");
	glad_glGetColorTableParameterfv = (PFNGLGETCOLORTABLEPARAMETERFVPROC)load("glGetColorTableParameterfv");
	glad_glGetColorTableParameteriv = (PFNGLGETCOLORTABLEPARAMETERIVPROC)load("glGetColorTableParameteriv");
	glad_glColorSubTable = (PFNGLCOLORSUBTABLEPROC)load("glColorSubTable");
	glad_glCopyColorSubTable = (PFNGLCOPYCOLORSUBTABLEPROC)load("glCopyColorSubTable");
	glad_glConvolutionFilter1D = (PFNGLCONVOLUTIONFILTER1DPROC)load("glConvolutionFilter1D");
	glad_glConvolutionFilter2D = (PFNGLCONVOLUTIONFILTER2DPROC)load("glConvolutionFilter2D");
	glad_glConvolutionParameterf = (PFNGLCONVOLUTIONPARAMETERFPROC)load("glConvolutionParameterf");
	glad_glConvolutionParameterfv = (PFNGLCONVOLUTIONPARAMETERFVPROC)load("glConvolutionParameterfv");
	glad_glConvolutionParameteri = (PFNGLCONVOLUTIONPARAMETERIPROC)load("glConvolutionParameteri");
	glad_glConvolutionParameteriv = (PFNGLCONVOLUTIONPARAMETERIVPROC)load("glConvolutionParameteriv");
	glad_glCopyConvolutionFilter1D = (PFNGLCOPYCONVOLUTIONFILTER1DPROC)load("glCopyConvolutionFilter1D");
	glad_glCopyConvolutionFilter2D = (PFNGLCOPYCONVOLUTIONFILTER2DPROC)load("glCopyConvolutionFilter2D");
	glad_glGetConvolutionFilter = (PFNGLGETCONVOLUTIONFILTERPROC)load("glGetConvolutionFilter");
	glad_glGetConvolutionParameterfv = (PFNGLGETCONVOLUTIONPARAMETERFVPROC)load("glGetConvolutionParameterfv");
	glad_glGetConvolutionParameteriv = (PFNGLGETCONVOLUTIONPARAMETERIVPROC)load("glGetConvolutionParameteriv");
	glad_glGetSeparableFilter = (PFNGLGETSEPARABLEFILTERPROC)load("glGetSeparableFilter");
	glad_glSeparableFilter2D = (PFNGLSEPARABLEFILTER2DPROC)load("glSeparableFilter2D");
	glad_glGetHistogram = (PFNGLGETHISTOGRAMPROC)load("glGetHistogram");
	glad_glGetHistogramParameterfv = (PFNGLGETHISTOGRAMPARAMETERFVPROC)load("glGetHistogramParameterfv");
	glad_glGetHistogramParameteriv = (PFNGLGETHISTOGRAMPARAMETERIVPROC)load("glGetHistogramParameteriv");
	glad_glGetMinmax = (PFNGLGETMINMAXPROC)load("glGetMinmax");
	glad_glGetMinmaxParameterfv = (PFNGLGETMINMAXPARAMETERFVPROC)load("glGetMinmaxParameterfv");
	glad_glGetMinmaxParameteriv = (PFNGLGETMINMAXPARAMETERIVPROC)load("glGetMinmaxParameteriv");
	glad_glHistogram = (PFNGLHISTOGRAMPROC)load("glHistogram");
	glad_glMinmax = (PFNGLMINMAXPROC)load("glMinmax");
	glad_glResetHistogram = (PFNGLRESETHISTOGRAMPROC)load("glResetHistogram");
	glad_glResetMinmax = (PFNGLRESETMINMAXPROC)load("glResetMinmax");
}
static void load_GL_ARB_indirect_parameters(GLADloadproc load) {
	if(!GLAD_GL_ARB_indirect_parameters) return;
	glad_glMultiDrawArraysIndirectCountARB = (PFNGLMULTIDRAWARRAYSINDIRECTCOUNTARBPROC)load("glMultiDrawArraysIndirectCountARB");
	glad_glMultiDrawElementsIndirectCountARB = (PFNGLMULTIDRAWELEMENTSINDIRECTCOUNTARBPROC)load("glMultiDrawElementsIndirectCountARB");
}
static void load_GL_ARB_instanced_arrays(GLADloadproc load) {
	if(!GLAD_GL_ARB_instanced_arrays) return;
	glad_glVertexAttribDivisorARB = (PFNGLVERTEXATTRIBDIVISORARBPROC)load("glVertexAttribDivisorARB");
}
static void load_GL_ARB_internalformat_query(GLADloadproc load) {
	if(!GLAD_GL_ARB_internalformat_query) return;
	glad_glGetInternalformativ = (PFNGLGETINTERNALFORMATIVPROC)load("glGetInternalformativ");
}
static void load_GL_ARB_internalformat_query2(GLADloadproc load) {
	if(!GLAD_GL_ARB_internalformat_query2) return;
	glad_glGetInternalformati64v = (PFNGLGETINTERNALFORMATI64VPROC)load("glGetInternalformati64v");
}
static void load_GL_ARB_invalidate_subdata(GLADloadproc load) {
	if(!GLAD_GL_ARB_invalidate_subdata) return;
	glad_glInvalidateTexSubImage = (PFNGLINVALIDATETEXSUBIMAGEPROC)load("glInvalidateTexSubImage");
	glad_glInvalidateTexImage = (PFNGLINVALIDATETEXIMAGEPROC)load("glInvalidateTexImage");
	glad_glInvalidateBufferSubData = (PFNGLINVALIDATEBUFFERSUBDATAPROC)load("glInvalidateBufferSubData");
	glad_glInvalidateBufferData = (PFNGLINVALIDATEBUFFERDATAPROC)load("glInvalidateBufferData");
	glad_glInvalidateFramebuffer = (PFNGLINVALIDATEFRAMEBUFFERPROC)load("glInvalidateFramebuffer");
	glad_glInvalidateSubFramebuffer = (PFNGLINVALIDATESUBFRAMEBUFFERPROC)load("glInvalidateSubFramebuffer");
}
static void load_GL_ARB_map_buffer_range(GLADloadproc load) {
	if(!GLAD_GL_ARB_map_buffer_range) return;
	glad_glMapBufferRange = (PFNGLMAPBUFFERRANGEPROC)load("glMapBufferRange");
	glad_glFlushMappedBufferRange = (PFNGLFLUSHMAPPEDBUFFERRANGEPROC)load("glFlushMappedBufferRange");
}
static void load_GL_ARB_matrix_palette(GLADloadproc load) {
	if(!GLAD_GL_ARB_matrix_palette) return;
	glad_glCurrentPaletteMatrixARB = (PFNGLCURRENTPALETTEMATRIXARBPROC)load("glCurrentPaletteMatrixARB");
	glad_glMatrixIndexubvARB = (PFNGLMATRIXINDEXUBVARBPROC)load("glMatrixIndexubvARB");
	glad_glMatrixIndexusvARB = (PFNGLMATRIXINDEXUSVARBPROC)load("glMatrixIndexusvARB");
	glad_glMatrixIndexuivARB = (PFNGLMATRIXINDEXUIVARBPROC)load("glMatrixIndexuivARB");
	glad_glMatrixIndexPointerARB = (PFNGLMATRIXINDEXPOINTERARBPROC)load("glMatrixIndexPointerARB");
}
static void load_GL_ARB_multi_bind(GLADloadproc load) {
	if(!GLAD_GL_ARB_multi_bind) return;
	glad_glBindBuffersBase = (PFNGLBINDBUFFERSBASEPROC)load("glBindBuffersBase");
	glad_glBindBuffersRange = (PFNGLBINDBUFFERSRANGEPROC)load("glBindBuffersRange");
	glad_glBindTextures = (PFNGLBINDTEXTURESPROC)load("glBindTextures");
	glad_glBindSamplers = (PFNGLBINDSAMPLERSPROC)load("glBindSamplers");
	glad_glBindImageTextures = (PFNGLBINDIMAGETEXTURESPROC)load("glBindImageTextures");
	glad_glBindVertexBuffers = (PFNGLBINDVERTEXBUFFERSPROC)load("glBindVertexBuffers");
}
static void load_GL_ARB_multi_draw_indirect(GLADloadproc load) {
	if(!GLAD_GL_ARB_multi_draw_indirect) return;
	glad_glMultiDrawArraysIndirect = (PFNGLMULTIDRAWARRAYSINDIRECTPROC)load("glMultiDrawArraysIndirect");
	glad_glMultiDrawElementsIndirect = (PFNGLMULTIDRAWELEMENTSINDIRECTPROC)load("glMultiDrawElementsIndirect");
}
static void load_GL_ARB_multisample(GLADloadproc load) {
	if(!GLAD_GL_ARB_multisample) return;
	glad_glSampleCoverageARB = (PFNGLSAMPLECOVERAGEARBPROC)load("glSampleCoverageARB");
}
static void load_GL_ARB_multitexture(GLADloadproc load) {
	if(!GLAD_GL_ARB_multitexture) return;
	glad_glActiveTextureARB = (PFNGLACTIVETEXTUREARBPROC)load("glActiveTextureARB");
	glad_glClientActiveTextureARB = (PFNGLCLIENTACTIVETEXTUREARBPROC)load("glClientActiveTextureARB");
	glad_glMultiTexCoord1dARB = (PFNGLMULTITEXCOORD1DARBPROC)load("glMultiTexCoord1dARB");
	glad_glMultiTexCoord1dvARB = (PFNGLMULTITEXCOORD1DVARBPROC)load("glMultiTexCoord1dvARB");
	glad_glMultiTexCoord1fARB = (PFNGLMULTITEXCOORD1FARBPROC)load("glMultiTexCoord1fARB");
	glad_glMultiTexCoord1fvARB = (PFNGLMULTITEXCOORD1FVARBPROC)load("glMultiTexCoord1fvARB");
	glad_glMultiTexCoord1iARB = (PFNGLMULTITEXCOORD1IARBPROC)load("glMultiTexCoord1iARB");
	glad_glMultiTexCoord1ivARB = (PFNGLMULTITEXCOORD1IVARBPROC)load("glMultiTexCoord1ivARB");
	glad_glMultiTexCoord1sARB = (PFNGLMULTITEXCOORD1SARBPROC)load("glMultiTexCoord1sARB");
	glad_glMultiTexCoord1svARB = (PFNGLMULTITEXCOORD1SVARBPROC)load("glMultiTexCoord1svARB");
	glad_glMultiTexCoord2dARB = (PFNGLMULTITEXCOORD2DARBPROC)load("glMultiTexCoord2dARB");
	glad_glMultiTexCoord2dvARB = (PFNGLMULTITEXCOORD2DVARBPROC)load("glMultiTexCoord2dvARB");
	glad_glMultiTexCoord2fARB = (PFNGLMULTITEXCOORD2FARBPROC)load("glMultiTexCoord2fARB");
	glad_glMultiTexCoord2fvARB = (PFNGLMULTITEXCOORD2FVARBPROC)load("glMultiTexCoord2fvARB");
	glad_glMultiTexCoord2iARB = (PFNGLMULTITEXCOORD2IARBPROC)load("glMultiTexCoord2iARB");
	glad_glMultiTexCoord2ivARB = (PFNGLMULTITEXCOORD2IVARBPROC)load("glMultiTexCoord2ivARB");
	glad_glMultiTexCoord2sARB = (PFNGLMULTITEXCOORD2SARBPROC)load("glMultiTexCoord2sARB");
	glad_glMultiTexCoord2svARB = (PFNGLMULTITEXCOORD2SVARBPROC)load("glMultiTexCoord2svARB");
	glad_glMultiTexCoord3dARB = (PFNGLMULTITEXCOORD3DARBPROC)load("glMultiTexCoord3dARB");
	glad_glMultiTexCoord3dvARB = (PFNGLMULTITEXCOORD3DVARBPROC)load("glMultiTexCoord3dvARB");
	glad_glMultiTexCoord3fARB = (PFNGLMULTITEXCOORD3FARBPROC)load("glMultiTexCoord3fARB");
	glad_glMultiTexCoord3fvARB = (PFNGLMULTITEXCOORD3FVARBPROC)load("glMultiTexCoord3fvARB");
	glad_glMultiTexCoord3iARB = (PFNGLMULTITEXCOORD3IARBPROC)load("glMultiTexCoord3iARB");
	glad_glMultiTexCoord3ivARB = (PFNGLMULTITEXCOORD3IVARBPROC)load("glMultiTexCoord3ivARB");
	glad_glMultiTexCoord3sARB = (PFNGLMULTITEXCOORD3SARBPROC)load("glMultiTexCoord3sARB");
	glad_glMultiTexCoord3svARB = (PFNGLMULTITEXCOORD3SVARBPROC)load("glMultiTexCoord3svARB");
	glad_glMultiTexCoord4dARB = (PFNGLMULTITEXCOORD4DARBPROC)load("glMultiTexCoord4dARB");
	glad_glMultiTexCoord4dvARB = (PFNGLMULTITEXCOORD4DVARBPROC)load("glMultiTexCoord4dvARB");
	glad_glMultiTexCoord4fARB = (PFNGLMULTITEXCOORD4FARBPROC)load("glMultiTexCoord4fARB");
	glad_glMultiTexCoord4fvARB = (PFNGLMULTITEXCOORD4FVARBPROC)load("glMultiTexCoord4fvARB");
	glad_glMultiTexCoord4iARB = (PFNGLMULTITEXCOORD4IARBPROC)load("glMultiTexCoord4iARB");
	glad_glMultiTexCoord4ivARB = (PFNGLMULTITEXCOORD4IVARBPROC)load("glMultiTexCoord4ivARB");
	glad_glMultiTexCoord4sARB = (PFNGLMULTITEXCOORD4SARBPROC)load("glMultiTexCoord4sARB");
	glad_glMultiTexCoord4svARB = (PFNGLMULTITEXCOORD4SVARBPROC)load("glMultiTexCoord4svARB");
}
static void load_GL_ARB_occlusion_query(GLADloadproc load) {
	if(!GLAD_GL_ARB_occlusion_query) return;
	glad_glGenQueriesARB = (PFNGLGENQUERIESARBPROC)load("glGenQueriesARB");
	glad_glDeleteQueriesARB = (PFNGLDELETEQUERIESARBPROC)load("glDeleteQueriesARB");
	glad_glIsQueryARB = (PFNGLISQUERYARBPROC)load("glIsQueryARB");
	glad_glBeginQueryARB = (PFNGLBEGINQUERYARBPROC)load("glBeginQueryARB");
	glad_glEndQueryARB = (PFNGLENDQUERYARBPROC)load("glEndQueryARB");
	glad_glGetQueryivARB = (PFNGLGETQUERYIVARBPROC)load("glGetQueryivARB");
	glad_glGetQueryObjectivARB = (PFNGLGETQUERYOBJECTIVARBPROC)load("glGetQueryObjectivARB");
	glad_glGetQueryObjectuivARB = (PFNGLGETQUERYOBJECTUIVARBPROC)load("glGetQueryObjectuivARB");
}
static void load_GL_ARB_parallel_shader_compile(GLADloadproc load) {
	if(!GLAD_GL_ARB_parallel_shader_compile) return;
	glad_glMaxShaderCompilerThreadsARB = (PFNGLMAXSHADERCOMPILERTHREADSARBPROC)load("glMaxShaderCompilerThreadsARB");
}
static void load_GL_ARB_point_parameters(GLADloadproc load) {
	if(!GLAD_GL_ARB_point_parameters) return;
	glad_glPointParameterfARB = (PFNGLPOINTPARAMETERFARBPROC)load("glPointParameterfARB");
	glad_glPointParameterfvARB = (PFNGLPOINTPARAMETERFVARBPROC)load("glPointParameterfvARB");
}
static void load_GL_ARB_polygon_offset_clamp(GLADloadproc load) {
	if(!GLAD_GL_ARB_polygon_offset_clamp) return;
	glad_glPolygonOffsetClamp = (PFNGLPOLYGONOFFSETCLAMPPROC)load("glPolygonOffsetClamp");
}
static void load_GL_ARB_program_interface_query(GLADloadproc load) {
	if(!GLAD_GL_ARB_program_interface_query) return;
	glad_glGetProgramInterfaceiv = (PFNGLGETPROGRAMINTERFACEIVPROC)load("glGetProgramInterfaceiv");
	glad_glGetProgramResourceIndex = (PFNGLGETPROGRAMRESOURCEINDEXPROC)load("glGetProgramResourceIndex");
	glad_glGetProgramResourceName = (PFNGLGETPROGRAMRESOURCENAMEPROC)load("glGetProgramResourceName");
	glad_glGetProgramResourceiv = (PFNGLGETPROGRAMRESOURCEIVPROC)load("glGetProgramResourceiv");
	glad_glGetProgramResourceLocation = (PFNGLGETPROGRAMRESOURCELOCATIONPROC)load("glGetProgramResourceLocation");
	glad_glGetProgramResourceLocationIndex = (PFNGLGETPROGRAMRESOURCELOCATIONINDEXPROC)load("glGetProgramResourceLocationIndex");
}
static void load_GL_ARB_provoking_vertex(GLADloadproc load) {
	if(!GLAD_GL_ARB_provoking_vertex) return;
	glad_glProvokingVertex = (PFNGLPROVOKINGVERTEXPROC)load("glProvokingVertex");
}
static void load_GL_ARB_robustness(GLADloadproc load) {
	if(!GLAD_GL_ARB_robustness) return;
	glad_glGetGraphicsResetStatusARB = (PFNGLGETGRAPHICSRESETSTATUSARBPROC)load("glGetGraphicsResetStatusARB");
	glad_glGetnTexImageARB = (PFNGLGETNTEXIMAGEARBPROC)load("glGetnTexImageARB");
	glad_glReadnPixelsARB = (PFNGLREADNPIXELSARBPROC)load("glReadnPixelsARB");
	glad_glGetnCompressedTexImageARB = (PFNGLGETNCOMPRESSEDTEXIMAGEARBPROC)load("glGetnCompressedTexImageARB");
	glad_glGetnUniformfvARB = (PFNGLGETNUNIFORMFVARBPROC)load("glGetnUniformfvARB");
	glad_glGetnUniformivARB = (PFNGLGETNUNIFORMIVARBPROC)load("glGetnUniformivARB");
	glad_glGetnUniformuivARB = (PFNGLGETNUNIFORMUIVARBPROC)load("glGetnUniformuivARB");
	glad_glGetnUniformdvARB = (PFNGLGETNUNIFORMDVARBPROC)load("glGetnUniformdvARB");
	glad_glGetnMapdvARB = (PFNGLGETNMAPDVARBPROC)load("glGetnMapdvARB");
	glad_glGetnMapfvARB = (PFNGLGETNMAPFVARBPROC)load("glGetnMapfvARB");
	glad_glGetnMapivARB = (PFNGLGETNMAPIVARBPROC)load("glGetnMapivARB");
	glad_glGetnPixelMapfvARB = (PFNGLGETNPIXELMAPFVARBPROC)load("glGetnPixelMapfvARB");
	glad_glGetnPixelMapuivARB = (PFNGLGETNPIXELMAPUIVARBPROC)load("glGetnPixelMapuivARB");
	glad_glGetnPixelMapusvARB = (PFNGLGETNPIXELMAPUSVARBPROC)load("glGetnPixelMapusvARB");
	glad_glGetnPolygonStippleARB = (PFNGLGETNPOLYGONSTIPPLEARBPROC)load("glGetnPolygonStippleARB");
	glad_glGetnColorTableARB = (PFNGLGETNCOLORTABLEARBPROC)load("glGetnColorTableARB");
	glad_glGetnConvolutionFilterARB = (PFNGLGETNCONVOLUTIONFILTERARBPROC)load("glGetnConvolutionFilterARB");
	glad_glGetnSeparableFilterARB = (PFNGLGETNSEPARABLEFILTERARBPROC)load("glGetnSeparableFilterARB");
	glad_glGetnHistogramARB = (PFNGLGETNHISTOGRAMARBPROC)load("glGetnHistogramARB");
	glad_glGetnMinmaxARB = (PFNGLGETNMINMAXARBPROC)load("glGetnMinmaxARB");
}
static void load_GL_ARB_sample_locations(GLADloadproc load) {
	if(!GLAD_GL_ARB_sample_locations) return;
	glad_glFramebufferSampleLocationsfvARB = (PFNGLFRAMEBUFFERSAMPLELOCATIONSFVARBPROC)load("glFramebufferSampleLocationsfvARB");
	glad_glNamedFramebufferSampleLocationsfvARB = (PFNGLNAMEDFRAMEBUFFERSAMPLELOCATIONSFVARBPROC)load("glNamedFramebufferSampleLocationsfvARB");
	glad_glEvaluateDepthValuesARB = (PFNGLEVALUATEDEPTHVALUESARBPROC)load("glEvaluateDepthValuesARB");
}
static void load_GL_ARB_sample_shading(GLADloadproc load) {
	if(!GLAD_GL_ARB_sample_shading) return;
	glad_glMinSampleShadingARB = (PFNGLMINSAMPLESHADINGARBPROC)load("glMinSampleShadingARB");
}
static void load_GL_ARB_sampler_objects(GLADloadproc load) {
	if(!GLAD_GL_ARB_sampler_objects) return;
	glad_glGenSamplers = (PFNGLGENSAMPLERSPROC)load("glGenSamplers");
	glad_glDeleteSamplers = (PFNGLDELETESAMPLERSPROC)load("glDeleteSamplers");
	glad_glIsSampler = (PFNGLISSAMPLERPROC)load("glIsSampler");
	glad_glBindSampler = (PFNGLBINDSAMPLERPROC)load("glBindSampler");
	glad_glSamplerParameteri = (PFNGLSAMPLERPARAMETERIPROC)load("glSamplerParameteri");
	glad_glSamplerParameteriv = (PFNGLSAMPLERPARAMETERIVPROC)load("glSamplerParameteriv");
	glad_glSamplerParameterf = (PFNGLSAMPLERPARAMETERFPROC)load("glSamplerParameterf");
	glad_glSamplerParameterfv = (PFNGLSAMPLERPARAMETERFVPROC)load("glSamplerParameterfv");
	glad_glSamplerParameterIiv = (PFNGLSAMPLERPARAMETERIIVPROC)load("glSamplerParameterIiv");
	glad_glSamplerParameterIuiv = (PFNGLSAMPLERPARAMETERIUIVPROC)load("glSamplerParameterIuiv");
	glad_glGetSamplerParameteriv = (PFNGLGETSAMPLERPARAMETERIVPROC)load("glGetSamplerParameteriv");
	glad_glGetSamplerParameterIiv = (PFNGLGETSAMPLERPARAMETERIIVPROC)load("glGetSamplerParameterIiv");
	glad_glGetSamplerParameterfv = (PFNGLGETSAMPLERPARAMETERFVPROC)load("glGetSamplerParameterfv");
	glad_glGetSamplerParameterIuiv = (PFNGLGETSAMPLERPARAMETERIUIVPROC)load("glGetSamplerParameterIuiv");
}
static void load_GL_ARB_separate_shader_objects(GLADloadproc load) {
	if(!GLAD_GL_ARB_separate_shader_objects) return;
	glad_glUseProgramStages = (PFNGLUSEPROGRAMSTAGESPROC)load("glUseProgramStages");
	glad_glActiveShaderProgram = (PFNGLACTIVESHADERPROGRAMPROC)load("glActiveShaderProgram");
	glad_glCreateShaderProgramv = (PFNGLCREATESHADERPROGRAMVPROC)load("glCreateShaderProgramv");
	glad_glBindProgramPipeline = (PFNGLBINDPROGRAMPIPELINEPROC)load("glBindProgramPipeline");
	glad_glDeleteProgramPipelines = (PFNGLDELETEPROGRAMPIPELINESPROC)load("glDeleteProgramPipelines");
	glad_glGenProgramPipelines = (PFNGLGENPROGRAMPIPELINESPROC)load("glGenProgramPipelines");
	glad_glIsProgramPipeline = (PFNGLISPROGRAMPIPELINEPROC)load("glIsProgramPipeline");
	glad_glGetProgramPipelineiv = (PFNGLGETPROGRAMPIPELINEIVPROC)load("glGetProgramPipelineiv");
	glad_glProgramParameteri = (PFNGLPROGRAMPARAMETERIPROC)load("glProgramParameteri");
	glad_glProgramUniform1i = (PFNGLPROGRAMUNIFORM1IPROC)load("glProgramUniform1i");
	glad_glProgramUniform1iv = (PFNGLPROGRAMUNIFORM1IVPROC)load("glProgramUniform1iv");
	glad_glProgramUniform1f = (PFNGLPROGRAMUNIFORM1FPROC)load("glProgramUniform1f");
	glad_glProgramUniform1fv = (PFNGLPROGRAMUNIFORM1FVPROC)load("glProgramUniform1fv");
	glad_glProgramUniform1d = (PFNGLPROGRAMUNIFORM1DPROC)load("glProgramUniform1d");
	glad_glProgramUniform1dv = (PFNGLPROGRAMUNIFORM1DVPROC)load("glProgramUniform1dv");
	glad_glProgramUniform1ui = (PFNGLPROGRAMUNIFORM1UIPROC)load("glProgramUniform1ui");
	glad_glProgramUniform1uiv = (PFNGLPROGRAMUNIFORM1UIVPROC)load("glProgramUniform1uiv");
	glad_glProgramUniform2i = (PFNGLPROGRAMUNIFORM2IPROC)load("glProgramUniform2i");
	glad_glProgramUniform2iv = (PFNGLPROGRAMUNIFORM2IVPROC)load("glProgramUniform2iv");
	glad_glProgramUniform2f = (PFNGLPROGRAMUNIFORM2FPROC)load("glProgramUniform2f");
	glad_glProgramUniform2fv = (PFNGLPROGRAMUNIFORM2FVPROC)load("glProgramUniform2fv");
	glad_glProgramUniform2d = (PFNGLPROGRAMUNIFORM2DPROC)load("glProgramUniform2d");
	glad_glProgramUniform2dv = (PFNGLPROGRAMUNIFORM2DVPROC)load("glProgramUniform2dv");
	glad_glProgramUniform2ui = (PFNGLPROGRAMUNIFORM2UIPROC)load("glProgramUniform2ui");
	glad_glProgramUniform2uiv = (PFNGLPROGRAMUNIFORM2UIVPROC)load("glProgramUniform2uiv");
	glad_glProgramUniform3i = (PFNGLPROGRAMUNIFORM3IPROC)load("glProgramUniform3i");
	glad_glProgramUniform3iv = (PFNGLPROGRAMUNIFORM3IVPROC)load("glProgramUniform3iv");
	glad_glProgramUniform3f = (PFNGLPROGRAMUNIFORM3FPROC)load("glProgramUniform3f");
	glad_glProgramUniform3fv = (PFNGLPROGRAMUNIFORM3FVPROC)load("glProgramUniform3fv");
	glad_glProgramUniform3d = (PFNGLPROGRAMUNIFORM3DPROC)load("glProgramUniform3d");
	glad_glProgramUniform3dv = (PFNGLPROGRAMUNIFORM3DVPROC)load("glProgramUniform3dv");
	glad_glProgramUniform3ui = (PFNGLPROGRAMUNIFORM3UIPROC)load("glProgramUniform3ui");
	glad_glProgramUniform3uiv = (PFNGLPROGRAMUNIFORM3UIVPROC)load("glProgramUniform3uiv");
	glad_glProgramUniform4i = (PFNGLPROGRAMUNIFORM4IPROC)load("glProgramUniform4i");
	glad_glProgramUniform4iv = (PFNGLPROGRAMUNIFORM4IVPROC)load("glProgramUniform4iv");
	glad_glProgramUniform4f = (PFNGLPROGRAMUNIFORM4FPROC)load("glProgramUniform4f");
	glad_glProgramUniform4fv = (PFNGLPROGRAMUNIFORM4FVPROC)load("glProgramUniform4fv");
	glad_glProgramUniform4d = (PFNGLPROGRAMUNIFORM4DPROC)load("glProgramUniform4d");
	glad_glProgramUniform4dv = (PFNGLPROGRAMUNIFORM4DVPROC)load("glProgramUniform4dv");
	glad_glProgramUniform4ui = (PFNGLPROGRAMUNIFORM4UIPROC)load("glProgramUniform4ui");
	glad_glProgramUniform4uiv = (PFNGLPROGRAMUNIFORM4UIVPROC)load("glProgramUniform4uiv");
	glad_glProgramUniformMatrix2fv = (PFNGLPROGRAMUNIFORMMATRIX2FVPROC)load("glProgramUniformMatrix2fv");
	glad_glProgramUniformMatrix3fv = (PFNGLPROGRAMUNIFORMMATRIX3FVPROC)load("glProgramUniformMatrix3fv");
	glad_glProgramUniformMatrix4fv = (PFNGLPROGRAMUNIFORMMATRIX4FVPROC)load("glProgramUniformMatrix4fv");
	glad_glProgramUniformMatrix2dv = (PFNGLPROGRAMUNIFORMMATRIX2DVPROC)load("glProgramUniformMatrix2dv");
	glad_glProgramUniformMatrix3dv = (PFNGLPROGRAMUNIFORMMATRIX3DVPROC)load("glProgramUniformMatrix3dv");
	glad_glProgramUniformMatrix4dv = (PFNGLPROGRAMUNIFORMMATRIX4DVPROC)load("glProgramUniformMatrix4dv");
	glad_glProgramUniformMatrix2x3fv = (PFNGLPROGRAMUNIFORMMATRIX2X3FVPROC)load("glProgramUniformMatrix2x3fv");
	glad_glProgramUniformMatrix3x2fv = (PFNGLPROGRAMUNIFORMMATRIX3X2FVPROC)load("glProgramUniformMatrix3x2fv");
	glad_glProgramUniformMatrix2x4fv = (PFNGLPROGRAMUNIFORMMATRIX2X4FVPROC)load("glProgramUniformMatrix2x4fv");
	glad_glProgramUniformMatrix4x2fv = (PFNGLPROGRAMUNIFORMMATRIX4X2FVPROC)load("glProgramUniformMatrix4x2fv");
	glad_glProgramUniformMatrix3x4fv = (PFNGLPROGRAMUNIFORMMATRIX3X4FVPROC)load("glProgramUniformMatrix3x4fv");
	glad_glProgramUniformMatrix4x3fv = (PFNGLPROGRAMUNIFORMMATRIX4X3FVPROC)load("glProgramUniformMatrix4x3fv");
	glad_glProgramUniformMatrix2x3dv = (PFNGLPROGRAMUNIFORMMATRIX2X3DVPROC)load("glProgramUniformMatrix2x3dv");
	glad_glProgramUniformMatrix3x2dv = (PFNGLPROGRAMUNIFORMMATRIX3X2DVPROC)load("glProgramUniformMatrix3x2dv");
	glad_glProgramUniformMatrix2x4dv = (PFNGLPROGRAMUNIFORMMATRIX2X4DVPROC)load("glProgramUniformMatrix2x4dv");
	glad_glProgramUniformMatrix4x2dv = (PFNGLPROGRAMUNIFORMMATRIX4X2DVPROC)load("glProgramUniformMatrix4x2dv");
	glad_glProgramUniformMatrix3x4dv = (PFNGLPROGRAMUNIFORMMATRIX3X4DVPROC)load("glProgramUniformMatrix3x4dv");
	glad_glProgramUniformMatrix4x3dv = (PFNGLPROGRAMUNIFORMMATRIX4X3DVPROC)load("glProgramUniformMatrix4x3dv");
	glad_glValidateProgramPipeline = (PFNGLVALIDATEPROGRAMPIPELINEPROC)load("glValidateProgramPipeline");
	glad_glGetProgramPipelineInfoLog = (PFNGLGETPROGRAMPIPELINEINFOLOGPROC)load("glGetProgramPipelineInfoLog");
}
static void load_GL_ARB_shader_atomic_counters(GLADloadproc load) {
	if(!GLAD_GL_ARB_shader_atomic_counters) return;
	glad_glGetActiveAtomicCounterBufferiv = (PFNGLGETACTIVEATOMICCOUNTERBUFFERIVPROC)load("glGetActiveAtomicCounterBufferiv");
}
static void load_GL_ARB_shader_image_load_store(GLADloadproc load) {
	if(!GLAD_GL_ARB_shader_image_load_store) return;
	glad_glBindImageTexture = (PFNGLBINDIMAGETEXTUREPROC)load("glBindImageTexture");
	glad_glMemoryBarrier = (PFNGLMEMORYBARRIERPROC)load("glMemoryBarrier");
}
static void load_GL_ARB_shader_objects(GLADloadproc load) {
	if(!GLAD_GL_ARB_shader_objects) return;
	glad_glDeleteObjectARB = (PFNGLDELETEOBJECTARBPROC)load("glDeleteObjectARB");
	glad_glGetHandleARB = (PFNGLGETHANDLEARBPROC)load("glGetHandleARB");
	glad_glDetachObjectARB = (PFNGLDETACHOBJECTARBPROC)load("glDetachObjectARB");
	glad_glCreateShaderObjectARB = (PFNGLCREATESHADEROBJECTARBPROC)load("glCreateShaderObjectARB");
	glad_glShaderSourceARB = (PFNGLSHADERSOURCEARBPROC)load("glShaderSourceARB");
	glad_glCompileShaderARB = (PFNGLCOMPILESHADERARBPROC)load("glCompileShaderARB");
	glad_glCreateProgramObjectARB = (PFNGLCREATEPROGRAMOBJECTARBPROC)load("glCreateProgramObjectARB");
	glad_glAttachObjectARB = (PFNGLATTACHOBJECTARBPROC)load("glAttachObjectARB");
	glad_glLinkProgramARB = (PFNGLLINKPROGRAMARBPROC)load("glLinkProgramARB");
	glad_glUseProgramObjectARB = (PFNGLUSEPROGRAMOBJECTARBPROC)load("glUseProgramObjectARB");
	glad_glValidateProgramARB = (PFNGLVALIDATEPROGRAMARBPROC)load("glValidateProgramARB");
	glad_glUniform1fARB = (PFNGLUNIFORM1FARBPROC)load("glUniform1fARB");
	glad_glUniform2fARB = (PFNGLUNIFORM2FARBPROC)load("glUniform2fARB");
	glad_glUniform3fARB = (PFNGLUNIFORM3FARBPROC)load("glUniform3fARB");
	glad_glUniform4fARB = (PFNGLUNIFORM4FARBPROC)load("glUniform4fARB");
	glad_glUniform1iARB = (PFNGLUNIFORM1IARBPROC)load("glUniform1iARB");
	glad_glUniform2iARB = (PFNGLUNIFORM2IARBPROC)load("glUniform2iARB");
	glad_glUniform3iARB = (PFNGLUNIFORM3IARBPROC)load("glUniform3iARB");
	glad_glUniform4iARB = (PFNGLUNIFORM4IARBPROC)load("glUniform4iARB");
	glad_glUniform1fvARB = (PFNGLUNIFORM1FVARBPROC)load("glUniform1fvARB");
	glad_glUniform2fvARB = (PFNGLUNIFORM2FVARBPROC)load("glUniform2fvARB");
	glad_glUniform3fvARB = (PFNGLUNIFORM3FVARBPROC)load("glUniform3fvARB");
	glad_glUniform4fvARB = (PFNGLUNIFORM4FVARBPROC)load("glUniform4fvARB");
	glad_glUniform1ivARB = (PFNGLUNIFORM1IVARBPROC)load("glUniform1ivARB");
	glad_glUniform2ivARB = (PFNGLUNIFORM2IVARBPROC)load("glUniform2ivARB");
	glad_glUniform3ivARB = (PFNGLUNIFORM3IVARBPROC)load("glUniform3ivARB");
	glad_glUniform4ivARB = (PFNGLUNIFORM4IVARBPROC)load("glUniform4ivARB");
	glad_glUniformMatrix2fvARB = (PFNGLUNIFORMMATRIX2FVARBPROC)load("glUniformMatrix2fvARB");
	glad_glUniformMatrix3fvARB = (PFNGLUNIFORMMATRIX3FVARBPROC)load("glUniformMatrix3fvARB");
	glad_glUniformMatrix4fvARB = (PFNGLUNIFORMMATRIX4FVARBPROC)load("glUniformMatrix4fvARB");
	glad_glGetObjectParameterfvARB = (PFNGLGETOBJECTPARAMETERFVARBPROC)load("glGetObjectParameterfvARB");
	glad_glGetObjectParameterivARB = (PFNGLGETOBJECTPARAMETERIVARBPROC)load("glGetObjectParameterivARB");
	glad_glGetInfoLogARB = (PFNGLGETINFOLOGARBPROC)load("glGetInfoLogARB");
	glad_glGetAttachedObjectsARB = (PFNGLGETATTACHEDOBJECTSARBPROC)load("glGetAttachedObjectsARB");
	glad_glGetUniformLocationARB = (PFNGLGETUNIFORMLOCATIONARBPROC)load("glGetUniformLocationARB");
	glad_glGetActiveUniformARB = (PFNGLGETACTIVEUNIFORMARBPROC)load("glGetActiveUniformARB");
	glad_glGetUniformfvARB = (PFNGLGETUNIFORMFVARBPROC)load("glGetUniformfvARB");
	glad_glGetUniformivARB = (PFNGLGETUNIFORMIVARBPROC)load("glGetUniformivARB");
	glad_glGetShaderSourceARB = (PFNGLGETSHADERSOURCEARBPROC)load("glGetShaderSourceARB");
}
static void load_GL_ARB_shader_storage_buffer_object(GLADloadproc load) {
	if(!GLAD_GL_ARB_shader_storage_buffer_object) return;
	glad_glShaderStorageBlockBinding = (PFNGLSHADERSTORAGEBLOCKBINDINGPROC)load("glShaderStorageBlockBinding");
}
static void load_GL_ARB_shader_subroutine(GLADloadproc load) {
	if(!GLAD_GL_ARB_shader_subroutine) return;
	glad_glGetSubroutineUniformLocation = (PFNGLGETSUBROUTINEUNIFORMLOCATIONPROC)load("glGetSubroutineUniformLocation");
	glad_glGetSubroutineIndex = (PFNGLGETSUBROUTINEINDEXPROC)load("glGetSubroutineIndex");
	glad_glGetActiveSubroutineUniformiv = (PFNGLGETACTIVESUBROUTINEUNIFORMIVPROC)load("glGetActiveSubroutineUniformiv");
	glad_glGetActiveSubroutineUniformName = (PFNGLGETACTIVESUBROUTINEUNIFORMNAMEPROC)load("glGetActiveSubroutineUniformName");
	glad_glGetActiveSubroutineName = (PFNGLGETACTIVESUBROUTINENAMEPROC)load("glGetActiveSubroutineName");
	glad_glUniformSubroutinesuiv = (PFNGLUNIFORMSUBROUTINESUIVPROC)load("glUniformSubroutinesuiv");
	glad_glGetUniformSubroutineuiv = (PFNGLGETUNIFORMSUBROUTINEUIVPROC)load("glGetUniformSubroutineuiv");
	glad_glGetProgramStageiv = (PFNGLGETPROGRAMSTAGEIVPROC)load("glGetProgramStageiv");
}
static void load_GL_ARB_shading_language_include(GLADloadproc load) {
	if(!GLAD_GL_ARB_shading_language_include) return;
	glad_glNamedStringARB = (PFNGLNAMEDSTRINGARBPROC)load("glNamedStringARB");
	glad_glDeleteNamedStringARB = (PFNGLDELETENAMEDSTRINGARBPROC)load("glDeleteNamedStringARB");
	glad_glCompileShaderIncludeARB = (PFNGLCOMPILESHADERINCLUDEARBPROC)load("glCompileShaderIncludeARB");
	glad_glIsNamedStringARB = (PFNGLISNAMEDSTRINGARBPROC)load("glIsNamedStringARB");
	glad_glGetNamedStringARB = (PFNGLGETNAMEDSTRINGARBPROC)load("glGetNamedStringARB");
	glad_glGetNamedStringivARB = (PFNGLGETNAMEDSTRINGIVARBPROC)load("glGetNamedStringivARB");
}
static void load_GL_ARB_sparse_buffer(GLADloadproc load) {
	if(!GLAD_GL_ARB_sparse_buffer) return;
	glad_glBufferPageCommitmentARB = (PFNGLBUFFERPAGECOMMITMENTARBPROC)load("glBufferPageCommitmentARB");
	glad_glNamedBufferPageCommitmentEXT = (PFNGLNAMEDBUFFERPAGECOMMITMENTEXTPROC)load("glNamedBufferPageCommitmentEXT");
	glad_glNamedBufferPageCommitmentARB = (PFNGLNAMEDBUFFERPAGECOMMITMENTARBPROC)load("glNamedBufferPageCommitmentARB");
}
static void load_GL_ARB_sparse_texture(GLADloadproc load) {
	if(!GLAD_GL_ARB_sparse_texture) return;
	glad_glTexPageCommitmentARB = (PFNGLTEXPAGECOMMITMENTARBPROC)load("glTexPageCommitmentARB");
}
static void load_GL_ARB_sync(GLADloadproc load) {
	if(!GLAD_GL_ARB_sync) return;
	glad_glFenceSync = (PFNGLFENCESYNCPROC)load("glFenceSync");
	glad_glIsSync = (PFNGLISSYNCPROC)load("glIsSync");
	glad_glDeleteSync = (PFNGLDELETESYNCPROC)load("glDeleteSync");
	glad_glClientWaitSync = (PFNGLCLIENTWAITSYNCPROC)load("glClientWaitSync");
	glad_glWaitSync = (PFNGLWAITSYNCPROC)load("glWaitSync");
	glad_glGetInteger64v = (PFNGLGETINTEGER64VPROC)load("glGetInteger64v");
	glad_glGetSynciv = (PFNGLGETSYNCIVPROC)load("glGetSynciv");
}
static void load_GL_ARB_tessellation_shader(GLADloadproc load) {
	if(!GLAD_GL_ARB_tessellation_shader) return;
	glad_glPatchParameteri = (PFNGLPATCHPARAMETERIPROC)load("glPatchParameteri");
	glad_glPatchParameterfv = (PFNGLPATCHPARAMETERFVPROC)load("glPatchParameterfv");
}
static void load_GL_ARB_texture_barrier(GLADloadproc load) {
	if(!GLAD_GL_ARB_texture_barrier) return;
	glad_glTextureBarrier = (PFNGLTEXTUREBARRIERPROC)load("glTextureBarrier");
}
static void load_GL_ARB_texture_buffer_object(GLADloadproc load) {
	if(!GLAD_GL_ARB_texture_buffer_object) return;
	glad_glTexBufferARB = (PFNGLTEXBUFFERARBPROC)load("glTexBufferARB");
}
static void load_GL_ARB_texture_buffer_range(GLADloadproc load) {
	if(!GLAD_GL_ARB_texture_buffer_range) return;
	glad_glTexBufferRange = (PFNGLTEXBUFFERRANGEPROC)load("glTexBufferRange");
}
static void load_GL_ARB_texture_compression(GLADloadproc load) {
	if(!GLAD_GL_ARB_texture_compression) return;
	glad_glCompressedTexImage3DARB = (PFNGLCOMPRESSEDTEXIMAGE3DARBPROC)load("glCompressedTexImage3DARB");
	glad_glCompressedTexImage2DARB = (PFNGLCOMPRESSEDTEXIMAGE2DARBPROC)load("glCompressedTexImage2DARB");
	glad_glCompressedTexImage1DARB = (PFNGLCOMPRESSEDTEXIMAGE1DARBPROC)load("glCompressedTexImage1DARB");
	glad_glCompressedTexSubImage3DARB = (PFNGLCOMPRESSEDTEXSUBIMAGE3DARBPROC)load("glCompressedTexSubImage3DARB");
	glad_glCompressedTexSubImage2DARB = (PFNGLCOMPRESSEDTEXSUBIMAGE2DARBPROC)load("glCompressedTexSubImage2DARB");
	glad_glCompressedTexSubImage1DARB = (PFNGLCOMPRESSEDTEXSUBIMAGE1DARBPROC)load("glCompressedTexSubImage1DARB");
	glad_glGetCompressedTexImageARB = (PFNGLGETCOMPRESSEDTEXIMAGEARBPROC)load("glGetCompressedTexImageARB");
}
static void load_GL_ARB_texture_multisample(GLADloadproc load) {
	if(!GLAD_GL_ARB_texture_multisample) return;
	glad_glTexImage2DMultisample = (PFNGLTEXIMAGE2DMULTISAMPLEPROC)load("glTexImage2DMultisample");
	glad_glTexImage3DMultisample = (PFNGLTEXIMAGE3DMULTISAMPLEPROC)load("glTexImage3DMultisample");
	glad_glGetMultisamplefv = (PFNGLGETMULTISAMPLEFVPROC)load("glGetMultisamplefv");
	glad_glSampleMaski = (PFNGLSAMPLEMASKIPROC)load("glSampleMaski");
}
static void load_GL_ARB_texture_storage(GLADloadproc load) {
	if(!GLAD_GL_ARB_texture_storage) return;
	glad_glTexStorage1D = (PFNGLTEXSTORAGE1DPROC)load("glTexStorage1D");
	glad_glTexStorage2D = (PFNGLTEXSTORAGE2DPROC)load("glTexStorage2D");
	glad_glTexStorage3D = (PFNGLTEXSTORAGE3DPROC)load("glTexStorage3D");
}
static void load_GL_ARB_texture_storage_multisample(GLADloadproc load) {
	if(!GLAD_GL_ARB_texture_storage_multisample) return;
	glad_glTexStorage2DMultisample = (PFNGLTEXSTORAGE2DMULTISAMPLEPROC)load("glTexStorage2DMultisample");
	glad_glTexStorage3DMultisample = (PFNGLTEXSTORAGE3DMULTISAMPLEPROC)load("glTexStorage3DMultisample");
}
static void load_GL_ARB_texture_view(GLADloadproc load) {
	if(!GLAD_GL_ARB_texture_view) return;
	glad_glTextureView = (PFNGLTEXTUREVIEWPROC)load("glTextureView");
}
static void load_GL_ARB_timer_query(GLADloadproc load) {
	if(!GLAD_GL_ARB_timer_query) return;
	glad_glQueryCounter = (PFNGLQUERYCOUNTERPROC)load("glQueryCounter");
	glad_glGetQueryObjecti64v = (PFNGLGETQUERYOBJECTI64VPROC)load("glGetQueryObjecti64v");
	glad_glGetQueryObjectui64v = (PFNGLGETQUERYOBJECTUI64VPROC)load("glGetQueryObjectui64v");
}
static void load_GL_ARB_transform_feedback2(GLADloadproc load) {
	if(!GLAD_GL_ARB_transform_feedback2) return;
	glad_glBindTransformFeedback = (PFNGLBINDTRANSFORMFEEDBACKPROC)load("glBindTransformFeedback");
	glad_glDeleteTransformFeedbacks = (PFNGLDELETETRANSFORMFEEDBACKSPROC)load("glDeleteTransformFeedbacks");
	glad_glGenTransformFeedbacks = (PFNGLGENTRANSFORMFEEDBACKSPROC)load("glGenTransformFeedbacks");
	glad_glIsTransformFeedback = (PFNGLISTRANSFORMFEEDBACKPROC)load("glIsTransformFeedback");
	glad_glPauseTransformFeedback = (PFNGLPAUSETRANSFORMFEEDBACKPROC)load("glPauseTransformFeedback");
	glad_glResumeTransformFeedback = (PFNGLRESUMETRANSFORMFEEDBACKPROC)load("glResumeTransformFeedback");
	glad_glDrawTransformFeedback = (PFNGLDRAWTRANSFORMFEEDBACKPROC)load("glDrawTransformFeedback");
}
static void load_GL_ARB_transform_feedback3(GLADloadproc load) {
	if(!GLAD_GL_ARB_transform_feedback3) return;
	glad_glDrawTransformFeedbackStream = (PFNGLDRAWTRANSFORMFEEDBACKSTREAMPROC)load("glDrawTransformFeedbackStream");
	glad_glBeginQueryIndexed = (PFNGLBEGINQUERYINDEXEDPROC)load("glBeginQueryIndexed");
	glad_glEndQueryIndexed = (PFNGLENDQUERYINDEXEDPROC)load("glEndQueryIndexed");
	glad_glGetQueryIndexediv = (PFNGLGETQUERYINDEXEDIVPROC)load("glGetQueryIndexediv");
}
static void load_GL_ARB_transform_feedback_instanced(GLADloadproc load) {
	if(!GLAD_GL_ARB_transform_feedback_instanced) return;
	glad_glDrawTransformFeedbackInstanced = (PFNGLDRAWTRANSFORMFEEDBACKINSTANCEDPROC)load("glDrawTransformFeedbackInstanced");
	glad_glDrawTransformFeedbackStreamInstanced = (PFNGLDRAWTRANSFORMFEEDBACKSTREAMINSTANCEDPROC)load("glDrawTransformFeedbackStreamInstanced");
}
static void load_GL_ARB_transpose_matrix(GLADloadproc load) {
	if(!GLAD_GL_ARB_transpose_matrix) return;
	glad_glLoadTransposeMatrixfARB = (PFNGLLOADTRANSPOSEMATRIXFARBPROC)load("glLoadTransposeMatrixfARB");
	glad_glLoadTransposeMatrixdARB = (PFNGLLOADTRANSPOSEMATRIXDARBPROC)load("glLoadTransposeMatrixdARB");
	glad_glMultTransposeMatrixfARB = (PFNGLMULTTRANSPOSEMATRIXFARBPROC)load("glMultTransposeMatrixfARB");
	glad_glMultTransposeMatrixdARB = (PFNGLMULTTRANSPOSEMATRIXDARBPROC)load("glMultTransposeMatrixdARB");
}
static void load_GL_ARB_uniform_buffer_object(GLADloadproc load) {
	if(!GLAD_GL_ARB_uniform_buffer_object) return;
	glad_glGetUniformIndices = (PFNGLGETUNIFORMINDICESPROC)load("glGetUniformIndices");
	glad_glGetActiveUniformsiv = (PFNGLGETACTIVEUNIFORMSIVPROC)load("glGetActiveUniformsiv");
	glad_glGetActiveUniformName = (PFNGLGETACTIVEUNIFORMNAMEPROC)load("glGetActiveUniformName");
	glad_glGetUniformBlockIndex = (PFNGLGETUNIFORMBLOCKINDEXPROC)load("glGetUniformBlockIndex");
	glad_glGetActiveUniformBlockiv = (PFNGLGETACTIVEUNIFORMBLOCKIVPROC)load("glGetActiveUniformBlockiv");
	glad_glGetActiveUniformBlockName = (PFNGLGETACTIVEUNIFORMBLOCKNAMEPROC)load("glGetActiveUniformBlockName");
	glad_glUniformBlockBinding = (PFNGLUNIFORMBLOCKBINDINGPROC)load("glUniformBlockBinding");
	glad_glBindBufferRange = (PFNGLBINDBUFFERRANGEPROC)load("glBindBufferRange");
	glad_glBindBufferBase = (PFNGLBINDBUFFERBASEPROC)load("glBindBufferBase");
	glad_glGetIntegeri_v = (PFNGLGETINTEGERI_VPROC)load("glGetIntegeri_v");
}
static void load_GL_ARB_vertex_array_object(GLADloadproc load) {
	if(!GLAD_GL_ARB_vertex_array_object) return;
	glad_glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)load("glBindVertexArray");
	glad_glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)load("glDeleteVertexArrays");
	glad_glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)load("glGenVertexArrays");
	glad_glIsVertexArray = (PFNGLISVERTEXARRAYPROC)load("glIsVertexArray");
}
static void load_GL_ARB_vertex_attrib_64bit(GLADloadproc load) {
	if(!GLAD_GL_ARB_vertex_attrib_64bit) return;
	glad_glVertexAttribL1d = (PFNGLVERTEXATTRIBL1DPROC)load("glVertexAttribL1d");
	glad_glVertexAttribL2d = (PFNGLVERTEXATTRIBL2DPROC)load("glVertexAttribL2d");
	glad_glVertexAttribL3d = (PFNGLVERTEXATTRIBL3DPROC)load("glVertexAttribL3d");
	glad_glVertexAttribL4d = (PFNGLVERTEXATTRIBL4DPROC)load("glVertexAttribL4d");
	glad_glVertexAttribL1dv = (PFNGLVERTEXATTRIBL1DVPROC)load("glVertexAttribL1dv");
	glad_glVertexAttribL2dv = (PFNGLVERTEXATTRIBL2DVPROC)load("glVertexAttribL2dv");
	glad_glVertexAttribL3dv = (PFNGLVERTEXATTRIBL3DVPROC)load("glVertexAttribL3dv");
	glad_glVertexAttribL4dv = (PFNGLVERTEXATTRIBL4DVPROC)load("glVertexAttribL4dv");
	glad_glVertexAttribLPointer = (PFNGLVERTEXATTRIBLPOINTERPROC)load("glVertexAttribLPointer");
	glad_glGetVertexAttribLdv = (PFNGLGETVERTEXATTRIBLDVPROC)load("glGetVertexAttribLdv");
}
static void load_GL_ARB_vertex_attrib_binding(GLADloadproc load) {
	if(!GLAD_GL_ARB_vertex_attrib_binding) return;
	glad_glBindVertexBuffer = (PFNGLBINDVERTEXBUFFERPROC)load("glBindVertexBuffer");
	glad_glVertexAttribFormat = (PFNGLVERTEXATTRIBFORMATPROC)load("glVertexAttribFormat");
	glad_glVertexAttribIFormat = (PFNGLVERTEXATTRIBIFORMATPROC)load("glVertexAttribIFormat");
	glad_glVertexAttribLFormat = (PFNGLVERTEXATTRIBLFORMATPROC)load("glVertexAttribLFormat");
	glad_glVertexAttribBinding = (PFNGLVERTEXATTRIBBINDINGPROC)load("glVertexAttribBinding");
	glad_glVertexBindingDivisor = (PFNGLVERTEXBINDINGDIVISORPROC)load("glVertexBindingDivisor");
}
static void load_GL_ARB_vertex_blend(GLADloadproc load) {
	if(!GLAD_GL_ARB_vertex_blend) return;
	glad_glWeightbvARB = (PFNGLWEIGHTBVARBPROC)load("glWeightbvARB");
	glad_glWeightsvARB = (PFNGLWEIGHTSVARBPROC)load("glWeightsvARB");
	glad_glWeightivARB = (PFNGLWEIGHTIVARBPROC)load("glWeightivARB");
	glad_glWeightfvARB = (PFNGLWEIGHTFVARBPROC)load("glWeightfvARB");
	glad_glWeightdvARB = (PFNGLWEIGHTDVARBPROC)load("glWeightdvARB");
	glad_glWeightubvARB = (PFNGLWEIGHTUBVARBPROC)load("glWeightubvARB");
	glad_glWeightusvARB = (PFNGLWEIGHTUSVARBPROC)load("glWeightusvARB");
	glad_glWeightuivARB = (PFNGLWEIGHTUIVARBPROC)load("glWeightuivARB");
	glad_glWeightPointerARB = (PFNGLWEIGHTPOINTERARBPROC)load("glWeightPointerARB");
	glad_glVertexBlendARB = (PFNGLVERTEXBLENDARBPROC)load("glVertexBlendARB");
}
static void load_GL_ARB_vertex_buffer_object(GLADloadproc load) {
	if(!GLAD_GL_ARB_vertex_buffer_object) return;
	glad_glBindBufferARB = (PFNGLBINDBUFFERARBPROC)load("glBindBufferARB");
	glad_glDeleteBuffersARB = (PFNGLDELETEBUFFERSARBPROC)load("glDeleteBuffersARB");
	glad_glGenBuffersARB = (PFNGLGENBUFFERSARBPROC)load("glGenBuffersARB");
	glad_glIsBufferARB = (PFNGLISBUFFERARBPROC)load("glIsBufferARB");
	glad_glBufferDataARB = (PFNGLBUFFERDATAARBPROC)load("glBufferDataARB");
	glad_glBufferSubDataARB = (PFNGLBUFFERSUBDATAARBPROC)load("glBufferSubDataARB");
	glad_glGetBufferSubDataARB = (PFNGLGETBUFFERSUBDATAARBPROC)load("glGetBufferSubDataARB");
	glad_glMapBufferARB = (PFNGLMAPBUFFERARBPROC)load("glMapBufferARB");
	glad_glUnmapBufferARB = (PFNGLUNMAPBUFFERARBPROC)load("glUnmapBufferARB");
	glad_glGetBufferParameterivARB = (PFNGLGETBUFFERPARAMETERIVARBPROC)load("glGetBufferParameterivARB");
	glad_glGetBufferPointervARB = (PFNGLGETBUFFERPOINTERVARBPROC)load("glGetBufferPointervARB");
}
static void load_GL_ARB_vertex_program(GLADloadproc load) {
	if(!GLAD_GL_ARB_vertex_program) return;
	glad_glVertexAttrib1dARB = (PFNGLVERTEXATTRIB1DARBPROC)load("glVertexAttrib1dARB");
	glad_glVertexAttrib1dvARB = (PFNGLVERTEXATTRIB1DVARBPROC)load("glVertexAttrib1dvARB");
	glad_glVertexAttrib1fARB = (PFNGLVERTEXATTRIB1FARBPROC)load("glVertexAttrib1fARB");
	glad_glVertexAttrib1fvARB = (PFNGLVERTEXATTRIB1FVARBPROC)load("glVertexAttrib1fvARB");
	glad_glVertexAttrib1sARB = (PFNGLVERTEXATTRIB1SARBPROC)load("glVertexAttrib1sARB");
	glad_glVertexAttrib1svARB = (PFNGLVERTEXATTRIB1SVARBPROC)load("glVertexAttrib1svARB");
	glad_glVertexAttrib2dARB = (PFNGLVERTEXATTRIB2DARBPROC)load("glVertexAttrib2dARB");
	glad_glVertexAttrib2dvARB = (PFNGLVERTEXATTRIB2DVARBPROC)load("glVertexAttrib2dvARB");
	glad_glVertexAttrib2fARB = (PFNGLVERTEXATTRIB2FARBPROC)load("glVertexAttrib2fARB");
	glad_glVertexAttrib2fvARB = (PFNGLVERTEXATTRIB2FVARBPROC)load("glVertexAttrib2fvARB");
	glad_glVertexAttrib2sARB = (PFNGLVERTEXATTRIB2SARBPROC)load("glVertexAttrib2sARB");
	glad_glVertexAttrib2svARB = (PFNGLVERTEXATTRIB2SVARBPROC)load("glVertexAttrib2svARB");
	glad_glVertexAttrib3dARB = (PFNGLVERTEXATTRIB3DARBPROC)load("glVertexAttrib3dARB");
	glad_glVertexAttrib3dvARB = (PFNGLVERTEXATTRIB3DVARBPROC)load("glVertexAttrib3dvARB");
	glad_glVertexAttrib3fARB = (PFNGLVERTEXATTRIB3FARBPROC)load("glVertexAttrib3fARB");
	glad_glVertexAttrib3fvARB = (PFNGLVERTEXATTRIB3FVARBPROC)load("glVertexAttrib3fvARB");
	glad_glVertexAttrib3sARB = (PFNGLVERTEXATTRIB3SARBPROC)load("glVertexAttrib3sARB");
	glad_glVertexAttrib3svARB = (PFNGLVERTEXATTRIB3SVARBPROC)load("glVertexAttrib3svARB");
	glad_glVertexAttrib4NbvARB = (PFNGLVERTEXATTRIB4NBVARBPROC)load("glVertexAttrib4NbvARB");
	glad_glVertexAttrib4NivARB = (PFNGLVERTEXATTRIB4NIVARBPROC)load("glVertexAttrib4NivARB");
	glad_glVertexAttrib4NsvARB = (PFNGLVERTEXATTRIB4NSVARBPROC)load("glVertexAttrib4NsvARB");
	glad_glVertexAttrib4NubARB = (PFNGLVERTEXATTRIB4NUBARBPROC)load("glVertexAttrib4NubARB");
	glad_glVertexAttrib4NubvARB = (PFNGLVERTEXATTRIB4NUBVARBPROC)load("glVertexAttrib4NubvARB");
	glad_glVertexAttrib4NuivARB = (PFNGLVERTEXATTRIB4NUIVARBPROC)load("glVertexAttrib4NuivARB");
	glad_glVertexAttrib4NusvARB = (PFNGLVERTEXATTRIB4NUSVARBPROC)load("glVertexAttrib4NusvARB");
	glad_glVertexAttrib4bvARB = (PFNGLVERTEXATTRIB4BVARBPROC)load("glVertexAttrib4bvARB");
	glad_glVertexAttrib4dARB = (PFNGLVERTEXATTRIB4DARBPROC)load("glVertexAttrib4dARB");
	glad_glVertexAttrib4dvARB = (PFNGLVERTEXATTRIB4DVARBPROC)load("glVertexAttrib4dvARB");
	glad_glVertexAttrib4fARB = (PFNGLVERTEXATTRIB4FARBPROC)load("glVertexAttrib4fARB");
	glad_glVertexAttrib4fvARB = (PFNGLVERTEXATTRIB4FVARBPROC)load("glVertexAttrib4fvARB");
	glad_glVertexAttrib4ivARB = (PFNGLVERTEXATTRIB4IVARBPROC)load("glVertexAttrib4ivARB");
	glad_glVertexAttrib4sARB = (PFNGLVERTEXATTRIB4SARBPROC)load("glVertexAttrib4sARB");
	glad_glVertexAttrib4svARB = (PFNGLVERTEXATTRIB4SVARBPROC)load("glVertexAttrib4svARB");
	glad_glVertexAttrib4ubvARB = (PFNGLVERTEXATTRIB4UBVARBPROC)load("glVertexAttrib4ubvARB");
	glad_glVertexAttrib4uivARB = (PFNGLVERTEXATTRIB4UIVARBPROC)load("glVertexAttrib4uivARB");
	glad_glVertexAttrib4usvARB = (PFNGLVERTEXATTRIB4USVARBPROC)load("glVertexAttrib4usvARB");
	glad_glVertexAttribPointerARB = (PFNGLVERTEXATTRIBPOINTERARBPROC)load("glVertexAttribPointerARB");
	glad_glEnableVertexAttribArrayARB = (PFNGLENABLEVERTEXATTRIBARRAYARBPROC)load("glEnableVertexAttribArrayARB");
	glad_glDisableVertexAttribArrayARB = (PFNGLDISABLEVERTEXATTRIBARRAYARBPROC)load("glDisableVertexAttribArrayARB");
	glad_glProgramStringARB = (PFNGLPROGRAMSTRINGARBPROC)load("glProgramStringARB");
	glad_glBindProgramARB = (PFNGLBINDPROGRAMARBPROC)load("glBindProgramARB");
	glad_glDeleteProgramsARB = (PFNGLDELETEPROGRAMSARBPROC)load("glDeleteProgramsARB");
	glad_glGenProgramsARB = (PFNGLGENPROGRAMSARBPROC)load("glGenProgramsARB");
	glad_glProgramEnvParameter4dARB = (PFNGLPROGRAMENVPARAMETER4DARBPROC)load("glProgramEnvParameter4dARB");
	glad_glProgramEnvParameter4dvARB = (PFNGLPROGRAMENVPARAMETER4DVARBPROC)load("glProgramEnvParameter4dvARB");
	glad_glProgramEnvParameter4fARB = (PFNGLPROGRAMENVPARAMETER4FARBPROC)load("glProgramEnvParameter4fARB");
	glad_glProgramEnvParameter4fvARB = (PFNGLPROGRAMENVPARAMETER4FVARBPROC)load("glProgramEnvParameter4fvARB");
	glad_glProgramLocalParameter4dARB = (PFNGLPROGRAMLOCALPARAMETER4DARBPROC)load("glProgramLocalParameter4dARB");
	glad_glProgramLocalParameter4dvARB = (PFNGLPROGRAMLOCALPARAMETER4DVARBPROC)load("glProgramLocalParameter4dvARB");
	glad_glProgramLocalParameter4fARB = (PFNGLPROGRAMLOCALPARAMETER4FARBPROC)load("glProgramLocalParameter4fARB");
	glad_glProgramLocalParameter4fvARB = (PFNGLPROGRAMLOCALPARAMETER4FVARBPROC)load("glProgramLocalParameter4fvARB");
	glad_glGetProgramEnvParameterdvARB = (PFNGLGETPROGRAMENVPARAMETERDVARBPROC)load("glGetProgramEnvParameterdvARB");
	glad_glGetProgramEnvParameterfvARB = (PFNGLGETPROGRAMENVPARAMETERFVARBPROC)load("glGetProgramEnvParameterfvARB");
	glad_glGetProgramLocalParameterdvARB = (PFNGLGETPROGRAMLOCALPARAMETERDVARBPROC)load("glGetProgramLocalParameterdvARB");
	glad_glGetProgramLocalParameterfvARB = (PFNGLGETPROGRAMLOCALPARAMETERFVARBPROC)load("glGetProgramLocalParameterfvARB");
	glad_glGetProgramivARB = (PFNGLGETPROGRAMIVARBPROC)load("glGetProgramivARB");
	glad_glGetProgramStringARB = (PFNGLGETPROGRAMSTRINGARBPROC)load("glGetProgramStringARB");
	glad_glGetVertexAttribdvARB = (PFNGLGETVERTEXATTRIBDVARBPROC)load("glGetVertexAttribdvARB");
	glad_glGetVertexAttribfvARB = (PFNGLGETVERTEXATTRIBFVARBPROC)load("glGetVertexAttribfvARB");
	glad_glGetVertexAttribivARB = (PFNGLGETVERTEXATTRIBIVARBPROC)load("glGetVertexAttribivARB");
	glad_glGetVertexAttribPointervARB = (PFNGLGETVERTEXATTRIBPOINTERVARBPROC)load("glGetVertexAttribPointervARB");
	glad_glIsProgramARB = (PFNGLISPROGRAMARBPROC)load("glIsProgramARB");
}
static void load_GL_ARB_vertex_shader(GLADloadproc load) {
	if(!GLAD_GL_ARB_vertex_shader) return;
	glad_glVertexAttrib1fARB = (PFNGLVERTEXATTRIB1FARBPROC)load("glVertexAttrib1fARB");
	glad_glVertexAttrib1sARB = (PFNGLVERTEXATTRIB1SARBPROC)load("glVertexAttrib1sARB");
	glad_glVertexAttrib1dARB = (PFNGLVERTEXATTRIB1DARBPROC)load("glVertexAttrib1dARB");
	glad_glVertexAttrib2fARB = (PFNGLVERTEXATTRIB2FARBPROC)load("glVertexAttrib2fARB");
	glad_glVertexAttrib2sARB = (PFNGLVERTEXATTRIB2SARBPROC)load("glVertexAttrib2sARB");
	glad_glVertexAttrib2dARB = (PFNGLVERTEXATTRIB2DARBPROC)load("glVertexAttrib2dARB");
	glad_glVertexAttrib3fARB = (PFNGLVERTEXATTRIB3FARBPROC)load("glVertexAttrib3fARB");
	glad_glVertexAttrib3sARB = (PFNGLVERTEXATTRIB3SARBPROC)load("glVertexAttrib3sARB");
	glad_glVertexAttrib3dARB = (PFNGLVERTEXATTRIB3DARBPROC)load("glVertexAttrib3dARB");
	glad_glVertexAttrib4fARB = (PFNGLVERTEXATTRIB4FARBPROC)load("glVertexAttrib4fARB");
	glad_glVertexAttrib4sARB = (PFNGLVERTEXATTRIB4SARBPROC)load("glVertexAttrib4sARB");
	glad_glVertexAttrib4dARB = (PFNGLVERTEXATTRIB4DARBPROC)load("glVertexAttrib4dARB");
	glad_glVertexAttrib4NubARB = (PFNGLVERTEXATTRIB4NUBARBPROC)load("glVertexAttrib4NubARB");
	glad_glVertexAttrib1fvARB = (PFNGLVERTEXATTRIB1FVARBPROC)load("glVertexAttrib1fvARB");
	glad_glVertexAttrib1svARB = (PFNGLVERTEXATTRIB1SVARBPROC)load("glVertexAttrib1svARB");
	glad_glVertexAttrib1dvARB = (PFNGLVERTEXATTRIB1DVARBPROC)load("glVertexAttrib1dvARB");
	glad_glVertexAttrib2fvARB = (PFNGLVERTEXATTRIB2FVARBPROC)load("glVertexAttrib2fvARB");
	glad_glVertexAttrib2svARB = (PFNGLVERTEXATTRIB2SVARBPROC)load("glVertexAttrib2svARB");
	glad_glVertexAttrib2dvARB = (PFNGLVERTEXATTRIB2DVARBPROC)load("glVertexAttrib2dvARB");
	glad_glVertexAttrib3fvARB = (PFNGLVERTEXATTRIB3FVARBPROC)load("glVertexAttrib3fvARB");
	glad_glVertexAttrib3svARB = (PFNGLVERTEXATTRIB3SVARBPROC)load("glVertexAttrib3svARB");
	glad_glVertexAttrib3dvARB = (PFNGLVERTEXATTRIB3DVARBPROC)load("glVertexAttrib3dvARB");
	glad_glVertexAttrib4fvARB = (PFNGLVERTEXATTRIB4FVARBPROC)load("glVertexAttrib4fvARB");
	glad_glVertexAttrib4svARB = (PFNGLVERTEXATTRIB4SVARBPROC)load("glVertexAttrib4svARB");
	glad_glVertexAttrib4dvARB = (PFNGLVERTEXATTRIB4DVARBPROC)load("glVertexAttrib4dvARB");
	glad_glVertexAttrib4ivARB = (PFNGLVERTEXATTRIB4IVARBPROC)load("glVertexAttrib4ivARB");
	glad_glVertexAttrib4bvARB = (PFNGLVERTEXATTRIB4BVARBPROC)load("glVertexAttrib4bvARB");
	glad_glVertexAttrib4ubvARB = (PFNGLVERTEXATTRIB4UBVARBPROC)load("glVertexAttrib4ubvARB");
	glad_glVertexAttrib4usvARB = (PFNGLVERTEXATTRIB4USVARBPROC)load("glVertexAttrib4usvARB");
	glad_glVertexAttrib4uivARB = (PFNGLVERTEXATTRIB4UIVARBPROC)load("glVertexAttrib4uivARB");
	glad_glVertexAttrib4NbvARB = (PFNGLVERTEXATTRIB4NBVARBPROC)load("glVertexAttrib4NbvARB");
	glad_glVertexAttrib4NsvARB = (PFNGLVERTEXATTRIB4NSVARBPROC)load("glVertexAttrib4NsvARB");
	glad_glVertexAttrib4NivARB = (PFNGLVERTEXATTRIB4NIVARBPROC)load("glVertexAttrib4NivARB");
	glad_glVertexAttrib4NubvARB = (PFNGLVERTEXATTRIB4NUBVARBPROC)load("glVertexAttrib4NubvARB");
	glad_glVertexAttrib4NusvARB = (PFNGLVERTEXATTRIB4NUSVARBPROC)load("glVertexAttrib4NusvARB");
	glad_glVertexAttrib4NuivARB = (PFNGLVERTEXATTRIB4NUIVARBPROC)load("glVertexAttrib4NuivARB");
	glad_glVertexAttribPointerARB = (PFNGLVERTEXATTRIBPOINTERARBPROC)load("glVertexAttribPointerARB");
	glad_glEnableVertexAttribArrayARB = (PFNGLENABLEVERTEXATTRIBARRAYARBPROC)load("glEnableVertexAttribArrayARB");
	glad_glDisableVertexAttribArrayARB = (PFNGLDISABLEVERTEXATTRIBARRAYARBPROC)load("glDisableVertexAttribArrayARB");
	glad_glBindAttribLocationARB = (PFNGLBINDATTRIBLOCATIONARBPROC)load("glBindAttribLocationARB");
	glad_glGetActiveAttribARB = (PFNGLGETACTIVEATTRIBARBPROC)load("glGetActiveAttribARB");
	glad_glGetAttribLocationARB = (PFNGLGETATTRIBLOCATIONARBPROC)load("glGetAttribLocationARB");
	glad_glGetVertexAttribdvARB = (PFNGLGETVERTEXATTRIBDVARBPROC)load("glGetVertexAttribdvARB");
	glad_glGetVertexAttribfvARB = (PFNGLGETVERTEXATTRIBFVARBPROC)load("glGetVertexAttribfvARB");
	glad_glGetVertexAttribivARB = (PFNGLGETVERTEXATTRIBIVARBPROC)load("glGetVertexAttribivARB");
	glad_glGetVertexAttribPointervARB = (PFNGLGETVERTEXATTRIBPOINTERVARBPROC)load("glGetVertexAttribPointervARB");
}
static void load_GL_ARB_vertex_type_2_10_10_10_rev(GLADloadproc load) {
	if(!GLAD_GL_ARB_vertex_type_2_10_10_10_rev) return;
	glad_glVertexAttribP1ui = (PFNGLVERTEXATTRIBP1UIPROC)load("glVertexAttribP1ui");
	glad_glVertexAttribP1uiv = (PFNGLVERTEXATTRIBP1UIVPROC)load("glVertexAttribP1uiv");
	glad_glVertexAttribP2ui = (PFNGLVERTEXATTRIBP2UIPROC)load("glVertexAttribP2ui");
	glad_glVertexAttribP2uiv = (PFNGLVERTEXATTRIBP2UIVPROC)load("glVertexAttribP2uiv");
	glad_glVertexAttribP3ui = (PFNGLVERTEXATTRIBP3UIPROC)load("glVertexAttribP3ui");
	glad_glVertexAttribP3uiv = (PFNGLVERTEXATTRIBP3UIVPROC)load("glVertexAttribP3uiv");
	glad_glVertexAttribP4ui = (PFNGLVERTEXATTRIBP4UIPROC)load("glVertexAttribP4ui");
	glad_glVertexAttribP4uiv = (PFNGLVERTEXATTRIBP4UIVPROC)load("glVertexAttribP4uiv");
	glad_glVertexP2ui = (PFNGLVERTEXP2UIPROC)load("glVertexP2ui");
	glad_glVertexP2uiv = (PFNGLVERTEXP2UIVPROC)load("glVertexP2uiv");
	glad_glVertexP3ui = (PFNGLVERTEXP3UIPROC)load("glVertexP3ui");
	glad_glVertexP3uiv = (PFNGLVERTEXP3UIVPROC)load("glVertexP3uiv");
	glad_glVertexP4ui = (PFNGLVERTEXP4UIPROC)load("glVertexP4ui");
	glad_glVertexP4uiv = (PFNGLVERTEXP4UIVPROC)load("glVertexP4uiv");
	glad_glTexCoordP1ui = (PFNGLTEXCOORDP1UIPROC)load("glTexCoordP1ui");
	glad_glTexCoordP1uiv = (PFNGLTEXCOORDP1UIVPROC)load("glTexCoordP1uiv");
	glad_glTexCoordP2ui = (PFNGLTEXCOORDP2UIPROC)load("glTexCoordP2ui");
	glad_glTexCoordP2uiv = (PFNGLTEXCOORDP2UIVPROC)load("glTexCoordP2uiv");
	glad_glTexCoordP3ui = (PFNGLTEXCOORDP3UIPROC)load("glTexCoordP3ui");
	glad_glTexCoordP3uiv = (PFNGLTEXCOORDP3UIVPROC)load("glTexCoordP3uiv");
	glad_glTexCoordP4ui = (PFNGLTEXCOORDP4UIPROC)load("glTexCoordP4ui");
	glad_glTexCoordP4uiv = (PFNGLTEXCOORDP4UIVPROC)load("glTexCoordP4uiv");
	glad_glMultiTexCoordP1ui = (PFNGLMULTITEXCOORDP1UIPROC)load("glMultiTexCoordP1ui");
	glad_glMultiTexCoordP1uiv = (PFNGLMULTITEXCOORDP1UIVPROC)load("glMultiTexCoordP1uiv");
	glad_glMultiTexCoordP2ui = (PFNGLMULTITEXCOORDP2UIPROC)load("glMultiTexCoordP2ui");
	glad_glMultiTexCoordP2uiv = (PFNGLMULTITEXCOORDP2UIVPROC)load("glMultiTexCoordP2uiv");
	glad_glMultiTexCoordP3ui = (PFNGLMULTITEXCOORDP3UIPROC)load("glMultiTexCoordP3ui");
	glad_glMultiTexCoordP3uiv = (PFNGLMULTITEXCOORDP3UIVPROC)load("glMultiTexCoordP3uiv");
	glad_glMultiTexCoordP4ui = (PFNGLMULTITEXCOORDP4UIPROC)load("glMultiTexCoordP4ui");
	glad_glMultiTexCoordP4uiv = (PFNGLMULTITEXCOORDP4UIVPROC)load("glMultiTexCoordP4uiv");
	glad_glNormalP3ui = (PFNGLNORMALP3UIPROC)load("glNormalP3ui");
	glad_glNormalP3uiv = (PFNGLNORMALP3UIVPROC)load("glNormalP3uiv");
	glad_glColorP3ui = (PFNGLCOLORP3UIPROC)load("glColorP3ui");
	glad_glColorP3uiv = (PFNGLCOLORP3UIVPROC)load("glColorP3uiv");
	glad_glColorP4ui = (PFNGLCOLORP4UIPROC)load("glColorP4ui");
	glad_glColorP4uiv = (PFNGLCOLORP4UIVPROC)load("glColorP4uiv");
	glad_glSecondaryColorP3ui = (PFNGLSECONDARYCOLORP3UIPROC)load("glSecondaryColorP3ui");
	glad_glSecondaryColorP3uiv = (PFNGLSECONDARYCOLORP3UIVPROC)load("glSecondaryColorP3uiv");
}
static void load_GL_ARB_viewport_array(GLADloadproc load) {
	if(!GLAD_GL_ARB_viewport_array) return;
	glad_glViewportArrayv = (PFNGLVIEWPORTARRAYVPROC)load("glViewportArrayv");
	glad_glViewportIndexedf = (PFNGLVIEWPORTINDEXEDFPROC)load("glViewportIndexedf");
	glad_glViewportIndexedfv = (PFNGLVIEWPORTINDEXEDFVPROC)load("glViewportIndexedfv");
	glad_glScissorArrayv = (PFNGLSCISSORARRAYVPROC)load("glScissorArrayv");
	glad_glScissorIndexed = (PFNGLSCISSORINDEXEDPROC)load("glScissorIndexed");
	glad_glScissorIndexedv = (PFNGLSCISSORINDEXEDVPROC)load("glScissorIndexedv");
	glad_glDepthRangeArrayv = (PFNGLDEPTHRANGEARRAYVPROC)load("glDepthRangeArrayv");
	glad_glDepthRangeIndexed = (PFNGLDEPTHRANGEINDEXEDPROC)load("glDepthRangeIndexed");
	glad_glGetFloati_v = (PFNGLGETFLOATI_VPROC)load("glGetFloati_v");
	glad_glGetDoublei_v = (PFNGLGETDOUBLEI_VPROC)load("glGetDoublei_v");
	glad_glDepthRangeArraydvNV = (PFNGLDEPTHRANGEARRAYDVNVPROC)load("glDepthRangeArraydvNV");
	glad_glDepthRangeIndexeddNV = (PFNGLDEPTHRANGEINDEXEDDNVPROC)load("glDepthRangeIndexeddNV");
}
static void load_GL_ARB_window_pos(GLADloadproc load) {
	if(!GLAD_GL_ARB_window_pos) return;
	glad_glWindowPos2dARB = (PFNGLWINDOWPOS2DARBPROC)load("glWindowPos2dARB");
	glad_glWindowPos2dvARB = (PFNGLWINDOWPOS2DVARBPROC)load("glWindowPos2dvARB");
	glad_glWindowPos2fARB = (PFNGLWINDOWPOS2FARBPROC)load("glWindowPos2fARB");
	glad_glWindowPos2fvARB = (PFNGLWINDOWPOS2FVARBPROC)load("glWindowPos2fvARB");
	glad_glWindowPos2iARB = (PFNGLWINDOWPOS2IARBPROC)load("glWindowPos2iARB");
	glad_glWindowPos2ivARB = (PFNGLWINDOWPOS2IVARBPROC)load("glWindowPos2ivARB");
	glad_glWindowPos2sARB = (PFNGLWINDOWPOS2SARBPROC)load("glWindowPos2sARB");
	glad_glWindowPos2svARB = (PFNGLWINDOWPOS2SVARBPROC)load("glWindowPos2svARB");
	glad_glWindowPos3dARB = (PFNGLWINDOWPOS3DARBPROC)load("glWindowPos3dARB");
	glad_glWindowPos3dvARB = (PFNGLWINDOWPOS3DVARBPROC)load("glWindowPos3dvARB");
	glad_glWindowPos3fARB = (PFNGLWINDOWPOS3FARBPROC)load("glWindowPos3fARB");
	glad_glWindowPos3fvARB = (PFNGLWINDOWPOS3FVARBPROC)load("glWindowPos3fvARB");
	glad_glWindowPos3iARB = (PFNGLWINDOWPOS3IARBPROC)load("glWindowPos3iARB");
	glad_glWindowPos3ivARB = (PFNGLWINDOWPOS3IVARBPROC)load("glWindowPos3ivARB");
	glad_glWindowPos3sARB = (PFNGLWINDOWPOS3SARBPROC)load("glWindowPos3sARB");
	glad_glWindowPos3svARB = (PFNGLWINDOWPOS3SVARBPROC)load("glWindowPos3svARB");
}
static void load_GL_ATI_draw_buffers(GLADloadproc load) {
	if(!GLAD_GL_ATI_draw_buffers) return;
	glad_glDrawBuffersATI = (PFNGLDRAWBUFFERSATIPROC)load("glDrawBuffersATI");
}
static void load_GL_ATI_element_array(GLADloadproc load) {
	if(!GLAD_GL_ATI_element_array) return;
	glad_glElementPointerATI = (PFNGLELEMENTPOINTERATIPROC)load("glElementPointerATI");
	glad_glDrawElementArrayATI = (PFNGLDRAWELEMENTARRAYATIPROC)load("glDrawElementArrayATI");
	glad_glDrawRangeElementArrayATI = (PFNGLDRAWRANGEELEMENTARRAYATIPROC)load("glDrawRangeElementArrayATI");
}
static void load_GL_ATI_envmap_bumpmap(GLADloadproc load) {
	if(!GLAD_GL_ATI_envmap_bumpmap) return;
	glad_glTexBumpParameterivATI = (PFNGLTEXBUMPPARAMETERIVATIPROC)load("glTexBumpParameterivATI");
	glad_glTexBumpParameterfvATI = (PFNGLTEXBUMPPARAMETERFVATIPROC)load("glTexBumpParameterfvATI");
	glad_glGetTexBumpParameterivATI = (PFNGLGETTEXBUMPPARAMETERIVATIPROC)load("glGetTexBumpParameterivATI");
	glad_glGetTexBumpParameterfvATI = (PFNGLGETTEXBUMPPARAMETERFVATIPROC)load("glGetTexBumpParameterfvATI");
}
static void load_GL_ATI_fragment_shader(GLADloadproc load) {
	if(!GLAD_GL_ATI_fragment_shader) return;
	glad_glGenFragmentShadersATI = (PFNGLGENFRAGMENTSHADERSATIPROC)load("glGenFragmentShadersATI");
	glad_glBindFragmentShaderATI = (PFNGLBINDFRAGMENTSHADERATIPROC)load("glBindFragmentShaderATI");
	glad_glDeleteFragmentShaderATI = (PFNGLDELETEFRAGMENTSHADERATIPROC)load("glDeleteFragmentShaderATI");
	glad_glBeginFragmentShaderATI = (PFNGLBEGINFRAGMENTSHADERATIPROC)load("glBeginFragmentShaderATI");
	glad_glEndFragmentShaderATI = (PFNGLENDFRAGMENTSHADERATIPROC)load("glEndFragmentShaderATI");
	glad_glPassTexCoordATI = (PFNGLPASSTEXCOORDATIPROC)load("glPassTexCoordATI");
	glad_glSampleMapATI = (PFNGLSAMPLEMAPATIPROC)load("glSampleMapATI");
	glad_glColorFragmentOp1ATI = (PFNGLCOLORFRAGMENTOP1ATIPROC)load("glColorFragmentOp1ATI");
	glad_glColorFragmentOp2ATI = (PFNGLCOLORFRAGMENTOP2ATIPROC)load("glColorFragmentOp2ATI");
	glad_glColorFragmentOp3ATI = (PFNGLCOLORFRAGMENTOP3ATIPROC)load("glColorFragmentOp3ATI");
	glad_glAlphaFragmentOp1ATI = (PFNGLALPHAFRAGMENTOP1ATIPROC)load("glAlphaFragmentOp1ATI");
	glad_glAlphaFragmentOp2ATI = (PFNGLALPHAFRAGMENTOP2ATIPROC)load("glAlphaFragmentOp2ATI");
	glad_glAlphaFragmentOp3ATI = (PFNGLALPHAFRAGMENTOP3ATIPROC)load("glAlphaFragmentOp3ATI");
	glad_glSetFragmentShaderConstantATI = (PFNGLSETFRAGMENTSHADERCONSTANTATIPROC)load("glSetFragmentShaderConstantATI");
}
static void load_GL_ATI_map_object_buffer(GLADloadproc load) {
	if(!GLAD_GL_ATI_map_object_buffer) return;
	glad_glMapObjectBufferATI = (PFNGLMAPOBJECTBUFFERATIPROC)load("glMapObjectBufferATI");
	glad_glUnmapObjectBufferATI = (PFNGLUNMAPOBJECTBUFFERATIPROC)load("glUnmapObjectBufferATI");
}
static void load_GL_ATI_pn_triangles(GLADloadproc load) {
	if(!GLAD_GL_ATI_pn_triangles) return;
	glad_glPNTrianglesiATI = (PFNGLPNTRIANGLESIATIPROC)load("glPNTrianglesiATI");
	glad_glPNTrianglesfATI = (PFNGLPNTRIANGLESFATIPROC)load("glPNTrianglesfATI");
}
static void load_GL_ATI_separate_stencil(GLADloadproc load) {
	if(!GLAD_GL_ATI_separate_stencil) return;
	glad_glStencilOpSeparateATI = (PFNGLSTENCILOPSEPARATEATIPROC)load("glStencilOpSeparateATI");
	glad_glStencilFuncSeparateATI = (PFNGLSTENCILFUNCSEPARATEATIPROC)load("glStencilFuncSeparateATI");
}
static void load_GL_ATI_vertex_array_object(GLADloadproc load) {
	if(!GLAD_GL_ATI_vertex_array_object) return;
	glad_glNewObjectBufferATI = (PFNGLNEWOBJECTBUFFERATIPROC)load("glNewObjectBufferATI");
	glad_glIsObjectBufferATI = (PFNGLISOBJECTBUFFERATIPROC)load("glIsObjectBufferATI");
	glad_glUpdateObjectBufferATI = (PFNGLUPDATEOBJECTBUFFERATIPROC)load("glUpdateObjectBufferATI");
	glad_glGetObjectBufferfvATI = (PFNGLGETOBJECTBUFFERFVATIPROC)load("glGetObjectBufferfvATI");
	glad_glGetObjectBufferivATI = (PFNGLGETOBJECTBUFFERIVATIPROC)load("glGetObjectBufferivATI");
	glad_glFreeObjectBufferATI = (PFNGLFREEOBJECTBUFFERATIPROC)load("glFreeObjectBufferATI");
	glad_glArrayObjectATI = (PFNGLARRAYOBJECTATIPROC)load("glArrayObjectATI");
	glad_glGetArrayObjectfvATI = (PFNGLGETARRAYOBJECTFVATIPROC)load("glGetArrayObjectfvATI");
	glad_glGetArrayObjectivATI = (PFNGLGETARRAYOBJECTIVATIPROC)load("glGetArrayObjectivATI");
	glad_glVariantArrayObjectATI = (PFNGLVARIANTARRAYOBJECTATIPROC)load("glVariantArrayObjectATI");
	glad_glGetVariantArrayObjectfvATI = (PFNGLGETVARIANTARRAYOBJECTFVATIPROC)load("glGetVariantArrayObjectfvATI");
	glad_glGetVariantArrayObjectivATI = (PFNGLGETVARIANTARRAYOBJECTIVATIPROC)load("glGetVariantArrayObjectivATI");
}
static void load_GL_ATI_vertex_attrib_array_object(GLADloadproc load) {
	if(!GLAD_GL_ATI_vertex_attrib_array_object) return;
	glad_glVertexAttribArrayObjectATI = (PFNGLVERTEXATTRIBARRAYOBJECTATIPROC)load("glVertexAttribArrayObjectATI");
	glad_glGetVertexAttribArrayObjectfvATI = (PFNGLGETVERTEXATTRIBARRAYOBJECTFVATIPROC)load("glGetVertexAttribArrayObjectfvATI");
	glad_glGetVertexAttribArrayObjectivATI = (PFNGLGETVERTEXATTRIBARRAYOBJECTIVATIPROC)load("glGetVertexAttribArrayObjectivATI");
}
static void load_GL_ATI_vertex_streams(GLADloadproc load) {
	if(!GLAD_GL_ATI_vertex_streams) return;
	glad_glVertexStream1sATI = (PFNGLVERTEXSTREAM1SATIPROC)load("glVertexStream1sATI");
	glad_glVertexStream1svATI = (PFNGLVERTEXSTREAM1SVATIPROC)load("glVertexStream1svATI");
	glad_glVertexStream1iATI = (PFNGLVERTEXSTREAM1IATIPROC)load("glVertexStream1iATI");
	glad_glVertexStream1ivATI = (PFNGLVERTEXSTREAM1IVATIPROC)load("glVertexStream1ivATI");
	glad_glVertexStream1fATI = (PFNGLVERTEXSTREAM1FATIPROC)load("glVertexStream1fATI");
	glad_glVertexStream1fvATI = (PFNGLVERTEXSTREAM1FVATIPROC)load("glVertexStream1fvATI");
	glad_glVertexStream1dATI = (PFNGLVERTEXSTREAM1DATIPROC)load("glVertexStream1dATI");
	glad_glVertexStream1dvATI = (PFNGLVERTEXSTREAM1DVATIPROC)load("glVertexStream1dvATI");
	glad_glVertexStream2sATI = (PFNGLVERTEXSTREAM2SATIPROC)load("glVertexStream2sATI");
	glad_glVertexStream2svATI = (PFNGLVERTEXSTREAM2SVATIPROC)load("glVertexStream2svATI");
	glad_glVertexStream2iATI = (PFNGLVERTEXSTREAM2IATIPROC)load("glVertexStream2iATI");
	glad_glVertexStream2ivATI = (PFNGLVERTEXSTREAM2IVATIPROC)load("glVertexStream2ivATI");
	glad_glVertexStream2fATI = (PFNGLVERTEXSTREAM2FATIPROC)load("glVertexStream2fATI");
	glad_glVertexStream2fvATI = (PFNGLVERTEXSTREAM2FVATIPROC)load("glVertexStream2fvATI");
	glad_glVertexStream2dATI = (PFNGLVERTEXSTREAM2DATIPROC)load("glVertexStream2dATI");
	glad_glVertexStream2dvATI = (PFNGLVERTEXSTREAM2DVATIPROC)load("glVertexStream2dvATI");
	glad_glVertexStream3sATI = (PFNGLVERTEXSTREAM3SATIPROC)load("glVertexStream3sATI");
	glad_glVertexStream3svATI = (PFNGLVERTEXSTREAM3SVATIPROC)load("glVertexStream3svATI");
	glad_glVertexStream3iATI = (PFNGLVERTEXSTREAM3IATIPROC)load("glVertexStream3iATI");
	glad_glVertexStream3ivATI = (PFNGLVERTEXSTREAM3IVATIPROC)load("glVertexStream3ivATI");
	glad_glVertexStream3fATI = (PFNGLVERTEXSTREAM3FATIPROC)load("glVertexStream3fATI");
	glad_glVertexStream3fvATI = (PFNGLVERTEXSTREAM3FVATIPROC)load("glVertexStream3fvATI");
	glad_glVertexStream3dATI = (PFNGLVERTEXSTREAM3DATIPROC)load("glVertexStream3dATI");
	glad_glVertexStream3dvATI = (PFNGLVERTEXSTREAM3DVATIPROC)load("glVertexStream3dvATI");
	glad_glVertexStream4sATI = (PFNGLVERTEXSTREAM4SATIPROC)load("glVertexStream4sATI");
	glad_glVertexStream4svATI = (PFNGLVERTEXSTREAM4SVATIPROC)load("glVertexStream4svATI");
	glad_glVertexStream4iATI = (PFNGLVERTEXSTREAM4IATIPROC)load("glVertexStream4iATI");
	glad_glVertexStream4ivATI = (PFNGLVERTEXSTREAM4IVATIPROC)load("glVertexStream4ivATI");
	glad_glVertexStream4fATI = (PFNGLVERTEXSTREAM4FATIPROC)load("glVertexStream4fATI");
	glad_glVertexStream4fvATI = (PFNGLVERTEXSTREAM4FVATIPROC)load("glVertexStream4fvATI");
	glad_glVertexStream4dATI = (PFNGLVERTEXSTREAM4DATIPROC)load("glVertexStream4dATI");
	glad_glVertexStream4dvATI = (PFNGLVERTEXSTREAM4DVATIPROC)load("glVertexStream4dvATI");
	glad_glNormalStream3bATI = (PFNGLNORMALSTREAM3BATIPROC)load("glNormalStream3bATI");
	glad_glNormalStream3bvATI = (PFNGLNORMALSTREAM3BVATIPROC)load("glNormalStream3bvATI");
	glad_glNormalStream3sATI = (PFNGLNORMALSTREAM3SATIPROC)load("glNormalStream3sATI");
	glad_glNormalStream3svATI = (PFNGLNORMALSTREAM3SVATIPROC)load("glNormalStream3svATI");
	glad_glNormalStream3iATI = (PFNGLNORMALSTREAM3IATIPROC)load("glNormalStream3iATI");
	glad_glNormalStream3ivATI = (PFNGLNORMALSTREAM3IVATIPROC)load("glNormalStream3ivATI");
	glad_glNormalStream3fATI = (PFNGLNORMALSTREAM3FATIPROC)load("glNormalStream3fATI");
	glad_glNormalStream3fvATI = (PFNGLNORMALSTREAM3FVATIPROC)load("glNormalStream3fvATI");
	glad_glNormalStream3dATI = (PFNGLNORMALSTREAM3DATIPROC)load("glNormalStream3dATI");
	glad_glNormalStream3dvATI = (PFNGLNORMALSTREAM3DVATIPROC)load("glNormalStream3dvATI");
	glad_glClientActiveVertexStreamATI = (PFNGLCLIENTACTIVEVERTEXSTREAMATIPROC)load("glClientActiveVertexStreamATI");
	glad_glVertexBlendEnviATI = (PFNGLVERTEXBLENDENVIATIPROC)load("glVertexBlendEnviATI");
	glad_glVertexBlendEnvfATI = (PFNGLVERTEXBLENDENVFATIPROC)load("glVertexBlendEnvfATI");
}
static void load_GL_EXT_EGL_image_storage(GLADloadproc load) {
	if(!GLAD_GL_EXT_EGL_image_storage) return;
	glad_glEGLImageTargetTexStorageEXT = (PFNGLEGLIMAGETARGETTEXSTORAGEEXTPROC)load("glEGLImageTargetTexStorageEXT");
	glad_glEGLImageTargetTextureStorageEXT = (PFNGLEGLIMAGETARGETTEXTURESTORAGEEXTPROC)load("glEGLImageTargetTextureStorageEXT");
}
static void load_GL_EXT_bindable_uniform(GLADloadproc load) {
	if(!GLAD_GL_EXT_bindable_uniform) return;
	glad_glUniformBufferEXT = (PFNGLUNIFORMBUFFEREXTPROC)load("glUniformBufferEXT");
	glad_glGetUniformBufferSizeEXT = (PFNGLGETUNIFORMBUFFERSIZEEXTPROC)load("glGetUniformBufferSizeEXT");
	glad_glGetUniformOffsetEXT = (PFNGLGETUNIFORMOFFSETEXTPROC)load("glGetUniformOffsetEXT");
}
static void load_GL_EXT_blend_color(GLADloadproc load) {
	if(!GLAD_GL_EXT_blend_color) return;
	glad_glBlendColorEXT = (PFNGLBLENDCOLOREXTPROC)load("glBlendColorEXT");
}
static void load_GL_EXT_blend_equation_separate(GLADloadproc load) {
	if(!GLAD_GL_EXT_blend_equation_separate) return;
	glad_glBlendEquationSeparateEXT = (PFNGLBLENDEQUATIONSEPARATEEXTPROC)load("glBlendEquationSeparateEXT");
}
static void load_GL_EXT_blend_func_separate(GLADloadproc load) {
	if(!GLAD_GL_EXT_blend_func_separate) return;
	glad_glBlendFuncSeparateEXT = (PFNGLBLENDFUNCSEPARATEEXTPROC)load("glBlendFuncSeparateEXT");
}
static void load_GL_EXT_blend_minmax(GLADloadproc load) {
	if(!GLAD_GL_EXT_blend_minmax) return;
	glad_glBlendEquationEXT = (PFNGLBLENDEQUATIONEXTPROC)load("glBlendEquationEXT");
}
static void load_GL_EXT_color_subtable(GLADloadproc load) {
	if(!GLAD_GL_EXT_color_subtable) return;
	glad_glColorSubTableEXT = (PFNGLCOLORSUBTABLEEXTPROC)load("glColorSubTableEXT");
	glad_glCopyColorSubTableEXT = (PFNGLCOPYCOLORSUBTABLEEXTPROC)load("glCopyColorSubTableEXT");
}
static void load_GL_EXT_compiled_vertex_array(GLADloadproc load) {
	if(!GLAD_GL_EXT_compiled_vertex_array) return;
	glad_glLockArraysEXT = (PFNGLLOCKARRAYSEXTPROC)load("glLockArraysEXT");
	glad_glUnlockArraysEXT = (PFNGLUNLOCKARRAYSEXTPROC)load("glUnlockArraysEXT");
}
static void load_GL_EXT_convolution(GLADloadproc load) {
	if(!GLAD_GL_EXT_convolution) return;
	glad_glConvolutionFilter1DEXT = (PFNGLCONVOLUTIONFILTER1DEXTPROC)load("glConvolutionFilter1DEXT");
	glad_glConvolutionFilter2DEXT = (PFNGLCONVOLUTIONFILTER2DEXTPROC)load("glConvolutionFilter2DEXT");
	glad_glConvolutionParameterfEXT = (PFNGLCONVOLUTIONPARAMETERFEXTPROC)load("glConvolutionParameterfEXT");
	glad_glConvolutionParameterfvEXT = (PFNGLCONVOLUTIONPARAMETERFVEXTPROC)load("glConvolutionParameterfvEXT");
	glad_glConvolutionParameteriEXT = (PFNGLCONVOLUTIONPARAMETERIEXTPROC)load("glConvolutionParameteriEXT");
	glad_glConvolutionParameterivEXT = (PFNGLCONVOLUTIONPARAMETERIVEXTPROC)load("glConvolutionParameterivEXT");
	glad_glCopyConvolutionFilter1DEXT = (PFNGLCOPYCONVOLUTIONFILTER1DEXTPROC)load("glCopyConvolutionFilter1DEXT");
	glad_glCopyConvolutionFilter2DEXT = (PFNGLCOPYCONVOLUTIONFILTER2DEXTPROC)load("glCopyConvolutionFilter2DEXT");
	glad_glGetConvolutionFilterEXT = (PFNGLGETCONVOLUTIONFILTEREXTPROC)load("glGetConvolutionFilterEXT");
	glad_glGetConvolutionParameterfvEXT = (PFNGLGETCONVOLUTIONPARAMETERFVEXTPROC)load("glGetConvolutionParameterfvEXT");
	glad_glGetConvolutionParameterivEXT = (PFNGLGETCONVOLUTIONPARAMETERIVEXTPROC)load("glGetConvolutionParameterivEXT");
	glad_glGetSeparableFilterEXT = (PFNGLGETSEPARABLEFILTEREXTPROC)load("glGetSeparableFilterEXT");
	glad_glSeparableFilter2DEXT = (PFNGLSEPARABLEFILTER2DEXTPROC)load("glSeparableFilter2DEXT");
}
static void load_GL_EXT_coordinate_frame(GLADloadproc load) {
	if(!GLAD_GL_EXT_coordinate_frame) return;
	glad_glTangent3bEXT = (PFNGLTANGENT3BEXTPROC)load("glTangent3bEXT");
	glad_glTangent3bvEXT = (PFNGLTANGENT3BVEXTPROC)load("glTangent3bvEXT");
	glad_glTangent3dEXT = (PFNGLTANGENT3DEXTPROC)load("glTangent3dEXT");
	glad_glTangent3dvEXT = (PFNGLTANGENT3DVEXTPROC)load("glTangent3dvEXT");
	glad_glTangent3fEXT = (PFNGLTANGENT3FEXTPROC)load("glTangent3fEXT");
	glad_glTangent3fvEXT = (PFNGLTANGENT3FVEXTPROC)load("glTangent3fvEXT");
	glad_glTangent3iEXT = (PFNGLTANGENT3IEXTPROC)load("glTangent3iEXT");
	glad_glTangent3ivEXT = (PFNGLTANGENT3IVEXTPROC)load("glTangent3ivEXT");
	glad_glTangent3sEXT = (PFNGLTANGENT3SEXTPROC)load("glTangent3sEXT");
	glad_glTangent3svEXT = (PFNGLTANGENT3SVEXTPROC)load("glTangent3svEXT");
	glad_glBinormal3bEXT = (PFNGLBINORMAL3BEXTPROC)load("glBinormal3bEXT");
	glad_glBinormal3bvEXT = (PFNGLBINORMAL3BVEXTPROC)load("glBinormal3bvEXT");
	glad_glBinormal3dEXT = (PFNGLBINORMAL3DEXTPROC)load("glBinormal3dEXT");
	glad_glBinormal3dvEXT = (PFNGLBINORMAL3DVEXTPROC)load("glBinormal3dvEXT");
	glad_glBinormal3fEXT = (PFNGLBINORMAL3FEXTPROC)load("glBinormal3fEXT");
	glad_glBinormal3fvEXT = (PFNGLBINORMAL3FVEXTPROC)load("glBinormal3fvEXT");
	glad_glBinormal3iEXT = (PFNGLBINORMAL3IEXTPROC)load("glBinormal3iEXT");
	glad_glBinormal3ivEXT = (PFNGLBINORMAL3IVEXTPROC)load("glBinormal3ivEXT");
	glad_glBinormal3sEXT = (PFNGLBINORMAL3SEXTPROC)load("glBinormal3sEXT");
	glad_glBinormal3svEXT = (PFNGLBINORMAL3SVEXTPROC)load("glBinormal3svEXT");
	glad_glTangentPointerEXT = (PFNGLTANGENTPOINTEREXTPROC)load("glTangentPointerEXT");
	glad_glBinormalPointerEXT = (PFNGLBINORMALPOINTEREXTPROC)load("glBinormalPointerEXT");
}
static void load_GL_EXT_copy_texture(GLADloadproc load) {
	if(!GLAD_GL_EXT_copy_texture) return;
	glad_glCopyTexImage1DEXT = (PFNGLCOPYTEXIMAGE1DEXTPROC)load("glCopyTexImage1DEXT");
	glad_glCopyTexImage2DEXT = (PFNGLCOPYTEXIMAGE2DEXTPROC)load("glCopyTexImage2DEXT");
	glad_glCopyTexSubImage1DEXT = (PFNGLCOPYTEXSUBIMAGE1DEXTPROC)load("glCopyTexSubImage1DEXT");
	glad_glCopyTexSubImage2DEXT = (PFNGLCOPYTEXSUBIMAGE2DEXTPROC)load("glCopyTexSubImage2DEXT");
	glad_glCopyTexSubImage3DEXT = (PFNGLCOPYTEXSUBIMAGE3DEXTPROC)load("glCopyTexSubImage3DEXT");
}
static void load_GL_EXT_cull_vertex(GLADloadproc load) {
	if(!GLAD_GL_EXT_cull_vertex) return;
	glad_glCullParameterdvEXT = (PFNGLCULLPARAMETERDVEXTPROC)load("glCullParameterdvEXT");
	glad_glCullParameterfvEXT = (PFNGLCULLPARAMETERFVEXTPROC)load("glCullParameterfvEXT");
}
static void load_GL_EXT_debug_label(GLADloadproc load) {
	if(!GLAD_GL_EXT_debug_label) return;
	glad_glLabelObjectEXT = (PFNGLLABELOBJECTEXTPROC)load("glLabelObjectEXT");
	glad_glGetObjectLabelEXT = (PFNGLGETOBJECTLABELEXTPROC)load("glGetObjectLabelEXT");
}
static void load_GL_EXT_debug_marker(GLADloadproc load) {
	if(!GLAD_GL_EXT_debug_marker) return;
	glad_glInsertEventMarkerEXT = (PFNGLINSERTEVENTMARKEREXTPROC)load("glInsertEventMarkerEXT");
	glad_glPushGroupMarkerEXT = (PFNGLPUSHGROUPMARKEREXTPROC)load("glPushGroupMarkerEXT");
	glad_glPopGroupMarkerEXT = (PFNGLPOPGROUPMARKEREXTPROC)load("glPopGroupMarkerEXT");
}
static void load_GL_EXT_depth_bounds_test(GLADloadproc load) {
	if(!GLAD_GL_EXT_depth_bounds_test) return;
	glad_glDepthBoundsEXT = (PFNGLDEPTHBOUNDSEXTPROC)load("glDepthBoundsEXT");
}
static void load_GL_EXT_direct_state_access(GLADloadproc load) {
	if(!GLAD_GL_EXT_direct_state_access) return;
	glad_glMatrixLoadfEXT = (PFNGLMATRIXLOADFEXTPROC)load("glMatrixLoadfEXT");
	glad_glMatrixLoaddEXT = (PFNGLMATRIXLOADDEXTPROC)load("glMatrixLoaddEXT");
	glad_glMatrixMultfEXT = (PFNGLMATRIXMULTFEXTPROC)load("glMatrixMultfEXT");
	glad_glMatrixMultdEXT = (PFNGLMATRIXMULTDEXTPROC)load("glMatrixMultdEXT");
	glad_glMatrixLoadIdentityEXT = (PFNGLMATRIXLOADIDENTITYEXTPROC)load("glMatrixLoadIdentityEXT");
	glad_glMatrixRotatefEXT = (PFNGLMATRIXROTATEFEXTPROC)load("glMatrixRotatefEXT");
	glad_glMatrixRotatedEXT = (PFNGLMATRIXROTATEDEXTPROC)load("glMatrixRotatedEXT");
	glad_glMatrixScalefEXT = (PFNGLMATRIXSCALEFEXTPROC)load("glMatrixScalefEXT");
	glad_glMatrixScaledEXT = (PFNGLMATRIXSCALEDEXTPROC)load("glMatrixScaledEXT");
	glad_glMatrixTranslatefEXT = (PFNGLMATRIXTRANSLATEFEXTPROC)load("glMatrixTranslatefEXT");
	glad_glMatrixTranslatedEXT = (PFNGLMATRIXTRANSLATEDEXTPROC)load("glMatrixTranslatedEXT");
	glad_glMatrixFrustumEXT = (PFNGLMATRIXFRUSTUMEXTPROC)load("glMatrixFrustumEXT");
	glad_glMatrixOrthoEXT = (PFNGLMATRIXORTHOEXTPROC)load("glMatrixOrthoEXT");
	glad_glMatrixPopEXT = (PFNGLMATRIXPOPEXTPROC)load("glMatrixPopEXT");
	glad_glMatrixPushEXT = (PFNGLMATRIXPUSHEXTPROC)load("glMatrixPushEXT");
	glad_glClientAttribDefaultEXT = (PFNGLCLIENTATTRIBDEFAULTEXTPROC)load("glClientAttribDefaultEXT");
	glad_glPushClientAttribDefaultEXT = (PFNGLPUSHCLIENTATTRIBDEFAULTEXTPROC)load("glPushClientAttribDefaultEXT");
	glad_glTextureParameterfEXT = (PFNGLTEXTUREPARAMETERFEXTPROC)load("glTextureParameterfEXT");
	glad_glTextureParameterfvEXT = (PFNGLTEXTUREPARAMETERFVEXTPROC)load("glTextureParameterfvEXT");
	glad_glTextureParameteriEXT = (PFNGLTEXTUREPARAMETERIEXTPROC)load("glTextureParameteriEXT");
	glad_glTextureParameterivEXT = (PFNGLTEXTUREPARAMETERIVEXTPROC)load("glTextureParameterivEXT");
	glad_glTextureImage1DEXT = (PFNGLTEXTUREIMAGE1DEXTPROC)load("glTextureImage1DEXT");
	glad_glTextureImage2DEXT = (PFNGLTEXTUREIMAGE2DEXTPROC)load("glTextureImage2DEXT");
	glad_glTextureSubImage1DEXT = (PFNGLTEXTURESUBIMAGE1DEXTPROC)load("glTextureSubImage1DEXT");
	glad_glTextureSubImage2DEXT = (PFNGLTEXTURESUBIMAGE2DEXTPROC)load("glTextureSubImage2DEXT");
	glad_glCopyTextureImage1DEXT = (PFNGLCOPYTEXTUREIMAGE1DEXTPROC)load("glCopyTextureImage1DEXT");
	glad_glCopyTextureImage2DEXT = (PFNGLCOPYTEXTUREIMAGE2DEXTPROC)load("glCopyTextureImage2DEXT");
	glad_glCopyTextureSubImage1DEXT = (PFNGLCOPYTEXTURESUBIMAGE1DEXTPROC)load("glCopyTextureSubImage1DEXT");
	glad_glCopyTextureSubImage2DEXT = (PFNGLCOPYTEXTURESUBIMAGE2DEXTPROC)load("glCopyTextureSubImage2DEXT");
	glad_glGetTextureImageEXT = (PFNGLGETTEXTUREIMAGEEXTPROC)load("glGetTextureImageEXT");
	glad_glGetTextureParameterfvEXT = (PFNGLGETTEXTUREPARAMETERFVEXTPROC)load("glGetTextureParameterfvEXT");
	glad_glGetTextureParameterivEXT = (PFNGLGETTEXTUREPARAMETERIVEXTPROC)load("glGetTextureParameterivEXT");
	glad_glGetTextureLevelParameterfvEXT = (PFNGLGETTEXTURELEVELPARAMETERFVEXTPROC)load("glGetTextureLevelParameterfvEXT");
	glad_glGetTextureLevelParameterivEXT = (PFNGLGETTEXTURELEVELPARAMETERIVEXTPROC)load("glGetTextureLevelParameterivEXT");
	glad_glTextureImage3DEXT = (PFNGLTEXTUREIMAGE3DEXTPROC)load("glTextureImage3DEXT");
	glad_glTextureSubImage3DEXT = (PFNGLTEXTURESUBIMAGE3DEXTPROC)load("glTextureSubImage3DEXT");
	glad_glCopyTextureSubImage3DEXT = (PFNGLCOPYTEXTURESUBIMAGE3DEXTPROC)load("glCopyTextureSubImage3DEXT");
	glad_glBindMultiTextureEXT = (PFNGLBINDMULTITEXTUREEXTPROC)load("glBindMultiTextureEXT");
	glad_glMultiTexCoordPointerEXT = (PFNGLMULTITEXCOORDPOINTEREXTPROC)load("glMultiTexCoordPointerEXT");
	glad_glMultiTexEnvfEXT = (PFNGLMULTITEXENVFEXTPROC)load("glMultiTexEnvfEXT");
	glad_glMultiTexEnvfvEXT = (PFNGLMULTITEXENVFVEXTPROC)load("glMultiTexEnvfvEXT");
	glad_glMultiTexEnviEXT = (PFNGLMULTITEXENVIEXTPROC)load("glMultiTexEnviEXT");
	glad_glMultiTexEnvivEXT = (PFNGLMULTITEXENVIVEXTPROC)load("glMultiTexEnvivEXT");
	glad_glMultiTexGendEXT = (PFNGLMULTITEXGENDEXTPROC)load("glMultiTexGendEXT");
	glad_glMultiTexGendvEXT = (PFNGLMULTITEXGENDVEXTPROC)load("glMultiTexGendvEXT");
	glad_glMultiTexGenfEXT = (PFNGLMULTITEXGENFEXTPROC)load("glMultiTexGenfEXT");
	glad_glMultiTexGenfvEXT = (PFNGLMULTITEXGENFVEXTPROC)load("glMultiTexGenfvEXT");
	glad_glMultiTexGeniEXT = (PFNGLMULTITEXGENIEXTPROC)load("glMultiTexGeniEXT");
	glad_glMultiTexGenivEXT = (PFNGLMULTITEXGENIVEXTPROC)load("glMultiTexGenivEXT");
	glad_glGetMultiTexEnvfvEXT = (PFNGLGETMULTITEXENVFVEXTPROC)load("glGetMultiTexEnvfvEXT");
	glad_glGetMultiTexEnvivEXT = (PFNGLGETMULTITEXENVIVEXTPROC)load("glGetMultiTexEnvivEXT");
	glad_glGetMultiTexGendvEXT = (PFNGLGETMULTITEXGENDVEXTPROC)load("glGetMultiTexGendvEXT");
	glad_glGetMultiTexGenfvEXT = (PFNGLGETMULTITEXGENFVEXTPROC)load("glGetMultiTexGenfvEXT");
	glad_glGetMultiTexGenivEXT = (PFNGLGETMULTITEXGENIVEXTPROC)load("glGetMultiTexGenivEXT");
	glad_glMultiTexParameteriEXT = (PFNGLMULTITEXPARAMETERIEXTPROC)load("glMultiTexParameteriEXT");
	glad_glMultiTexParameterivEXT = (PFNGLMULTITEXPARAMETERIVEXTPROC)load("glMultiTexParameterivEXT");
	glad_glMultiTexParameterfEXT = (PFNGLMULTITEXPARAMETERFEXTPROC)load("glMultiTexParameterfEXT");
	glad_glMultiTexParameterfvEXT = (PFNGLMULTITEXPARAMETERFVEXTPROC)load("glMultiTexParameterfvEXT");
	glad_glMultiTexImage1DEXT = (PFNGLMULTITEXIMAGE1DEXTPROC)load("glMultiTexImage1DEXT");
	glad_glMultiTexImage2DEXT = (PFNGLMULTITEXIMAGE2DEXTPROC)load("glMultiTexImage2DEXT");
	glad_glMultiTexSubImage1DEXT = (PFNGLMULTITEXSUBIMAGE1DEXTPROC)load("glMultiTexSubImage1DEXT");
	glad_glMultiTexSubImage2DEXT = (PFNGLMULTITEXSUBIMAGE2DEXTPROC)load("glMultiTexSubImage2DEXT");
	glad_glCopyMultiTexImage1DEXT = (PFNGLCOPYMULTITEXIMAGE1DEXTPROC)load("glCopyMultiTexImage1DEXT");
	glad_glCopyMultiTexImage2DEXT = (PFNGLCOPYMULTITEXIMAGE2DEXTPROC)load("glCopyMultiTexImage2DEXT");
	glad_glCopyMultiTexSubImage1DEXT = (PFNGLCOPYMULTITEXSUBIMAGE1DEXTPROC)load("glCopyMultiTexSubImage1DEXT");
	glad_glCopyMultiTexSubImage2DEXT = (PFNGLCOPYMULTITEXSUBIMAGE2DEXTPROC)load("glCopyMultiTexSubImage2DEXT");
	glad_glGetMultiTexImageEXT = (PFNGLGETMULTITEXIMAGEEXTPROC)load("glGetMultiTexImageEXT");
	glad_glGetMultiTexParameterfvEXT = (PFNGLGETMULTITEXPARAMETERFVEXTPROC)load("glGetMultiTexParameterfvEXT");
	glad_glGetMultiTexParameterivEXT = (PFNGLGETMULTITEXPARAMETERIVEXTPROC)load("glGetMultiTexParameterivEXT");
	glad_glGetMultiTexLevelParameterfvEXT = (PFNGLGETMULTITEXLEVELPARAMETERFVEXTPROC)load("glGetMultiTexLevelParameterfvEXT");
	glad_glGetMultiTexLevelParameterivEXT = (PFNGLGETMULTITEXLEVELPARAMETERIVEXTPROC)load("glGetMultiTexLevelParameterivEXT");
	glad_glMultiTexImage3DEXT = (PFNGLMULTITEXIMAGE3DEXTPROC)load("glMultiTexImage3DEXT");
	glad_glMultiTexSubImage3DEXT = (PFNGLMULTITEXSUBIMAGE3DEXTPROC)load("glMultiTexSubImage3DEXT");
	glad_glCopyMultiTexSubImage3DEXT = (PFNGLCOPYMULTITEXSUBIMAGE3DEXTPROC)load("glCopyMultiTexSubImage3DEXT");
	glad_glEnableClientStateIndexedEXT = (PFNGLENABLECLIENTSTATEINDEXEDEXTPROC)load("glEnableClientStateIndexedEXT");
	glad_glDisableClientStateIndexedEXT = (PFNGLDISABLECLIENTSTATEINDEXEDEXTPROC)load("glDisableClientStateIndexedEXT");
	glad_glGetFloatIndexedvEXT = (PFNGLGETFLOATINDEXEDVEXTPROC)load("glGetFloatIndexedvEXT");
	glad_glGetDoubleIndexedvEXT = (PFNGLGETDOUBLEINDEXEDVEXTPROC)load("glGetDoubleIndexedvEXT");
	glad_glGetPointerIndexedvEXT = (PFNGLGETPOINTERINDEXEDVEXTPROC)load("glGetPointerIndexedvEXT");
	glad_glEnableIndexedEXT = (PFNGLENABLEINDEXEDEXTPROC)load("glEnableIndexedEXT");
	glad_glDisableIndexedEXT = (PFNGLDISABLEINDEXEDEXTPROC)load("glDisableIndexedEXT");
	glad_glIsEnabledIndexedEXT = (PFNGLISENABLEDINDEXEDEXTPROC)load("glIsEnabledIndexedEXT");
	glad_glGetIntegerIndexedvEXT = (PFNGLGETINTEGERINDEXEDVEXTPROC)load("glGetIntegerIndexedvEXT");
	glad_glGetBooleanIndexedvEXT = (PFNGLGETBOOLEANINDEXEDVEXTPROC)load("glGetBooleanIndexedvEXT");
	glad_glCompressedTextureImage3DEXT = (PFNGLCOMPRESSEDTEXTUREIMAGE3DEXTPROC)load("glCompressedTextureImage3DEXT");
	glad_glCompressedTextureImage2DEXT = (PFNGLCOMPRESSEDTEXTUREIMAGE2DEXTPROC)load("glCompressedTextureImage2DEXT");
	glad_glCompressedTextureImage1DEXT = (PFNGLCOMPRESSEDTEXTUREIMAGE1DEXTPROC)load("glCompressedTextureImage1DEXT");
	glad_glCompressedTextureSubImage3DEXT = (PFNGLCOMPRESSEDTEXTURESUBIMAGE3DEXTPROC)load("glCompressedTextureSubImage3DEXT");
	glad_glCompressedTextureSubImage2DEXT = (PFNGLCOMPRESSEDTEXTURESUBIMAGE2DEXTPROC)load("glCompressedTextureSubImage2DEXT");
	glad_glCompressedTextureSubImage1DEXT = (PFNGLCOMPRESSEDTEXTURESUBIMAGE1DEXTPROC)load("glCompressedTextureSubImage1DEXT");
	glad_glGetCompressedTextureImageEXT = (PFNGLGETCOMPRESSEDTEXTUREIMAGEEXTPROC)load("glGetCompressedTextureImageEXT");
	glad_glCompressedMultiTexImage3DEXT = (PFNGLCOMPRESSEDMULTITEXIMAGE3DEXTPROC)load("glCompressedMultiTexImage3DEXT");
	glad_glCompressedMultiTexImage2DEXT = (PFNGLCOMPRESSEDMULTITEXIMAGE2DEXTPROC)load("glCompressedMultiTexImage2DEXT");
	glad_glCompressedMultiTexImage1DEXT = (PFNGLCOMPRESSEDMULTITEXIMAGE1DEXTPROC)load("glCompressedMultiTexImage1DEXT");
	glad_glCompressedMultiTexSubImage3DEXT = (PFNGLCOMPRESSEDMULTITEXSUBIMAGE3DEXTPROC)load("glCompressedMultiTexSubImage3DEXT");
	glad_glCompressedMultiTexSubImage2DEXT = (PFNGLCOMPRESSEDMULTITEXSUBIMAGE2DEXTPROC)load("glCompressedMultiTexSubImage2DEXT");
	glad_glCompressedMultiTexSubImage1DEXT = (PFNGLCOMPRESSEDMULTITEXSUBIMAGE1DEXTPROC)load("glCompressedMultiTexSubImage1DEXT");
	glad_glGetCompressedMultiTexImageEXT = (PFNGLGETCOMPRESSEDMULTITEXIMAGEEXTPROC)load("glGetCompressedMultiTexImageEXT");
	glad_glMatrixLoadTransposefEXT = (PFNGLMATRIXLOADTRANSPOSEFEXTPROC)load("glMatrixLoadTransposefEXT");
	glad_glMatrixLoadTransposedEXT = (PFNGLMATRIXLOADTRANSPOSEDEXTPROC)load("glMatrixLoadTransposedEXT");
	glad_glMatrixMultTransposefEXT = (PFNGLMATRIXMULTTRANSPOSEFEXTPROC)load("glMatrixMultTransposefEXT");
	glad_glMatrixMultTransposedEXT = (PFNGLMATRIXMULTTRANSPOSEDEXTPROC)load("glMatrixMultTransposedEXT");
	glad_glNamedBufferDataEXT = (PFNGLNAMEDBUFFERDATAEXTPROC)load("glNamedBufferDataEXT");
	glad_glNamedBufferSubDataEXT = (PFNGLNAMEDBUFFERSUBDATAEXTPROC)load("glNamedBufferSubDataEXT");
	glad_glMapNamedBufferEXT = (PFNGLMAPNAMEDBUFFEREXTPROC)load("glMapNamedBufferEXT");
	glad_glUnmapNamedBufferEXT = (PFNGLUNMAPNAMEDBUFFEREXTPROC)load("glUnmapNamedBufferEXT");
	glad_glGetNamedBufferParameterivEXT = (PFNGLGETNAMEDBUFFERPARAMETERIVEXTPROC)load("glGetNamedBufferParameterivEXT");
	glad_glGetNamedBufferPointervEXT = (PFNGLGETNAMEDBUFFERPOINTERVEXTPROC)load("glGetNamedBufferPointervEXT");
	glad_glGetNamedBufferSubDataEXT = (PFNGLGETNAMEDBUFFERSUBDATAEXTPROC)load("glGetNamedBufferSubDataEXT");
	glad_glProgramUniform1fEXT = (PFNGLPROGRAMUNIFORM1FEXTPROC)load("glProgramUniform1fEXT");
	glad_glProgramUniform2fEXT = (PFNGLPROGRAMUNIFORM2FEXTPROC)load("glProgramUniform2fEXT");
	glad_glProgramUniform3fEXT = (PFNGLPROGRAMUNIFORM3FEXTPROC)load("glProgramUniform3fEXT");
	glad_glProgramUniform4fEXT = (PFNGLPROGRAMUNIFORM4FEXTPROC)load("glProgramUniform4fEXT");
	glad_glProgramUniform1iEXT = (PFNGLPROGRAMUNIFORM1IEXTPROC)load("glProgramUniform1iEXT");
	glad_glProgramUniform2iEXT = (PFNGLPROGRAMUNIFORM2IEXTPROC)load("glProgramUniform2iEXT");
	glad_glProgramUniform3iEXT = (PFNGLPROGRAMUNIFORM3IEXTPROC)load("glProgramUniform3iEXT");
	glad_glProgramUniform4iEXT = (PFNGLPROGRAMUNIFORM4IEXTPROC)load("glProgramUniform4iEXT");
	glad_glProgramUniform1fvEXT = (PFNGLPROGRAMUNIFORM1FVEXTPROC)load("glProgramUniform1fvEXT");
	glad_glProgramUniform2fvEXT = (PFNGLPROGRAMUNIFORM2FVEXTPROC)load("glProgramUniform2fvEXT");
	glad_glProgramUniform3fvEXT = (PFNGLPROGRAMUNIFORM3FVEXTPROC)load("glProgramUniform3fvEXT");
	glad_glProgramUniform4fvEXT = (PFNGLPROGRAMUNIFORM4FVEXTPROC)load("glProgramUniform4fvEXT");
	glad_glProgramUniform1ivEXT = (PFNGLPROGRAMUNIFORM1IVEXTPROC)load("glProgramUniform1ivEXT");
	glad_glProgramUniform2ivEXT = (PFNGLPROGRAMUNIFORM2IVEXTPROC)load("glProgramUniform2ivEXT");
	glad_glProgramUniform3ivEXT = (PFNGLPROGRAMUNIFORM3IVEXTPROC)load("glProgramUniform3ivEXT");
	glad_glProgramUniform4ivEXT = (PFNGLPROGRAMUNIFORM4IVEXTPROC)load("glProgramUniform4ivEXT");
	glad_glProgramUniformMatrix2fvEXT = (PFNGLPROGRAMUNIFORMMATRIX2FVEXTPROC)load("glProgramUniformMatrix2fvEXT");
	glad_glProgramUniformMatrix3fvEXT = (PFNGLPROGRAMUNIFORMMATRIX3FVEXTPROC)load("glProgramUniformMatrix3fvEXT");
	glad_glProgramUniformMatrix4fvEXT = (PFNGLPROGRAMUNIFORMMATRIX4FVEXTPROC)load("glProgramUniformMatrix4fvEXT");
	glad_glProgramUniformMatrix2x3fvEXT = (PFNGLPROGRAMUNIFORMMATRIX2X3FVEXTPROC)load("glProgramUniformMatrix2x3fvEXT");
	glad_glProgramUniformMatrix3x2fvEXT = (PFNGLPROGRAMUNIFORMMATRIX3X2FVEXTPROC)load("glProgramUniformMatrix3x2fvEXT");
	glad_glProgramUniformMatrix2x4fvEXT = (PFNGLPROGRAMUNIFORMMATRIX2X4FVEXTPROC)load("glProgramUniformMatrix2x4fvEXT");
	glad_glProgramUniformMatrix4x2fvEXT = (PFNGLPROGRAMUNIFORMMATRIX4X2FVEXTPROC)load("glProgramUniformMatrix4x2fvEXT");
	glad_glProgramUniformMatrix3x4fvEXT = (PFNGLPROGRAMUNIFORMMATRIX3X4FVEXTPROC)load("glProgramUniformMatrix3x4fvEXT");
	glad_glProgramUniformMatrix4x3fvEXT = (PFNGLPROGRAMUNIFORMMATRIX4X3FVEXTPROC)load("glProgramUniformMatrix4x3fvEXT");
	glad_glTextureBufferEXT = (PFNGLTEXTUREBUFFEREXTPROC)load("glTextureBufferEXT");
	glad_glMultiTexBufferEXT = (PFNGLMULTITEXBUFFEREXTPROC)load("glMultiTexBufferEXT");
	glad_glTextureParameterIivEXT = (PFNGLTEXTUREPARAMETERIIVEXTPROC)load("glTextureParameterIivEXT");
	glad_glTextureParameterIuivEXT = (PFNGLTEXTUREPARAMETERIUIVEXTPROC)load("glTextureParameterIuivEXT");
	glad_glGetTextureParameterIivEXT = (PFNGLGETTEXTUREPARAMETERIIVEXTPROC)load("glGetTextureParameterIivEXT");
	glad_glGetTextureParameterIuivEXT = (PFNGLGETTEXTUREPARAMETERIUIVEXTPROC)load("glGetTextureParameterIuivEXT");
	glad_glMultiTexParameterIivEXT = (PFNGLMULTITEXPARAMETERIIVEXTPROC)load("glMultiTexParameterIivEXT");
	glad_glMultiTexParameterIuivEXT = (PFNGLMULTITEXPARAMETERIUIVEXTPROC)load("glMultiTexParameterIuivEXT");
	glad_glGetMultiTexParameterIivEXT = (PFNGLGETMULTITEXPARAMETERIIVEXTPROC)load("glGetMultiTexParameterIivEXT");
	glad_glGetMultiTexParameterIuivEXT = (PFNGLGETMULTITEXPARAMETERIUIVEXTPROC)load("glGetMultiTexParameterIuivEXT");
	glad_glProgramUniform1uiEXT = (PFNGLPROGRAMUNIFORM1UIEXTPROC)load("glProgramUniform1uiEXT");
	glad_glProgramUniform2uiEXT = (PFNGLPROGRAMUNIFORM2UIEXTPROC)load("glProgramUniform2uiEXT");
	glad_glProgramUniform3uiEXT = (PFNGLPROGRAMUNIFORM3UIEXTPROC)load("glProgramUniform3uiEXT");
	glad_glProgramUniform4uiEXT = (PFNGLPROGRAMUNIFORM4UIEXTPROC)load("glProgramUniform4uiEXT");
	glad_glProgramUniform1uivEXT = (PFNGLPROGRAMUNIFORM1UIVEXTPROC)load("glProgramUniform1uivEXT");
	glad_glProgramUniform2uivEXT = (PFNGLPROGRAMUNIFORM2UIVEXTPROC)load("glProgramUniform2uivEXT");
	glad_glProgramUniform3uivEXT = (PFNGLPROGRAMUNIFORM3UIVEXTPROC)load("glProgramUniform3uivEXT");
	glad_glProgramUniform4uivEXT = (PFNGLPROGRAMUNIFORM4UIVEXTPROC)load("glProgramUniform4uivEXT");
	glad_glNamedProgramLocalParameters4fvEXT = (PFNGLNAMEDPROGRAMLOCALPARAMETERS4FVEXTPROC)load("glNamedProgramLocalParameters4fvEXT");
	glad_glNamedProgramLocalParameterI4iEXT = (PFNGLNAMEDPROGRAMLOCALPARAMETERI4IEXTPROC)load("glNamedProgramLocalParameterI4iEXT");
	glad_glNamedProgramLocalParameterI4ivEXT = (PFNGLNAMEDPROGRAMLOCALPARAMETERI4IVEXTPROC)load("glNamedProgramLocalParameterI4ivEXT");
	glad_glNamedProgramLocalParametersI4ivEXT = (PFNGLNAMEDPROGRAMLOCALPARAMETERSI4IVEXTPROC)load("glNamedProgramLocalParametersI4ivEXT");
	glad_glNamedProgramLocalParameterI4uiEXT = (PFNGLNAMEDPROGRAMLOCALPARAMETERI4UIEXTPROC)load("glNamedProgramLocalParameterI4uiEXT");
	glad_glNamedProgramLocalParameterI4uivEXT = (PFNGLNAMEDPROGRAMLOCALPARAMETERI4UIVEXTPROC)load("glNamedProgramLocalParameterI4uivEXT");
	glad_glNamedProgramLocalParametersI4uivEXT = (PFNGLNAMEDPROGRAMLOCALPARAMETERSI4UIVEXTPROC)load("glNamedProgramLocalParametersI4uivEXT");
	glad_glGetNamedProgramLocalParameterIivEXT = (PFNGLGETNAMEDPROGRAMLOCALPARAMETERIIVEXTPROC)load("glGetNamedProgramLocalParameterIivEXT");
	glad_glGetNamedProgramLocalParameterIuivEXT = (PFNGLGETNAMEDPROGRAMLOCALPARAMETERIUIVEXTPROC)load("glGetNamedProgramLocalParameterIuivEXT");
	glad_glEnableClientStateiEXT = (PFNGLENABLECLIENTSTATEIEXTPROC)load("glEnableClientStateiEXT");
	glad_glDisableClientStateiEXT = (PFNGLDISABLECLIENTSTATEIEXTPROC)load("glDisableClientStateiEXT");
	glad_glGetFloati_vEXT = (PFNGLGETFLOATI_VEXTPROC)load("glGetFloati_vEXT");
	glad_glGetDoublei_vEXT = (PFNGLGETDOUBLEI_VEXTPROC)load("glGetDoublei_vEXT");
	glad_glGetPointeri_vEXT = (PFNGLGETPOINTERI_VEXTPROC)load("glGetPointeri_vEXT");
	glad_glNamedProgramStringEXT = (PFNGLNAMEDPROGRAMSTRINGEXTPROC)load("glNamedProgramStringEXT");
	glad_glNamedProgramLocalParameter4dEXT = (PFNGLNAMEDPROGRAMLOCALPARAMETER4DEXTPROC)load("glNamedProgramLocalParameter4dEXT");
	glad_glNamedProgramLocalParameter4dvEXT = (PFNGLNAMEDPROGRAMLOCALPARAMETER4DVEXTPROC)load("glNamedProgramLocalParameter4dvEXT");
	glad_glNamedProgramLocalParameter4fEXT = (PFNGLNAMEDPROGRAMLOCALPARAMETER4FEXTPROC)load("glNamedProgramLocalParameter4fEXT");
	glad_glNamedProgramLocalParameter4fvEXT = (PFNGLNAMEDPROGRAMLOCALPARAMETER4FVEXTPROC)load("glNamedProgramLocalParameter4fvEXT");
	glad_glGetNamedProgramLocalParameterdvEXT = (PFNGLGETNAMEDPROGRAMLOCALPARAMETERDVEXTPROC)load("glGetNamedProgramLocalParameterdvEXT");
	glad_glGetNamedProgramLocalParameterfvEXT = (PFNGLGETNAMEDPROGRAMLOCALPARAMETERFVEXTPROC)load("glGetNamedProgramLocalParameterfvEXT");
	glad_glGetNamedProgramivEXT = (PFNGLGETNAMEDPROGRAMIVEXTPROC)load("glGetNamedProgramivEXT");
	glad_glGetNamedProgramStringEXT = (PFNGLGETNAMEDPROGRAMSTRINGEXTPROC)load("glGetNamedProgramStringEXT");
	glad_glNamedRenderbufferStorageEXT = (PFNGLNAMEDRENDERBUFFERSTORAGEEXTPROC)load("glNamedRenderbufferStorageEXT");
	glad_glGetNamedRenderbufferParameterivEXT = (PFNGLGETNAMEDRENDERBUFFERPARAMETERIVEXTPROC)load("glGetNamedRenderbufferParameterivEXT");
	glad_glNamedRenderbufferStorageMultisampleEXT = (PFNGLNAMEDRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC)load("glNamedRenderbufferStorageMultisampleEXT");
	glad_glNamedRenderbufferStorageMultisampleCoverageEXT = (PFNGLNAMEDRENDERBUFFERSTORAGEMULTISAMPLECOVERAGEEXTPROC)load("glNamedRenderbufferStorageMultisampleCoverageEXT");
	glad_glCheckNamedFramebufferStatusEXT = (PFNGLCHECKNAMEDFRAMEBUFFERSTATUSEXTPROC)load("glCheckNamedFramebufferStatusEXT");
	glad_glNamedFramebufferTexture1DEXT = (PFNGLNAMEDFRAMEBUFFERTEXTURE1DEXTPROC)load("glNamedFramebufferTexture1DEXT");
	glad_glNamedFramebufferTexture2DEXT = (PFNGLNAMEDFRAMEBUFFERTEXTURE2DEXTPROC)load("glNamedFramebufferTexture2DEXT");
	glad_glNamedFramebufferTexture3DEXT = (PFNGLNAMEDFRAMEBUFFERTEXTURE3DEXTPROC)load("glNamedFramebufferTexture3DEXT");
	glad_glNamedFramebufferRenderbufferEXT = (PFNGLNAMEDFRAMEBUFFERRENDERBUFFEREXTPROC)load("glNamedFramebufferRenderbufferEXT");
	glad_glGetNamedFramebufferAttachmentParameterivEXT = (PFNGLGETNAMEDFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC)load("glGetNamedFramebufferAttachmentParameterivEXT");
	glad_glGenerateTextureMipmapEXT = (PFNGLGENERATETEXTUREMIPMAPEXTPROC)load("glGenerateTextureMipmapEXT");
	glad_glGenerateMultiTexMipmapEXT = (PFNGLGENERATEMULTITEXMIPMAPEXTPROC)load("glGenerateMultiTexMipmapEXT");
	glad_glFramebufferDrawBufferEXT = (PFNGLFRAMEBUFFERDRAWBUFFEREXTPROC)load("glFramebufferDrawBufferEXT");
	glad_glFramebufferDrawBuffersEXT = (PFNGLFRAMEBUFFERDRAWBUFFERSEXTPROC)load("glFramebufferDrawBuffersEXT");
	glad_glFramebufferReadBufferEXT = (PFNGLFRAMEBUFFERREADBUFFEREXTPROC)load("glFramebufferReadBufferEXT");
	glad_glGetFramebufferParameterivEXT = (PFNGLGETFRAMEBUFFERPARAMETERIVEXTPROC)load("glGetFramebufferParameterivEXT");
	glad_glNamedCopyBufferSubDataEXT = (PFNGLNAMEDCOPYBUFFERSUBDATAEXTPROC)load("glNamedCopyBufferSubDataEXT");
	glad_glNamedFramebufferTextureEXT = (PFNGLNAMEDFRAMEBUFFERTEXTUREEXTPROC)load("glNamedFramebufferTextureEXT");
	glad_glNamedFramebufferTextureLayerEXT = (PFNGLNAMEDFRAMEBUFFERTEXTURELAYEREXTPROC)load("glNamedFramebufferTextureLayerEXT");
	glad_glNamedFramebufferTextureFaceEXT = (PFNGLNAMEDFRAMEBUFFERTEXTUREFACEEXTPROC)load("glNamedFramebufferTextureFaceEXT");
	glad_glTextureRenderbufferEXT = (PFNGLTEXTURERENDERBUFFEREXTPROC)load("glTextureRenderbufferEXT");
	glad_glMultiTexRenderbufferEXT = (PFNGLMULTITEXRENDERBUFFEREXTPROC)load("glMultiTexRenderbufferEXT");
	glad_glVertexArrayVertexOffsetEXT = (PFNGLVERTEXARRAYVERTEXOFFSETEXTPROC)load("glVertexArrayVertexOffsetEXT");
	glad_glVertexArrayColorOffsetEXT = (PFNGLVERTEXARRAYCOLOROFFSETEXTPROC)load("glVertexArrayColorOffsetEXT");
	glad_glVertexArrayEdgeFlagOffsetEXT = (PFNGLVERTEXARRAYEDGEFLAGOFFSETEXTPROC)load("glVertexArrayEdgeFlagOffsetEXT");
	glad_glVertexArrayIndexOffsetEXT = (PFNGLVERTEXARRAYINDEXOFFSETEXTPROC)load("glVertexArrayIndexOffsetEXT");
	glad_glVertexArrayNormalOffsetEXT = (PFNGLVERTEXARRAYNORMALOFFSETEXTPROC)load("glVertexArrayNormalOffsetEXT");
	glad_glVertexArrayTexCoordOffsetEXT = (PFNGLVERTEXARRAYTEXCOORDOFFSETEXTPROC)load("glVertexArrayTexCoordOffsetEXT");
	glad_glVertexArrayMultiTexCoordOffsetEXT = (PFNGLVERTEXARRAYMULTITEXCOORDOFFSETEXTPROC)load("glVertexArrayMultiTexCoordOffsetEXT");
	glad_glVertexArrayFogCoordOffsetEXT = (PFNGLVERTEXARRAYFOGCOORDOFFSETEXTPROC)load("glVertexArrayFogCoordOffsetEXT");
	glad_glVertexArraySecondaryColorOffsetEXT = (PFNGLVERTEXARRAYSECONDARYCOLOROFFSETEXTPROC)load("glVertexArraySecondaryColorOffsetEXT");
	glad_glVertexArrayVertexAttribOffsetEXT = (PFNGLVERTEXARRAYVERTEXATTRIBOFFSETEXTPROC)load("glVertexArrayVertexAttribOffsetEXT");
	glad_glVertexArrayVertexAttribIOffsetEXT = (PFNGLVERTEXARRAYVERTEXATTRIBIOFFSETEXTPROC)load("glVertexArrayVertexAttribIOffsetEXT");
	glad_glEnableVertexArrayEXT = (PFNGLENABLEVERTEXARRAYEXTPROC)load("glEnableVertexArrayEXT");
	glad_glDisableVertexArrayEXT = (PFNGLDISABLEVERTEXARRAYEXTPROC)load("glDisableVertexArrayEXT");
	glad_glEnableVertexArrayAttribEXT = (PFNGLENABLEVERTEXARRAYATTRIBEXTPROC)load("glEnableVertexArrayAttribEXT");
	glad_glDisableVertexArrayAttribEXT = (PFNGLDISABLEVERTEXARRAYATTRIBEXTPROC)load("glDisableVertexArrayAttribEXT");
	glad_glGetVertexArrayIntegervEXT = (PFNGLGETVERTEXARRAYINTEGERVEXTPROC)load("glGetVertexArrayIntegervEXT");
	glad_glGetVertexArrayPointervEXT = (PFNGLGETVERTEXARRAYPOINTERVEXTPROC)load("glGetVertexArrayPointervEXT");
	glad_glGetVertexArrayIntegeri_vEXT = (PFNGLGETVERTEXARRAYINTEGERI_VEXTPROC)load("glGetVertexArrayIntegeri_vEXT");
	glad_glGetVertexArrayPointeri_vEXT = (PFNGLGETVERTEXARRAYPOINTERI_VEXTPROC)load("glGetVertexArrayPointeri_vEXT");
	glad_glMapNamedBufferRangeEXT = (PFNGLMAPNAMEDBUFFERRANGEEXTPROC)load("glMapNamedBufferRangeEXT");
	glad_glFlushMappedNamedBufferRangeEXT = (PFNGLFLUSHMAPPEDNAMEDBUFFERRANGEEXTPROC)load("glFlushMappedNamedBufferRangeEXT");
	glad_glNamedBufferStorageEXT = (PFNGLNAMEDBUFFERSTORAGEEXTPROC)load("glNamedBufferStorageEXT");
	glad_glClearNamedBufferDataEXT = (PFNGLCLEARNAMEDBUFFERDATAEXTPROC)load("glClearNamedBufferDataEXT");
	glad_glClearNamedBufferSubDataEXT = (PFNGLCLEARNAMEDBUFFERSUBDATAEXTPROC)load("glClearNamedBufferSubDataEXT");
	glad_glNamedFramebufferParameteriEXT = (PFNGLNAMEDFRAMEBUFFERPARAMETERIEXTPROC)load("glNamedFramebufferParameteriEXT");
	glad_glGetNamedFramebufferParameterivEXT = (PFNGLGETNAMEDFRAMEBUFFERPARAMETERIVEXTPROC)load("glGetNamedFramebufferParameterivEXT");
	glad_glProgramUniform1dEXT = (PFNGLPROGRAMUNIFORM1DEXTPROC)load("glProgramUniform1dEXT");
	glad_glProgramUniform2dEXT = (PFNGLPROGRAMUNIFORM2DEXTPROC)load("glProgramUniform2dEXT");
	glad_glProgramUniform3dEXT = (PFNGLPROGRAMUNIFORM3DEXTPROC)load("glProgramUniform3dEXT");
	glad_glProgramUniform4dEXT = (PFNGLPROGRAMUNIFORM4DEXTPROC)load("glProgramUniform4dEXT");
	glad_glProgramUniform1dvEXT = (PFNGLPROGRAMUNIFORM1DVEXTPROC)load("glProgramUniform1dvEXT");
	glad_glProgramUniform2dvEXT = (PFNGLPROGRAMUNIFORM2DVEXTPROC)load("glProgramUniform2dvEXT");
	glad_glProgramUniform3dvEXT = (PFNGLPROGRAMUNIFORM3DVEXTPROC)load("glProgramUniform3dvEXT");
	glad_glProgramUniform4dvEXT = (PFNGLPROGRAMUNIFORM4DVEXTPROC)load("glProgramUniform4dvEXT");
	glad_glProgramUniformMatrix2dvEXT = (PFNGLPROGRAMUNIFORMMATRIX2DVEXTPROC)load("glProgramUniformMatrix2dvEXT");
	glad_glProgramUniformMatrix3dvEXT = (PFNGLPROGRAMUNIFORMMATRIX3DVEXTPROC)load("glProgramUniformMatrix3dvEXT");
	glad_glProgramUniformMatrix4dvEXT = (PFNGLPROGRAMUNIFORMMATRIX4DVEXTPROC)load("glProgramUniformMatrix4dvEXT");
	glad_glProgramUniformMatrix2x3dvEXT = (PFNGLPROGRAMUNIFORMMATRIX2X3DVEXTPROC)load("glProgramUniformMatrix2x3dvEXT");
	glad_glProgramUniformMatrix2x4dvEXT = (PFNGLPROGRAMUNIFORMMATRIX2X4DVEXTPROC)load("glProgramUniformMatrix2x4dvEXT");
	glad_glProgramUniformMatrix3x2dvEXT = (PFNGLPROGRAMUNIFORMMATRIX3X2DVEXTPROC)load("glProgramUniformMatrix3x2dvEXT");
	glad_glProgramUniformMatrix3x4dvEXT = (PFNGLPROGRAMUNIFORMMATRIX3X4DVEXTPROC)load("glProgramUniformMatrix3x4dvEXT");
	glad_glProgramUniformMatrix4x2dvEXT = (PFNGLPROGRAMUNIFORMMATRIX4X2DVEXTPROC)load("glProgramUniformMatrix4x2dvEXT");
	glad_glProgramUniformMatrix4x3dvEXT = (PFNGLPROGRAMUNIFORMMATRIX4X3DVEXTPROC)load("glProgramUniformMatrix4x3dvEXT");
	glad_glTextureBufferRangeEXT = (PFNGLTEXTUREBUFFERRANGEEXTPROC)load("glTextureBufferRangeEXT");
	glad_glTextureStorage1DEXT = (PFNGLTEXTURESTORAGE1DEXTPROC)load("glTextureStorage1DEXT");
	glad_glTextureStorage2DEXT = (PFNGLTEXTURESTORAGE2DEXTPROC)load("glTextureStorage2DEXT");
	glad_glTextureStorage3DEXT = (PFNGLTEXTURESTORAGE3DEXTPROC)load("glTextureStorage3DEXT");
	glad_glTextureStorage2DMultisampleEXT = (PFNGLTEXTURESTORAGE2DMULTISAMPLEEXTPROC)load("glTextureStorage2DMultisampleEXT");
	glad_glTextureStorage3DMultisampleEXT = (PFNGLTEXTURESTORAGE3DMULTISAMPLEEXTPROC)load("glTextureStorage3DMultisampleEXT");
	glad_glVertexArrayBindVertexBufferEXT = (PFNGLVERTEXARRAYBINDVERTEXBUFFEREXTPROC)load("glVertexArrayBindVertexBufferEXT");
	glad_glVertexArrayVertexAttribFormatEXT = (PFNGLVERTEXARRAYVERTEXATTRIBFORMATEXTPROC)load("glVertexArrayVertexAttribFormatEXT");
	glad_glVertexArrayVertexAttribIFormatEXT = (PFNGLVERTEXARRAYVERTEXATTRIBIFORMATEXTPROC)load("glVertexArrayVertexAttribIFormatEXT");
	glad_glVertexArrayVertexAttribLFormatEXT = (PFNGLVERTEXARRAYVERTEXATTRIBLFORMATEXTPROC)load("glVertexArrayVertexAttribLFormatEXT");
	glad_glVertexArrayVertexAttribBindingEXT = (PFNGLVERTEXARRAYVERTEXATTRIBBINDINGEXTPROC)load("glVertexArrayVertexAttribBindingEXT");
	glad_glVertexArrayVertexBindingDivisorEXT = (PFNGLVERTEXARRAYVERTEXBINDINGDIVISOREXTPROC)load("glVertexArrayVertexBindingDivisorEXT");
	glad_glVertexArrayVertexAttribLOffsetEXT = (PFNGLVERTEXARRAYVERTEXATTRIBLOFFSETEXTPROC)load("glVertexArrayVertexAttribLOffsetEXT");
	glad_glTexturePageCommitmentEXT = (PFNGLTEXTUREPAGECOMMITMENTEXTPROC)load("glTexturePageCommitmentEXT");
	glad_glVertexArrayVertexAttribDivisorEXT = (PFNGLVERTEXARRAYVERTEXATTRIBDIVISOREXTPROC)load("glVertexArrayVertexAttribDivisorEXT");
}
static void load_GL_EXT_draw_buffers2(GLADloadproc load) {
	if(!GLAD_GL_EXT_draw_buffers2) return;
	glad_glColorMaskIndexedEXT = (PFNGLCOLORMASKINDEXEDEXTPROC)load("glColorMaskIndexedEXT");
	glad_glGetBooleanIndexedvEXT = (PFNGLGETBOOLEANINDEXEDVEXTPROC)load("glGetBooleanIndexedvEXT");
	glad_glGetIntegerIndexedvEXT = (PFNGLGETINTEGERINDEXEDVEXTPROC)load("glGetIntegerIndexedvEXT");
	glad_glEnableIndexedEXT = (PFNGLENABLEINDEXEDEXTPROC)load("glEnableIndexedEXT");
	glad_glDisableIndexedEXT = (PFNGLDISABLEINDEXEDEXTPROC)load("glDisableIndexedEXT");
	glad_glIsEnabledIndexedEXT = (PFNGLISENABLEDINDEXEDEXTPROC)load("glIsEnabledIndexedEXT");
}
static void load_GL_EXT_draw_instanced(GLADloadproc load) {
	if(!GLAD_GL_EXT_draw_instanced) return;
	glad_glDrawArraysInstancedEXT = (PFNGLDRAWARRAYSINSTANCEDEXTPROC)load("glDrawArraysInstancedEXT");
	glad_glDrawElementsInstancedEXT = (PFNGLDRAWELEMENTSINSTANCEDEXTPROC)load("glDrawElementsInstancedEXT");
}
static void load_GL_EXT_draw_range_elements(GLADloadproc load) {
	if(!GLAD_GL_EXT_draw_range_elements) return;
	glad_glDrawRangeElementsEXT = (PFNGLDRAWRANGEELEMENTSEXTPROC)load("glDrawRangeElementsEXT");
}
static void load_GL_EXT_external_buffer(GLADloadproc load) {
	if(!GLAD_GL_EXT_external_buffer) return;
	glad_glBufferStorageExternalEXT = (PFNGLBUFFERSTORAGEEXTERNALEXTPROC)load("glBufferStorageExternalEXT");
	glad_glNamedBufferStorageExternalEXT = (PFNGLNAMEDBUFFERSTORAGEEXTERNALEXTPROC)load("glNamedBufferStorageExternalEXT");
}
static void load_GL_EXT_fog_coord(GLADloadproc load) {
	if(!GLAD_GL_EXT_fog_coord) return;
	glad_glFogCoordfEXT = (PFNGLFOGCOORDFEXTPROC)load("glFogCoordfEXT");
	glad_glFogCoordfvEXT = (PFNGLFOGCOORDFVEXTPROC)load("glFogCoordfvEXT");
	glad_glFogCoorddEXT = (PFNGLFOGCOORDDEXTPROC)load("glFogCoorddEXT");
	glad_glFogCoorddvEXT = (PFNGLFOGCOORDDVEXTPROC)load("glFogCoorddvEXT");
	glad_glFogCoordPointerEXT = (PFNGLFOGCOORDPOINTEREXTPROC)load("glFogCoordPointerEXT");
}
static void load_GL_EXT_framebuffer_blit(GLADloadproc load) {
	if(!GLAD_GL_EXT_framebuffer_blit) return;
	glad_glBlitFramebufferEXT = (PFNGLBLITFRAMEBUFFEREXTPROC)load("glBlitFramebufferEXT");
}
static void load_GL_EXT_framebuffer_blit_layers(GLADloadproc load) {
	if(!GLAD_GL_EXT_framebuffer_blit_layers) return;
	glad_glBlitFramebufferLayersEXT = (PFNGLBLITFRAMEBUFFERLAYERSEXTPROC)load("glBlitFramebufferLayersEXT");
	glad_glBlitFramebufferLayerEXT = (PFNGLBLITFRAMEBUFFERLAYEREXTPROC)load("glBlitFramebufferLayerEXT");
}
static void load_GL_EXT_framebuffer_multisample(GLADloadproc load) {
	if(!GLAD_GL_EXT_framebuffer_multisample) return;
	glad_glRenderbufferStorageMultisampleEXT = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC)load("glRenderbufferStorageMultisampleEXT");
}
static void load_GL_EXT_framebuffer_object(GLADloadproc load) {
	if(!GLAD_GL_EXT_framebuffer_object) return;
	glad_glIsRenderbufferEXT = (PFNGLISRENDERBUFFEREXTPROC)load("glIsRenderbufferEXT");
	glad_glBindRenderbufferEXT = (PFNGLBINDRENDERBUFFEREXTPROC)load("glBindRenderbufferEXT");
	glad_glDeleteRenderbuffersEXT = (PFNGLDELETERENDERBUFFERSEXTPROC)load("glDeleteRenderbuffersEXT");
	glad_glGenRenderbuffersEXT = (PFNGLGENRENDERBUFFERSEXTPROC)load("glGenRenderbuffersEXT");
	glad_glRenderbufferStorageEXT = (PFNGLRENDERBUFFERSTORAGEEXTPROC)load("glRenderbufferStorageEXT");
	glad_glGetRenderbufferParameterivEXT = (PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC)load("glGetRenderbufferParameterivEXT");
	glad_glIsFramebufferEXT = (PFNGLISFRAMEBUFFEREXTPROC)load("glIsFramebufferEXT");
	glad_glBindFramebufferEXT = (PFNGLBINDFRAMEBUFFEREXTPROC)load("glBindFramebufferEXT");
	glad_glDeleteFramebuffersEXT = (PFNGLDELETEFRAMEBUFFERSEXTPROC)load("glDeleteFramebuffersEXT");
	glad_glGenFramebuffersEXT = (PFNGLGENFRAMEBUFFERSEXTPROC)load("glGenFramebuffersEXT");
	glad_glCheckFramebufferStatusEXT = (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC)load("glCheckFramebufferStatusEXT");
	glad_glFramebufferTexture1DEXT = (PFNGLFRAMEBUFFERTEXTURE1DEXTPROC)load("glFramebufferTexture1DEXT");
	glad_glFramebufferTexture2DEXT = (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)load("glFramebufferTexture2DEXT");
	glad_glFramebufferTexture3DEXT = (PFNGLFRAMEBUFFERTEXTURE3DEXTPROC)load("glFramebufferTexture3DEXT");
	glad_glFramebufferRenderbufferEXT = (PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC)load("glFramebufferRenderbufferEXT");
	glad_glGetFramebufferAttachmentParameterivEXT = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC)load("glGetFramebufferAttachmentParameterivEXT");
	glad_glGenerateMipmapEXT = (PFNGLGENERATEMIPMAPEXTPROC)load("glGenerateMipmapEXT");
}
static void load_GL_EXT_geometry_shader4(GLADloadproc load) {
	if(!GLAD_GL_EXT_geometry_shader4) return;
	glad_glProgramParameteriEXT = (PFNGLPROGRAMPARAMETERIEXTPROC)load("glProgramParameteriEXT");
}
static void load_GL_EXT_gpu_program_parameters(GLADloadproc load) {
	if(!GLAD_GL_EXT_gpu_program_parameters) return;
	glad_glProgramEnvParameters4fvEXT = (PFNGLPROGRAMENVPARAMETERS4FVEXTPROC)load("glProgramEnvParameters4fvEXT");
	glad_glProgramLocalParameters4fvEXT = (PFNGLPROGRAMLOCALPARAMETERS4FVEXTPROC)load("glProgramLocalParameters4fvEXT");
}
static void load_GL_EXT_gpu_shader4(GLADloadproc load) {
	if(!GLAD_GL_EXT_gpu_shader4) return;
	glad_glGetUniformuivEXT = (PFNGLGETUNIFORMUIVEXTPROC)load("glGetUniformuivEXT");
	glad_glBindFragDataLocationEXT = (PFNGLBINDFRAGDATALOCATIONEXTPROC)load("glBindFragDataLocationEXT");
	glad_glGetFragDataLocationEXT = (PFNGLGETFRAGDATALOCATIONEXTPROC)load("glGetFragDataLocationEXT");
	glad_glUniform1uiEXT = (PFNGLUNIFORM1UIEXTPROC)load("glUniform1uiEXT");
	glad_glUniform2uiEXT = (PFNGLUNIFORM2UIEXTPROC)load("glUniform2uiEXT");
	glad_glUniform3uiEXT = (PFNGLUNIFORM3UIEXTPROC)load("glUniform3uiEXT");
	glad_glUniform4uiEXT = (PFNGLUNIFORM4UIEXTPROC)load("glUniform4uiEXT");
	glad_glUniform1uivEXT = (PFNGLUNIFORM1UIVEXTPROC)load("glUniform1uivEXT");
	glad_glUniform2uivEXT = (PFNGLUNIFORM2UIVEXTPROC)load("glUniform2uivEXT");
	glad_glUniform3uivEXT = (PFNGLUNIFORM3UIVEXTPROC)load("glUniform3uivEXT");
	glad_glUniform4uivEXT = (PFNGLUNIFORM4UIVEXTPROC)load("glUniform4uivEXT");
	glad_glVertexAttribI1iEXT = (PFNGLVERTEXATTRIBI1IEXTPROC)load("glVertexAttribI1iEXT");
	glad_glVertexAttribI2iEXT = (PFNGLVERTEXATTRIBI2IEXTPROC)load("glVertexAttribI2iEXT");
	glad_glVertexAttribI3iEXT = (PFNGLVERTEXATTRIBI3IEXTPROC)load("glVertexAttribI3iEXT");
	glad_glVertexAttribI4iEXT = (PFNGLVERTEXATTRIBI4IEXTPROC)load("glVertexAttribI4iEXT");
	glad_glVertexAttribI1uiEXT = (PFNGLVERTEXATTRIBI1UIEXTPROC)load("glVertexAttribI1uiEXT");
	glad_glVertexAttribI2uiEXT = (PFNGLVERTEXATTRIBI2UIEXTPROC)load("glVertexAttribI2uiEXT");
	glad_glVertexAttribI3uiEXT = (PFNGLVERTEXATTRIBI3UIEXTPROC)load("glVertexAttribI3uiEXT");
	glad_glVertexAttribI4uiEXT = (PFNGLVERTEXATTRIBI4UIEXTPROC)load("glVertexAttribI4uiEXT");
	glad_glVertexAttribI1ivEXT = (PFNGLVERTEXATTRIBI1IVEXTPROC)load("glVertexAttribI1ivEXT");
	glad_glVertexAttribI2ivEXT = (PFNGLVERTEXATTRIBI2IVEXTPROC)load("glVertexAttribI2ivEXT");
	glad_glVertexAttribI3ivEXT = (PFNGLVERTEXATTRIBI3IVEXTPROC)load("glVertexAttribI3ivEXT");
	glad_glVertexAttribI4ivEXT = (PFNGLVERTEXATTRIBI4IVEXTPROC)load("glVertexAttribI4ivEXT");
	glad_glVertexAttribI1uivEXT = (PFNGLVERTEXATTRIBI1UIVEXTPROC)load("glVertexAttribI1uivEXT");
	glad_glVertexAttribI2uivEXT = (PFNGLVERTEXATTRIBI2UIVEXTPROC)load("glVertexAttribI2uivEXT");
	glad_glVertexAttribI3uivEXT = (PFNGLVERTEXATTRIBI3UIVEXTPROC)load("glVertexAttribI3uivEXT");
	glad_glVertexAttribI4uivEXT = (PFNGLVERTEXATTRIBI4UIVEXTPROC)load("glVertexAttribI4uivEXT");
	glad_glVertexAttribI4bvEXT = (PFNGLVERTEXATTRIBI4BVEXTPROC)load("glVertexAttribI4bvEXT");
	glad_glVertexAttribI4svEXT = (PFNGLVERTEXATTRIBI4SVEXTPROC)load("glVertexAttribI4svEXT");
	glad_glVertexAttribI4ubvEXT = (PFNGLVERTEXATTRIBI4UBVEXTPROC)load("glVertexAttribI4ubvEXT");
	glad_glVertexAttribI4usvEXT = (PFNGLVERTEXATTRIBI4USVEXTPROC)load("glVertexAttribI4usvEXT");
	glad_glVertexAttribIPointerEXT = (PFNGLVERTEXATTRIBIPOINTEREXTPROC)load("glVertexAttribIPointerEXT");
	glad_glGetVertexAttribIivEXT = (PFNGLGETVERTEXATTRIBIIVEXTPROC)load("glGetVertexAttribIivEXT");
	glad_glGetVertexAttribIuivEXT = (PFNGLGETVERTEXATTRIBIUIVEXTPROC)load("glGetVertexAttribIuivEXT");
}
static void load_GL_EXT_histogram(GLADloadproc load) {
	if(!GLAD_GL_EXT_histogram) return;
	glad_glGetHistogramEXT = (PFNGLGETHISTOGRAMEXTPROC)load("glGetHistogramEXT");
	glad_glGetHistogramParameterfvEXT = (PFNGLGETHISTOGRAMPARAMETERFVEXTPROC)load("glGetHistogramParameterfvEXT");
	glad_glGetHistogramParameterivEXT = (PFNGLGETHISTOGRAMPARAMETERIVEXTPROC)load("glGetHistogramParameterivEXT");
	glad_glGetMinmaxEXT = (PFNGLGETMINMAXEXTPROC)load("glGetMinmaxEXT");
	glad_glGetMinmaxParameterfvEXT = (PFNGLGETMINMAXPARAMETERFVEXTPROC)load("glGetMinmaxParameterfvEXT");
	glad_glGetMinmaxParameterivEXT = (PFNGLGETMINMAXPARAMETERIVEXTPROC)load("glGetMinmaxParameterivEXT");
	glad_glHistogramEXT = (PFNGLHISTOGRAMEXTPROC)load("glHistogramEXT");
	glad_glMinmaxEXT = (PFNGLMINMAXEXTPROC)load("glMinmaxEXT");
	glad_glResetHistogramEXT = (PFNGLRESETHISTOGRAMEXTPROC)load("glResetHistogramEXT");
	glad_glResetMinmaxEXT = (PFNGLRESETMINMAXEXTPROC)load("glResetMinmaxEXT");
}
static void load_GL_EXT_index_func(GLADloadproc load) {
	if(!GLAD_GL_EXT_index_func) return;
	glad_glIndexFuncEXT = (PFNGLINDEXFUNCEXTPROC)load("glIndexFuncEXT");
}
static void load_GL_EXT_index_material(GLADloadproc load) {
	if(!GLAD_GL_EXT_index_material) return;
	glad_glIndexMaterialEXT = (PFNGLINDEXMATERIALEXTPROC)load("glIndexMaterialEXT");
}
static void load_GL_EXT_light_texture(GLADloadproc load) {
	if(!GLAD_GL_EXT_light_texture) return;
	glad_glApplyTextureEXT = (PFNGLAPPLYTEXTUREEXTPROC)load("glApplyTextureEXT");
	glad_glTextureLightEXT = (PFNGLTEXTURELIGHTEXTPROC)load("glTextureLightEXT");
	glad_glTextureMaterialEXT = (PFNGLTEXTUREMATERIALEXTPROC)load("glTextureMaterialEXT");
}
static void load_GL_EXT_memory_object(GLADloadproc load) {
	if(!GLAD_GL_EXT_memory_object) return;
	glad_glGetUnsignedBytevEXT = (PFNGLGETUNSIGNEDBYTEVEXTPROC)load("glGetUnsignedBytevEXT");
	glad_glGetUnsignedBytei_vEXT = (PFNGLGETUNSIGNEDBYTEI_VEXTPROC)load("glGetUnsignedBytei_vEXT");
	glad_glDeleteMemoryObjectsEXT = (PFNGLDELETEMEMORYOBJECTSEXTPROC)load("glDeleteMemoryObjectsEXT");
	glad_glIsMemoryObjectEXT = (PFNGLISMEMORYOBJECTEXTPROC)load("glIsMemoryObjectEXT");
	glad_glCreateMemoryObjectsEXT = (PFNGLCREATEMEMORYOBJECTSEXTPROC)load("glCreateMemoryObjectsEXT");
	glad_glMemoryObjectParameterivEXT = (PFNGLMEMORYOBJECTPARAMETERIVEXTPROC)load("glMemoryObjectParameterivEXT");
	glad_glGetMemoryObjectParameterivEXT = (PFNGLGETMEMORYOBJECTPARAMETERIVEXTPROC)load("glGetMemoryObjectParameterivEXT");
	glad_glTexStorageMem2DEXT = (PFNGLTEXSTORAGEMEM2DEXTPROC)load("glTexStorageMem2DEXT");
	glad_glTexStorageMem2DMultisampleEXT = (PFNGLTEXSTORAGEMEM2DMULTISAMPLEEXTPROC)load("glTexStorageMem2DMultisampleEXT");
	glad_glTexStorageMem3DEXT = (PFNGLTEXSTORAGEMEM3DEXTPROC)load("glTexStorageMem3DEXT");
	glad_glTexStorageMem3DMultisampleEXT = (PFNGLTEXSTORAGEMEM3DMULTISAMPLEEXTPROC)load("glTexStorageMem3DMultisampleEXT");
	glad_glBufferStorageMemEXT = (PFNGLBUFFERSTORAGEMEMEXTPROC)load("glBufferStorageMemEXT");
	glad_glTextureStorageMem2DEXT = (PFNGLTEXTURESTORAGEMEM2DEXTPROC)load("glTextureStorageMem2DEXT");
	glad_glTextureStorageMem2DMultisampleEXT = (PFNGLTEXTURESTORAGEMEM2DMULTISAMPLEEXTPROC)load("glTextureStorageMem2DMultisampleEXT");
	glad_glTextureStorageMem3DEXT = (PFNGLTEXTURESTORAGEMEM3DEXTPROC)load("glTextureStorageMem3DEXT");
	glad_glTextureStorageMem3DMultisampleEXT = (PFNGLTEXTURESTORAGEMEM3DMULTISAMPLEEXTPROC)load("glTextureStorageMem3DMultisampleEXT");
	glad_glNamedBufferStorageMemEXT = (PFNGLNAMEDBUFFERSTORAGEMEMEXTPROC)load("glNamedBufferStorageMemEXT");
	glad_glTexStorageMem1DEXT = (PFNGLTEXSTORAGEMEM1DEXTPROC)load("glTexStorageMem1DEXT");
	glad_glTextureStorageMem1DEXT = (PFNGLTEXTURESTORAGEMEM1DEXTPROC)load("glTextureStorageMem1DEXT");
}
static void load_GL_EXT_memory_object_fd(GLADloadproc load) {
	if(!GLAD_GL_EXT_memory_object_fd) return;
	glad_glImportMemoryFdEXT = (PFNGLIMPORTMEMORYFDEXTPROC)load("glImportMemoryFdEXT");
}
static void load_GL_EXT_memory_object_win32(GLADloadproc load) {
	if(!GLAD_GL_EXT_memory_object_win32) return;
	glad_glImportMemoryWin32HandleEXT = (PFNGLIMPORTMEMORYWIN32HANDLEEXTPROC)load("glImportMemoryWin32HandleEXT");
	glad_glImportMemoryWin32NameEXT = (PFNGLIMPORTMEMORYWIN32NAMEEXTPROC)load("glImportMemoryWin32NameEXT");
}
static void load_GL_EXT_multi_draw_arrays(GLADloadproc load) {
	if(!GLAD_GL_EXT_multi_draw_arrays) return;
	glad_glMultiDrawArraysEXT = (PFNGLMULTIDRAWARRAYSEXTPROC)load("glMultiDrawArraysEXT");
	glad_glMultiDrawElementsEXT = (PFNGLMULTIDRAWELEMENTSEXTPROC)load("glMultiDrawElementsEXT");
}
static void load_GL_EXT_multisample(GLADloadproc load) {
	if(!GLAD_GL_EXT_multisample) return;
	glad_glSampleMaskEXT = (PFNGLSAMPLEMASKEXTPROC)load("glSampleMaskEXT");
	glad_glSamplePatternEXT = (PFNGLSAMPLEPATTERNEXTPROC)load("glSamplePatternEXT");
}
static void load_GL_EXT_paletted_texture(GLADloadproc load) {
	if(!GLAD_GL_EXT_paletted_texture) return;
	glad_glColorTableEXT = (PFNGLCOLORTABLEEXTPROC)load("glColorTableEXT");
	glad_glGetColorTableEXT = (PFNGLGETCOLORTABLEEXTPROC)load("glGetColorTableEXT");
	glad_glGetColorTableParameterivEXT = (PFNGLGETCOLORTABLEPARAMETERIVEXTPROC)load("glGetColorTableParameterivEXT");
	glad_glGetColorTableParameterfvEXT = (PFNGLGETCOLORTABLEPARAMETERFVEXTPROC)load("glGetColorTableParameterfvEXT");
}
static void load_GL_EXT_pixel_transform(GLADloadproc load) {
	if(!GLAD_GL_EXT_pixel_transform) return;
	glad_glPixelTransformParameteriEXT = (PFNGLPIXELTRANSFORMPARAMETERIEXTPROC)load("glPixelTransformParameteriEXT");
	glad_glPixelTransformParameterfEXT = (PFNGLPIXELTRANSFORMPARAMETERFEXTPROC)load("glPixelTransformParameterfEXT");
	glad_glPixelTransformParameterivEXT = (PFNGLPIXELTRANSFORMPARAMETERIVEXTPROC)load("glPixelTransformParameterivEXT");
	glad_glPixelTransformParameterfvEXT = (PFNGLPIXELTRANSFORMPARAMETERFVEXTPROC)load("glPixelTransformParameterfvEXT");
	glad_glGetPixelTransformParameterivEXT = (PFNGLGETPIXELTRANSFORMPARAMETERIVEXTPROC)load("glGetPixelTransformParameterivEXT");
	glad_glGetPixelTransformParameterfvEXT = (PFNGLGETPIXELTRANSFORMPARAMETERFVEXTPROC)load("glGetPixelTransformParameterfvEXT");
}
static void load_GL_EXT_point_parameters(GLADloadproc load) {
	if(!GLAD_GL_EXT_point_parameters) return;
	glad_glPointParameterfEXT = (PFNGLPOINTPARAMETERFEXTPROC)load("glPointParameterfEXT");
	glad_glPointParameterfvEXT = (PFNGLPOINTPARAMETERFVEXTPROC)load("glPointParameterfvEXT");
}
static void load_GL_EXT_polygon_offset(GLADloadproc load) {
	if(!GLAD_GL_EXT_polygon_offset) return;
	glad_glPolygonOffsetEXT = (PFNGLPOLYGONOFFSETEXTPROC)load("glPolygonOffsetEXT");
}
static void load_GL_EXT_polygon_offset_clamp(GLADloadproc load) {
	if(!GLAD_GL_EXT_polygon_offset_clamp) return;
	glad_glPolygonOffsetClampEXT = (PFNGLPOLYGONOFFSETCLAMPEXTPROC)load("glPolygonOffsetClampEXT");
}
static void load_GL_EXT_provoking_vertex(GLADloadproc load) {
	if(!GLAD_GL_EXT_provoking_vertex) return;
	glad_glProvokingVertexEXT = (PFNGLPROVOKINGVERTEXEXTPROC)load("glProvokingVertexEXT");
}
static void load_GL_EXT_raster_multisample(GLADloadproc load) {
	if(!GLAD_GL_EXT_raster_multisample) return;
	glad_glRasterSamplesEXT = (PFNGLRASTERSAMPLESEXTPROC)load("glRasterSamplesEXT");
}
static void load_GL_EXT_secondary_color(GLADloadproc load) {
	if(!GLAD_GL_EXT_secondary_color) return;
	glad_glSecondaryColor3bEXT = (PFNGLSECONDARYCOLOR3BEXTPROC)load("glSecondaryColor3bEXT");
	glad_glSecondaryColor3bvEXT = (PFNGLSECONDARYCOLOR3BVEXTPROC)load("glSecondaryColor3bvEXT");
	glad_glSecondaryColor3dEXT = (PFNGLSECONDARYCOLOR3DEXTPROC)load("glSecondaryColor3dEXT");
	glad_glSecondaryColor3dvEXT = (PFNGLSECONDARYCOLOR3DVEXTPROC)load("glSecondaryColor3dvEXT");
	glad_glSecondaryColor3fEXT = (PFNGLSECONDARYCOLOR3FEXTPROC)load("glSecondaryColor3fEXT");
	glad_glSecondaryColor3fvEXT = (PFNGLSECONDARYCOLOR3FVEXTPROC)load("glSecondaryColor3fvEXT");
	glad_glSecondaryColor3iEXT = (PFNGLSECONDARYCOLOR3IEXTPROC)load("glSecondaryColor3iEXT");
	glad_glSecondaryColor3ivEXT = (PFNGLSECONDARYCOLOR3IVEXTPROC)load("glSecondaryColor3ivEXT");
	glad_glSecondaryColor3sEXT = (PFNGLSECONDARYCOLOR3SEXTPROC)load("glSecondaryColor3sEXT");
	glad_glSecondaryColor3svEXT = (PFNGLSECONDARYCOLOR3SVEXTPROC)load("glSecondaryColor3svEXT");
	glad_glSecondaryColor3ubEXT = (PFNGLSECONDARYCOLOR3UBEXTPROC)load("glSecondaryColor3ubEXT");
	glad_glSecondaryColor3ubvEXT = (PFNGLSECONDARYCOLOR3UBVEXTPROC)load("glSecondaryColor3ubvEXT");
	glad_glSecondaryColor3uiEXT = (PFNGLSECONDARYCOLOR3UIEXTPROC)load("glSecondaryColor3uiEXT");
	glad_glSecondaryColor3uivEXT = (PFNGLSECONDARYCOLOR3UIVEXTPROC)load("glSecondaryColor3uivEXT");
	glad_glSecondaryColor3usEXT = (PFNGLSECONDARYCOLOR3USEXTPROC)load("glSecondaryColor3usEXT");
	glad_glSecondaryColor3usvEXT = (PFNGLSECONDARYCOLOR3USVEXTPROC)load("glSecondaryColor3usvEXT");
	glad_glSecondaryColorPointerEXT = (PFNGLSECONDARYCOLORPOINTEREXTPROC)load("glSecondaryColorPointerEXT");
}
static void load_GL_EXT_semaphore(GLADloadproc load) {
	if(!GLAD_GL_EXT_semaphore) return;
	glad_glGetUnsignedBytevEXT = (PFNGLGETUNSIGNEDBYTEVEXTPROC)load("glGetUnsignedBytevEXT");
	glad_glGetUnsignedBytei_vEXT = (PFNGLGETUNSIGNEDBYTEI_VEXTPROC)load("glGetUnsignedBytei_vEXT");
	glad_glGenSemaphoresEXT = (PFNGLGENSEMAPHORESEXTPROC)load("glGenSemaphoresEXT");
	glad_glDeleteSemaphoresEXT = (PFNGLDELETESEMAPHORESEXTPROC)load("glDeleteSemaphoresEXT");
	glad_glIsSemaphoreEXT = (PFNGLISSEMAPHOREEXTPROC)load("glIsSemaphoreEXT");
	glad_glSemaphoreParameterui64vEXT = (PFNGLSEMAPHOREPARAMETERUI64VEXTPROC)load("glSemaphoreParameterui64vEXT");
	glad_glGetSemaphoreParameterui64vEXT = (PFNGLGETSEMAPHOREPARAMETERUI64VEXTPROC)load("glGetSemaphoreParameterui64vEXT");
	glad_glWaitSemaphoreEXT = (PFNGLWAITSEMAPHOREEXTPROC)load("glWaitSemaphoreEXT");
	glad_glSignalSemaphoreEXT = (PFNGLSIGNALSEMAPHOREEXTPROC)load("glSignalSemaphoreEXT");
}
static void load_GL_EXT_semaphore_fd(GLADloadproc load) {
	if(!GLAD_GL_EXT_semaphore_fd) return;
	glad_glImportSemaphoreFdEXT = (PFNGLIMPORTSEMAPHOREFDEXTPROC)load("glImportSemaphoreFdEXT");
}
static void load_GL_EXT_semaphore_win32(GLADloadproc load) {
	if(!GLAD_GL_EXT_semaphore_win32) return;
	glad_glImportSemaphoreWin32HandleEXT = (PFNGLIMPORTSEMAPHOREWIN32HANDLEEXTPROC)load("glImportSemaphoreWin32HandleEXT");
	glad_glImportSemaphoreWin32NameEXT = (PFNGLIMPORTSEMAPHOREWIN32NAMEEXTPROC)load("glImportSemaphoreWin32NameEXT");
}
static void load_GL_EXT_separate_shader_objects(GLADloadproc load) {
	if(!GLAD_GL_EXT_separate_shader_objects) return;
	glad_glUseShaderProgramEXT = (PFNGLUSESHADERPROGRAMEXTPROC)load("glUseShaderProgramEXT");
	glad_glActiveProgramEXT = (PFNGLACTIVEPROGRAMEXTPROC)load("glActiveProgramEXT");
	glad_glCreateShaderProgramEXT = (PFNGLCREATESHADERPROGRAMEXTPROC)load("glCreateShaderProgramEXT");
	glad_glActiveShaderProgramEXT = (PFNGLACTIVESHADERPROGRAMEXTPROC)load("glActiveShaderProgramEXT");
	glad_glBindProgramPipelineEXT = (PFNGLBINDPROGRAMPIPELINEEXTPROC)load("glBindProgramPipelineEXT");
	glad_glCreateShaderProgramvEXT = (PFNGLCREATESHADERPROGRAMVEXTPROC)load("glCreateShaderProgramvEXT");
	glad_glDeleteProgramPipelinesEXT = (PFNGLDELETEPROGRAMPIPELINESEXTPROC)load("glDeleteProgramPipelinesEXT");
	glad_glGenProgramPipelinesEXT = (PFNGLGENPROGRAMPIPELINESEXTPROC)load("glGenProgramPipelinesEXT");
	glad_glGetProgramPipelineInfoLogEXT = (PFNGLGETPROGRAMPIPELINEINFOLOGEXTPROC)load("glGetProgramPipelineInfoLogEXT");
	glad_glGetProgramPipelineivEXT = (PFNGLGETPROGRAMPIPELINEIVEXTPROC)load("glGetProgramPipelineivEXT");
	glad_glIsProgramPipelineEXT = (PFNGLISPROGRAMPIPELINEEXTPROC)load("glIsProgramPipelineEXT");
	glad_glProgramParameteriEXT = (PFNGLPROGRAMPARAMETERIEXTPROC)load("glProgramParameteriEXT");
	glad_glProgramUniform1fEXT = (PFNGLPROGRAMUNIFORM1FEXTPROC)load("glProgramUniform1fEXT");
	glad_glProgramUniform1fvEXT = (PFNGLPROGRAMUNIFORM1FVEXTPROC)load("glProgramUniform1fvEXT");
	glad_glProgramUniform1iEXT = (PFNGLPROGRAMUNIFORM1IEXTPROC)load("glProgramUniform1iEXT");
	glad_glProgramUniform1ivEXT = (PFNGLPROGRAMUNIFORM1IVEXTPROC)load("glProgramUniform1ivEXT");
	glad_glProgramUniform2fEXT = (PFNGLPROGRAMUNIFORM2FEXTPROC)load("glProgramUniform2fEXT");
	glad_glProgramUniform2fvEXT = (PFNGLPROGRAMUNIFORM2FVEXTPROC)load("glProgramUniform2fvEXT");
	glad_glProgramUniform2iEXT = (PFNGLPROGRAMUNIFORM2IEXTPROC)load("glProgramUniform2iEXT");
	glad_glProgramUniform2ivEXT = (PFNGLPROGRAMUNIFORM2IVEXTPROC)load("glProgramUniform2ivEXT");
	glad_glProgramUniform3fEXT = (PFNGLPROGRAMUNIFORM3FEXTPROC)load("glProgramUniform3fEXT");
	glad_glProgramUniform3fvEXT = (PFNGLPROGRAMUNIFORM3FVEXTPROC)load("glProgramUniform3fvEXT");
	glad_glProgramUniform3iEXT = (PFNGLPROGRAMUNIFORM3IEXTPROC)load("glProgramUniform3iEXT");
	glad_glProgramUniform3ivEXT = (PFNGLPROGRAMUNIFORM3IVEXTPROC)load("glProgramUniform3ivEXT");
	glad_glProgramUniform4fEXT = (PFNGLPROGRAMUNIFORM4FEXTPROC)load("glProgramUniform4fEXT");
	glad_glProgramUniform4fvEXT = (PFNGLPROGRAMUNIFORM4FVEXTPROC)load("glProgramUniform4fvEXT");
	glad_glProgramUniform4iEXT = (PFNGLPROGRAMUNIFORM4IEXTPROC)load("glProgramUniform4iEXT");
	glad_glProgramUniform4ivEXT = (PFNGLPROGRAMUNIFORM4IVEXTPROC)load("glProgramUniform4ivEXT");
	glad_glProgramUniformMatrix2fvEXT = (PFNGLPROGRAMUNIFORMMATRIX2FVEXTPROC)load("glProgramUniformMatrix2fvEXT");
	glad_glProgramUniformMatrix3fvEXT = (PFNGLPROGRAMUNIFORMMATRIX3FVEXTPROC)load("glProgramUniformMatrix3fvEXT");
	glad_glProgramUniformMatrix4fvEXT = (PFNGLPROGRAMUNIFORMMATRIX4FVEXTPROC)load("glProgramUniformMatrix4fvEXT");
	glad_glUseProgramStagesEXT = (PFNGLUSEPROGRAMSTAGESEXTPROC)load("glUseProgramStagesEXT");
	glad_glValidateProgramPipelineEXT = (PFNGLVALIDATEPROGRAMPIPELINEEXTPROC)load("glValidateProgramPipelineEXT");
	glad_glProgramUniform1uiEXT = (PFNGLPROGRAMUNIFORM1UIEXTPROC)load("glProgramUniform1uiEXT");
	glad_glProgramUniform2uiEXT = (PFNGLPROGRAMUNIFORM2UIEXTPROC)load("glProgramUniform2uiEXT");
	glad_glProgramUniform3uiEXT = (PFNGLPROGRAMUNIFORM3UIEXTPROC)load("glProgramUniform3uiEXT");
	glad_glProgramUniform4uiEXT = (PFNGLPROGRAMUNIFORM4UIEXTPROC)load("glProgramUniform4uiEXT");
	glad_glProgramUniform1uivEXT = (PFNGLPROGRAMUNIFORM1UIVEXTPROC)load("glProgramUniform1uivEXT");
	glad_glProgramUniform2uivEXT = (PFNGLPROGRAMUNIFORM2UIVEXTPROC)load("glProgramUniform2uivEXT");
	glad_glProgramUniform3uivEXT = (PFNGLPROGRAMUNIFORM3UIVEXTPROC)load("glProgramUniform3uivEXT");
	glad_glProgramUniform4uivEXT = (PFNGLPROGRAMUNIFORM4UIVEXTPROC)load("glProgramUniform4uivEXT");
	glad_glProgramUniformMatrix2x3fvEXT = (PFNGLPROGRAMUNIFORMMATRIX2X3FVEXTPROC)load("glProgramUniformMatrix2x3fvEXT");
	glad_glProgramUniformMatrix3x2fvEXT = (PFNGLPROGRAMUNIFORMMATRIX3X2FVEXTPROC)load("glProgramUniformMatrix3x2fvEXT");
	glad_glProgramUniformMatrix2x4fvEXT = (PFNGLPROGRAMUNIFORMMATRIX2X4FVEXTPROC)load("glProgramUniformMatrix2x4fvEXT");
	glad_glProgramUniformMatrix4x2fvEXT = (PFNGLPROGRAMUNIFORMMATRIX4X2FVEXTPROC)load("glProgramUniformMatrix4x2fvEXT");
	glad_glProgramUniformMatrix3x4fvEXT = (PFNGLPROGRAMUNIFORMMATRIX3X4FVEXTPROC)load("glProgramUniformMatrix3x4fvEXT");
	glad_glProgramUniformMatrix4x3fvEXT = (PFNGLPROGRAMUNIFORMMATRIX4X3FVEXTPROC)load("glProgramUniformMatrix4x3fvEXT");
}
static void load_GL_EXT_shader_framebuffer_fetch_non_coherent(GLADloadproc load) {
	if(!GLAD_GL_EXT_shader_framebuffer_fetch_non_coherent) return;
	glad_glFramebufferFetchBarrierEXT = (PFNGLFRAMEBUFFERFETCHBARRIEREXTPROC)load("glFramebufferFetchBarrierEXT");
}
static void load_GL_EXT_shader_image_load_store(GLADloadproc load) {
	if(!GLAD_GL_EXT_shader_image_load_store) return;
	glad_glBindImageTextureEXT = (PFNGLBINDIMAGETEXTUREEXTPROC)load("glBindImageTextureEXT");
	glad_glMemoryBarrierEXT = (PFNGLMEMORYBARRIEREXTPROC)load("glMemoryBarrierEXT");
}
static void load_GL_EXT_stencil_clear_tag(GLADloadproc load) {
	if(!GLAD_GL_EXT_stencil_clear_tag) return;
	glad_glStencilClearTagEXT = (PFNGLSTENCILCLEARTAGEXTPROC)load("glStencilClearTagEXT");
}
static void load_GL_EXT_stencil_two_side(GLADloadproc load) {
	if(!GLAD_GL_EXT_stencil_two_side) return;
	glad_glActiveStencilFaceEXT = (PFNGLACTIVESTENCILFACEEXTPROC)load("glActiveStencilFaceEXT");
}
static void load_GL_EXT_subtexture(GLADloadproc load) {
	if(!GLAD_GL_EXT_subtexture) return;
	glad_glTexSubImage1DEXT = (PFNGLTEXSUBIMAGE1DEXTPROC)load("glTexSubImage1DEXT");
	glad_glTexSubImage2DEXT = (PFNGLTEXSUBIMAGE2DEXTPROC)load("glTexSubImage2DEXT");
}
static void load_GL_EXT_texture3D(GLADloadproc load) {
	if(!GLAD_GL_EXT_texture3D) return;
	glad_glTexImage3DEXT = (PFNGLTEXIMAGE3DEXTPROC)load("glTexImage3DEXT");
	glad_glTexSubImage3DEXT = (PFNGLTEXSUBIMAGE3DEXTPROC)load("glTexSubImage3DEXT");
}
static void load_GL_EXT_texture_array(GLADloadproc load) {
	if(!GLAD_GL_EXT_texture_array) return;
	glad_glFramebufferTextureLayerEXT = (PFNGLFRAMEBUFFERTEXTURELAYEREXTPROC)load("glFramebufferTextureLayerEXT");
}
static void load_GL_EXT_texture_buffer_object(GLADloadproc load) {
	if(!GLAD_GL_EXT_texture_buffer_object) return;
	glad_glTexBufferEXT = (PFNGLTEXBUFFEREXTPROC)load("glTexBufferEXT");
}
static void load_GL_EXT_texture_integer(GLADloadproc load) {
	if(!GLAD_GL_EXT_texture_integer) return;
	glad_glTexParameterIivEXT = (PFNGLTEXPARAMETERIIVEXTPROC)load("glTexParameterIivEXT");
	glad_glTexParameterIuivEXT = (PFNGLTEXPARAMETERIUIVEXTPROC)load("glTexParameterIuivEXT");
	glad_glGetTexParameterIivEXT = (PFNGLGETTEXPARAMETERIIVEXTPROC)load("glGetTexParameterIivEXT");
	glad_glGetTexParameterIuivEXT = (PFNGLGETTEXPARAMETERIUIVEXTPROC)load("glGetTexParameterIuivEXT");
	glad_glClearColorIiEXT = (PFNGLCLEARCOLORIIEXTPROC)load("glClearColorIiEXT");
	glad_glClearColorIuiEXT = (PFNGLCLEARCOLORIUIEXTPROC)load("glClearColorIuiEXT");
}
static void load_GL_EXT_texture_object(GLADloadproc load) {
	if(!GLAD_GL_EXT_texture_object) return;
	glad_glAreTexturesResidentEXT = (PFNGLARETEXTURESRESIDENTEXTPROC)load("glAreTexturesResidentEXT");
	glad_glBindTextureEXT = (PFNGLBINDTEXTUREEXTPROC)load("glBindTextureEXT");
	glad_glDeleteTexturesEXT = (PFNGLDELETETEXTURESEXTPROC)load("glDeleteTexturesEXT");
	glad_glGenTexturesEXT = (PFNGLGENTEXTURESEXTPROC)load("glGenTexturesEXT");
	glad_glIsTextureEXT = (PFNGLISTEXTUREEXTPROC)load("glIsTextureEXT");
	glad_glPrioritizeTexturesEXT = (PFNGLPRIORITIZETEXTURESEXTPROC)load("glPrioritizeTexturesEXT");
}
static void load_GL_EXT_texture_perturb_normal(GLADloadproc load) {
	if(!GLAD_GL_EXT_texture_perturb_normal) return;
	glad_glTextureNormalEXT = (PFNGLTEXTURENORMALEXTPROC)load("glTextureNormalEXT");
}
static void load_GL_EXT_texture_storage(GLADloadproc load) {
	if(!GLAD_GL_EXT_texture_storage) return;
	glad_glTexStorage1DEXT = (PFNGLTEXSTORAGE1DEXTPROC)load("glTexStorage1DEXT");
	glad_glTexStorage2DEXT = (PFNGLTEXSTORAGE2DEXTPROC)load("glTexStorage2DEXT");
	glad_glTexStorage3DEXT = (PFNGLTEXSTORAGE3DEXTPROC)load("glTexStorage3DEXT");
	glad_glTextureStorage1DEXT = (PFNGLTEXTURESTORAGE1DEXTPROC)load("glTextureStorage1DEXT");
	glad_glTextureStorage2DEXT = (PFNGLTEXTURESTORAGE2DEXTPROC)load("glTextureStorage2DEXT");
	glad_glTextureStorage3DEXT = (PFNGLTEXTURESTORAGE3DEXTPROC)load("glTextureStorage3DEXT");
}
static void load_GL_EXT_timer_query(GLADloadproc load) {
	if(!GLAD_GL_EXT_timer_query) return;
	glad_glGetQueryObjecti64vEXT = (PFNGLGETQUERYOBJECTI64VEXTPROC)load("glGetQueryObjecti64vEXT");
	glad_glGetQueryObjectui64vEXT = (PFNGLGETQUERYOBJECTUI64VEXTPROC)load("glGetQueryObjectui64vEXT");
}
static void load_GL_EXT_transform_feedback(GLADloadproc load) {
	if(!GLAD_GL_EXT_transform_feedback) return;
	glad_glBeginTransformFeedbackEXT = (PFNGLBEGINTRANSFORMFEEDBACKEXTPROC)load("glBeginTransformFeedbackEXT");
	glad_glEndTransformFeedbackEXT = (PFNGLENDTRANSFORMFEEDBACKEXTPROC)load("glEndTransformFeedbackEXT");
	glad_glBindBufferRangeEXT = (PFNGLBINDBUFFERRANGEEXTPROC)load("glBindBufferRangeEXT");
	glad_glBindBufferOffsetEXT = (PFNGLBINDBUFFEROFFSETEXTPROC)load("glBindBufferOffsetEXT");
	glad_glBindBufferBaseEXT = (PFNGLBINDBUFFERBASEEXTPROC)load("glBindBufferBaseEXT");
	glad_glTransformFeedbackVaryingsEXT = (PFNGLTRANSFORMFEEDBACKVARYINGSEXTPROC)load("glTransformFeedbackVaryingsEXT");
	glad_glGetTransformFeedbackVaryingEXT = (PFNGLGETTRANSFORMFEEDBACKVARYINGEXTPROC)load("glGetTransformFeedbackVaryingEXT");
}
static void load_GL_EXT_vertex_array(GLADloadproc load) {
	if(!GLAD_GL_EXT_vertex_array) return;
	glad_glArrayElementEXT = (PFNGLARRAYELEMENTEXTPROC)load("glArrayElementEXT");
	glad_glColorPointerEXT = (PFNGLCOLORPOINTEREXTPROC)load("glColorPointerEXT");
	glad_glDrawArraysEXT = (PFNGLDRAWARRAYSEXTPROC)load("glDrawArraysEXT");
	glad_glEdgeFlagPointerEXT = (PFNGLEDGEFLAGPOINTEREXTPROC)load("glEdgeFlagPointerEXT");
	glad_glGetPointervEXT = (PFNGLGETPOINTERVEXTPROC)load("glGetPointervEXT");
	glad_glIndexPointerEXT = (PFNGLINDEXPOINTEREXTPROC)load("glIndexPointerEXT");
	glad_glNormalPointerEXT = (PFNGLNORMALPOINTEREXTPROC)load("glNormalPointerEXT");
	glad_glTexCoordPointerEXT = (PFNGLTEXCOORDPOINTEREXTPROC)load("glTexCoordPointerEXT");
	glad_glVertexPointerEXT = (PFNGLVERTEXPOINTEREXTPROC)load("glVertexPointerEXT");
}
static void load_GL_EXT_vertex_attrib_64bit(GLADloadproc load) {
	if(!GLAD_GL_EXT_vertex_attrib_64bit) return;
	glad_glVertexAttribL1dEXT = (PFNGLVERTEXATTRIBL1DEXTPROC)load("glVertexAttribL1dEXT");
	glad_glVertexAttribL2dEXT = (PFNGLVERTEXATTRIBL2DEXTPROC)load("glVertexAttribL2dEXT");
	glad_glVertexAttribL3dEXT = (PFNGLVERTEXATTRIBL3DEXTPROC)load("glVertexAttribL3dEXT");
	glad_glVertexAttribL4dEXT = (PFNGLVERTEXATTRIBL4DEXTPROC)load("glVertexAttribL4dEXT");
	glad_glVertexAttribL1dvEXT = (PFNGLVERTEXATTRIBL1DVEXTPROC)load("glVertexAttribL1dvEXT");
	glad_glVertexAttribL2dvEXT = (PFNGLVERTEXATTRIBL2DVEXTPROC)load("glVertexAttribL2dvEXT");
	glad_glVertexAttribL3dvEXT = (PFNGLVERTEXATTRIBL3DVEXTPROC)load("glVertexAttribL3dvEXT");
	glad_glVertexAttribL4dvEXT = (PFNGLVERTEXATTRIBL4DVEXTPROC)load("glVertexAttribL4dvEXT");
	glad_glVertexAttribLPointerEXT = (PFNGLVERTEXATTRIBLPOINTEREXTPROC)load("glVertexAttribLPointerEXT");
	glad_glGetVertexAttribLdvEXT = (PFNGLGETVERTEXATTRIBLDVEXTPROC)load("glGetVertexAttribLdvEXT");
}
static void load_GL_EXT_vertex_shader(GLADloadproc load) {
	if(!GLAD_GL_EXT_vertex_shader) return;
	glad_glBeginVertexShaderEXT = (PFNGLBEGINVERTEXSHADEREXTPROC)load("glBeginVertexShaderEXT");
	glad_glEndVertexShaderEXT = (PFNGLENDVERTEXSHADEREXTPROC)load("glEndVertexShaderEXT");
	glad_glBindVertexShaderEXT = (PFNGLBINDVERTEXSHADEREXTPROC)load("glBindVertexShaderEXT");
	glad_glGenVertexShadersEXT = (PFNGLGENVERTEXSHADERSEXTPROC)load("glGenVertexShadersEXT");
	glad_glDeleteVertexShaderEXT = (PFNGLDELETEVERTEXSHADEREXTPROC)load("glDeleteVertexShaderEXT");
	glad_glShaderOp1EXT = (PFNGLSHADEROP1EXTPROC)load("glShaderOp1EXT");
	glad_glShaderOp2EXT = (PFNGLSHADEROP2EXTPROC)load("glShaderOp2EXT");
	glad_glShaderOp3EXT = (PFNGLSHADEROP3EXTPROC)load("glShaderOp3EXT");
	glad_glSwizzleEXT = (PFNGLSWIZZLEEXTPROC)load("glSwizzleEXT");
	glad_glWriteMaskEXT = (PFNGLWRITEMASKEXTPROC)load("glWriteMaskEXT");
	glad_glInsertComponentEXT = (PFNGLINSERTCOMPONENTEXTPROC)load("glInsertComponentEXT");
	glad_glExtractComponentEXT = (PFNGLEXTRACTCOMPONENTEXTPROC)load("glExtractComponentEXT");
	glad_glGenSymbolsEXT = (PFNGLGENSYMBOLSEXTPROC)load("glGenSymbolsEXT");
	glad_glSetInvariantEXT = (PFNGLSETINVARIANTEXTPROC)load("glSetInvariantEXT");
	glad_glSetLocalConstantEXT = (PFNGLSETLOCALCONSTANTEXTPROC)load("glSetLocalConstantEXT");
	glad_glVariantbvEXT = (PFNGLVARIANTBVEXTPROC)load("glVariantbvEXT");
	glad_glVariantsvEXT = (PFNGLVARIANTSVEXTPROC)load("glVariantsvEXT");
	glad_glVariantivEXT = (PFNGLVARIANTIVEXTPROC)load("glVariantivEXT");
	glad_glVariantfvEXT = (PFNGLVARIANTFVEXTPROC)load("glVariantfvEXT");
	glad_glVariantdvEXT = (PFNGLVARIANTDVEXTPROC)load("glVariantdvEXT");
	glad_glVariantubvEXT = (PFNGLVARIANTUBVEXTPROC)load("glVariantubvEXT");
	glad_glVariantusvEXT = (PFNGLVARIANTUSVEXTPROC)load("glVariantusvEXT");
	glad_glVariantuivEXT = (PFNGLVARIANTUIVEXTPROC)load("glVariantuivEXT");
	glad_glVariantPointerEXT = (PFNGLVARIANTPOINTEREXTPROC)load("glVariantPointerEXT");
	glad_glEnableVariantClientStateEXT = (PFNGLENABLEVARIANTCLIENTSTATEEXTPROC)load("glEnableVariantClientStateEXT");
	glad_glDisableVariantClientStateEXT = (PFNGLDISABLEVARIANTCLIENTSTATEEXTPROC)load("glDisableVariantClientStateEXT");
	glad_glBindLightParameterEXT = (PFNGLBINDLIGHTPARAMETEREXTPROC)load("glBindLightParameterEXT");
	glad_glBindMaterialParameterEXT = (PFNGLBINDMATERIALPARAMETEREXTPROC)load("glBindMaterialParameterEXT");
	glad_glBindTexGenParameterEXT = (PFNGLBINDTEXGENPARAMETEREXTPROC)load("glBindTexGenParameterEXT");
	glad_glBindTextureUnitParameterEXT = (PFNGLBINDTEXTUREUNITPARAMETEREXTPROC)load("glBindTextureUnitParameterEXT");
	glad_glBindParameterEXT = (PFNGLBINDPARAMETEREXTPROC)load("glBindParameterEXT");
	glad_glIsVariantEnabledEXT = (PFNGLISVARIANTENABLEDEXTPROC)load("glIsVariantEnabledEXT");
	glad_glGetVariantBooleanvEXT = (PFNGLGETVARIANTBOOLEANVEXTPROC)load("glGetVariantBooleanvEXT");
	glad_glGetVariantIntegervEXT = (PFNGLGETVARIANTINTEGERVEXTPROC)load("glGetVariantIntegervEXT");
	glad_glGetVariantFloatvEXT = (PFNGLGETVARIANTFLOATVEXTPROC)load("glGetVariantFloatvEXT");
	glad_glGetVariantPointervEXT = (PFNGLGETVARIANTPOINTERVEXTPROC)load("glGetVariantPointervEXT");
	glad_glGetInvariantBooleanvEXT = (PFNGLGETINVARIANTBOOLEANVEXTPROC)load("glGetInvariantBooleanvEXT");
	glad_glGetInvariantIntegervEXT = (PFNGLGETINVARIANTINTEGERVEXTPROC)load("glGetInvariantIntegervEXT");
	glad_glGetInvariantFloatvEXT = (PFNGLGETINVARIANTFLOATVEXTPROC)load("glGetInvariantFloatvEXT");
	glad_glGetLocalConstantBooleanvEXT = (PFNGLGETLOCALCONSTANTBOOLEANVEXTPROC)load("glGetLocalConstantBooleanvEXT");
	glad_glGetLocalConstantIntegervEXT = (PFNGLGETLOCALCONSTANTINTEGERVEXTPROC)load("glGetLocalConstantIntegervEXT");
	glad_glGetLocalConstantFloatvEXT = (PFNGLGETLOCALCONSTANTFLOATVEXTPROC)load("glGetLocalConstantFloatvEXT");
}
static void load_GL_EXT_vertex_weighting(GLADloadproc load) {
	if(!GLAD_GL_EXT_vertex_weighting) return;
	glad_glVertexWeightfEXT = (PFNGLVERTEXWEIGHTFEXTPROC)load("glVertexWeightfEXT");
	glad_glVertexWeightfvEXT = (PFNGLVERTEXWEIGHTFVEXTPROC)load("glVertexWeightfvEXT");
	glad_glVertexWeightPointerEXT = (PFNGLVERTEXWEIGHTPOINTEREXTPROC)load("glVertexWeightPointerEXT");
}
static void load_GL_EXT_win32_keyed_mutex(GLADloadproc load) {
	if(!GLAD_GL_EXT_win32_keyed_mutex) return;
	glad_glAcquireKeyedMutexWin32EXT = (PFNGLACQUIREKEYEDMUTEXWIN32EXTPROC)load("glAcquireKeyedMutexWin32EXT");
	glad_glReleaseKeyedMutexWin32EXT = (PFNGLRELEASEKEYEDMUTEXWIN32EXTPROC)load("glReleaseKeyedMutexWin32EXT");
}
static void load_GL_EXT_window_rectangles(GLADloadproc load) {
	if(!GLAD_GL_EXT_window_rectangles) return;
	glad_glWindowRectanglesEXT = (PFNGLWINDOWRECTANGLESEXTPROC)load("glWindowRectanglesEXT");
}
static void load_GL_EXT_x11_sync_object(GLADloadproc load) {
	if(!GLAD_GL_EXT_x11_sync_object) return;
	glad_glImportSyncEXT = (PFNGLIMPORTSYNCEXTPROC)load("glImportSyncEXT");
}
static void load_GL_GREMEDY_frame_terminator(GLADloadproc load) {
	if(!GLAD_GL_GREMEDY_frame_terminator) return;
	glad_glFrameTerminatorGREMEDY = (PFNGLFRAMETERMINATORGREMEDYPROC)load("glFrameTerminatorGREMEDY");
}
static void load_GL_GREMEDY_string_marker(GLADloadproc load) {
	if(!GLAD_GL_GREMEDY_string_marker) return;
	glad_glStringMarkerGREMEDY = (PFNGLSTRINGMARKERGREMEDYPROC)load("glStringMarkerGREMEDY");
}
static void load_GL_HP_image_transform(GLADloadproc load) {
	if(!GLAD_GL_HP_image_transform) return;
	glad_glImageTransformParameteriHP = (PFNGLIMAGETRANSFORMPARAMETERIHPPROC)load("glImageTransformParameteriHP");
	glad_glImageTransformParameterfHP = (PFNGLIMAGETRANSFORMPARAMETERFHPPROC)load("glImageTransformParameterfHP");
	glad_glImageTransformParameterivHP = (PFNGLIMAGETRANSFORMPARAMETERIVHPPROC)load("glImageTransformParameterivHP");
	glad_glImageTransformParameterfvHP = (PFNGLIMAGETRANSFORMPARAMETERFVHPPROC)load("glImageTransformParameterfvHP");
	glad_glGetImageTransformParameterivHP = (PFNGLGETIMAGETRANSFORMPARAMETERIVHPPROC)load("glGetImageTransformParameterivHP");
	glad_glGetImageTransformParameterfvHP = (PFNGLGETIMAGETRANSFORMPARAMETERFVHPPROC)load("glGetImageTransformParameterfvHP");
}
static void load_GL_IBM_multimode_draw_arrays(GLADloadproc load) {
	if(!GLAD_GL_IBM_multimode_draw_arrays) return;
	glad_glMultiModeDrawArraysIBM = (PFNGLMULTIMODEDRAWARRAYSIBMPROC)load("glMultiModeDrawArraysIBM");
	glad_glMultiModeDrawElementsIBM = (PFNGLMULTIMODEDRAWELEMENTSIBMPROC)load("glMultiModeDrawElementsIBM");
}
static void load_GL_IBM_static_data(GLADloadproc load) {
	if(!GLAD_GL_IBM_static_data) return;
	glad_glFlushStaticDataIBM = (PFNGLFLUSHSTATICDATAIBMPROC)load("glFlushStaticDataIBM");
}
static void load_GL_IBM_vertex_array_lists(GLADloadproc load) {
	if(!GLAD_GL_IBM_vertex_array_lists) return;
	glad_glColorPointerListIBM = (PFNGLCOLORPOINTERLISTIBMPROC)load("glColorPointerListIBM");
	glad_glSecondaryColorPointerListIBM = (PFNGLSECONDARYCOLORPOINTERLISTIBMPROC)load("glSecondaryColorPointerListIBM");
	glad_glEdgeFlagPointerListIBM = (PFNGLEDGEFLAGPOINTERLISTIBMPROC)load("glEdgeFlagPointerListIBM");
	glad_glFogCoordPointerListIBM = (PFNGLFOGCOORDPOINTERLISTIBMPROC)load("glFogCoordPointerListIBM");
	glad_glIndexPointerListIBM = (PFNGLINDEXPOINTERLISTIBMPROC)load("glIndexPointerListIBM");
	glad_glNormalPointerListIBM = (PFNGLNORMALPOINTERLISTIBMPROC)load("glNormalPointerListIBM");
	glad_glTexCoordPointerListIBM = (PFNGLTEXCOORDPOINTERLISTIBMPROC)load("glTexCoordPointerListIBM");
	glad_glVertexPointerListIBM = (PFNGLVERTEXPOINTERLISTIBMPROC)load("glVertexPointerListIBM");
}
static void load_GL_INGR_blend_func_separate(GLADloadproc load) {
	if(!GLAD_GL_INGR_blend_func_separate) return;
	glad_glBlendFuncSeparateINGR = (PFNGLBLENDFUNCSEPARATEINGRPROC)load("glBlendFuncSeparateINGR");
}
static void load_GL_INTEL_framebuffer_CMAA(GLADloadproc load) {
	if(!GLAD_GL_INTEL_framebuffer_CMAA) return;
	glad_glApplyFramebufferAttachmentCMAAINTEL = (PFNGLAPPLYFRAMEBUFFERATTACHMENTCMAAINTELPROC)load("glApplyFramebufferAttachmentCMAAINTEL");
}
static void load_GL_INTEL_map_texture(GLADloadproc load) {
	if(!GLAD_GL_INTEL_map_texture) return;
	glad_glSyncTextureINTEL = (PFNGLSYNCTEXTUREINTELPROC)load("glSyncTextureINTEL");
	glad_glUnmapTexture2DINTEL = (PFNGLUNMAPTEXTURE2DINTELPROC)load("glUnmapTexture2DINTEL");
	glad_glMapTexture2DINTEL = (PFNGLMAPTEXTURE2DINTELPROC)load("glMapTexture2DINTEL");
}
static void load_GL_INTEL_parallel_arrays(GLADloadproc load) {
	if(!GLAD_GL_INTEL_parallel_arrays) return;
	glad_glVertexPointervINTEL = (PFNGLVERTEXPOINTERVINTELPROC)load("glVertexPointervINTEL");
	glad_glNormalPointervINTEL = (PFNGLNORMALPOINTERVINTELPROC)load("glNormalPointervINTEL");
	glad_glColorPointervINTEL = (PFNGLCOLORPOINTERVINTELPROC)load("glColorPointervINTEL");
	glad_glTexCoordPointervINTEL = (PFNGLTEXCOORDPOINTERVINTELPROC)load("glTexCoordPointervINTEL");
}
static void load_GL_INTEL_performance_query(GLADloadproc load) {
	if(!GLAD_GL_INTEL_performance_query) return;
	glad_glBeginPerfQueryINTEL = (PFNGLBEGINPERFQUERYINTELPROC)load("glBeginPerfQueryINTEL");
	glad_glCreatePerfQueryINTEL = (PFNGLCREATEPERFQUERYINTELPROC)load("glCreatePerfQueryINTEL");
	glad_glDeletePerfQueryINTEL = (PFNGLDELETEPERFQUERYINTELPROC)load("glDeletePerfQueryINTEL");
	glad_glEndPerfQueryINTEL = (PFNGLENDPERFQUERYINTELPROC)load("glEndPerfQueryINTEL");
	glad_glGetFirstPerfQueryIdINTEL = (PFNGLGETFIRSTPERFQUERYIDINTELPROC)load("glGetFirstPerfQueryIdINTEL");
	glad_glGetNextPerfQueryIdINTEL = (PFNGLGETNEXTPERFQUERYIDINTELPROC)load("glGetNextPerfQueryIdINTEL");
	glad_glGetPerfCounterInfoINTEL = (PFNGLGETPERFCOUNTERINFOINTELPROC)load("glGetPerfCounterInfoINTEL");
	glad_glGetPerfQueryDataINTEL = (PFNGLGETPERFQUERYDATAINTELPROC)load("glGetPerfQueryDataINTEL");
	glad_glGetPerfQueryIdByNameINTEL = (PFNGLGETPERFQUERYIDBYNAMEINTELPROC)load("glGetPerfQueryIdByNameINTEL");
	glad_glGetPerfQueryInfoINTEL = (PFNGLGETPERFQUERYINFOINTELPROC)load("glGetPerfQueryInfoINTEL");
}
static void load_GL_KHR_blend_equation_advanced(GLADloadproc load) {
	if(!GLAD_GL_KHR_blend_equation_advanced) return;
	glad_glBlendBarrierKHR = (PFNGLBLENDBARRIERKHRPROC)load("glBlendBarrierKHR");
}
static void load_GL_KHR_debug(GLADloadproc load) {
	if(!GLAD_GL_KHR_debug) return;
	glad_glDebugMessageControl = (PFNGLDEBUGMESSAGECONTROLPROC)load("glDebugMessageControl");
	glad_glDebugMessageInsert = (PFNGLDEBUGMESSAGEINSERTPROC)load("glDebugMessageInsert");
	glad_glDebugMessageCallback = (PFNGLDEBUGMESSAGECALLBACKPROC)load("glDebugMessageCallback");
	glad_glGetDebugMessageLog = (PFNGLGETDEBUGMESSAGELOGPROC)load("glGetDebugMessageLog");
	glad_glPushDebugGroup = (PFNGLPUSHDEBUGGROUPPROC)load("glPushDebugGroup");
	glad_glPopDebugGroup = (PFNGLPOPDEBUGGROUPPROC)load("glPopDebugGroup");
	glad_glObjectLabel = (PFNGLOBJECTLABELPROC)load("glObjectLabel");
	glad_glGetObjectLabel = (PFNGLGETOBJECTLABELPROC)load("glGetObjectLabel");
	glad_glObjectPtrLabel = (PFNGLOBJECTPTRLABELPROC)load("glObjectPtrLabel");
	glad_glGetObjectPtrLabel = (PFNGLGETOBJECTPTRLABELPROC)load("glGetObjectPtrLabel");
	glad_glGetPointerv = (PFNGLGETPOINTERVPROC)load("glGetPointerv");
	glad_glDebugMessageControlKHR = (PFNGLDEBUGMESSAGECONTROLKHRPROC)load("glDebugMessageControlKHR");
	glad_glDebugMessageInsertKHR = (PFNGLDEBUGMESSAGEINSERTKHRPROC)load("glDebugMessageInsertKHR");
	glad_glDebugMessageCallbackKHR = (PFNGLDEBUGMESSAGECALLBACKKHRPROC)load("glDebugMessageCallbackKHR");
	glad_glGetDebugMessageLogKHR = (PFNGLGETDEBUGMESSAGELOGKHRPROC)load("glGetDebugMessageLogKHR");
	glad_glPushDebugGroupKHR = (PFNGLPUSHDEBUGGROUPKHRPROC)load("glPushDebugGroupKHR");
	glad_glPopDebugGroupKHR = (PFNGLPOPDEBUGGROUPKHRPROC)load("glPopDebugGroupKHR");
	glad_glObjectLabelKHR = (PFNGLOBJECTLABELKHRPROC)load("glObjectLabelKHR");
	glad_glGetObjectLabelKHR = (PFNGLGETOBJECTLABELKHRPROC)load("glGetObjectLabelKHR");
	glad_glObjectPtrLabelKHR = (PFNGLOBJECTPTRLABELKHRPROC)load("glObjectPtrLabelKHR");
	glad_glGetObjectPtrLabelKHR = (PFNGLGETOBJECTPTRLABELKHRPROC)load("glGetObjectPtrLabelKHR");
	glad_glGetPointervKHR = (PFNGLGETPOINTERVKHRPROC)load("glGetPointervKHR");
}
static void load_GL_KHR_parallel_shader_compile(GLADloadproc load) {
	if(!GLAD_GL_KHR_parallel_shader_compile) return;
	glad_glMaxShaderCompilerThreadsKHR = (PFNGLMAXSHADERCOMPILERTHREADSKHRPROC)load("glMaxShaderCompilerThreadsKHR");
}
static void load_GL_KHR_robustness(GLADloadproc load) {
	if(!GLAD_GL_KHR_robustness) return;
	glad_glGetGraphicsResetStatus = (PFNGLGETGRAPHICSRESETSTATUSPROC)load("glGetGraphicsResetStatus");
	glad_glReadnPixels = (PFNGLREADNPIXELSPROC)load("glReadnPixels");
	glad_glGetnUniformfv = (PFNGLGETNUNIFORMFVPROC)load("glGetnUniformfv");
	glad_glGetnUniformiv = (PFNGLGETNUNIFORMIVPROC)load("glGetnUniformiv");
	glad_glGetnUniformuiv = (PFNGLGETNUNIFORMUIVPROC)load("glGetnUniformuiv");
	glad_glGetGraphicsResetStatusKHR = (PFNGLGETGRAPHICSRESETSTATUSKHRPROC)load("glGetGraphicsResetStatusKHR");
	glad_glReadnPixelsKHR = (PFNGLREADNPIXELSKHRPROC)load("glReadnPixelsKHR");
	glad_glGetnUniformfvKHR = (PFNGLGETNUNIFORMFVKHRPROC)load("glGetnUniformfvKHR");
	glad_glGetnUniformivKHR = (PFNGLGETNUNIFORMIVKHRPROC)load("glGetnUniformivKHR");
	glad_glGetnUniformuivKHR = (PFNGLGETNUNIFORMUIVKHRPROC)load("glGetnUniformuivKHR");
}
static void load_GL_MESA_framebuffer_flip_y(GLADloadproc load) {
	if(!GLAD_GL_MESA_framebuffer_flip_y) return;
	glad_glFramebufferParameteriMESA = (PFNGLFRAMEBUFFERPARAMETERIMESAPROC)load("glFramebufferParameteriMESA");
	glad_glGetFramebufferParameterivMESA = (PFNGLGETFRAMEBUFFERPARAMETERIVMESAPROC)load("glGetFramebufferParameterivMESA");
}
static void load_GL_MESA_resize_buffers(GLADloadproc load) {
	if(!GLAD_GL_MESA_resize_buffers) return;
	glad_glResizeBuffersMESA = (PFNGLRESIZEBUFFERSMESAPROC)load("glResizeBuffersMESA");
}
static void load_GL_MESA_window_pos(GLADloadproc load) {
	if(!GLAD_GL_MESA_window_pos) return;
	glad_glWindowPos2dMESA = (PFNGLWINDOWPOS2DMESAPROC)load("glWindowPos2dMESA");
	glad_glWindowPos2dvMESA = (PFNGLWINDOWPOS2DVMESAPROC)load("glWindowPos2dvMESA");
	glad_glWindowPos2fMESA = (PFNGLWINDOWPOS2FMESAPROC)load("glWindowPos2fMESA");
	glad_glWindowPos2fvMESA = (PFNGLWINDOWPOS2FVMESAPROC)load("glWindowPos2fvMESA");
	glad_glWindowPos2iMESA = (PFNGLWINDOWPOS2IMESAPROC)load("glWindowPos2iMESA");
	glad_glWindowPos2ivMESA = (PFNGLWINDOWPOS2IVMESAPROC)load("glWindowPos2ivMESA");
	glad_glWindowPos2sMESA = (PFNGLWINDOWPOS2SMESAPROC)load("glWindowPos2sMESA");
	glad_glWindowPos2svMESA = (PFNGLWINDOWPOS2SVMESAPROC)load("glWindowPos2svMESA");
	glad_glWindowPos3dMESA = (PFNGLWINDOWPOS3DMESAPROC)load("glWindowPos3dMESA");
	glad_glWindowPos3dvMESA = (PFNGLWINDOWPOS3DVMESAPROC)load("glWindowPos3dvMESA");
	glad_glWindowPos3fMESA = (PFNGLWINDOWPOS3FMESAPROC)load("glWindowPos3fMESA");
	glad_glWindowPos3fvMESA = (PFNGLWINDOWPOS3FVMESAPROC)load("glWindowPos3fvMESA");
	glad_glWindowPos3iMESA = (PFNGLWINDOWPOS3IMESAPROC)load("glWindowPos3iMESA");
	glad_glWindowPos3ivMESA = (PFNGLWINDOWPOS3IVMESAPROC)load("glWindowPos3ivMESA");
	glad_glWindowPos3sMESA = (PFNGLWINDOWPOS3SMESAPROC)load("glWindowPos3sMESA");
	glad_glWindowPos3svMESA = (PFNGLWINDOWPOS3SVMESAPROC)load("glWindowPos3svMESA");
	glad_glWindowPos4dMESA = (PFNGLWINDOWPOS4DMESAPROC)load("glWindowPos4dMESA");
	glad_glWindowPos4dvMESA = (PFNGLWINDOWPOS4DVMESAPROC)load("glWindowPos4dvMESA");
	glad_glWindowPos4fMESA = (PFNGLWINDOWPOS4FMESAPROC)load("glWindowPos4fMESA");
	glad_glWindowPos4fvMESA = (PFNGLWINDOWPOS4FVMESAPROC)load("glWindowPos4fvMESA");
	glad_glWindowPos4iMESA = (PFNGLWINDOWPOS4IMESAPROC)load("glWindowPos4iMESA");
	glad_glWindowPos4ivMESA = (PFNGLWINDOWPOS4IVMESAPROC)load("glWindowPos4ivMESA");
	glad_glWindowPos4sMESA = (PFNGLWINDOWPOS4SMESAPROC)load("glWindowPos4sMESA");
	glad_glWindowPos4svMESA = (PFNGLWINDOWPOS4SVMESAPROC)load("glWindowPos4svMESA");
}
static void load_GL_NVX_conditional_render(GLADloadproc load) {
	if(!GLAD_GL_NVX_conditional_render) return;
	glad_glBeginConditionalRenderNVX = (PFNGLBEGINCONDITIONALRENDERNVXPROC)load("glBeginConditionalRenderNVX");
	glad_glEndConditionalRenderNVX = (PFNGLENDCONDITIONALRENDERNVXPROC)load("glEndConditionalRenderNVX");
}
static void load_GL_NVX_gpu_multicast2(GLADloadproc load) {
	if(!GLAD_GL_NVX_gpu_multicast2) return;
	glad_glUploadGpuMaskNVX = (PFNGLUPLOADGPUMASKNVXPROC)load("glUploadGpuMaskNVX");
	glad_glMulticastViewportArrayvNVX = (PFNGLMULTICASTVIEWPORTARRAYVNVXPROC)load("glMulticastViewportArrayvNVX");
	glad_glMulticastViewportPositionWScaleNVX = (PFNGLMULTICASTVIEWPORTPOSITIONWSCALENVXPROC)load("glMulticastViewportPositionWScaleNVX");
	glad_glMulticastScissorArrayvNVX = (PFNGLMULTICASTSCISSORARRAYVNVXPROC)load("glMulticastScissorArrayvNVX");
	glad_glAsyncCopyBufferSubDataNVX = (PFNGLASYNCCOPYBUFFERSUBDATANVXPROC)load("glAsyncCopyBufferSubDataNVX");
	glad_glAsyncCopyImageSubDataNVX = (PFNGLASYNCCOPYIMAGESUBDATANVXPROC)load("glAsyncCopyImageSubDataNVX");
}
static void load_GL_NVX_linked_gpu_multicast(GLADloadproc load) {
	if(!GLAD_GL_NVX_linked_gpu_multicast) return;
	glad_glLGPUNamedBufferSubDataNVX = (PFNGLLGPUNAMEDBUFFERSUBDATANVXPROC)load("glLGPUNamedBufferSubDataNVX");
	glad_glLGPUCopyImageSubDataNVX = (PFNGLLGPUCOPYIMAGESUBDATANVXPROC)load("glLGPUCopyImageSubDataNVX");
	glad_glLGPUInterlockNVX = (PFNGLLGPUINTERLOCKNVXPROC)load("glLGPUInterlockNVX");
}
static void load_GL_NVX_progress_fence(GLADloadproc load) {
	if(!GLAD_GL_NVX_progress_fence) return;
	glad_glCreateProgressFenceNVX = (PFNGLCREATEPROGRESSFENCENVXPROC)load("glCreateProgressFenceNVX");
	glad_glSignalSemaphoreui64NVX = (PFNGLSIGNALSEMAPHOREUI64NVXPROC)load("glSignalSemaphoreui64NVX");
	glad_glWaitSemaphoreui64NVX = (PFNGLWAITSEMAPHOREUI64NVXPROC)load("glWaitSemaphoreui64NVX");
	glad_glClientWaitSemaphoreui64NVX = (PFNGLCLIENTWAITSEMAPHOREUI64NVXPROC)load("glClientWaitSemaphoreui64NVX");
}
static void load_GL_NV_alpha_to_coverage_dither_control(GLADloadproc load) {
	if(!GLAD_GL_NV_alpha_to_coverage_dither_control) return;
	glad_glAlphaToCoverageDitherControlNV = (PFNGLALPHATOCOVERAGEDITHERCONTROLNVPROC)load("glAlphaToCoverageDitherControlNV");
}
static void load_GL_NV_bindless_multi_draw_indirect(GLADloadproc load) {
	if(!GLAD_GL_NV_bindless_multi_draw_indirect) return;
	glad_glMultiDrawArraysIndirectBindlessNV = (PFNGLMULTIDRAWARRAYSINDIRECTBINDLESSNVPROC)load("glMultiDrawArraysIndirectBindlessNV");
	glad_glMultiDrawElementsIndirectBindlessNV = (PFNGLMULTIDRAWELEMENTSINDIRECTBINDLESSNVPROC)load("glMultiDrawElementsIndirectBindlessNV");
}
static void load_GL_NV_bindless_multi_draw_indirect_count(GLADloadproc load) {
	if(!GLAD_GL_NV_bindless_multi_draw_indirect_count) return;
	glad_glMultiDrawArraysIndirectBindlessCountNV = (PFNGLMULTIDRAWARRAYSINDIRECTBINDLESSCOUNTNVPROC)load("glMultiDrawArraysIndirectBindlessCountNV");
	glad_glMultiDrawElementsIndirectBindlessCountNV = (PFNGLMULTIDRAWELEMENTSINDIRECTBINDLESSCOUNTNVPROC)load("glMultiDrawElementsIndirectBindlessCountNV");
}
static void load_GL_NV_bindless_texture(GLADloadproc load) {
	if(!GLAD_GL_NV_bindless_texture) return;
	glad_glGetTextureHandleNV = (PFNGLGETTEXTUREHANDLENVPROC)load("glGetTextureHandleNV");
	glad_glGetTextureSamplerHandleNV = (PFNGLGETTEXTURESAMPLERHANDLENVPROC)load("glGetTextureSamplerHandleNV");
	glad_glMakeTextureHandleResidentNV = (PFNGLMAKETEXTUREHANDLERESIDENTNVPROC)load("glMakeTextureHandleResidentNV");
	glad_glMakeTextureHandleNonResidentNV = (PFNGLMAKETEXTUREHANDLENONRESIDENTNVPROC)load("glMakeTextureHandleNonResidentNV");
	glad_glGetImageHandleNV = (PFNGLGETIMAGEHANDLENVPROC)load("glGetImageHandleNV");
	glad_glMakeImageHandleResidentNV = (PFNGLMAKEIMAGEHANDLERESIDENTNVPROC)load("glMakeImageHandleResidentNV");
	glad_glMakeImageHandleNonResidentNV = (PFNGLMAKEIMAGEHANDLENONRESIDENTNVPROC)load("glMakeImageHandleNonResidentNV");
	glad_glUniformHandleui64NV = (PFNGLUNIFORMHANDLEUI64NVPROC)load("glUniformHandleui64NV");
	glad_glUniformHandleui64vNV = (PFNGLUNIFORMHANDLEUI64VNVPROC)load("glUniformHandleui64vNV");
	glad_glProgramUniformHandleui64NV = (PFNGLPROGRAMUNIFORMHANDLEUI64NVPROC)load("glProgramUniformHandleui64NV");
	glad_glProgramUniformHandleui64vNV = (PFNGLPROGRAMUNIFORMHANDLEUI64VNVPROC)load("glProgramUniformHandleui64vNV");
	glad_glIsTextureHandleResidentNV = (PFNGLISTEXTUREHANDLERESIDENTNVPROC)load("glIsTextureHandleResidentNV");
	glad_glIsImageHandleResidentNV = (PFNGLISIMAGEHANDLERESIDENTNVPROC)load("glIsImageHandleResidentNV");
}
static void load_GL_NV_blend_equation_advanced(GLADloadproc load) {
	if(!GLAD_GL_NV_blend_equation_advanced) return;
	glad_glBlendParameteriNV = (PFNGLBLENDPARAMETERINVPROC)load("glBlendParameteriNV");
	glad_glBlendBarrierNV = (PFNGLBLENDBARRIERNVPROC)load("glBlendBarrierNV");
}
static void load_GL_NV_clip_space_w_scaling(GLADloadproc load) {
	if(!GLAD_GL_NV_clip_space_w_scaling) return;
	glad_glViewportPositionWScaleNV = (PFNGLVIEWPORTPOSITIONWSCALENVPROC)load("glViewportPositionWScaleNV");
}
static void load_GL_NV_command_list(GLADloadproc load) {
	if(!GLAD_GL_NV_command_list) return;
	glad_glCreateStatesNV = (PFNGLCREATESTATESNVPROC)load("glCreateStatesNV");
	glad_glDeleteStatesNV = (PFNGLDELETESTATESNVPROC)load("glDeleteStatesNV");
	glad_glIsStateNV = (PFNGLISSTATENVPROC)load("glIsStateNV");
	glad_glStateCaptureNV = (PFNGLSTATECAPTURENVPROC)load("glStateCaptureNV");
	glad_glGetCommandHeaderNV = (PFNGLGETCOMMANDHEADERNVPROC)load("glGetCommandHeaderNV");
	glad_glGetStageIndexNV = (PFNGLGETSTAGEINDEXNVPROC)load("glGetStageIndexNV");
	glad_glDrawCommandsNV = (PFNGLDRAWCOMMANDSNVPROC)load("glDrawCommandsNV");
	glad_glDrawCommandsAddressNV = (PFNGLDRAWCOMMANDSADDRESSNVPROC)load("glDrawCommandsAddressNV");
	glad_glDrawCommandsStatesNV = (PFNGLDRAWCOMMANDSSTATESNVPROC)load("glDrawCommandsStatesNV");
	glad_glDrawCommandsStatesAddressNV = (PFNGLDRAWCOMMANDSSTATESADDRESSNVPROC)load("glDrawCommandsStatesAddressNV");
	glad_glCreateCommandListsNV = (PFNGLCREATECOMMANDLISTSNVPROC)load("glCreateCommandListsNV");
	glad_glDeleteCommandListsNV = (PFNGLDELETECOMMANDLISTSNVPROC)load("glDeleteCommandListsNV");
	glad_glIsCommandListNV = (PFNGLISCOMMANDLISTNVPROC)load("glIsCommandListNV");
	glad_glListDrawCommandsStatesClientNV = (PFNGLLISTDRAWCOMMANDSSTATESCLIENTNVPROC)load("glListDrawCommandsStatesClientNV");
	glad_glCommandListSegmentsNV = (PFNGLCOMMANDLISTSEGMENTSNVPROC)load("glCommandListSegmentsNV");
	glad_glCompileCommandListNV = (PFNGLCOMPILECOMMANDLISTNVPROC)load("glCompileCommandListNV");
	glad_glCallCommandListNV = (PFNGLCALLCOMMANDLISTNVPROC)load("glCallCommandListNV");
}
static void load_GL_NV_conditional_render(GLADloadproc load) {
	if(!GLAD_GL_NV_conditional_render) return;
	glad_glBeginConditionalRenderNV = (PFNGLBEGINCONDITIONALRENDERNVPROC)load("glBeginConditionalRenderNV");
	glad_glEndConditionalRenderNV = (PFNGLENDCONDITIONALRENDERNVPROC)load("glEndConditionalRenderNV");
}
static void load_GL_NV_conservative_raster(GLADloadproc load) {
	if(!GLAD_GL_NV_conservative_raster) return;
	glad_glSubpixelPrecisionBiasNV = (PFNGLSUBPIXELPRECISIONBIASNVPROC)load("glSubpixelPrecisionBiasNV");
}
static void load_GL_NV_conservative_raster_dilate(GLADloadproc load) {
	if(!GLAD_GL_NV_conservative_raster_dilate) return;
	glad_glConservativeRasterParameterfNV = (PFNGLCONSERVATIVERASTERPARAMETERFNVPROC)load("glConservativeRasterParameterfNV");
}
static void load_GL_NV_conservative_raster_pre_snap_triangles(GLADloadproc load) {
	if(!GLAD_GL_NV_conservative_raster_pre_snap_triangles) return;
	glad_glConservativeRasterParameteriNV = (PFNGLCONSERVATIVERASTERPARAMETERINVPROC)load("glConservativeRasterParameteriNV");
}
static void load_GL_NV_copy_image(GLADloadproc load) {
	if(!GLAD_GL_NV_copy_image) return;
	glad_glCopyImageSubDataNV = (PFNGLCOPYIMAGESUBDATANVPROC)load("glCopyImageSubDataNV");
}
static void load_GL_NV_depth_buffer_float(GLADloadproc load) {
	if(!GLAD_GL_NV_depth_buffer_float) return;
	glad_glDepthRangedNV = (PFNGLDEPTHRANGEDNVPROC)load("glDepthRangedNV");
	glad_glClearDepthdNV = (PFNGLCLEARDEPTHDNVPROC)load("glClearDepthdNV");
	glad_glDepthBoundsdNV = (PFNGLDEPTHBOUNDSDNVPROC)load("glDepthBoundsdNV");
}
static void load_GL_NV_draw_texture(GLADloadproc load) {
	if(!GLAD_GL_NV_draw_texture) return;
	glad_glDrawTextureNV = (PFNGLDRAWTEXTURENVPROC)load("glDrawTextureNV");
}
static void load_GL_NV_draw_vulkan_image(GLADloadproc load) {
	if(!GLAD_GL_NV_draw_vulkan_image) return;
	glad_glDrawVkImageNV = (PFNGLDRAWVKIMAGENVPROC)load("glDrawVkImageNV");
	glad_glGetVkProcAddrNV = (PFNGLGETVKPROCADDRNVPROC)load("glGetVkProcAddrNV");
	glad_glWaitVkSemaphoreNV = (PFNGLWAITVKSEMAPHORENVPROC)load("glWaitVkSemaphoreNV");
	glad_glSignalVkSemaphoreNV = (PFNGLSIGNALVKSEMAPHORENVPROC)load("glSignalVkSemaphoreNV");
	glad_glSignalVkFenceNV = (PFNGLSIGNALVKFENCENVPROC)load("glSignalVkFenceNV");
}
static void load_GL_NV_evaluators(GLADloadproc load) {
	if(!GLAD_GL_NV_evaluators) return;
	glad_glMapControlPointsNV = (PFNGLMAPCONTROLPOINTSNVPROC)load("glMapControlPointsNV");
	glad_glMapParameterivNV = (PFNGLMAPPARAMETERIVNVPROC)load("glMapParameterivNV");
	glad_glMapParameterfvNV = (PFNGLMAPPARAMETERFVNVPROC)load("glMapParameterfvNV");
	glad_glGetMapControlPointsNV = (PFNGLGETMAPCONTROLPOINTSNVPROC)load("glGetMapControlPointsNV");
	glad_glGetMapParameterivNV = (PFNGLGETMAPPARAMETERIVNVPROC)load("glGetMapParameterivNV");
	glad_glGetMapParameterfvNV = (PFNGLGETMAPPARAMETERFVNVPROC)load("glGetMapParameterfvNV");
	glad_glGetMapAttribParameterivNV = (PFNGLGETMAPATTRIBPARAMETERIVNVPROC)load("glGetMapAttribParameterivNV");
	glad_glGetMapAttribParameterfvNV = (PFNGLGETMAPATTRIBPARAMETERFVNVPROC)load("glGetMapAttribParameterfvNV");
	glad_glEvalMapsNV = (PFNGLEVALMAPSNVPROC)load("glEvalMapsNV");
}
static void load_GL_NV_explicit_multisample(GLADloadproc load) {
	if(!GLAD_GL_NV_explicit_multisample) return;
	glad_glGetMultisamplefvNV = (PFNGLGETMULTISAMPLEFVNVPROC)load("glGetMultisamplefvNV");
	glad_glSampleMaskIndexedNV = (PFNGLSAMPLEMASKINDEXEDNVPROC)load("glSampleMaskIndexedNV");
	glad_glTexRenderbufferNV = (PFNGLTEXRENDERBUFFERNVPROC)load("glTexRenderbufferNV");
}
static void load_GL_NV_fence(GLADloadproc load) {
	if(!GLAD_GL_NV_fence) return;
	glad_glDeleteFencesNV = (PFNGLDELETEFENCESNVPROC)load("glDeleteFencesNV");
	glad_glGenFencesNV = (PFNGLGENFENCESNVPROC)load("glGenFencesNV");
	glad_glIsFenceNV = (PFNGLISFENCENVPROC)load("glIsFenceNV");
	glad_glTestFenceNV = (PFNGLTESTFENCENVPROC)load("glTestFenceNV");
	glad_glGetFenceivNV = (PFNGLGETFENCEIVNVPROC)load("glGetFenceivNV");
	glad_glFinishFenceNV = (PFNGLFINISHFENCENVPROC)load("glFinishFenceNV");
	glad_glSetFenceNV = (PFNGLSETFENCENVPROC)load("glSetFenceNV");
}
static void load_GL_NV_fragment_coverage_to_color(GLADloadproc load) {
	if(!GLAD_GL_NV_fragment_coverage_to_color) return;
	glad_glFragmentCoverageColorNV = (PFNGLFRAGMENTCOVERAGECOLORNVPROC)load("glFragmentCoverageColorNV");
}
static void load_GL_NV_fragment_program(GLADloadproc load) {
	if(!GLAD_GL_NV_fragment_program) return;
	glad_glProgramNamedParameter4fNV = (PFNGLPROGRAMNAMEDPARAMETER4FNVPROC)load("glProgramNamedParameter4fNV");
	glad_glProgramNamedParameter4fvNV = (PFNGLPROGRAMNAMEDPARAMETER4FVNVPROC)load("glProgramNamedParameter4fvNV");
	glad_glProgramNamedParameter4dNV = (PFNGLPROGRAMNAMEDPARAMETER4DNVPROC)load("glProgramNamedParameter4dNV");
	glad_glProgramNamedParameter4dvNV = (PFNGLPROGRAMNAMEDPARAMETER4DVNVPROC)load("glProgramNamedParameter4dvNV");
	glad_glGetProgramNamedParameterfvNV = (PFNGLGETPROGRAMNAMEDPARAMETERFVNVPROC)load("glGetProgramNamedParameterfvNV");
	glad_glGetProgramNamedParameterdvNV = (PFNGLGETPROGRAMNAMEDPARAMETERDVNVPROC)load("glGetProgramNamedParameterdvNV");
}
static void load_GL_NV_framebuffer_mixed_samples(GLADloadproc load) {
	if(!GLAD_GL_NV_framebuffer_mixed_samples) return;
	glad_glRasterSamplesEXT = (PFNGLRASTERSAMPLESEXTPROC)load("glRasterSamplesEXT");
	glad_glCoverageModulationTableNV = (PFNGLCOVERAGEMODULATIONTABLENVPROC)load("glCoverageModulationTableNV");
	glad_glGetCoverageModulationTableNV = (PFNGLGETCOVERAGEMODULATIONTABLENVPROC)load("glGetCoverageModulationTableNV");
	glad_glCoverageModulationNV = (PFNGLCOVERAGEMODULATIONNVPROC)load("glCoverageModulationNV");
}
static void load_GL_NV_framebuffer_multisample_coverage(GLADloadproc load) {
	if(!GLAD_GL_NV_framebuffer_multisample_coverage) return;
	glad_glRenderbufferStorageMultisampleCoverageNV = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLECOVERAGENVPROC)load("glRenderbufferStorageMultisampleCoverageNV");
}
static void load_GL_NV_geometry_program4(GLADloadproc load) {
	if(!GLAD_GL_NV_geometry_program4) return;
	glad_glProgramVertexLimitNV = (PFNGLPROGRAMVERTEXLIMITNVPROC)load("glProgramVertexLimitNV");
	glad_glFramebufferTextureEXT = (PFNGLFRAMEBUFFERTEXTUREEXTPROC)load("glFramebufferTextureEXT");
	glad_glFramebufferTextureLayerEXT = (PFNGLFRAMEBUFFERTEXTURELAYEREXTPROC)load("glFramebufferTextureLayerEXT");
	glad_glFramebufferTextureFaceEXT = (PFNGLFRAMEBUFFERTEXTUREFACEEXTPROC)load("glFramebufferTextureFaceEXT");
}
static void load_GL_NV_gpu_multicast(GLADloadproc load) {
	if(!GLAD_GL_NV_gpu_multicast) return;
	glad_glRenderGpuMaskNV = (PFNGLRENDERGPUMASKNVPROC)load("glRenderGpuMaskNV");
	glad_glMulticastBufferSubDataNV = (PFNGLMULTICASTBUFFERSUBDATANVPROC)load("glMulticastBufferSubDataNV");
	glad_glMulticastCopyBufferSubDataNV = (PFNGLMULTICASTCOPYBUFFERSUBDATANVPROC)load("glMulticastCopyBufferSubDataNV");
	glad_glMulticastCopyImageSubDataNV = (PFNGLMULTICASTCOPYIMAGESUBDATANVPROC)load("glMulticastCopyImageSubDataNV");
	glad_glMulticastBlitFramebufferNV = (PFNGLMULTICASTBLITFRAMEBUFFERNVPROC)load("glMulticastBlitFramebufferNV");
	glad_glMulticastFramebufferSampleLocationsfvNV = (PFNGLMULTICASTFRAMEBUFFERSAMPLELOCATIONSFVNVPROC)load("glMulticastFramebufferSampleLocationsfvNV");
	glad_glMulticastBarrierNV = (PFNGLMULTICASTBARRIERNVPROC)load("glMulticastBarrierNV");
	glad_glMulticastWaitSyncNV = (PFNGLMULTICASTWAITSYNCNVPROC)load("glMulticastWaitSyncNV");
	glad_glMulticastGetQueryObjectivNV = (PFNGLMULTICASTGETQUERYOBJECTIVNVPROC)load("glMulticastGetQueryObjectivNV");
	glad_glMulticastGetQueryObjectuivNV = (PFNGLMULTICASTGETQUERYOBJECTUIVNVPROC)load("glMulticastGetQueryObjectuivNV");
	glad_glMulticastGetQueryObjecti64vNV = (PFNGLMULTICASTGETQUERYOBJECTI64VNVPROC)load("glMulticastGetQueryObjecti64vNV");
	glad_glMulticastGetQueryObjectui64vNV = (PFNGLMULTICASTGETQUERYOBJECTUI64VNVPROC)load("glMulticastGetQueryObjectui64vNV");
}
static void load_GL_NV_gpu_program4(GLADloadproc load) {
	if(!GLAD_GL_NV_gpu_program4) return;
	glad_glProgramLocalParameterI4iNV = (PFNGLPROGRAMLOCALPARAMETERI4INVPROC)load("glProgramLocalParameterI4iNV");
	glad_glProgramLocalParameterI4ivNV = (PFNGLPROGRAMLOCALPARAMETERI4IVNVPROC)load("glProgramLocalParameterI4ivNV");
	glad_glProgramLocalParametersI4ivNV = (PFNGLPROGRAMLOCALPARAMETERSI4IVNVPROC)load("glProgramLocalParametersI4ivNV");
	glad_glProgramLocalParameterI4uiNV = (PFNGLPROGRAMLOCALPARAMETERI4UINVPROC)load("glProgramLocalParameterI4uiNV");
	glad_glProgramLocalParameterI4uivNV = (PFNGLPROGRAMLOCALPARAMETERI4UIVNVPROC)load("glProgramLocalParameterI4uivNV");
	glad_glProgramLocalParametersI4uivNV = (PFNGLPROGRAMLOCALPARAMETERSI4UIVNVPROC)load("glProgramLocalParametersI4uivNV");
	glad_glProgramEnvParameterI4iNV = (PFNGLPROGRAMENVPARAMETERI4INVPROC)load("glProgramEnvParameterI4iNV");
	glad_glProgramEnvParameterI4ivNV = (PFNGLPROGRAMENVPARAMETERI4IVNVPROC)load("glProgramEnvParameterI4ivNV");
	glad_glProgramEnvParametersI4ivNV = (PFNGLPROGRAMENVPARAMETERSI4IVNVPROC)load("glProgramEnvParametersI4ivNV");
	glad_glProgramEnvParameterI4uiNV = (PFNGLPROGRAMENVPARAMETERI4UINVPROC)load("glProgramEnvParameterI4uiNV");
	glad_glProgramEnvParameterI4uivNV = (PFNGLPROGRAMENVPARAMETERI4UIVNVPROC)load("glProgramEnvParameterI4uivNV");
	glad_glProgramEnvParametersI4uivNV = (PFNGLPROGRAMENVPARAMETERSI4UIVNVPROC)load("glProgramEnvParametersI4uivNV");
	glad_glGetProgramLocalParameterIivNV = (PFNGLGETPROGRAMLOCALPARAMETERIIVNVPROC)load("glGetProgramLocalParameterIivNV");
	glad_glGetProgramLocalParameterIuivNV = (PFNGLGETPROGRAMLOCALPARAMETERIUIVNVPROC)load("glGetProgramLocalParameterIuivNV");
	glad_glGetProgramEnvParameterIivNV = (PFNGLGETPROGRAMENVPARAMETERIIVNVPROC)load("glGetProgramEnvParameterIivNV");
	glad_glGetProgramEnvParameterIuivNV = (PFNGLGETPROGRAMENVPARAMETERIUIVNVPROC)load("glGetProgramEnvParameterIuivNV");
}
static void load_GL_NV_gpu_program5(GLADloadproc load) {
	if(!GLAD_GL_NV_gpu_program5) return;
	glad_glProgramSubroutineParametersuivNV = (PFNGLPROGRAMSUBROUTINEPARAMETERSUIVNVPROC)load("glProgramSubroutineParametersuivNV");
	glad_glGetProgramSubroutineParameteruivNV = (PFNGLGETPROGRAMSUBROUTINEPARAMETERUIVNVPROC)load("glGetProgramSubroutineParameteruivNV");
}
static void load_GL_NV_gpu_shader5(GLADloadproc load) {
	if(!GLAD_GL_NV_gpu_shader5) return;
	glad_glUniform1i64NV = (PFNGLUNIFORM1I64NVPROC)load("glUniform1i64NV");
	glad_glUniform2i64NV = (PFNGLUNIFORM2I64NVPROC)load("glUniform2i64NV");
	glad_glUniform3i64NV = (PFNGLUNIFORM3I64NVPROC)load("glUniform3i64NV");
	glad_glUniform4i64NV = (PFNGLUNIFORM4I64NVPROC)load("glUniform4i64NV");
	glad_glUniform1i64vNV = (PFNGLUNIFORM1I64VNVPROC)load("glUniform1i64vNV");
	glad_glUniform2i64vNV = (PFNGLUNIFORM2I64VNVPROC)load("glUniform2i64vNV");
	glad_glUniform3i64vNV = (PFNGLUNIFORM3I64VNVPROC)load("glUniform3i64vNV");
	glad_glUniform4i64vNV = (PFNGLUNIFORM4I64VNVPROC)load("glUniform4i64vNV");
	glad_glUniform1ui64NV = (PFNGLUNIFORM1UI64NVPROC)load("glUniform1ui64NV");
	glad_glUniform2ui64NV = (PFNGLUNIFORM2UI64NVPROC)load("glUniform2ui64NV");
	glad_glUniform3ui64NV = (PFNGLUNIFORM3UI64NVPROC)load("glUniform3ui64NV");
	glad_glUniform4ui64NV = (PFNGLUNIFORM4UI64NVPROC)load("glUniform4ui64NV");
	glad_glUniform1ui64vNV = (PFNGLUNIFORM1UI64VNVPROC)load("glUniform1ui64vNV");
	glad_glUniform2ui64vNV = (PFNGLUNIFORM2UI64VNVPROC)load("glUniform2ui64vNV");
	glad_glUniform3ui64vNV = (PFNGLUNIFORM3UI64VNVPROC)load("glUniform3ui64vNV");
	glad_glUniform4ui64vNV = (PFNGLUNIFORM4UI64VNVPROC)load("glUniform4ui64vNV");
	glad_glGetUniformi64vNV = (PFNGLGETUNIFORMI64VNVPROC)load("glGetUniformi64vNV");
	glad_glProgramUniform1i64NV = (PFNGLPROGRAMUNIFORM1I64NVPROC)load("glProgramUniform1i64NV");
	glad_glProgramUniform2i64NV = (PFNGLPROGRAMUNIFORM2I64NVPROC)load("glProgramUniform2i64NV");
	glad_glProgramUniform3i64NV = (PFNGLPROGRAMUNIFORM3I64NVPROC)load("glProgramUniform3i64NV");
	glad_glProgramUniform4i64NV = (PFNGLPROGRAMUNIFORM4I64NVPROC)load("glProgramUniform4i64NV");
	glad_glProgramUniform1i64vNV = (PFNGLPROGRAMUNIFORM1I64VNVPROC)load("glProgramUniform1i64vNV");
	glad_glProgramUniform2i64vNV = (PFNGLPROGRAMUNIFORM2I64VNVPROC)load("glProgramUniform2i64vNV");
	glad_glProgramUniform3i64vNV = (PFNGLPROGRAMUNIFORM3I64VNVPROC)load("glProgramUniform3i64vNV");
	glad_glProgramUniform4i64vNV = (PFNGLPROGRAMUNIFORM4I64VNVPROC)load("glProgramUniform4i64vNV");
	glad_glProgramUniform1ui64NV = (PFNGLPROGRAMUNIFORM1UI64NVPROC)load("glProgramUniform1ui64NV");
	glad_glProgramUniform2ui64NV = (PFNGLPROGRAMUNIFORM2UI64NVPROC)load("glProgramUniform2ui64NV");
	glad_glProgramUniform3ui64NV = (PFNGLPROGRAMUNIFORM3UI64NVPROC)load("glProgramUniform3ui64NV");
	glad_glProgramUniform4ui64NV = (PFNGLPROGRAMUNIFORM4UI64NVPROC)load("glProgramUniform4ui64NV");
	glad_glProgramUniform1ui64vNV = (PFNGLPROGRAMUNIFORM1UI64VNVPROC)load("glProgramUniform1ui64vNV");
	glad_glProgramUniform2ui64vNV = (PFNGLPROGRAMUNIFORM2UI64VNVPROC)load("glProgramUniform2ui64vNV");
	glad_glProgramUniform3ui64vNV = (PFNGLPROGRAMUNIFORM3UI64VNVPROC)load("glProgramUniform3ui64vNV");
	glad_glProgramUniform4ui64vNV = (PFNGLPROGRAMUNIFORM4UI64VNVPROC)load("glProgramUniform4ui64vNV");
}
static void load_GL_NV_half_float(GLADloadproc load) {
	if(!GLAD_GL_NV_half_float) return;
	glad_glVertex2hNV = (PFNGLVERTEX2HNVPROC)load("glVertex2hNV");
	glad_glVertex2hvNV = (PFNGLVERTEX2HVNVPROC)load("glVertex2hvNV");
	glad_glVertex3hNV = (PFNGLVERTEX3HNVPROC)load("glVertex3hNV");
	glad_glVertex3hvNV = (PFNGLVERTEX3HVNVPROC)load("glVertex3hvNV");
	glad_glVertex4hNV = (PFNGLVERTEX4HNVPROC)load("glVertex4hNV");
	glad_glVertex4hvNV = (PFNGLVERTEX4HVNVPROC)load("glVertex4hvNV");
	glad_glNormal3hNV = (PFNGLNORMAL3HNVPROC)load("glNormal3hNV");
	glad_glNormal3hvNV = (PFNGLNORMAL3HVNVPROC)load("glNormal3hvNV");
	glad_glColor3hNV = (PFNGLCOLOR3HNVPROC)load("glColor3hNV");
	glad_glColor3hvNV = (PFNGLCOLOR3HVNVPROC)load("glColor3hvNV");
	glad_glColor4hNV = (PFNGLCOLOR4HNVPROC)load("glColor4hNV");
	glad_glColor4hvNV = (PFNGLCOLOR4HVNVPROC)load("glColor4hvNV");
	glad_glTexCoord1hNV = (PFNGLTEXCOORD1HNVPROC)load("glTexCoord1hNV");
	glad_glTexCoord1hvNV = (PFNGLTEXCOORD1HVNVPROC)load("glTexCoord1hvNV");
	glad_glTexCoord2hNV = (PFNGLTEXCOORD2HNVPROC)load("glTexCoord2hNV");
	glad_glTexCoord2hvNV = (PFNGLTEXCOORD2HVNVPROC)load("glTexCoord2hvNV");
	glad_glTexCoord3hNV = (PFNGLTEXCOORD3HNVPROC)load("glTexCoord3hNV");
	glad_glTexCoord3hvNV = (PFNGLTEXCOORD3HVNVPROC)load("glTexCoord3hvNV");
	glad_glTexCoord4hNV = (PFNGLTEXCOORD4HNVPROC)load("glTexCoord4hNV");
	glad_glTexCoord4hvNV = (PFNGLTEXCOORD4HVNVPROC)load("glTexCoord4hvNV");
	glad_glMultiTexCoord1hNV = (PFNGLMULTITEXCOORD1HNVPROC)load("glMultiTexCoord1hNV");
	glad_glMultiTexCoord1hvNV = (PFNGLMULTITEXCOORD1HVNVPROC)load("glMultiTexCoord1hvNV");
	glad_glMultiTexCoord2hNV = (PFNGLMULTITEXCOORD2HNVPROC)load("glMultiTexCoord2hNV");
	glad_glMultiTexCoord2hvNV = (PFNGLMULTITEXCOORD2HVNVPROC)load("glMultiTexCoord2hvNV");
	glad_glMultiTexCoord3hNV = (PFNGLMULTITEXCOORD3HNVPROC)load("glMultiTexCoord3hNV");
	glad_glMultiTexCoord3hvNV = (PFNGLMULTITEXCOORD3HVNVPROC)load("glMultiTexCoord3hvNV");
	glad_glMultiTexCoord4hNV = (PFNGLMULTITEXCOORD4HNVPROC)load("glMultiTexCoord4hNV");
	glad_glMultiTexCoord4hvNV = (PFNGLMULTITEXCOORD4HVNVPROC)load("glMultiTexCoord4hvNV");
	glad_glFogCoordhNV = (PFNGLFOGCOORDHNVPROC)load("glFogCoordhNV");
	glad_glFogCoordhvNV = (PFNGLFOGCOORDHVNVPROC)load("glFogCoordhvNV");
	glad_glSecondaryColor3hNV = (PFNGLSECONDARYCOLOR3HNVPROC)load("glSecondaryColor3hNV");
	glad_glSecondaryColor3hvNV = (PFNGLSECONDARYCOLOR3HVNVPROC)load("glSecondaryColor3hvNV");
	glad_glVertexWeighthNV = (PFNGLVERTEXWEIGHTHNVPROC)load("glVertexWeighthNV");
	glad_glVertexWeighthvNV = (PFNGLVERTEXWEIGHTHVNVPROC)load("glVertexWeighthvNV");
	glad_glVertexAttrib1hNV = (PFNGLVERTEXATTRIB1HNVPROC)load("glVertexAttrib1hNV");
	glad_glVertexAttrib1hvNV = (PFNGLVERTEXATTRIB1HVNVPROC)load("glVertexAttrib1hvNV");
	glad_glVertexAttrib2hNV = (PFNGLVERTEXATTRIB2HNVPROC)load("glVertexAttrib2hNV");
	glad_glVertexAttrib2hvNV = (PFNGLVERTEXATTRIB2HVNVPROC)load("glVertexAttrib2hvNV");
	glad_glVertexAttrib3hNV = (PFNGLVERTEXATTRIB3HNVPROC)load("glVertexAttrib3hNV");
	glad_glVertexAttrib3hvNV = (PFNGLVERTEXATTRIB3HVNVPROC)load("glVertexAttrib3hvNV");
	glad_glVertexAttrib4hNV = (PFNGLVERTEXATTRIB4HNVPROC)load("glVertexAttrib4hNV");
	glad_glVertexAttrib4hvNV = (PFNGLVERTEXATTRIB4HVNVPROC)load("glVertexAttrib4hvNV");
	glad_glVertexAttribs1hvNV = (PFNGLVERTEXATTRIBS1HVNVPROC)load("glVertexAttribs1hvNV");
	glad_glVertexAttribs2hvNV = (PFNGLVERTEXATTRIBS2HVNVPROC)load("glVertexAttribs2hvNV");
	glad_glVertexAttribs3hvNV = (PFNGLVERTEXATTRIBS3HVNVPROC)load("glVertexAttribs3hvNV");
	glad_glVertexAttribs4hvNV = (PFNGLVERTEXATTRIBS4HVNVPROC)load("glVertexAttribs4hvNV");
}
static void load_GL_NV_internalformat_sample_query(GLADloadproc load) {
	if(!GLAD_GL_NV_internalformat_sample_query) return;
	glad_glGetInternalformatSampleivNV = (PFNGLGETINTERNALFORMATSAMPLEIVNVPROC)load("glGetInternalformatSampleivNV");
}
static void load_GL_NV_memory_attachment(GLADloadproc load) {
	if(!GLAD_GL_NV_memory_attachment) return;
	glad_glGetMemoryObjectDetachedResourcesuivNV = (PFNGLGETMEMORYOBJECTDETACHEDRESOURCESUIVNVPROC)load("glGetMemoryObjectDetachedResourcesuivNV");
	glad_glResetMemoryObjectParameterNV = (PFNGLRESETMEMORYOBJECTPARAMETERNVPROC)load("glResetMemoryObjectParameterNV");
	glad_glTexAttachMemoryNV = (PFNGLTEXATTACHMEMORYNVPROC)load("glTexAttachMemoryNV");
	glad_glBufferAttachMemoryNV = (PFNGLBUFFERATTACHMEMORYNVPROC)load("glBufferAttachMemoryNV");
	glad_glTextureAttachMemoryNV = (PFNGLTEXTUREATTACHMEMORYNVPROC)load("glTextureAttachMemoryNV");
	glad_glNamedBufferAttachMemoryNV = (PFNGLNAMEDBUFFERATTACHMEMORYNVPROC)load("glNamedBufferAttachMemoryNV");
}
static void load_GL_NV_memory_object_sparse(GLADloadproc load) {
	if(!GLAD_GL_NV_memory_object_sparse) return;
	glad_glBufferPageCommitmentMemNV = (PFNGLBUFFERPAGECOMMITMENTMEMNVPROC)load("glBufferPageCommitmentMemNV");
	glad_glTexPageCommitmentMemNV = (PFNGLTEXPAGECOMMITMENTMEMNVPROC)load("glTexPageCommitmentMemNV");
	glad_glNamedBufferPageCommitmentMemNV = (PFNGLNAMEDBUFFERPAGECOMMITMENTMEMNVPROC)load("glNamedBufferPageCommitmentMemNV");
	glad_glTexturePageCommitmentMemNV = (PFNGLTEXTUREPAGECOMMITMENTMEMNVPROC)load("glTexturePageCommitmentMemNV");
}
static void load_GL_NV_mesh_shader(GLADloadproc load) {
	if(!GLAD_GL_NV_mesh_shader) return;
	glad_glDrawMeshTasksNV = (PFNGLDRAWMESHTASKSNVPROC)load("glDrawMeshTasksNV");
	glad_glDrawMeshTasksIndirectNV = (PFNGLDRAWMESHTASKSINDIRECTNVPROC)load("glDrawMeshTasksIndirectNV");
	glad_glMultiDrawMeshTasksIndirectNV = (PFNGLMULTIDRAWMESHTASKSINDIRECTNVPROC)load("glMultiDrawMeshTasksIndirectNV");
	glad_glMultiDrawMeshTasksIndirectCountNV = (PFNGLMULTIDRAWMESHTASKSINDIRECTCOUNTNVPROC)load("glMultiDrawMeshTasksIndirectCountNV");
}
static void load_GL_NV_occlusion_query(GLADloadproc load) {
	if(!GLAD_GL_NV_occlusion_query) return;
	glad_glGenOcclusionQueriesNV = (PFNGLGENOCCLUSIONQUERIESNVPROC)load("glGenOcclusionQueriesNV");
	glad_glDeleteOcclusionQueriesNV = (PFNGLDELETEOCCLUSIONQUERIESNVPROC)load("glDeleteOcclusionQueriesNV");
	glad_glIsOcclusionQueryNV = (PFNGLISOCCLUSIONQUERYNVPROC)load("glIsOcclusionQueryNV");
	glad_glBeginOcclusionQueryNV = (PFNGLBEGINOCCLUSIONQUERYNVPROC)load("glBeginOcclusionQueryNV");
	glad_glEndOcclusionQueryNV = (PFNGLENDOCCLUSIONQUERYNVPROC)load("glEndOcclusionQueryNV");
	glad_glGetOcclusionQueryivNV = (PFNGLGETOCCLUSIONQUERYIVNVPROC)load("glGetOcclusionQueryivNV");
	glad_glGetOcclusionQueryuivNV = (PFNGLGETOCCLUSIONQUERYUIVNVPROC)load("glGetOcclusionQueryuivNV");
}
static void load_GL_NV_parameter_buffer_object(GLADloadproc load) {
	if(!GLAD_GL_NV_parameter_buffer_object) return;
	glad_glProgramBufferParametersfvNV = (PFNGLPROGRAMBUFFERPARAMETERSFVNVPROC)load("glProgramBufferParametersfvNV");
	glad_glProgramBufferParametersIivNV = (PFNGLPROGRAMBUFFERPARAMETERSIIVNVPROC)load("glProgramBufferParametersIivNV");
	glad_glProgramBufferParametersIuivNV = (PFNGLPROGRAMBUFFERPARAMETERSIUIVNVPROC)load("glProgramBufferParametersIuivNV");
}
static void load_GL_NV_path_rendering(GLADloadproc load) {
	if(!GLAD_GL_NV_path_rendering) return;
	glad_glGenPathsNV = (PFNGLGENPATHSNVPROC)load("glGenPathsNV");
	glad_glDeletePathsNV = (PFNGLDELETEPATHSNVPROC)load("glDeletePathsNV");
	glad_glIsPathNV = (PFNGLISPATHNVPROC)load("glIsPathNV");
	glad_glPathCommandsNV = (PFNGLPATHCOMMANDSNVPROC)load("glPathCommandsNV");
	glad_glPathCoordsNV = (PFNGLPATHCOORDSNVPROC)load("glPathCoordsNV");
	glad_glPathSubCommandsNV = (PFNGLPATHSUBCOMMANDSNVPROC)load("glPathSubCommandsNV");
	glad_glPathSubCoordsNV = (PFNGLPATHSUBCOORDSNVPROC)load("glPathSubCoordsNV");
	glad_glPathStringNV = (PFNGLPATHSTRINGNVPROC)load("glPathStringNV");
	glad_glPathGlyphsNV = (PFNGLPATHGLYPHSNVPROC)load("glPathGlyphsNV");
	glad_glPathGlyphRangeNV = (PFNGLPATHGLYPHRANGENVPROC)load("glPathGlyphRangeNV");
	glad_glWeightPathsNV = (PFNGLWEIGHTPATHSNVPROC)load("glWeightPathsNV");
	glad_glCopyPathNV = (PFNGLCOPYPATHNVPROC)load("glCopyPathNV");
	glad_glInterpolatePathsNV = (PFNGLINTERPOLATEPATHSNVPROC)load("glInterpolatePathsNV");
	glad_glTransformPathNV = (PFNGLTRANSFORMPATHNVPROC)load("glTransformPathNV");
	glad_glPathParameterivNV = (PFNGLPATHPARAMETERIVNVPROC)load("glPathParameterivNV");
	glad_glPathParameteriNV = (PFNGLPATHPARAMETERINVPROC)load("glPathParameteriNV");
	glad_glPathParameterfvNV = (PFNGLPATHPARAMETERFVNVPROC)load("glPathParameterfvNV");
	glad_glPathParameterfNV = (PFNGLPATHPARAMETERFNVPROC)load("glPathParameterfNV");
	glad_glPathDashArrayNV = (PFNGLPATHDASHARRAYNVPROC)load("glPathDashArrayNV");
	glad_glPathStencilFuncNV = (PFNGLPATHSTENCILFUNCNVPROC)load("glPathStencilFuncNV");
	glad_glPathStencilDepthOffsetNV = (PFNGLPATHSTENCILDEPTHOFFSETNVPROC)load("glPathStencilDepthOffsetNV");
	glad_glStencilFillPathNV = (PFNGLSTENCILFILLPATHNVPROC)load("glStencilFillPathNV");
	glad_glStencilStrokePathNV = (PFNGLSTENCILSTROKEPATHNVPROC)load("glStencilStrokePathNV");
	glad_glStencilFillPathInstancedNV = (PFNGLSTENCILFILLPATHINSTANCEDNVPROC)load("glStencilFillPathInstancedNV");
	glad_glStencilStrokePathInstancedNV = (PFNGLSTENCILSTROKEPATHINSTANCEDNVPROC)load("glStencilStrokePathInstancedNV");
	glad_glPathCoverDepthFuncNV = (PFNGLPATHCOVERDEPTHFUNCNVPROC)load("glPathCoverDepthFuncNV");
	glad_glCoverFillPathNV = (PFNGLCOVERFILLPATHNVPROC)load("glCoverFillPathNV");
	glad_glCoverStrokePathNV = (PFNGLCOVERSTROKEPATHNVPROC)load("glCoverStrokePathNV");
	glad_glCoverFillPathInstancedNV = (PFNGLCOVERFILLPATHINSTANCEDNVPROC)load("glCoverFillPathInstancedNV");
	glad_glCoverStrokePathInstancedNV = (PFNGLCOVERSTROKEPATHINSTANCEDNVPROC)load("glCoverStrokePathInstancedNV");
	glad_glGetPathParameterivNV = (PFNGLGETPATHPARAMETERIVNVPROC)load("glGetPathParameterivNV");
	glad_glGetPathParameterfvNV = (PFNGLGETPATHPARAMETERFVNVPROC)load("glGetPathParameterfvNV");
	glad_glGetPathCommandsNV = (PFNGLGETPATHCOMMANDSNVPROC)load("glGetPathCommandsNV");
	glad_glGetPathCoordsNV = (PFNGLGETPATHCOORDSNVPROC)load("glGetPathCoordsNV");
	glad_glGetPathDashArrayNV = (PFNGLGETPATHDASHARRAYNVPROC)load("glGetPathDashArrayNV");
	glad_glGetPathMetricsNV = (PFNGLGETPATHMETRICSNVPROC)load("glGetPathMetricsNV");
	glad_glGetPathMetricRangeNV = (PFNGLGETPATHMETRICRANGENVPROC)load("glGetPathMetricRangeNV");
	glad_glGetPathSpacingNV = (PFNGLGETPATHSPACINGNVPROC)load("glGetPathSpacingNV");
	glad_glIsPointInFillPathNV = (PFNGLISPOINTINFILLPATHNVPROC)load("glIsPointInFillPathNV");
	glad_glIsPointInStrokePathNV = (PFNGLISPOINTINSTROKEPATHNVPROC)load("glIsPointInStrokePathNV");
	glad_glGetPathLengthNV = (PFNGLGETPATHLENGTHNVPROC)load("glGetPathLengthNV");
	glad_glPointAlongPathNV = (PFNGLPOINTALONGPATHNVPROC)load("glPointAlongPathNV");
	glad_glMatrixLoad3x2fNV = (PFNGLMATRIXLOAD3X2FNVPROC)load("glMatrixLoad3x2fNV");
	glad_glMatrixLoad3x3fNV = (PFNGLMATRIXLOAD3X3FNVPROC)load("glMatrixLoad3x3fNV");
	glad_glMatrixLoadTranspose3x3fNV = (PFNGLMATRIXLOADTRANSPOSE3X3FNVPROC)load("glMatrixLoadTranspose3x3fNV");
	glad_glMatrixMult3x2fNV = (PFNGLMATRIXMULT3X2FNVPROC)load("glMatrixMult3x2fNV");
	glad_glMatrixMult3x3fNV = (PFNGLMATRIXMULT3X3FNVPROC)load("glMatrixMult3x3fNV");
	glad_glMatrixMultTranspose3x3fNV = (PFNGLMATRIXMULTTRANSPOSE3X3FNVPROC)load("glMatrixMultTranspose3x3fNV");
	glad_glStencilThenCoverFillPathNV = (PFNGLSTENCILTHENCOVERFILLPATHNVPROC)load("glStencilThenCoverFillPathNV");
	glad_glStencilThenCoverStrokePathNV = (PFNGLSTENCILTHENCOVERSTROKEPATHNVPROC)load("glStencilThenCoverStrokePathNV");
	glad_glStencilThenCoverFillPathInstancedNV = (PFNGLSTENCILTHENCOVERFILLPATHINSTANCEDNVPROC)load("glStencilThenCoverFillPathInstancedNV");
	glad_glStencilThenCoverStrokePathInstancedNV = (PFNGLSTENCILTHENCOVERSTROKEPATHINSTANCEDNVPROC)load("glStencilThenCoverStrokePathInstancedNV");
	glad_glPathGlyphIndexRangeNV = (PFNGLPATHGLYPHINDEXRANGENVPROC)load("glPathGlyphIndexRangeNV");
	glad_glPathGlyphIndexArrayNV = (PFNGLPATHGLYPHINDEXARRAYNVPROC)load("glPathGlyphIndexArrayNV");
	glad_glPathMemoryGlyphIndexArrayNV = (PFNGLPATHMEMORYGLYPHINDEXARRAYNVPROC)load("glPathMemoryGlyphIndexArrayNV");
	glad_glProgramPathFragmentInputGenNV = (PFNGLPROGRAMPATHFRAGMENTINPUTGENNVPROC)load("glProgramPathFragmentInputGenNV");
	glad_glGetProgramResourcefvNV = (PFNGLGETPROGRAMRESOURCEFVNVPROC)load("glGetProgramResourcefvNV");
	glad_glPathColorGenNV = (PFNGLPATHCOLORGENNVPROC)load("glPathColorGenNV");
	glad_glPathTexGenNV = (PFNGLPATHTEXGENNVPROC)load("glPathTexGenNV");
	glad_glPathFogGenNV = (PFNGLPATHFOGGENNVPROC)load("glPathFogGenNV");
	glad_glGetPathColorGenivNV = (PFNGLGETPATHCOLORGENIVNVPROC)load("glGetPathColorGenivNV");
	glad_glGetPathColorGenfvNV = (PFNGLGETPATHCOLORGENFVNVPROC)load("glGetPathColorGenfvNV");
	glad_glGetPathTexGenivNV = (PFNGLGETPATHTEXGENIVNVPROC)load("glGetPathTexGenivNV");
	glad_glGetPathTexGenfvNV = (PFNGLGETPATHTEXGENFVNVPROC)load("glGetPathTexGenfvNV");
	glad_glMatrixFrustumEXT = (PFNGLMATRIXFRUSTUMEXTPROC)load("glMatrixFrustumEXT");
	glad_glMatrixLoadIdentityEXT = (PFNGLMATRIXLOADIDENTITYEXTPROC)load("glMatrixLoadIdentityEXT");
	glad_glMatrixLoadTransposefEXT = (PFNGLMATRIXLOADTRANSPOSEFEXTPROC)load("glMatrixLoadTransposefEXT");
	glad_glMatrixLoadTransposedEXT = (PFNGLMATRIXLOADTRANSPOSEDEXTPROC)load("glMatrixLoadTransposedEXT");
	glad_glMatrixLoadfEXT = (PFNGLMATRIXLOADFEXTPROC)load("glMatrixLoadfEXT");
	glad_glMatrixLoaddEXT = (PFNGLMATRIXLOADDEXTPROC)load("glMatrixLoaddEXT");
	glad_glMatrixMultTransposefEXT = (PFNGLMATRIXMULTTRANSPOSEFEXTPROC)load("glMatrixMultTransposefEXT");
	glad_glMatrixMultTransposedEXT = (PFNGLMATRIXMULTTRANSPOSEDEXTPROC)load("glMatrixMultTransposedEXT");
	glad_glMatrixMultfEXT = (PFNGLMATRIXMULTFEXTPROC)load("glMatrixMultfEXT");
	glad_glMatrixMultdEXT = (PFNGLMATRIXMULTDEXTPROC)load("glMatrixMultdEXT");
	glad_glMatrixOrthoEXT = (PFNGLMATRIXORTHOEXTPROC)load("glMatrixOrthoEXT");
	glad_glMatrixPopEXT = (PFNGLMATRIXPOPEXTPROC)load("glMatrixPopEXT");
	glad_glMatrixPushEXT = (PFNGLMATRIXPUSHEXTPROC)load("glMatrixPushEXT");
	glad_glMatrixRotatefEXT = (PFNGLMATRIXROTATEFEXTPROC)load("glMatrixRotatefEXT");
	glad_glMatrixRotatedEXT = (PFNGLMATRIXROTATEDEXTPROC)load("glMatrixRotatedEXT");
	glad_glMatrixScalefEXT = (PFNGLMATRIXSCALEFEXTPROC)load("glMatrixScalefEXT");
	glad_glMatrixScaledEXT = (PFNGLMATRIXSCALEDEXTPROC)load("glMatrixScaledEXT");
	glad_glMatrixTranslatefEXT = (PFNGLMATRIXTRANSLATEFEXTPROC)load("glMatrixTranslatefEXT");
	glad_glMatrixTranslatedEXT = (PFNGLMATRIXTRANSLATEDEXTPROC)load("glMatrixTranslatedEXT");
}
static void load_GL_NV_pixel_data_range(GLADloadproc load) {
	if(!GLAD_GL_NV_pixel_data_range) return;
	glad_glPixelDataRangeNV = (PFNGLPIXELDATARANGENVPROC)load("glPixelDataRangeNV");
	glad_glFlushPixelDataRangeNV = (PFNGLFLUSHPIXELDATARANGENVPROC)load("glFlushPixelDataRangeNV");
}
static void load_GL_NV_point_sprite(GLADloadproc load) {
	if(!GLAD_GL_NV_point_sprite) return;
	glad_glPointParameteriNV = (PFNGLPOINTPARAMETERINVPROC)load("glPointParameteriNV");
	glad_glPointParameterivNV = (PFNGLPOINTPARAMETERIVNVPROC)load("glPointParameterivNV");
}
static void load_GL_NV_present_video(GLADloadproc load) {
	if(!GLAD_GL_NV_present_video) return;
	glad_glPresentFrameKeyedNV = (PFNGLPRESENTFRAMEKEYEDNVPROC)load("glPresentFrameKeyedNV");
	glad_glPresentFrameDualFillNV = (PFNGLPRESENTFRAMEDUALFILLNVPROC)load("glPresentFrameDualFillNV");
	glad_glGetVideoivNV = (PFNGLGETVIDEOIVNVPROC)load("glGetVideoivNV");
	glad_glGetVideouivNV = (PFNGLGETVIDEOUIVNVPROC)load("glGetVideouivNV");
	glad_glGetVideoi64vNV = (PFNGLGETVIDEOI64VNVPROC)load("glGetVideoi64vNV");
	glad_glGetVideoui64vNV = (PFNGLGETVIDEOUI64VNVPROC)load("glGetVideoui64vNV");
}
static void load_GL_NV_primitive_restart(GLADloadproc load) {
	if(!GLAD_GL_NV_primitive_restart) return;
	glad_glPrimitiveRestartNV = (PFNGLPRIMITIVERESTARTNVPROC)load("glPrimitiveRestartNV");
	glad_glPrimitiveRestartIndexNV = (PFNGLPRIMITIVERESTARTINDEXNVPROC)load("glPrimitiveRestartIndexNV");
}
static void load_GL_NV_query_resource(GLADloadproc load) {
	if(!GLAD_GL_NV_query_resource) return;
	glad_glQueryResourceNV = (PFNGLQUERYRESOURCENVPROC)load("glQueryResourceNV");
}
static void load_GL_NV_query_resource_tag(GLADloadproc load) {
	if(!GLAD_GL_NV_query_resource_tag) return;
	glad_glGenQueryResourceTagNV = (PFNGLGENQUERYRESOURCETAGNVPROC)load("glGenQueryResourceTagNV");
	glad_glDeleteQueryResourceTagNV = (PFNGLDELETEQUERYRESOURCETAGNVPROC)load("glDeleteQueryResourceTagNV");
	glad_glQueryResourceTagNV = (PFNGLQUERYRESOURCETAGNVPROC)load("glQueryResourceTagNV");
}
static void load_GL_NV_register_combiners(GLADloadproc load) {
	if(!GLAD_GL_NV_register_combiners) return;
	glad_glCombinerParameterfvNV = (PFNGLCOMBINERPARAMETERFVNVPROC)load("glCombinerParameterfvNV");
	glad_glCombinerParameterfNV = (PFNGLCOMBINERPARAMETERFNVPROC)load("glCombinerParameterfNV");
	glad_glCombinerParameterivNV = (PFNGLCOMBINERPARAMETERIVNVPROC)load("glCombinerParameterivNV");
	glad_glCombinerParameteriNV = (PFNGLCOMBINERPARAMETERINVPROC)load("glCombinerParameteriNV");
	glad_glCombinerInputNV = (PFNGLCOMBINERINPUTNVPROC)load("glCombinerInputNV");
	glad_glCombinerOutputNV = (PFNGLCOMBINEROUTPUTNVPROC)load("glCombinerOutputNV");
	glad_glFinalCombinerInputNV = (PFNGLFINALCOMBINERINPUTNVPROC)load("glFinalCombinerInputNV");
	glad_glGetCombinerInputParameterfvNV = (PFNGLGETCOMBINERINPUTPARAMETERFVNVPROC)load("glGetCombinerInputParameterfvNV");
	glad_glGetCombinerInputParameterivNV = (PFNGLGETCOMBINERINPUTPARAMETERIVNVPROC)load("glGetCombinerInputParameterivNV");
	glad_glGetCombinerOutputParameterfvNV = (PFNGLGETCOMBINEROUTPUTPARAMETERFVNVPROC)load("glGetCombinerOutputParameterfvNV");
	glad_glGetCombinerOutputParameterivNV = (PFNGLGETCOMBINEROUTPUTPARAMETERIVNVPROC)load("glGetCombinerOutputParameterivNV");
	glad_glGetFinalCombinerInputParameterfvNV = (PFNGLGETFINALCOMBINERINPUTPARAMETERFVNVPROC)load("glGetFinalCombinerInputParameterfvNV");
	glad_glGetFinalCombinerInputParameterivNV = (PFNGLGETFINALCOMBINERINPUTPARAMETERIVNVPROC)load("glGetFinalCombinerInputParameterivNV");
}
static void load_GL_NV_register_combiners2(GLADloadproc load) {
	if(!GLAD_GL_NV_register_combiners2) return;
	glad_glCombinerStageParameterfvNV = (PFNGLCOMBINERSTAGEPARAMETERFVNVPROC)load("glCombinerStageParameterfvNV");
	glad_glGetCombinerStageParameterfvNV = (PFNGLGETCOMBINERSTAGEPARAMETERFVNVPROC)load("glGetCombinerStageParameterfvNV");
}
static void load_GL_NV_sample_locations(GLADloadproc load) {
	if(!GLAD_GL_NV_sample_locations) return;
	glad_glFramebufferSampleLocationsfvNV = (PFNGLFRAMEBUFFERSAMPLELOCATIONSFVNVPROC)load("glFramebufferSampleLocationsfvNV");
	glad_glNamedFramebufferSampleLocationsfvNV = (PFNGLNAMEDFRAMEBUFFERSAMPLELOCATIONSFVNVPROC)load("glNamedFramebufferSampleLocationsfvNV");
	glad_glResolveDepthValuesNV = (PFNGLRESOLVEDEPTHVALUESNVPROC)load("glResolveDepthValuesNV");
}
static void load_GL_NV_scissor_exclusive(GLADloadproc load) {
	if(!GLAD_GL_NV_scissor_exclusive) return;
	glad_glScissorExclusiveNV = (PFNGLSCISSOREXCLUSIVENVPROC)load("glScissorExclusiveNV");
	glad_glScissorExclusiveArrayvNV = (PFNGLSCISSOREXCLUSIVEARRAYVNVPROC)load("glScissorExclusiveArrayvNV");
}
static void load_GL_NV_shader_buffer_load(GLADloadproc load) {
	if(!GLAD_GL_NV_shader_buffer_load) return;
	glad_glMakeBufferResidentNV = (PFNGLMAKEBUFFERRESIDENTNVPROC)load("glMakeBufferResidentNV");
	glad_glMakeBufferNonResidentNV = (PFNGLMAKEBUFFERNONRESIDENTNVPROC)load("glMakeBufferNonResidentNV");
	glad_glIsBufferResidentNV = (PFNGLISBUFFERRESIDENTNVPROC)load("glIsBufferResidentNV");
	glad_glMakeNamedBufferResidentNV = (PFNGLMAKENAMEDBUFFERRESIDENTNVPROC)load("glMakeNamedBufferResidentNV");
	glad_glMakeNamedBufferNonResidentNV = (PFNGLMAKENAMEDBUFFERNONRESIDENTNVPROC)load("glMakeNamedBufferNonResidentNV");
	glad_glIsNamedBufferResidentNV = (PFNGLISNAMEDBUFFERRESIDENTNVPROC)load("glIsNamedBufferResidentNV");
	glad_glGetBufferParameterui64vNV = (PFNGLGETBUFFERPARAMETERUI64VNVPROC)load("glGetBufferParameterui64vNV");
	glad_glGetNamedBufferParameterui64vNV = (PFNGLGETNAMEDBUFFERPARAMETERUI64VNVPROC)load("glGetNamedBufferParameterui64vNV");
	glad_glGetIntegerui64vNV = (PFNGLGETINTEGERUI64VNVPROC)load("glGetIntegerui64vNV");
	glad_glUniformui64NV = (PFNGLUNIFORMUI64NVPROC)load("glUniformui64NV");
	glad_glUniformui64vNV = (PFNGLUNIFORMUI64VNVPROC)load("glUniformui64vNV");
	glad_glGetUniformui64vNV = (PFNGLGETUNIFORMUI64VNVPROC)load("glGetUniformui64vNV");
	glad_glProgramUniformui64NV = (PFNGLPROGRAMUNIFORMUI64NVPROC)load("glProgramUniformui64NV");
	glad_glProgramUniformui64vNV = (PFNGLPROGRAMUNIFORMUI64VNVPROC)load("glProgramUniformui64vNV");
}
static void load_GL_NV_shading_rate_image(GLADloadproc load) {
	if(!GLAD_GL_NV_shading_rate_image) return;
	glad_glBindShadingRateImageNV = (PFNGLBINDSHADINGRATEIMAGENVPROC)load("glBindShadingRateImageNV");
	glad_glGetShadingRateImagePaletteNV = (PFNGLGETSHADINGRATEIMAGEPALETTENVPROC)load("glGetShadingRateImagePaletteNV");
	glad_glGetShadingRateSampleLocationivNV = (PFNGLGETSHADINGRATESAMPLELOCATIONIVNVPROC)load("glGetShadingRateSampleLocationivNV");
	glad_glShadingRateImageBarrierNV = (PFNGLSHADINGRATEIMAGEBARRIERNVPROC)load("glShadingRateImageBarrierNV");
	glad_glShadingRateImagePaletteNV = (PFNGLSHADINGRATEIMAGEPALETTENVPROC)load("glShadingRateImagePaletteNV");
	glad_glShadingRateSampleOrderNV = (PFNGLSHADINGRATESAMPLEORDERNVPROC)load("glShadingRateSampleOrderNV");
	glad_glShadingRateSampleOrderCustomNV = (PFNGLSHADINGRATESAMPLEORDERCUSTOMNVPROC)load("glShadingRateSampleOrderCustomNV");
}
static void load_GL_NV_texture_barrier(GLADloadproc load) {
	if(!GLAD_GL_NV_texture_barrier) return;
	glad_glTextureBarrierNV = (PFNGLTEXTUREBARRIERNVPROC)load("glTextureBarrierNV");
}
static void load_GL_NV_texture_multisample(GLADloadproc load) {
	if(!GLAD_GL_NV_texture_multisample) return;
	glad_glTexImage2DMultisampleCoverageNV = (PFNGLTEXIMAGE2DMULTISAMPLECOVERAGENVPROC)load("glTexImage2DMultisampleCoverageNV");
	glad_glTexImage3DMultisampleCoverageNV = (PFNGLTEXIMAGE3DMULTISAMPLECOVERAGENVPROC)load("glTexImage3DMultisampleCoverageNV");
	glad_glTextureImage2DMultisampleNV = (PFNGLTEXTUREIMAGE2DMULTISAMPLENVPROC)load("glTextureImage2DMultisampleNV");
	glad_glTextureImage3DMultisampleNV = (PFNGLTEXTUREIMAGE3DMULTISAMPLENVPROC)load("glTextureImage3DMultisampleNV");
	glad_glTextureImage2DMultisampleCoverageNV = (PFNGLTEXTUREIMAGE2DMULTISAMPLECOVERAGENVPROC)load("glTextureImage2DMultisampleCoverageNV");
	glad_glTextureImage3DMultisampleCoverageNV = (PFNGLTEXTUREIMAGE3DMULTISAMPLECOVERAGENVPROC)load("glTextureImage3DMultisampleCoverageNV");
}
static void load_GL_NV_timeline_semaphore(GLADloadproc load) {
	if(!GLAD_GL_NV_timeline_semaphore) return;
	glad_glCreateSemaphoresNV = (PFNGLCREATESEMAPHORESNVPROC)load("glCreateSemaphoresNV");
	glad_glSemaphoreParameterivNV = (PFNGLSEMAPHOREPARAMETERIVNVPROC)load("glSemaphoreParameterivNV");
	glad_glGetSemaphoreParameterivNV = (PFNGLGETSEMAPHOREPARAMETERIVNVPROC)load("glGetSemaphoreParameterivNV");
}
static void load_GL_NV_transform_feedback(GLADloadproc load) {
	if(!GLAD_GL_NV_transform_feedback) return;
	glad_glBeginTransformFeedbackNV = (PFNGLBEGINTRANSFORMFEEDBACKNVPROC)load("glBeginTransformFeedbackNV");
	glad_glEndTransformFeedbackNV = (PFNGLENDTRANSFORMFEEDBACKNVPROC)load("glEndTransformFeedbackNV");
	glad_glTransformFeedbackAttribsNV = (PFNGLTRANSFORMFEEDBACKATTRIBSNVPROC)load("glTransformFeedbackAttribsNV");
	glad_glBindBufferRangeNV = (PFNGLBINDBUFFERRANGENVPROC)load("glBindBufferRangeNV");
	glad_glBindBufferOffsetNV = (PFNGLBINDBUFFEROFFSETNVPROC)load("glBindBufferOffsetNV");
	glad_glBindBufferBaseNV = (PFNGLBINDBUFFERBASENVPROC)load("glBindBufferBaseNV");
	glad_glTransformFeedbackVaryingsNV = (PFNGLTRANSFORMFEEDBACKVARYINGSNVPROC)load("glTransformFeedbackVaryingsNV");
	glad_glActiveVaryingNV = (PFNGLACTIVEVARYINGNVPROC)load("glActiveVaryingNV");
	glad_glGetVaryingLocationNV = (PFNGLGETVARYINGLOCATIONNVPROC)load("glGetVaryingLocationNV");
	glad_glGetActiveVaryingNV = (PFNGLGETACTIVEVARYINGNVPROC)load("glGetActiveVaryingNV");
	glad_glGetTransformFeedbackVaryingNV = (PFNGLGETTRANSFORMFEEDBACKVARYINGNVPROC)load("glGetTransformFeedbackVaryingNV");
	glad_glTransformFeedbackStreamAttribsNV = (PFNGLTRANSFORMFEEDBACKSTREAMATTRIBSNVPROC)load("glTransformFeedbackStreamAttribsNV");
}
static void load_GL_NV_transform_feedback2(GLADloadproc load) {
	if(!GLAD_GL_NV_transform_feedback2) return;
	glad_glBindTransformFeedbackNV = (PFNGLBINDTRANSFORMFEEDBACKNVPROC)load("glBindTransformFeedbackNV");
	glad_glDeleteTransformFeedbacksNV = (PFNGLDELETETRANSFORMFEEDBACKSNVPROC)load("glDeleteTransformFeedbacksNV");
	glad_glGenTransformFeedbacksNV = (PFNGLGENTRANSFORMFEEDBACKSNVPROC)load("glGenTransformFeedbacksNV");
	glad_glIsTransformFeedbackNV = (PFNGLISTRANSFORMFEEDBACKNVPROC)load("glIsTransformFeedbackNV");
	glad_glPauseTransformFeedbackNV = (PFNGLPAUSETRANSFORMFEEDBACKNVPROC)load("glPauseTransformFeedbackNV");
	glad_glResumeTransformFeedbackNV = (PFNGLRESUMETRANSFORMFEEDBACKNVPROC)load("glResumeTransformFeedbackNV");
	glad_glDrawTransformFeedbackNV = (PFNGLDRAWTRANSFORMFEEDBACKNVPROC)load("glDrawTransformFeedbackNV");
}
static void load_GL_NV_vdpau_interop(GLADloadproc load) {
	if(!GLAD_GL_NV_vdpau_interop) return;
	glad_glVDPAUInitNV = (PFNGLVDPAUINITNVPROC)load("glVDPAUInitNV");
	glad_glVDPAUFiniNV = (PFNGLVDPAUFININVPROC)load("glVDPAUFiniNV");
	glad_glVDPAURegisterVideoSurfaceNV = (PFNGLVDPAUREGISTERVIDEOSURFACENVPROC)load("glVDPAURegisterVideoSurfaceNV");
	glad_glVDPAURegisterOutputSurfaceNV = (PFNGLVDPAUREGISTEROUTPUTSURFACENVPROC)load("glVDPAURegisterOutputSurfaceNV");
	glad_glVDPAUIsSurfaceNV = (PFNGLVDPAUISSURFACENVPROC)load("glVDPAUIsSurfaceNV");
	glad_glVDPAUUnregisterSurfaceNV = (PFNGLVDPAUUNREGISTERSURFACENVPROC)load("glVDPAUUnregisterSurfaceNV");
	glad_glVDPAUGetSurfaceivNV = (PFNGLVDPAUGETSURFACEIVNVPROC)load("glVDPAUGetSurfaceivNV");
	glad_glVDPAUSurfaceAccessNV = (PFNGLVDPAUSURFACEACCESSNVPROC)load("glVDPAUSurfaceAccessNV");
	glad_glVDPAUMapSurfacesNV = (PFNGLVDPAUMAPSURFACESNVPROC)load("glVDPAUMapSurfacesNV");
	glad_glVDPAUUnmapSurfacesNV = (PFNGLVDPAUUNMAPSURFACESNVPROC)load("glVDPAUUnmapSurfacesNV");
}
static void load_GL_NV_vdpau_interop2(GLADloadproc load) {
	if(!GLAD_GL_NV_vdpau_interop2) return;
	glad_glVDPAURegisterVideoSurfaceWithPictureStructureNV = (PFNGLVDPAUREGISTERVIDEOSURFACEWITHPICTURESTRUCTURENVPROC)load("glVDPAURegisterVideoSurfaceWithPictureStructureNV");
}
static void load_GL_NV_vertex_array_range(GLADloadproc load) {
	if(!GLAD_GL_NV_vertex_array_range) return;
	glad_glFlushVertexArrayRangeNV = (PFNGLFLUSHVERTEXARRAYRANGENVPROC)load("glFlushVertexArrayRangeNV");
	glad_glVertexArrayRangeNV = (PFNGLVERTEXARRAYRANGENVPROC)load("glVertexArrayRangeNV");
}
static void load_GL_NV_vertex_attrib_integer_64bit(GLADloadproc load) {
	if(!GLAD_GL_NV_vertex_attrib_integer_64bit) return;
	glad_glVertexAttribL1i64NV = (PFNGLVERTEXATTRIBL1I64NVPROC)load("glVertexAttribL1i64NV");
	glad_glVertexAttribL2i64NV = (PFNGLVERTEXATTRIBL2I64NVPROC)load("glVertexAttribL2i64NV");
	glad_glVertexAttribL3i64NV = (PFNGLVERTEXATTRIBL3I64NVPROC)load("glVertexAttribL3i64NV");
	glad_glVertexAttribL4i64NV = (PFNGLVERTEXATTRIBL4I64NVPROC)load("glVertexAttribL4i64NV");
	glad_glVertexAttribL1i64vNV = (PFNGLVERTEXATTRIBL1I64VNVPROC)load("glVertexAttribL1i64vNV");
	glad_glVertexAttribL2i64vNV = (PFNGLVERTEXATTRIBL2I64VNVPROC)load("glVertexAttribL2i64vNV");
	glad_glVertexAttribL3i64vNV = (PFNGLVERTEXATTRIBL3I64VNVPROC)load("glVertexAttribL3i64vNV");
	glad_glVertexAttribL4i64vNV = (PFNGLVERTEXATTRIBL4I64VNVPROC)load("glVertexAttribL4i64vNV");
	glad_glVertexAttribL1ui64NV = (PFNGLVERTEXATTRIBL1UI64NVPROC)load("glVertexAttribL1ui64NV");
	glad_glVertexAttribL2ui64NV = (PFNGLVERTEXATTRIBL2UI64NVPROC)load("glVertexAttribL2ui64NV");
	glad_glVertexAttribL3ui64NV = (PFNGLVERTEXATTRIBL3UI64NVPROC)load("glVertexAttribL3ui64NV");
	glad_glVertexAttribL4ui64NV = (PFNGLVERTEXATTRIBL4UI64NVPROC)load("glVertexAttribL4ui64NV");
	glad_glVertexAttribL1ui64vNV = (PFNGLVERTEXATTRIBL1UI64VNVPROC)load("glVertexAttribL1ui64vNV");
	glad_glVertexAttribL2ui64vNV = (PFNGLVERTEXATTRIBL2UI64VNVPROC)load("glVertexAttribL2ui64vNV");
	glad_glVertexAttribL3ui64vNV = (PFNGLVERTEXATTRIBL3UI64VNVPROC)load("glVertexAttribL3ui64vNV");
	glad_glVertexAttribL4ui64vNV = (PFNGLVERTEXATTRIBL4UI64VNVPROC)load("glVertexAttribL4ui64vNV");
	glad_glGetVertexAttribLi64vNV = (PFNGLGETVERTEXATTRIBLI64VNVPROC)load("glGetVertexAttribLi64vNV");
	glad_glGetVertexAttribLui64vNV = (PFNGLGETVERTEXATTRIBLUI64VNVPROC)load("glGetVertexAttribLui64vNV");
	glad_glVertexAttribLFormatNV = (PFNGLVERTEXATTRIBLFORMATNVPROC)load("glVertexAttribLFormatNV");
}
static void load_GL_NV_vertex_buffer_unified_memory(GLADloadproc load) {
	if(!GLAD_GL_NV_vertex_buffer_unified_memory) return;
	glad_glBufferAddressRangeNV = (PFNGLBUFFERADDRESSRANGENVPROC)load("glBufferAddressRangeNV");
	glad_glVertexFormatNV = (PFNGLVERTEXFORMATNVPROC)load("glVertexFormatNV");
	glad_glNormalFormatNV = (PFNGLNORMALFORMATNVPROC)load("glNormalFormatNV");
	glad_glColorFormatNV = (PFNGLCOLORFORMATNVPROC)load("glColorFormatNV");
	glad_glIndexFormatNV = (PFNGLINDEXFORMATNVPROC)load("glIndexFormatNV");
	glad_glTexCoordFormatNV = (PFNGLTEXCOORDFORMATNVPROC)load("glTexCoordFormatNV");
	glad_glEdgeFlagFormatNV = (PFNGLEDGEFLAGFORMATNVPROC)load("glEdgeFlagFormatNV");
	glad_glSecondaryColorFormatNV = (PFNGLSECONDARYCOLORFORMATNVPROC)load("glSecondaryColorFormatNV");
	glad_glFogCoordFormatNV = (PFNGLFOGCOORDFORMATNVPROC)load("glFogCoordFormatNV");
	glad_glVertexAttribFormatNV = (PFNGLVERTEXATTRIBFORMATNVPROC)load("glVertexAttribFormatNV");
	glad_glVertexAttribIFormatNV = (PFNGLVERTEXATTRIBIFORMATNVPROC)load("glVertexAttribIFormatNV");
	glad_glGetIntegerui64i_vNV = (PFNGLGETINTEGERUI64I_VNVPROC)load("glGetIntegerui64i_vNV");
}
static void load_GL_NV_vertex_program(GLADloadproc load) {
	if(!GLAD_GL_NV_vertex_program) return;
	glad_glAreProgramsResidentNV = (PFNGLAREPROGRAMSRESIDENTNVPROC)load("glAreProgramsResidentNV");
	glad_glBindProgramNV = (PFNGLBINDPROGRAMNVPROC)load("glBindProgramNV");
	glad_glDeleteProgramsNV = (PFNGLDELETEPROGRAMSNVPROC)load("glDeleteProgramsNV");
	glad_glExecuteProgramNV = (PFNGLEXECUTEPROGRAMNVPROC)load("glExecuteProgramNV");
	glad_glGenProgramsNV = (PFNGLGENPROGRAMSNVPROC)load("glGenProgramsNV");
	glad_glGetProgramParameterdvNV = (PFNGLGETPROGRAMPARAMETERDVNVPROC)load("glGetProgramParameterdvNV");
	glad_glGetProgramParameterfvNV = (PFNGLGETPROGRAMPARAMETERFVNVPROC)load("glGetProgramParameterfvNV");
	glad_glGetProgramivNV = (PFNGLGETPROGRAMIVNVPROC)load("glGetProgramivNV");
	glad_glGetProgramStringNV = (PFNGLGETPROGRAMSTRINGNVPROC)load("glGetProgramStringNV");
	glad_glGetTrackMatrixivNV = (PFNGLGETTRACKMATRIXIVNVPROC)load("glGetTrackMatrixivNV");
	glad_glGetVertexAttribdvNV = (PFNGLGETVERTEXATTRIBDVNVPROC)load("glGetVertexAttribdvNV");
	glad_glGetVertexAttribfvNV = (PFNGLGETVERTEXATTRIBFVNVPROC)load("glGetVertexAttribfvNV");
	glad_glGetVertexAttribivNV = (PFNGLGETVERTEXATTRIBIVNVPROC)load("glGetVertexAttribivNV");
	glad_glGetVertexAttribPointervNV = (PFNGLGETVERTEXATTRIBPOINTERVNVPROC)load("glGetVertexAttribPointervNV");
	glad_glIsProgramNV = (PFNGLISPROGRAMNVPROC)load("glIsProgramNV");
	glad_glLoadProgramNV = (PFNGLLOADPROGRAMNVPROC)load("glLoadProgramNV");
	glad_glProgramParameter4dNV = (PFNGLPROGRAMPARAMETER4DNVPROC)load("glProgramParameter4dNV");
	glad_glProgramParameter4dvNV = (PFNGLPROGRAMPARAMETER4DVNVPROC)load("glProgramParameter4dvNV");
	glad_glProgramParameter4fNV = (PFNGLPROGRAMPARAMETER4FNVPROC)load("glProgramParameter4fNV");
	glad_glProgramParameter4fvNV = (PFNGLPROGRAMPARAMETER4FVNVPROC)load("glProgramParameter4fvNV");
	glad_glProgramParameters4dvNV = (PFNGLPROGRAMPARAMETERS4DVNVPROC)load("glProgramParameters4dvNV");
	glad_glProgramParameters4fvNV = (PFNGLPROGRAMPARAMETERS4FVNVPROC)load("glProgramParameters4fvNV");
	glad_glRequestResidentProgramsNV = (PFNGLREQUESTRESIDENTPROGRAMSNVPROC)load("glRequestResidentProgramsNV");
	glad_glTrackMatrixNV = (PFNGLTRACKMATRIXNVPROC)load("glTrackMatrixNV");
	glad_glVertexAttribPointerNV = (PFNGLVERTEXATTRIBPOINTERNVPROC)load("glVertexAttribPointerNV");
	glad_glVertexAttrib1dNV = (PFNGLVERTEXATTRIB1DNVPROC)load("glVertexAttrib1dNV");
	glad_glVertexAttrib1dvNV = (PFNGLVERTEXATTRIB1DVNVPROC)load("glVertexAttrib1dvNV");
	glad_glVertexAttrib1fNV = (PFNGLVERTEXATTRIB1FNVPROC)load("glVertexAttrib1fNV");
	glad_glVertexAttrib1fvNV = (PFNGLVERTEXATTRIB1FVNVPROC)load("glVertexAttrib1fvNV");
	glad_glVertexAttrib1sNV = (PFNGLVERTEXATTRIB1SNVPROC)load("glVertexAttrib1sNV");
	glad_glVertexAttrib1svNV = (PFNGLVERTEXATTRIB1SVNVPROC)load("glVertexAttrib1svNV");
	glad_glVertexAttrib2dNV = (PFNGLVERTEXATTRIB2DNVPROC)load("glVertexAttrib2dNV");
	glad_glVertexAttrib2dvNV = (PFNGLVERTEXATTRIB2DVNVPROC)load("glVertexAttrib2dvNV");
	glad_glVertexAttrib2fNV = (PFNGLVERTEXATTRIB2FNVPROC)load("glVertexAttrib2fNV");
	glad_glVertexAttrib2fvNV = (PFNGLVERTEXATTRIB2FVNVPROC)load("glVertexAttrib2fvNV");
	glad_glVertexAttrib2sNV = (PFNGLVERTEXATTRIB2SNVPROC)load("glVertexAttrib2sNV");
	glad_glVertexAttrib2svNV = (PFNGLVERTEXATTRIB2SVNVPROC)load("glVertexAttrib2svNV");
	glad_glVertexAttrib3dNV = (PFNGLVERTEXATTRIB3DNVPROC)load("glVertexAttrib3dNV");
	glad_glVertexAttrib3dvNV = (PFNGLVERTEXATTRIB3DVNVPROC)load("glVertexAttrib3dvNV");
	glad_glVertexAttrib3fNV = (PFNGLVERTEXATTRIB3FNVPROC)load("glVertexAttrib3fNV");
	glad_glVertexAttrib3fvNV = (PFNGLVERTEXATTRIB3FVNVPROC)load("glVertexAttrib3fvNV");
	glad_glVertexAttrib3sNV = (PFNGLVERTEXATTRIB3SNVPROC)load("glVertexAttrib3sNV");
	glad_glVertexAttrib3svNV = (PFNGLVERTEXATTRIB3SVNVPROC)load("glVertexAttrib3svNV");
	glad_glVertexAttrib4dNV = (PFNGLVERTEXATTRIB4DNVPROC)load("glVertexAttrib4dNV");
	glad_glVertexAttrib4dvNV = (PFNGLVERTEXATTRIB4DVNVPROC)load("glVertexAttrib4dvNV");
	glad_glVertexAttrib4fNV = (PFNGLVERTEXATTRIB4FNVPROC)load("glVertexAttrib4fNV");
	glad_glVertexAttrib4fvNV = (PFNGLVERTEXATTRIB4FVNVPROC)load("glVertexAttrib4fvNV");
	glad_glVertexAttrib4sNV = (PFNGLVERTEXATTRIB4SNVPROC)load("glVertexAttrib4sNV");
	glad_glVertexAttrib4svNV = (PFNGLVERTEXATTRIB4SVNVPROC)load("glVertexAttrib4svNV");
	glad_glVertexAttrib4ubNV = (PFNGLVERTEXATTRIB4UBNVPROC)load("glVertexAttrib4ubNV");
	glad_glVertexAttrib4ubvNV = (PFNGLVERTEXATTRIB4UBVNVPROC)load("glVertexAttrib4ubvNV");
	glad_glVertexAttribs1dvNV = (PFNGLVERTEXATTRIBS1DVNVPROC)load("glVertexAttribs1dvNV");
	glad_glVertexAttribs1fvNV = (PFNGLVERTEXATTRIBS1FVNVPROC)load("glVertexAttribs1fvNV");
	glad_glVertexAttribs1svNV = (PFNGLVERTEXATTRIBS1SVNVPROC)load("glVertexAttribs1svNV");
	glad_glVertexAttribs2dvNV = (PFNGLVERTEXATTRIBS2DVNVPROC)load("glVertexAttribs2dvNV");
	glad_glVertexAttribs2fvNV = (PFNGLVERTEXATTRIBS2FVNVPROC)load("glVertexAttribs2fvNV");
	glad_glVertexAttribs2svNV = (PFNGLVERTEXATTRIBS2SVNVPROC)load("glVertexAttribs2svNV");
	glad_glVertexAttribs3dvNV = (PFNGLVERTEXATTRIBS3DVNVPROC)load("glVertexAttribs3dvNV");
	glad_glVertexAttribs3fvNV = (PFNGLVERTEXATTRIBS3FVNVPROC)load("glVertexAttribs3fvNV");
	glad_glVertexAttribs3svNV = (PFNGLVERTEXATTRIBS3SVNVPROC)load("glVertexAttribs3svNV");
	glad_glVertexAttribs4dvNV = (PFNGLVERTEXATTRIBS4DVNVPROC)load("glVertexAttribs4dvNV");
	glad_glVertexAttribs4fvNV = (PFNGLVERTEXATTRIBS4FVNVPROC)load("glVertexAttribs4fvNV");
	glad_glVertexAttribs4svNV = (PFNGLVERTEXATTRIBS4SVNVPROC)load("glVertexAttribs4svNV");
	glad_glVertexAttribs4ubvNV = (PFNGLVERTEXATTRIBS4UBVNVPROC)load("glVertexAttribs4ubvNV");
}
static void load_GL_NV_vertex_program4(GLADloadproc load) {
	if(!GLAD_GL_NV_vertex_program4) return;
	glad_glVertexAttribI1iEXT = (PFNGLVERTEXATTRIBI1IEXTPROC)load("glVertexAttribI1iEXT");
	glad_glVertexAttribI2iEXT = (PFNGLVERTEXATTRIBI2IEXTPROC)load("glVertexAttribI2iEXT");
	glad_glVertexAttribI3iEXT = (PFNGLVERTEXATTRIBI3IEXTPROC)load("glVertexAttribI3iEXT");
	glad_glVertexAttribI4iEXT = (PFNGLVERTEXATTRIBI4IEXTPROC)load("glVertexAttribI4iEXT");
	glad_glVertexAttribI1uiEXT = (PFNGLVERTEXATTRIBI1UIEXTPROC)load("glVertexAttribI1uiEXT");
	glad_glVertexAttribI2uiEXT = (PFNGLVERTEXATTRIBI2UIEXTPROC)load("glVertexAttribI2uiEXT");
	glad_glVertexAttribI3uiEXT = (PFNGLVERTEXATTRIBI3UIEXTPROC)load("glVertexAttribI3uiEXT");
	glad_glVertexAttribI4uiEXT = (PFNGLVERTEXATTRIBI4UIEXTPROC)load("glVertexAttribI4uiEXT");
	glad_glVertexAttribI1ivEXT = (PFNGLVERTEXATTRIBI1IVEXTPROC)load("glVertexAttribI1ivEXT");
	glad_glVertexAttribI2ivEXT = (PFNGLVERTEXATTRIBI2IVEXTPROC)load("glVertexAttribI2ivEXT");
	glad_glVertexAttribI3ivEXT = (PFNGLVERTEXATTRIBI3IVEXTPROC)load("glVertexAttribI3ivEXT");
	glad_glVertexAttribI4ivEXT = (PFNGLVERTEXATTRIBI4IVEXTPROC)load("glVertexAttribI4ivEXT");
	glad_glVertexAttribI1uivEXT = (PFNGLVERTEXATTRIBI1UIVEXTPROC)load("glVertexAttribI1uivEXT");
	glad_glVertexAttribI2uivEXT = (PFNGLVERTEXATTRIBI2UIVEXTPROC)load("glVertexAttribI2uivEXT");
	glad_glVertexAttribI3uivEXT = (PFNGLVERTEXATTRIBI3UIVEXTPROC)load("glVertexAttribI3uivEXT");
	glad_glVertexAttribI4uivEXT = (PFNGLVERTEXATTRIBI4UIVEXTPROC)load("glVertexAttribI4uivEXT");
	glad_glVertexAttribI4bvEXT = (PFNGLVERTEXATTRIBI4BVEXTPROC)load("glVertexAttribI4bvEXT");
	glad_glVertexAttribI4svEXT = (PFNGLVERTEXATTRIBI4SVEXTPROC)load("glVertexAttribI4svEXT");
	glad_glVertexAttribI4ubvEXT = (PFNGLVERTEXATTRIBI4UBVEXTPROC)load("glVertexAttribI4ubvEXT");
	glad_glVertexAttribI4usvEXT = (PFNGLVERTEXATTRIBI4USVEXTPROC)load("glVertexAttribI4usvEXT");
	glad_glVertexAttribIPointerEXT = (PFNGLVERTEXATTRIBIPOINTEREXTPROC)load("glVertexAttribIPointerEXT");
	glad_glGetVertexAttribIivEXT = (PFNGLGETVERTEXATTRIBIIVEXTPROC)load("glGetVertexAttribIivEXT");
	glad_glGetVertexAttribIuivEXT = (PFNGLGETVERTEXATTRIBIUIVEXTPROC)load("glGetVertexAttribIuivEXT");
}
static void load_GL_NV_video_capture(GLADloadproc load) {
	if(!GLAD_GL_NV_video_capture) return;
	glad_glBeginVideoCaptureNV = (PFNGLBEGINVIDEOCAPTURENVPROC)load("glBeginVideoCaptureNV");
	glad_glBindVideoCaptureStreamBufferNV = (PFNGLBINDVIDEOCAPTURESTREAMBUFFERNVPROC)load("glBindVideoCaptureStreamBufferNV");
	glad_glBindVideoCaptureStreamTextureNV = (PFNGLBINDVIDEOCAPTURESTREAMTEXTURENVPROC)load("glBindVideoCaptureStreamTextureNV");
	glad_glEndVideoCaptureNV = (PFNGLENDVIDEOCAPTURENVPROC)load("glEndVideoCaptureNV");
	glad_glGetVideoCaptureivNV = (PFNGLGETVIDEOCAPTUREIVNVPROC)load("glGetVideoCaptureivNV");
	glad_glGetVideoCaptureStreamivNV = (PFNGLGETVIDEOCAPTURESTREAMIVNVPROC)load("glGetVideoCaptureStreamivNV");
	glad_glGetVideoCaptureStreamfvNV = (PFNGLGETVIDEOCAPTURESTREAMFVNVPROC)load("glGetVideoCaptureStreamfvNV");
	glad_glGetVideoCaptureStreamdvNV = (PFNGLGETVIDEOCAPTURESTREAMDVNVPROC)load("glGetVideoCaptureStreamdvNV");
	glad_glVideoCaptureNV = (PFNGLVIDEOCAPTURENVPROC)load("glVideoCaptureNV");
	glad_glVideoCaptureStreamParameterivNV = (PFNGLVIDEOCAPTURESTREAMPARAMETERIVNVPROC)load("glVideoCaptureStreamParameterivNV");
	glad_glVideoCaptureStreamParameterfvNV = (PFNGLVIDEOCAPTURESTREAMPARAMETERFVNVPROC)load("glVideoCaptureStreamParameterfvNV");
	glad_glVideoCaptureStreamParameterdvNV = (PFNGLVIDEOCAPTURESTREAMPARAMETERDVNVPROC)load("glVideoCaptureStreamParameterdvNV");
}
static void load_GL_NV_viewport_swizzle(GLADloadproc load) {
	if(!GLAD_GL_NV_viewport_swizzle) return;
	glad_glViewportSwizzleNV = (PFNGLVIEWPORTSWIZZLENVPROC)load("glViewportSwizzleNV");
}
static void load_GL_OES_byte_coordinates(GLADloadproc load) {
	if(!GLAD_GL_OES_byte_coordinates) return;
	glad_glMultiTexCoord1bOES = (PFNGLMULTITEXCOORD1BOESPROC)load("glMultiTexCoord1bOES");
	glad_glMultiTexCoord1bvOES = (PFNGLMULTITEXCOORD1BVOESPROC)load("glMultiTexCoord1bvOES");
	glad_glMultiTexCoord2bOES = (PFNGLMULTITEXCOORD2BOESPROC)load("glMultiTexCoord2bOES");
	glad_glMultiTexCoord2bvOES = (PFNGLMULTITEXCOORD2BVOESPROC)load("glMultiTexCoord2bvOES");
	glad_glMultiTexCoord3bOES = (PFNGLMULTITEXCOORD3BOESPROC)load("glMultiTexCoord3bOES");
	glad_glMultiTexCoord3bvOES = (PFNGLMULTITEXCOORD3BVOESPROC)load("glMultiTexCoord3bvOES");
	glad_glMultiTexCoord4bOES = (PFNGLMULTITEXCOORD4BOESPROC)load("glMultiTexCoord4bOES");
	glad_glMultiTexCoord4bvOES = (PFNGLMULTITEXCOORD4BVOESPROC)load("glMultiTexCoord4bvOES");
	glad_glTexCoord1bOES = (PFNGLTEXCOORD1BOESPROC)load("glTexCoord1bOES");
	glad_glTexCoord1bvOES = (PFNGLTEXCOORD1BVOESPROC)load("glTexCoord1bvOES");
	glad_glTexCoord2bOES = (PFNGLTEXCOORD2BOESPROC)load("glTexCoord2bOES");
	glad_glTexCoord2bvOES = (PFNGLTEXCOORD2BVOESPROC)load("glTexCoord2bvOES");
	glad_glTexCoord3bOES = (PFNGLTEXCOORD3BOESPROC)load("glTexCoord3bOES");
	glad_glTexCoord3bvOES = (PFNGLTEXCOORD3BVOESPROC)load("glTexCoord3bvOES");
	glad_glTexCoord4bOES = (PFNGLTEXCOORD4BOESPROC)load("glTexCoord4bOES");
	glad_glTexCoord4bvOES = (PFNGLTEXCOORD4BVOESPROC)load("glTexCoord4bvOES");
	glad_glVertex2bOES = (PFNGLVERTEX2BOESPROC)load("glVertex2bOES");
	glad_glVertex2bvOES = (PFNGLVERTEX2BVOESPROC)load("glVertex2bvOES");
	glad_glVertex3bOES = (PFNGLVERTEX3BOESPROC)load("glVertex3bOES");
	glad_glVertex3bvOES = (PFNGLVERTEX3BVOESPROC)load("glVertex3bvOES");
	glad_glVertex4bOES = (PFNGLVERTEX4BOESPROC)load("glVertex4bOES");
	glad_glVertex4bvOES = (PFNGLVERTEX4BVOESPROC)load("glVertex4bvOES");
}
static void load_GL_OES_fixed_point(GLADloadproc load) {
	if(!GLAD_GL_OES_fixed_point) return;
	glad_glAlphaFuncxOES = (PFNGLALPHAFUNCXOESPROC)load("glAlphaFuncxOES");
	glad_glClearColorxOES = (PFNGLCLEARCOLORXOESPROC)load("glClearColorxOES");
	glad_glClearDepthxOES = (PFNGLCLEARDEPTHXOESPROC)load("glClearDepthxOES");
	glad_glClipPlanexOES = (PFNGLCLIPPLANEXOESPROC)load("glClipPlanexOES");
	glad_glColor4xOES = (PFNGLCOLOR4XOESPROC)load("glColor4xOES");
	glad_glDepthRangexOES = (PFNGLDEPTHRANGEXOESPROC)load("glDepthRangexOES");
	glad_glFogxOES = (PFNGLFOGXOESPROC)load("glFogxOES");
	glad_glFogxvOES = (PFNGLFOGXVOESPROC)load("glFogxvOES");
	glad_glFrustumxOES = (PFNGLFRUSTUMXOESPROC)load("glFrustumxOES");
	glad_glGetClipPlanexOES = (PFNGLGETCLIPPLANEXOESPROC)load("glGetClipPlanexOES");
	glad_glGetFixedvOES = (PFNGLGETFIXEDVOESPROC)load("glGetFixedvOES");
	glad_glGetTexEnvxvOES = (PFNGLGETTEXENVXVOESPROC)load("glGetTexEnvxvOES");
	glad_glGetTexParameterxvOES = (PFNGLGETTEXPARAMETERXVOESPROC)load("glGetTexParameterxvOES");
	glad_glLightModelxOES = (PFNGLLIGHTMODELXOESPROC)load("glLightModelxOES");
	glad_glLightModelxvOES = (PFNGLLIGHTMODELXVOESPROC)load("glLightModelxvOES");
	glad_glLightxOES = (PFNGLLIGHTXOESPROC)load("glLightxOES");
	glad_glLightxvOES = (PFNGLLIGHTXVOESPROC)load("glLightxvOES");
	glad_glLineWidthxOES = (PFNGLLINEWIDTHXOESPROC)load("glLineWidthxOES");
	glad_glLoadMatrixxOES = (PFNGLLOADMATRIXXOESPROC)load("glLoadMatrixxOES");
	glad_glMaterialxOES = (PFNGLMATERIALXOESPROC)load("glMaterialxOES");
	glad_glMaterialxvOES = (PFNGLMATERIALXVOESPROC)load("glMaterialxvOES");
	glad_glMultMatrixxOES = (PFNGLMULTMATRIXXOESPROC)load("glMultMatrixxOES");
	glad_glMultiTexCoord4xOES = (PFNGLMULTITEXCOORD4XOESPROC)load("glMultiTexCoord4xOES");
	glad_glNormal3xOES = (PFNGLNORMAL3XOESPROC)load("glNormal3xOES");
	glad_glOrthoxOES = (PFNGLORTHOXOESPROC)load("glOrthoxOES");
	glad_glPointParameterxvOES = (PFNGLPOINTPARAMETERXVOESPROC)load("glPointParameterxvOES");
	glad_glPointSizexOES = (PFNGLPOINTSIZEXOESPROC)load("glPointSizexOES");
	glad_glPolygonOffsetxOES = (PFNGLPOLYGONOFFSETXOESPROC)load("glPolygonOffsetxOES");
	glad_glRotatexOES = (PFNGLROTATEXOESPROC)load("glRotatexOES");
	glad_glScalexOES = (PFNGLSCALEXOESPROC)load("glScalexOES");
	glad_glTexEnvxOES = (PFNGLTEXENVXOESPROC)load("glTexEnvxOES");
	glad_glTexEnvxvOES = (PFNGLTEXENVXVOESPROC)load("glTexEnvxvOES");
	glad_glTexParameterxOES = (PFNGLTEXPARAMETERXOESPROC)load("glTexParameterxOES");
	glad_glTexParameterxvOES = (PFNGLTEXPARAMETERXVOESPROC)load("glTexParameterxvOES");
	glad_glTranslatexOES = (PFNGLTRANSLATEXOESPROC)load("glTranslatexOES");
	glad_glGetLightxvOES = (PFNGLGETLIGHTXVOESPROC)load("glGetLightxvOES");
	glad_glGetMaterialxvOES = (PFNGLGETMATERIALXVOESPROC)load("glGetMaterialxvOES");
	glad_glPointParameterxOES = (PFNGLPOINTPARAMETERXOESPROC)load("glPointParameterxOES");
	glad_glSampleCoveragexOES = (PFNGLSAMPLECOVERAGEXOESPROC)load("glSampleCoveragexOES");
	glad_glAccumxOES = (PFNGLACCUMXOESPROC)load("glAccumxOES");
	glad_glBitmapxOES = (PFNGLBITMAPXOESPROC)load("glBitmapxOES");
	glad_glBlendColorxOES = (PFNGLBLENDCOLORXOESPROC)load("glBlendColorxOES");
	glad_glClearAccumxOES = (PFNGLCLEARACCUMXOESPROC)load("glClearAccumxOES");
	glad_glColor3xOES = (PFNGLCOLOR3XOESPROC)load("glColor3xOES");
	glad_glColor3xvOES = (PFNGLCOLOR3XVOESPROC)load("glColor3xvOES");
	glad_glColor4xvOES = (PFNGLCOLOR4XVOESPROC)load("glColor4xvOES");
	glad_glConvolutionParameterxOES = (PFNGLCONVOLUTIONPARAMETERXOESPROC)load("glConvolutionParameterxOES");
	glad_glConvolutionParameterxvOES = (PFNGLCONVOLUTIONPARAMETERXVOESPROC)load("glConvolutionParameterxvOES");
	glad_glEvalCoord1xOES = (PFNGLEVALCOORD1XOESPROC)load("glEvalCoord1xOES");
	glad_glEvalCoord1xvOES = (PFNGLEVALCOORD1XVOESPROC)load("glEvalCoord1xvOES");
	glad_glEvalCoord2xOES = (PFNGLEVALCOORD2XOESPROC)load("glEvalCoord2xOES");
	glad_glEvalCoord2xvOES = (PFNGLEVALCOORD2XVOESPROC)load("glEvalCoord2xvOES");
	glad_glFeedbackBufferxOES = (PFNGLFEEDBACKBUFFERXOESPROC)load("glFeedbackBufferxOES");
	glad_glGetConvolutionParameterxvOES = (PFNGLGETCONVOLUTIONPARAMETERXVOESPROC)load("glGetConvolutionParameterxvOES");
	glad_glGetHistogramParameterxvOES = (PFNGLGETHISTOGRAMPARAMETERXVOESPROC)load("glGetHistogramParameterxvOES");
	glad_glGetLightxOES = (PFNGLGETLIGHTXOESPROC)load("glGetLightxOES");
	glad_glGetMapxvOES = (PFNGLGETMAPXVOESPROC)load("glGetMapxvOES");
	glad_glGetMaterialxOES = (PFNGLGETMATERIALXOESPROC)load("glGetMaterialxOES");
	glad_glGetPixelMapxv = (PFNGLGETPIXELMAPXVPROC)load("glGetPixelMapxv");
	glad_glGetTexGenxvOES = (PFNGLGETTEXGENXVOESPROC)load("glGetTexGenxvOES");
	glad_glGetTexLevelParameterxvOES = (PFNGLGETTEXLEVELPARAMETERXVOESPROC)load("glGetTexLevelParameterxvOES");
	glad_glIndexxOES = (PFNGLINDEXXOESPROC)load("glIndexxOES");
	glad_glIndexxvOES = (PFNGLINDEXXVOESPROC)load("glIndexxvOES");
	glad_glLoadTransposeMatrixxOES = (PFNGLLOADTRANSPOSEMATRIXXOESPROC)load("glLoadTransposeMatrixxOES");
	glad_glMap1xOES = (PFNGLMAP1XOESPROC)load("glMap1xOES");
	glad_glMap2xOES = (PFNGLMAP2XOESPROC)load("glMap2xOES");
	glad_glMapGrid1xOES = (PFNGLMAPGRID1XOESPROC)load("glMapGrid1xOES");
	glad_glMapGrid2xOES = (PFNGLMAPGRID2XOESPROC)load("glMapGrid2xOES");
	glad_glMultTransposeMatrixxOES = (PFNGLMULTTRANSPOSEMATRIXXOESPROC)load("glMultTransposeMatrixxOES");
	glad_glMultiTexCoord1xOES = (PFNGLMULTITEXCOORD1XOESPROC)load("glMultiTexCoord1xOES");
	glad_glMultiTexCoord1xvOES = (PFNGLMULTITEXCOORD1XVOESPROC)load("glMultiTexCoord1xvOES");
	glad_glMultiTexCoord2xOES = (PFNGLMULTITEXCOORD2XOESPROC)load("glMultiTexCoord2xOES");
	glad_glMultiTexCoord2xvOES = (PFNGLMULTITEXCOORD2XVOESPROC)load("glMultiTexCoord2xvOES");
	glad_glMultiTexCoord3xOES = (PFNGLMULTITEXCOORD3XOESPROC)load("glMultiTexCoord3xOES");
	glad_glMultiTexCoord3xvOES = (PFNGLMULTITEXCOORD3XVOESPROC)load("glMultiTexCoord3xvOES");
	glad_glMultiTexCoord4xvOES = (PFNGLMULTITEXCOORD4XVOESPROC)load("glMultiTexCoord4xvOES");
	glad_glNormal3xvOES = (PFNGLNORMAL3XVOESPROC)load("glNormal3xvOES");
	glad_glPassThroughxOES = (PFNGLPASSTHROUGHXOESPROC)load("glPassThroughxOES");
	glad_glPixelMapx = (PFNGLPIXELMAPXPROC)load("glPixelMapx");
	glad_glPixelStorex = (PFNGLPIXELSTOREXPROC)load("glPixelStorex");
	glad_glPixelTransferxOES = (PFNGLPIXELTRANSFERXOESPROC)load("glPixelTransferxOES");
	glad_glPixelZoomxOES = (PFNGLPIXELZOOMXOESPROC)load("glPixelZoomxOES");
	glad_glPrioritizeTexturesxOES = (PFNGLPRIORITIZETEXTURESXOESPROC)load("glPrioritizeTexturesxOES");
	glad_glRasterPos2xOES = (PFNGLRASTERPOS2XOESPROC)load("glRasterPos2xOES");
	glad_glRasterPos2xvOES = (PFNGLRASTERPOS2XVOESPROC)load("glRasterPos2xvOES");
	glad_glRasterPos3xOES = (PFNGLRASTERPOS3XOESPROC)load("glRasterPos3xOES");
	glad_glRasterPos3xvOES = (PFNGLRASTERPOS3XVOESPROC)load("glRasterPos3xvOES");
	glad_glRasterPos4xOES = (PFNGLRASTERPOS4XOESPROC)load("glRasterPos4xOES");
	glad_glRasterPos4xvOES = (PFNGLRASTERPOS4XVOESPROC)load("glRasterPos4xvOES");
	glad_glRectxOES = (PFNGLRECTXOESPROC)load("glRectxOES");
	glad_glRectxvOES = (PFNGLRECTXVOESPROC)load("glRectxvOES");
	glad_glTexCoord1xOES = (PFNGLTEXCOORD1XOESPROC)load("glTexCoord1xOES");
	glad_glTexCoord1xvOES = (PFNGLTEXCOORD1XVOESPROC)load("glTexCoord1xvOES");
	glad_glTexCoord2xOES = (PFNGLTEXCOORD2XOESPROC)load("glTexCoord2xOES");
	glad_glTexCoord2xvOES = (PFNGLTEXCOORD2XVOESPROC)load("glTexCoord2xvOES");
	glad_glTexCoord3xOES = (PFNGLTEXCOORD3XOESPROC)load("glTexCoord3xOES");
	glad_glTexCoord3xvOES = (PFNGLTEXCOORD3XVOESPROC)load("glTexCoord3xvOES");
	glad_glTexCoord4xOES = (PFNGLTEXCOORD4XOESPROC)load("glTexCoord4xOES");
	glad_glTexCoord4xvOES = (PFNGLTEXCOORD4XVOESPROC)load("glTexCoord4xvOES");
	glad_glTexGenxOES = (PFNGLTEXGENXOESPROC)load("glTexGenxOES");
	glad_glTexGenxvOES = (PFNGLTEXGENXVOESPROC)load("glTexGenxvOES");
	glad_glVertex2xOES = (PFNGLVERTEX2XOESPROC)load("glVertex2xOES");
	glad_glVertex2xvOES = (PFNGLVERTEX2XVOESPROC)load("glVertex2xvOES");
	glad_glVertex3xOES = (PFNGLVERTEX3XOESPROC)load("glVertex3xOES");
	glad_glVertex3xvOES = (PFNGLVERTEX3XVOESPROC)load("glVertex3xvOES");
	glad_glVertex4xOES = (PFNGLVERTEX4XOESPROC)load("glVertex4xOES");
	glad_glVertex4xvOES = (PFNGLVERTEX4XVOESPROC)load("glVertex4xvOES");
}
static void load_GL_OES_query_matrix(GLADloadproc load) {
	if(!GLAD_GL_OES_query_matrix) return;
	glad_glQueryMatrixxOES = (PFNGLQUERYMATRIXXOESPROC)load("glQueryMatrixxOES");
}
static void load_GL_OES_single_precision(GLADloadproc load) {
	if(!GLAD_GL_OES_single_precision) return;
	glad_glClearDepthfOES = (PFNGLCLEARDEPTHFOESPROC)load("glClearDepthfOES");
	glad_glClipPlanefOES = (PFNGLCLIPPLANEFOESPROC)load("glClipPlanefOES");
	glad_glDepthRangefOES = (PFNGLDEPTHRANGEFOESPROC)load("glDepthRangefOES");
	glad_glFrustumfOES = (PFNGLFRUSTUMFOESPROC)load("glFrustumfOES");
	glad_glGetClipPlanefOES = (PFNGLGETCLIPPLANEFOESPROC)load("glGetClipPlanefOES");
	glad_glOrthofOES = (PFNGLORTHOFOESPROC)load("glOrthofOES");
}
static void load_GL_OVR_multiview(GLADloadproc load) {
	if(!GLAD_GL_OVR_multiview) return;
	glad_glFramebufferTextureMultiviewOVR = (PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC)load("glFramebufferTextureMultiviewOVR");
}
static void load_GL_PGI_misc_hints(GLADloadproc load) {
	if(!GLAD_GL_PGI_misc_hints) return;
	glad_glHintPGI = (PFNGLHINTPGIPROC)load("glHintPGI");
}
static void load_GL_SGIS_detail_texture(GLADloadproc load) {
	if(!GLAD_GL_SGIS_detail_texture) return;
	glad_glDetailTexFuncSGIS = (PFNGLDETAILTEXFUNCSGISPROC)load("glDetailTexFuncSGIS");
	glad_glGetDetailTexFuncSGIS = (PFNGLGETDETAILTEXFUNCSGISPROC)load("glGetDetailTexFuncSGIS");
}
static void load_GL_SGIS_fog_function(GLADloadproc load) {
	if(!GLAD_GL_SGIS_fog_function) return;
	glad_glFogFuncSGIS = (PFNGLFOGFUNCSGISPROC)load("glFogFuncSGIS");
	glad_glGetFogFuncSGIS = (PFNGLGETFOGFUNCSGISPROC)load("glGetFogFuncSGIS");
}
static void load_GL_SGIS_multisample(GLADloadproc load) {
	if(!GLAD_GL_SGIS_multisample) return;
	glad_glSampleMaskSGIS = (PFNGLSAMPLEMASKSGISPROC)load("glSampleMaskSGIS");
	glad_glSamplePatternSGIS = (PFNGLSAMPLEPATTERNSGISPROC)load("glSamplePatternSGIS");
}
static void load_GL_SGIS_pixel_texture(GLADloadproc load) {
	if(!GLAD_GL_SGIS_pixel_texture) return;
	glad_glPixelTexGenParameteriSGIS = (PFNGLPIXELTEXGENPARAMETERISGISPROC)load("glPixelTexGenParameteriSGIS");
	glad_glPixelTexGenParameterivSGIS = (PFNGLPIXELTEXGENPARAMETERIVSGISPROC)load("glPixelTexGenParameterivSGIS");
	glad_glPixelTexGenParameterfSGIS = (PFNGLPIXELTEXGENPARAMETERFSGISPROC)load("glPixelTexGenParameterfSGIS");
	glad_glPixelTexGenParameterfvSGIS = (PFNGLPIXELTEXGENPARAMETERFVSGISPROC)load("glPixelTexGenParameterfvSGIS");
	glad_glGetPixelTexGenParameterivSGIS = (PFNGLGETPIXELTEXGENPARAMETERIVSGISPROC)load("glGetPixelTexGenParameterivSGIS");
	glad_glGetPixelTexGenParameterfvSGIS = (PFNGLGETPIXELTEXGENPARAMETERFVSGISPROC)load("glGetPixelTexGenParameterfvSGIS");
}
static void load_GL_SGIS_point_parameters(GLADloadproc load) {
	if(!GLAD_GL_SGIS_point_parameters) return;
	glad_glPointParameterfSGIS = (PFNGLPOINTPARAMETERFSGISPROC)load("glPointParameterfSGIS");
	glad_glPointParameterfvSGIS = (PFNGLPOINTPARAMETERFVSGISPROC)load("glPointParameterfvSGIS");
}
static void load_GL_SGIS_sharpen_texture(GLADloadproc load) {
	if(!GLAD_GL_SGIS_sharpen_texture) return;
	glad_glSharpenTexFuncSGIS = (PFNGLSHARPENTEXFUNCSGISPROC)load("glSharpenTexFuncSGIS");
	glad_glGetSharpenTexFuncSGIS = (PFNGLGETSHARPENTEXFUNCSGISPROC)load("glGetSharpenTexFuncSGIS");
}
static void load_GL_SGIS_texture4D(GLADloadproc load) {
	if(!GLAD_GL_SGIS_texture4D) return;
	glad_glTexImage4DSGIS = (PFNGLTEXIMAGE4DSGISPROC)load("glTexImage4DSGIS");
	glad_glTexSubImage4DSGIS = (PFNGLTEXSUBIMAGE4DSGISPROC)load("glTexSubImage4DSGIS");
}
static void load_GL_SGIS_texture_color_mask(GLADloadproc load) {
	if(!GLAD_GL_SGIS_texture_color_mask) return;
	glad_glTextureColorMaskSGIS = (PFNGLTEXTURECOLORMASKSGISPROC)load("glTextureColorMaskSGIS");
}
static void load_GL_SGIS_texture_filter4(GLADloadproc load) {
	if(!GLAD_GL_SGIS_texture_filter4) return;
	glad_glGetTexFilterFuncSGIS = (PFNGLGETTEXFILTERFUNCSGISPROC)load("glGetTexFilterFuncSGIS");
	glad_glTexFilterFuncSGIS = (PFNGLTEXFILTERFUNCSGISPROC)load("glTexFilterFuncSGIS");
}
static void load_GL_SGIX_async(GLADloadproc load) {
	if(!GLAD_GL_SGIX_async) return;
	glad_glAsyncMarkerSGIX = (PFNGLASYNCMARKERSGIXPROC)load("glAsyncMarkerSGIX");
	glad_glFinishAsyncSGIX = (PFNGLFINISHASYNCSGIXPROC)load("glFinishAsyncSGIX");
	glad_glPollAsyncSGIX = (PFNGLPOLLASYNCSGIXPROC)load("glPollAsyncSGIX");
	glad_glGenAsyncMarkersSGIX = (PFNGLGENASYNCMARKERSSGIXPROC)load("glGenAsyncMarkersSGIX");
	glad_glDeleteAsyncMarkersSGIX = (PFNGLDELETEASYNCMARKERSSGIXPROC)load("glDeleteAsyncMarkersSGIX");
	glad_glIsAsyncMarkerSGIX = (PFNGLISASYNCMARKERSGIXPROC)load("glIsAsyncMarkerSGIX");
}
static void load_GL_SGIX_flush_raster(GLADloadproc load) {
	if(!GLAD_GL_SGIX_flush_raster) return;
	glad_glFlushRasterSGIX = (PFNGLFLUSHRASTERSGIXPROC)load("glFlushRasterSGIX");
}
static void load_GL_SGIX_fragment_lighting(GLADloadproc load) {
	if(!GLAD_GL_SGIX_fragment_lighting) return;
	glad_glFragmentColorMaterialSGIX = (PFNGLFRAGMENTCOLORMATERIALSGIXPROC)load("glFragmentColorMaterialSGIX");
	glad_glFragmentLightfSGIX = (PFNGLFRAGMENTLIGHTFSGIXPROC)load("glFragmentLightfSGIX");
	glad_glFragmentLightfvSGIX = (PFNGLFRAGMENTLIGHTFVSGIXPROC)load("glFragmentLightfvSGIX");
	glad_glFragmentLightiSGIX = (PFNGLFRAGMENTLIGHTISGIXPROC)load("glFragmentLightiSGIX");
	glad_glFragmentLightivSGIX = (PFNGLFRAGMENTLIGHTIVSGIXPROC)load("glFragmentLightivSGIX");
	glad_glFragmentLightModelfSGIX = (PFNGLFRAGMENTLIGHTMODELFSGIXPROC)load("glFragmentLightModelfSGIX");
	glad_glFragmentLightModelfvSGIX = (PFNGLFRAGMENTLIGHTMODELFVSGIXPROC)load("glFragmentLightModelfvSGIX");
	glad_glFragmentLightModeliSGIX = (PFNGLFRAGMENTLIGHTMODELISGIXPROC)load("glFragmentLightModeliSGIX");
	glad_glFragmentLightModelivSGIX = (PFNGLFRAGMENTLIGHTMODELIVSGIXPROC)load("glFragmentLightModelivSGIX");
	glad_glFragmentMaterialfSGIX = (PFNGLFRAGMENTMATERIALFSGIXPROC)load("glFragmentMaterialfSGIX");
	glad_glFragmentMaterialfvSGIX = (PFNGLFRAGMENTMATERIALFVSGIXPROC)load("glFragmentMaterialfvSGIX");
	glad_glFragmentMaterialiSGIX = (PFNGLFRAGMENTMATERIALISGIXPROC)load("glFragmentMaterialiSGIX");
	glad_glFragmentMaterialivSGIX = (PFNGLFRAGMENTMATERIALIVSGIXPROC)load("glFragmentMaterialivSGIX");
	glad_glGetFragmentLightfvSGIX = (PFNGLGETFRAGMENTLIGHTFVSGIXPROC)load("glGetFragmentLightfvSGIX");
	glad_glGetFragmentLightivSGIX = (PFNGLGETFRAGMENTLIGHTIVSGIXPROC)load("glGetFragmentLightivSGIX");
	glad_glGetFragmentMaterialfvSGIX = (PFNGLGETFRAGMENTMATERIALFVSGIXPROC)load("glGetFragmentMaterialfvSGIX");
	glad_glGetFragmentMaterialivSGIX = (PFNGLGETFRAGMENTMATERIALIVSGIXPROC)load("glGetFragmentMaterialivSGIX");
	glad_glLightEnviSGIX = (PFNGLLIGHTENVISGIXPROC)load("glLightEnviSGIX");
}
static void load_GL_SGIX_framezoom(GLADloadproc load) {
	if(!GLAD_GL_SGIX_framezoom) return;
	glad_glFrameZoomSGIX = (PFNGLFRAMEZOOMSGIXPROC)load("glFrameZoomSGIX");
}
static void load_GL_SGIX_igloo_interface(GLADloadproc load) {
	if(!GLAD_GL_SGIX_igloo_interface) return;
	glad_glIglooInterfaceSGIX = (PFNGLIGLOOINTERFACESGIXPROC)load("glIglooInterfaceSGIX");
}
static void load_GL_SGIX_instruments(GLADloadproc load) {
	if(!GLAD_GL_SGIX_instruments) return;
	glad_glGetInstrumentsSGIX = (PFNGLGETINSTRUMENTSSGIXPROC)load("glGetInstrumentsSGIX");
	glad_glInstrumentsBufferSGIX = (PFNGLINSTRUMENTSBUFFERSGIXPROC)load("glInstrumentsBufferSGIX");
	glad_glPollInstrumentsSGIX = (PFNGLPOLLINSTRUMENTSSGIXPROC)load("glPollInstrumentsSGIX");
	glad_glReadInstrumentsSGIX = (PFNGLREADINSTRUMENTSSGIXPROC)load("glReadInstrumentsSGIX");
	glad_glStartInstrumentsSGIX = (PFNGLSTARTINSTRUMENTSSGIXPROC)load("glStartInstrumentsSGIX");
	glad_glStopInstrumentsSGIX = (PFNGLSTOPINSTRUMENTSSGIXPROC)load("glStopInstrumentsSGIX");
}
static void load_GL_SGIX_list_priority(GLADloadproc load) {
	if(!GLAD_GL_SGIX_list_priority) return;
	glad_glGetListParameterfvSGIX = (PFNGLGETLISTPARAMETERFVSGIXPROC)load("glGetListParameterfvSGIX");
	glad_glGetListParameterivSGIX = (PFNGLGETLISTPARAMETERIVSGIXPROC)load("glGetListParameterivSGIX");
	glad_glListParameterfSGIX = (PFNGLLISTPARAMETERFSGIXPROC)load("glListParameterfSGIX");
	glad_glListParameterfvSGIX = (PFNGLLISTPARAMETERFVSGIXPROC)load("glListParameterfvSGIX");
	glad_glListParameteriSGIX = (PFNGLLISTPARAMETERISGIXPROC)load("glListParameteriSGIX");
	glad_glListParameterivSGIX = (PFNGLLISTPARAMETERIVSGIXPROC)load("glListParameterivSGIX");
}
static void load_GL_SGIX_pixel_texture(GLADloadproc load) {
	if(!GLAD_GL_SGIX_pixel_texture) return;
	glad_glPixelTexGenSGIX = (PFNGLPIXELTEXGENSGIXPROC)load("glPixelTexGenSGIX");
}
static void load_GL_SGIX_polynomial_ffd(GLADloadproc load) {
	if(!GLAD_GL_SGIX_polynomial_ffd) return;
	glad_glDeformationMap3dSGIX = (PFNGLDEFORMATIONMAP3DSGIXPROC)load("glDeformationMap3dSGIX");
	glad_glDeformationMap3fSGIX = (PFNGLDEFORMATIONMAP3FSGIXPROC)load("glDeformationMap3fSGIX");
	glad_glDeformSGIX = (PFNGLDEFORMSGIXPROC)load("glDeformSGIX");
	glad_glLoadIdentityDeformationMapSGIX = (PFNGLLOADIDENTITYDEFORMATIONMAPSGIXPROC)load("glLoadIdentityDeformationMapSGIX");
}
static void load_GL_SGIX_reference_plane(GLADloadproc load) {
	if(!GLAD_GL_SGIX_reference_plane) return;
	glad_glReferencePlaneSGIX = (PFNGLREFERENCEPLANESGIXPROC)load("glReferencePlaneSGIX");
}
static void load_GL_SGIX_sprite(GLADloadproc load) {
	if(!GLAD_GL_SGIX_sprite) return;
	glad_glSpriteParameterfSGIX = (PFNGLSPRITEPARAMETERFSGIXPROC)load("glSpriteParameterfSGIX");
	glad_glSpriteParameterfvSGIX = (PFNGLSPRITEPARAMETERFVSGIXPROC)load("glSpriteParameterfvSGIX");
	glad_glSpriteParameteriSGIX = (PFNGLSPRITEPARAMETERISGIXPROC)load("glSpriteParameteriSGIX");
	glad_glSpriteParameterivSGIX = (PFNGLSPRITEPARAMETERIVSGIXPROC)load("glSpriteParameterivSGIX");
}
static void load_GL_SGIX_tag_sample_buffer(GLADloadproc load) {
	if(!GLAD_GL_SGIX_tag_sample_buffer) return;
	glad_glTagSampleBufferSGIX = (PFNGLTAGSAMPLEBUFFERSGIXPROC)load("glTagSampleBufferSGIX");
}
static void load_GL_SGI_color_table(GLADloadproc load) {
	if(!GLAD_GL_SGI_color_table) return;
	glad_glColorTableSGI = (PFNGLCOLORTABLESGIPROC)load("glColorTableSGI");
	glad_glColorTableParameterfvSGI = (PFNGLCOLORTABLEPARAMETERFVSGIPROC)load("glColorTableParameterfvSGI");
	glad_glColorTableParameterivSGI = (PFNGLCOLORTABLEPARAMETERIVSGIPROC)load("glColorTableParameterivSGI");
	glad_glCopyColorTableSGI = (PFNGLCOPYCOLORTABLESGIPROC)load("glCopyColorTableSGI");
	glad_glGetColorTableSGI = (PFNGLGETCOLORTABLESGIPROC)load("glGetColorTableSGI");
	glad_glGetColorTableParameterfvSGI = (PFNGLGETCOLORTABLEPARAMETERFVSGIPROC)load("glGetColorTableParameterfvSGI");
	glad_glGetColorTableParameterivSGI = (PFNGLGETCOLORTABLEPARAMETERIVSGIPROC)load("glGetColorTableParameterivSGI");
}
static void load_GL_SUNX_constant_data(GLADloadproc load) {
	if(!GLAD_GL_SUNX_constant_data) return;
	glad_glFinishTextureSUNX = (PFNGLFINISHTEXTURESUNXPROC)load("glFinishTextureSUNX");
}
static void load_GL_SUN_global_alpha(GLADloadproc load) {
	if(!GLAD_GL_SUN_global_alpha) return;
	glad_glGlobalAlphaFactorbSUN = (PFNGLGLOBALALPHAFACTORBSUNPROC)load("glGlobalAlphaFactorbSUN");
	glad_glGlobalAlphaFactorsSUN = (PFNGLGLOBALALPHAFACTORSSUNPROC)load("glGlobalAlphaFactorsSUN");
	glad_glGlobalAlphaFactoriSUN = (PFNGLGLOBALALPHAFACTORISUNPROC)load("glGlobalAlphaFactoriSUN");
	glad_glGlobalAlphaFactorfSUN = (PFNGLGLOBALALPHAFACTORFSUNPROC)load("glGlobalAlphaFactorfSUN");
	glad_glGlobalAlphaFactordSUN = (PFNGLGLOBALALPHAFACTORDSUNPROC)load("glGlobalAlphaFactordSUN");
	glad_glGlobalAlphaFactorubSUN = (PFNGLGLOBALALPHAFACTORUBSUNPROC)load("glGlobalAlphaFactorubSUN");
	glad_glGlobalAlphaFactorusSUN = (PFNGLGLOBALALPHAFACTORUSSUNPROC)load("glGlobalAlphaFactorusSUN");
	glad_glGlobalAlphaFactoruiSUN = (PFNGLGLOBALALPHAFACTORUISUNPROC)load("glGlobalAlphaFactoruiSUN");
}
static void load_GL_SUN_mesh_array(GLADloadproc load) {
	if(!GLAD_GL_SUN_mesh_array) return;
	glad_glDrawMeshArraysSUN = (PFNGLDRAWMESHARRAYSSUNPROC)load("glDrawMeshArraysSUN");
}
static void load_GL_SUN_triangle_list(GLADloadproc load) {
	if(!GLAD_GL_SUN_triangle_list) return;
	glad_glReplacementCodeuiSUN = (PFNGLREPLACEMENTCODEUISUNPROC)load("glReplacementCodeuiSUN");
	glad_glReplacementCodeusSUN = (PFNGLREPLACEMENTCODEUSSUNPROC)load("glReplacementCodeusSUN");
	glad_glReplacementCodeubSUN = (PFNGLREPLACEMENTCODEUBSUNPROC)load("glReplacementCodeubSUN");
	glad_glReplacementCodeuivSUN = (PFNGLREPLACEMENTCODEUIVSUNPROC)load("glReplacementCodeuivSUN");
	glad_glReplacementCodeusvSUN = (PFNGLREPLACEMENTCODEUSVSUNPROC)load("glReplacementCodeusvSUN");
	glad_glReplacementCodeubvSUN = (PFNGLREPLACEMENTCODEUBVSUNPROC)load("glReplacementCodeubvSUN");
	glad_glReplacementCodePointerSUN = (PFNGLREPLACEMENTCODEPOINTERSUNPROC)load("glReplacementCodePointerSUN");
}
static void load_GL_SUN_vertex(GLADloadproc load) {
	if(!GLAD_GL_SUN_vertex) return;
	glad_glColor4ubVertex2fSUN = (PFNGLCOLOR4UBVERTEX2FSUNPROC)load("glColor4ubVertex2fSUN");
	glad_glColor4ubVertex2fvSUN = (PFNGLCOLOR4UBVERTEX2FVSUNPROC)load("glColor4ubVertex2fvSUN");
	glad_glColor4ubVertex3fSUN = (PFNGLCOLOR4UBVERTEX3FSUNPROC)load("glColor4ubVertex3fSUN");
	glad_glColor4ubVertex3fvSUN = (PFNGLCOLOR4UBVERTEX3FVSUNPROC)load("glColor4ubVertex3fvSUN");
	glad_glColor3fVertex3fSUN = (PFNGLCOLOR3FVERTEX3FSUNPROC)load("glColor3fVertex3fSUN");
	glad_glColor3fVertex3fvSUN = (PFNGLCOLOR3FVERTEX3FVSUNPROC)load("glColor3fVertex3fvSUN");
	glad_glNormal3fVertex3fSUN = (PFNGLNORMAL3FVERTEX3FSUNPROC)load("glNormal3fVertex3fSUN");
	glad_glNormal3fVertex3fvSUN = (PFNGLNORMAL3FVERTEX3FVSUNPROC)load("glNormal3fVertex3fvSUN");
	glad_glColor4fNormal3fVertex3fSUN = (PFNGLCOLOR4FNORMAL3FVERTEX3FSUNPROC)load("glColor4fNormal3fVertex3fSUN");
	glad_glColor4fNormal3fVertex3fvSUN = (PFNGLCOLOR4FNORMAL3FVERTEX3FVSUNPROC)load("glColor4fNormal3fVertex3fvSUN");
	glad_glTexCoord2fVertex3fSUN = (PFNGLTEXCOORD2FVERTEX3FSUNPROC)load("glTexCoord2fVertex3fSUN");
	glad_glTexCoord2fVertex3fvSUN = (PFNGLTEXCOORD2FVERTEX3FVSUNPROC)load("glTexCoord2fVertex3fvSUN");
	glad_glTexCoord4fVertex4fSUN = (PFNGLTEXCOORD4FVERTEX4FSUNPROC)load("glTexCoord4fVertex4fSUN");
	glad_glTexCoord4fVertex4fvSUN = (PFNGLTEXCOORD4FVERTEX4FVSUNPROC)load("glTexCoord4fVertex4fvSUN");
	glad_glTexCoord2fColor4ubVertex3fSUN = (PFNGLTEXCOORD2FCOLOR4UBVERTEX3FSUNPROC)load("glTexCoord2fColor4ubVertex3fSUN");
	glad_glTexCoord2fColor4ubVertex3fvSUN = (PFNGLTEXCOORD2FCOLOR4UBVERTEX3FVSUNPROC)load("glTexCoord2fColor4ubVertex3fvSUN");
	glad_glTexCoord2fColor3fVertex3fSUN = (PFNGLTEXCOORD2FCOLOR3FVERTEX3FSUNPROC)load("glTexCoord2fColor3fVertex3fSUN");
	glad_glTexCoord2fColor3fVertex3fvSUN = (PFNGLTEXCOORD2FCOLOR3FVERTEX3FVSUNPROC)load("glTexCoord2fColor3fVertex3fvSUN");
	glad_glTexCoord2fNormal3fVertex3fSUN = (PFNGLTEXCOORD2FNORMAL3FVERTEX3FSUNPROC)load("glTexCoord2fNormal3fVertex3fSUN");
	glad_glTexCoord2fNormal3fVertex3fvSUN = (PFNGLTEXCOORD2FNORMAL3FVERTEX3FVSUNPROC)load("glTexCoord2fNormal3fVertex3fvSUN");
	glad_glTexCoord2fColor4fNormal3fVertex3fSUN = (PFNGLTEXCOORD2FCOLOR4FNORMAL3FVERTEX3FSUNPROC)load("glTexCoord2fColor4fNormal3fVertex3fSUN");
	glad_glTexCoord2fColor4fNormal3fVertex3fvSUN = (PFNGLTEXCOORD2FCOLOR4FNORMAL3FVERTEX3FVSUNPROC)load("glTexCoord2fColor4fNormal3fVertex3fvSUN");
	glad_glTexCoord4fColor4fNormal3fVertex4fSUN = (PFNGLTEXCOORD4FCOLOR4FNORMAL3FVERTEX4FSUNPROC)load("glTexCoord4fColor4fNormal3fVertex4fSUN");
	glad_glTexCoord4fColor4fNormal3fVertex4fvSUN = (PFNGLTEXCOORD4FCOLOR4FNORMAL3FVERTEX4FVSUNPROC)load("glTexCoord4fColor4fNormal3fVertex4fvSUN");
	glad_glReplacementCodeuiVertex3fSUN = (PFNGLREPLACEMENTCODEUIVERTEX3FSUNPROC)load("glReplacementCodeuiVertex3fSUN");
	glad_glReplacementCodeuiVertex3fvSUN = (PFNGLREPLACEMENTCODEUIVERTEX3FVSUNPROC)load("glReplacementCodeuiVertex3fvSUN");
	glad_glReplacementCodeuiColor4ubVertex3fSUN = (PFNGLREPLACEMENTCODEUICOLOR4UBVERTEX3FSUNPROC)load("glReplacementCodeuiColor4ubVertex3fSUN");
	glad_glReplacementCodeuiColor4ubVertex3fvSUN = (PFNGLREPLACEMENTCODEUICOLOR4UBVERTEX3FVSUNPROC)load("glReplacementCodeuiColor4ubVertex3fvSUN");
	glad_glReplacementCodeuiColor3fVertex3fSUN = (PFNGLREPLACEMENTCODEUICOLOR3FVERTEX3FSUNPROC)load("glReplacementCodeuiColor3fVertex3fSUN");
	glad_glReplacementCodeuiColor3fVertex3fvSUN = (PFNGLREPLACEMENTCODEUICOLOR3FVERTEX3FVSUNPROC)load("glReplacementCodeuiColor3fVertex3fvSUN");
	glad_glReplacementCodeuiNormal3fVertex3fSUN = (PFNGLREPLACEMENTCODEUINORMAL3FVERTEX3FSUNPROC)load("glReplacementCodeuiNormal3fVertex3fSUN");
	glad_glReplacementCodeuiNormal3fVertex3fvSUN = (PFNGLREPLACEMENTCODEUINORMAL3FVERTEX3FVSUNPROC)load("glReplacementCodeuiNormal3fVertex3fvSUN");
	glad_glReplacementCodeuiColor4fNormal3fVertex3fSUN = (PFNGLREPLACEMENTCODEUICOLOR4FNORMAL3FVERTEX3FSUNPROC)load("glReplacementCodeuiColor4fNormal3fVertex3fSUN");
	glad_glReplacementCodeuiColor4fNormal3fVertex3fvSUN = (PFNGLREPLACEMENTCODEUICOLOR4FNORMAL3FVERTEX3FVSUNPROC)load("glReplacementCodeuiColor4fNormal3fVertex3fvSUN");
	glad_glReplacementCodeuiTexCoord2fVertex3fSUN = (PFNGLREPLACEMENTCODEUITEXCOORD2FVERTEX3FSUNPROC)load("glReplacementCodeuiTexCoord2fVertex3fSUN");
	glad_glReplacementCodeuiTexCoord2fVertex3fvSUN = (PFNGLREPLACEMENTCODEUITEXCOORD2FVERTEX3FVSUNPROC)load("glReplacementCodeuiTexCoord2fVertex3fvSUN");
	glad_glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN = (PFNGLREPLACEMENTCODEUITEXCOORD2FNORMAL3FVERTEX3FSUNPROC)load("glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN");
	glad_glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN = (PFNGLREPLACEMENTCODEUITEXCOORD2FNORMAL3FVERTEX3FVSUNPROC)load("glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN");
	glad_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN = (PFNGLREPLACEMENTCODEUITEXCOORD2FCOLOR4FNORMAL3FVERTEX3FSUNPROC)load("glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN");
	glad_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN = (PFNGLREPLACEMENTCODEUITEXCOORD2FCOLOR4FNORMAL3FVERTEX3FVSUNPROC)load("glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN");
}
static int find_extensionsGL(void) {
	if (!get_exts()) return 0;
	GLAD_GL_3DFX_multisample = has_ext("GL_3DFX_multisample");
	GLAD_GL_3DFX_tbuffer = has_ext("GL_3DFX_tbuffer");
	GLAD_GL_3DFX_texture_compression_FXT1 = has_ext("GL_3DFX_texture_compression_FXT1");
	GLAD_GL_AMD_blend_minmax_factor = has_ext("GL_AMD_blend_minmax_factor");
	GLAD_GL_AMD_conservative_depth = has_ext("GL_AMD_conservative_depth");
	GLAD_GL_AMD_debug_output = has_ext("GL_AMD_debug_output");
	GLAD_GL_AMD_depth_clamp_separate = has_ext("GL_AMD_depth_clamp_separate");
	GLAD_GL_AMD_draw_buffers_blend = has_ext("GL_AMD_draw_buffers_blend");
	GLAD_GL_AMD_framebuffer_multisample_advanced = has_ext("GL_AMD_framebuffer_multisample_advanced");
	GLAD_GL_AMD_framebuffer_sample_positions = has_ext("GL_AMD_framebuffer_sample_positions");
	GLAD_GL_AMD_gcn_shader = has_ext("GL_AMD_gcn_shader");
	GLAD_GL_AMD_gpu_shader_half_float = has_ext("GL_AMD_gpu_shader_half_float");
	GLAD_GL_AMD_gpu_shader_int16 = has_ext("GL_AMD_gpu_shader_int16");
	GLAD_GL_AMD_gpu_shader_int64 = has_ext("GL_AMD_gpu_shader_int64");
	GLAD_GL_AMD_interleaved_elements = has_ext("GL_AMD_interleaved_elements");
	GLAD_GL_AMD_multi_draw_indirect = has_ext("GL_AMD_multi_draw_indirect");
	GLAD_GL_AMD_name_gen_delete = has_ext("GL_AMD_name_gen_delete");
	GLAD_GL_AMD_occlusion_query_event = has_ext("GL_AMD_occlusion_query_event");
	GLAD_GL_AMD_performance_monitor = has_ext("GL_AMD_performance_monitor");
	GLAD_GL_AMD_pinned_memory = has_ext("GL_AMD_pinned_memory");
	GLAD_GL_AMD_query_buffer_object = has_ext("GL_AMD_query_buffer_object");
	GLAD_GL_AMD_sample_positions = has_ext("GL_AMD_sample_positions");
	GLAD_GL_AMD_seamless_cubemap_per_texture = has_ext("GL_AMD_seamless_cubemap_per_texture");
	GLAD_GL_AMD_shader_atomic_counter_ops = has_ext("GL_AMD_shader_atomic_counter_ops");
	GLAD_GL_AMD_shader_ballot = has_ext("GL_AMD_shader_ballot");
	GLAD_GL_AMD_shader_explicit_vertex_parameter = has_ext("GL_AMD_shader_explicit_vertex_parameter");
	GLAD_GL_AMD_shader_gpu_shader_half_float_fetch = has_ext("GL_AMD_shader_gpu_shader_half_float_fetch");
	GLAD_GL_AMD_shader_image_load_store_lod = has_ext("GL_AMD_shader_image_load_store_lod");
	GLAD_GL_AMD_shader_stencil_export = has_ext("GL_AMD_shader_stencil_export");
	GLAD_GL_AMD_shader_trinary_minmax = has_ext("GL_AMD_shader_trinary_minmax");
	GLAD_GL_AMD_sparse_texture = has_ext("GL_AMD_sparse_texture");
	GLAD_GL_AMD_stencil_operation_extended = has_ext("GL_AMD_stencil_operation_extended");
	GLAD_GL_AMD_texture_gather_bias_lod = has_ext("GL_AMD_texture_gather_bias_lod");
	GLAD_GL_AMD_texture_texture4 = has_ext("GL_AMD_texture_texture4");
	GLAD_GL_AMD_transform_feedback3_lines_triangles = has_ext("GL_AMD_transform_feedback3_lines_triangles");
	GLAD_GL_AMD_transform_feedback4 = has_ext("GL_AMD_transform_feedback4");
	GLAD_GL_AMD_vertex_shader_layer = has_ext("GL_AMD_vertex_shader_layer");
	GLAD_GL_AMD_vertex_shader_tessellator = has_ext("GL_AMD_vertex_shader_tessellator");
	GLAD_GL_AMD_vertex_shader_viewport_index = has_ext("GL_AMD_vertex_shader_viewport_index");
	GLAD_GL_APPLE_aux_depth_stencil = has_ext("GL_APPLE_aux_depth_stencil");
	GLAD_GL_APPLE_client_storage = has_ext("GL_APPLE_client_storage");
	GLAD_GL_APPLE_element_array = has_ext("GL_APPLE_element_array");
	GLAD_GL_APPLE_fence = has_ext("GL_APPLE_fence");
	GLAD_GL_APPLE_float_pixels = has_ext("GL_APPLE_float_pixels");
	GLAD_GL_APPLE_flush_buffer_range = has_ext("GL_APPLE_flush_buffer_range");
	GLAD_GL_APPLE_object_purgeable = has_ext("GL_APPLE_object_purgeable");
	GLAD_GL_APPLE_rgb_422 = has_ext("GL_APPLE_rgb_422");
	GLAD_GL_APPLE_row_bytes = has_ext("GL_APPLE_row_bytes");
	GLAD_GL_APPLE_specular_vector = has_ext("GL_APPLE_specular_vector");
	GLAD_GL_APPLE_texture_range = has_ext("GL_APPLE_texture_range");
	GLAD_GL_APPLE_transform_hint = has_ext("GL_APPLE_transform_hint");
	GLAD_GL_APPLE_vertex_array_object = has_ext("GL_APPLE_vertex_array_object");
	GLAD_GL_APPLE_vertex_array_range = has_ext("GL_APPLE_vertex_array_range");
	GLAD_GL_APPLE_vertex_program_evaluators = has_ext("GL_APPLE_vertex_program_evaluators");
	GLAD_GL_APPLE_ycbcr_422 = has_ext("GL_APPLE_ycbcr_422");
	GLAD_GL_ARB_ES2_compatibility = has_ext("GL_ARB_ES2_compatibility");
	GLAD_GL_ARB_ES3_1_compatibility = has_ext("GL_ARB_ES3_1_compatibility");
	GLAD_GL_ARB_ES3_2_compatibility = has_ext("GL_ARB_ES3_2_compatibility");
	GLAD_GL_ARB_ES3_compatibility = has_ext("GL_ARB_ES3_compatibility");
	GLAD_GL_ARB_arrays_of_arrays = has_ext("GL_ARB_arrays_of_arrays");
	GLAD_GL_ARB_base_instance = has_ext("GL_ARB_base_instance");
	GLAD_GL_ARB_bindless_texture = has_ext("GL_ARB_bindless_texture");
	GLAD_GL_ARB_blend_func_extended = has_ext("GL_ARB_blend_func_extended");
	GLAD_GL_ARB_buffer_storage = has_ext("GL_ARB_buffer_storage");
	GLAD_GL_ARB_cl_event = has_ext("GL_ARB_cl_event");
	GLAD_GL_ARB_clear_buffer_object = has_ext("GL_ARB_clear_buffer_object");
	GLAD_GL_ARB_clear_texture = has_ext("GL_ARB_clear_texture");
	GLAD_GL_ARB_clip_control = has_ext("GL_ARB_clip_control");
	GLAD_GL_ARB_color_buffer_float = has_ext("GL_ARB_color_buffer_float");
	GLAD_GL_ARB_compatibility = has_ext("GL_ARB_compatibility");
	GLAD_GL_ARB_compressed_texture_pixel_storage = has_ext("GL_ARB_compressed_texture_pixel_storage");
	GLAD_GL_ARB_compute_shader = has_ext("GL_ARB_compute_shader");
	GLAD_GL_ARB_compute_variable_group_size = has_ext("GL_ARB_compute_variable_group_size");
	GLAD_GL_ARB_conditional_render_inverted = has_ext("GL_ARB_conditional_render_inverted");
	GLAD_GL_ARB_conservative_depth = has_ext("GL_ARB_conservative_depth");
	GLAD_GL_ARB_copy_buffer = has_ext("GL_ARB_copy_buffer");
	GLAD_GL_ARB_copy_image = has_ext("GL_ARB_copy_image");
	GLAD_GL_ARB_cull_distance = has_ext("GL_ARB_cull_distance");
	GLAD_GL_ARB_debug_output = has_ext("GL_ARB_debug_output");
	GLAD_GL_ARB_depth_buffer_float = has_ext("GL_ARB_depth_buffer_float");
	GLAD_GL_ARB_depth_clamp = has_ext("GL_ARB_depth_clamp");
	GLAD_GL_ARB_depth_texture = has_ext("GL_ARB_depth_texture");
	GLAD_GL_ARB_derivative_control = has_ext("GL_ARB_derivative_control");
	GLAD_GL_ARB_direct_state_access = has_ext("GL_ARB_direct_state_access");
	GLAD_GL_ARB_draw_buffers = has_ext("GL_ARB_draw_buffers");
	GLAD_GL_ARB_draw_buffers_blend = has_ext("GL_ARB_draw_buffers_blend");
	GLAD_GL_ARB_draw_elements_base_vertex = has_ext("GL_ARB_draw_elements_base_vertex");
	GLAD_GL_ARB_draw_indirect = has_ext("GL_ARB_draw_indirect");
	GLAD_GL_ARB_draw_instanced = has_ext("GL_ARB_draw_instanced");
	GLAD_GL_ARB_enhanced_layouts = has_ext("GL_ARB_enhanced_layouts");
	GLAD_GL_ARB_explicit_attrib_location = has_ext("GL_ARB_explicit_attrib_location");
	GLAD_GL_ARB_explicit_uniform_location = has_ext("GL_ARB_explicit_uniform_location");
	GLAD_GL_ARB_fragment_coord_conventions = has_ext("GL_ARB_fragment_coord_conventions");
	GLAD_GL_ARB_fragment_layer_viewport = has_ext("GL_ARB_fragment_layer_viewport");
	GLAD_GL_ARB_fragment_program = has_ext("GL_ARB_fragment_program");
	GLAD_GL_ARB_fragment_program_shadow = has_ext("GL_ARB_fragment_program_shadow");
	GLAD_GL_ARB_fragment_shader = has_ext("GL_ARB_fragment_shader");
	GLAD_GL_ARB_fragment_shader_interlock = has_ext("GL_ARB_fragment_shader_interlock");
	GLAD_GL_ARB_framebuffer_no_attachments = has_ext("GL_ARB_framebuffer_no_attachments");
	GLAD_GL_ARB_framebuffer_object = has_ext("GL_ARB_framebuffer_object");
	GLAD_GL_ARB_framebuffer_sRGB = has_ext("GL_ARB_framebuffer_sRGB");
	GLAD_GL_ARB_geometry_shader4 = has_ext("GL_ARB_geometry_shader4");
	GLAD_GL_ARB_get_program_binary = has_ext("GL_ARB_get_program_binary");
	GLAD_GL_ARB_get_texture_sub_image = has_ext("GL_ARB_get_texture_sub_image");
	GLAD_GL_ARB_gl_spirv = has_ext("GL_ARB_gl_spirv");
	GLAD_GL_ARB_gpu_shader5 = has_ext("GL_ARB_gpu_shader5");
	GLAD_GL_ARB_gpu_shader_fp64 = has_ext("GL_ARB_gpu_shader_fp64");
	GLAD_GL_ARB_gpu_shader_int64 = has_ext("GL_ARB_gpu_shader_int64");
	GLAD_GL_ARB_half_float_pixel = has_ext("GL_ARB_half_float_pixel");
	GLAD_GL_ARB_half_float_vertex = has_ext("GL_ARB_half_float_vertex");
	GLAD_GL_ARB_imaging = has_ext("GL_ARB_imaging");
	GLAD_GL_ARB_indirect_parameters = has_ext("GL_ARB_indirect_parameters");
	GLAD_GL_ARB_instanced_arrays = has_ext("GL_ARB_instanced_arrays");
	GLAD_GL_ARB_internalformat_query = has_ext("GL_ARB_internalformat_query");
	GLAD_GL_ARB_internalformat_query2 = has_ext("GL_ARB_internalformat_query2");
	GLAD_GL_ARB_invalidate_subdata = has_ext("GL_ARB_invalidate_subdata");
	GLAD_GL_ARB_map_buffer_alignment = has_ext("GL_ARB_map_buffer_alignment");
	GLAD_GL_ARB_map_buffer_range = has_ext("GL_ARB_map_buffer_range");
	GLAD_GL_ARB_matrix_palette = has_ext("GL_ARB_matrix_palette");
	GLAD_GL_ARB_multi_bind = has_ext("GL_ARB_multi_bind");
	GLAD_GL_ARB_multi_draw_indirect = has_ext("GL_ARB_multi_draw_indirect");
	GLAD_GL_ARB_multisample = has_ext("GL_ARB_multisample");
	GLAD_GL_ARB_multitexture = has_ext("GL_ARB_multitexture");
	GLAD_GL_ARB_occlusion_query = has_ext("GL_ARB_occlusion_query");
	GLAD_GL_ARB_occlusion_query2 = has_ext("GL_ARB_occlusion_query2");
	GLAD_GL_ARB_parallel_shader_compile = has_ext("GL_ARB_parallel_shader_compile");
	GLAD_GL_ARB_pipeline_statistics_query = has_ext("GL_ARB_pipeline_statistics_query");
	GLAD_GL_ARB_pixel_buffer_object = has_ext("GL_ARB_pixel_buffer_object");
	GLAD_GL_ARB_point_parameters = has_ext("GL_ARB_point_parameters");
	GLAD_GL_ARB_point_sprite = has_ext("GL_ARB_point_sprite");
	GLAD_GL_ARB_polygon_offset_clamp = has_ext("GL_ARB_polygon_offset_clamp");
	GLAD_GL_ARB_post_depth_coverage = has_ext("GL_ARB_post_depth_coverage");
	GLAD_GL_ARB_program_interface_query = has_ext("GL_ARB_program_interface_query");
	GLAD_GL_ARB_provoking_vertex = has_ext("GL_ARB_provoking_vertex");
	GLAD_GL_ARB_query_buffer_object = has_ext("GL_ARB_query_buffer_object");
	GLAD_GL_ARB_robust_buffer_access_behavior = has_ext("GL_ARB_robust_buffer_access_behavior");
	GLAD_GL_ARB_robustness = has_ext("GL_ARB_robustness");
	GLAD_GL_ARB_robustness_isolation = has_ext("GL_ARB_robustness_isolation");
	GLAD_GL_ARB_sample_locations = has_ext("GL_ARB_sample_locations");
	GLAD_GL_ARB_sample_shading = has_ext("GL_ARB_sample_shading");
	GLAD_GL_ARB_sampler_objects = has_ext("GL_ARB_sampler_objects");
	GLAD_GL_ARB_seamless_cube_map = has_ext("GL_ARB_seamless_cube_map");
	GLAD_GL_ARB_seamless_cubemap_per_texture = has_ext("GL_ARB_seamless_cubemap_per_texture");
	GLAD_GL_ARB_separate_shader_objects = has_ext("GL_ARB_separate_shader_objects");
	GLAD_GL_ARB_shader_atomic_counter_ops = has_ext("GL_ARB_shader_atomic_counter_ops");
	GLAD_GL_ARB_shader_atomic_counters = has_ext("GL_ARB_shader_atomic_counters");
	GLAD_GL_ARB_shader_ballot = has_ext("GL_ARB_shader_ballot");
	GLAD_GL_ARB_shader_bit_encoding = has_ext("GL_ARB_shader_bit_encoding");
	GLAD_GL_ARB_shader_clock = has_ext("GL_ARB_shader_clock");
	GLAD_GL_ARB_shader_draw_parameters = has_ext("GL_ARB_shader_draw_parameters");
	GLAD_GL_ARB_shader_group_vote = has_ext("GL_ARB_shader_group_vote");
	GLAD_GL_ARB_shader_image_load_store = has_ext("GL_ARB_shader_image_load_store");
	GLAD_GL_ARB_shader_image_size = has_ext("GL_ARB_shader_image_size");
	GLAD_GL_ARB_shader_objects = has_ext("GL_ARB_shader_objects");
	GLAD_GL_ARB_shader_precision = has_ext("GL_ARB_shader_precision");
	GLAD_GL_ARB_shader_stencil_export = has_ext("GL_ARB_shader_stencil_export");
	GLAD_GL_ARB_shader_storage_buffer_object = has_ext("GL_ARB_shader_storage_buffer_object");
	GLAD_GL_ARB_shader_subroutine = has_ext("GL_ARB_shader_subroutine");
	GLAD_GL_ARB_shader_texture_image_samples = has_ext("GL_ARB_shader_texture_image_samples");
	GLAD_GL_ARB_shader_texture_lod = has_ext("GL_ARB_shader_texture_lod");
	GLAD_GL_ARB_shader_viewport_layer_array = has_ext("GL_ARB_shader_viewport_layer_array");
	GLAD_GL_ARB_shading_language_100 = has_ext("GL_ARB_shading_language_100");
	GLAD_GL_ARB_shading_language_420pack = has_ext("GL_ARB_shading_language_420pack");
	GLAD_GL_ARB_shading_language_include = has_ext("GL_ARB_shading_language_include");
	GLAD_GL_ARB_shading_language_packing = has_ext("GL_ARB_shading_language_packing");
	GLAD_GL_ARB_shadow = has_ext("GL_ARB_shadow");
	GLAD_GL_ARB_shadow_ambient = has_ext("GL_ARB_shadow_ambient");
	GLAD_GL_ARB_sparse_buffer = has_ext("GL_ARB_sparse_buffer");
	GLAD_GL_ARB_sparse_texture = has_ext("GL_ARB_sparse_texture");
	GLAD_GL_ARB_sparse_texture2 = has_ext("GL_ARB_sparse_texture2");
	GLAD_GL_ARB_sparse_texture_clamp = has_ext("GL_ARB_sparse_texture_clamp");
	GLAD_GL_ARB_spirv_extensions = has_ext("GL_ARB_spirv_extensions");
	GLAD_GL_ARB_stencil_texturing = has_ext("GL_ARB_stencil_texturing");
	GLAD_GL_ARB_sync = has_ext("GL_ARB_sync");
	GLAD_GL_ARB_tessellation_shader = has_ext("GL_ARB_tessellation_shader");
	GLAD_GL_ARB_texture_barrier = has_ext("GL_ARB_texture_barrier");
	GLAD_GL_ARB_texture_border_clamp = has_ext("GL_ARB_texture_border_clamp");
	GLAD_GL_ARB_texture_buffer_object = has_ext("GL_ARB_texture_buffer_object");
	GLAD_GL_ARB_texture_buffer_object_rgb32 = has_ext("GL_ARB_texture_buffer_object_rgb32");
	GLAD_GL_ARB_texture_buffer_range = has_ext("GL_ARB_texture_buffer_range");
	GLAD_GL_ARB_texture_compression = has_ext("GL_ARB_texture_compression");
	GLAD_GL_ARB_texture_compression_bptc = has_ext("GL_ARB_texture_compression_bptc");
	GLAD_GL_ARB_texture_compression_rgtc = has_ext("GL_ARB_texture_compression_rgtc");
	GLAD_GL_ARB_texture_cube_map = has_ext("GL_ARB_texture_cube_map");
	GLAD_GL_ARB_texture_cube_map_array = has_ext("GL_ARB_texture_cube_map_array");
	GLAD_GL_ARB_texture_env_add = has_ext("GL_ARB_texture_env_add");
	GLAD_GL_ARB_texture_env_combine = has_ext("GL_ARB_texture_env_combine");
	GLAD_GL_ARB_texture_env_crossbar = has_ext("GL_ARB_texture_env_crossbar");
	GLAD_GL_ARB_texture_env_dot3 = has_ext("GL_ARB_texture_env_dot3");
	GLAD_GL_ARB_texture_filter_anisotropic = has_ext("GL_ARB_texture_filter_anisotropic");
	GLAD_GL_ARB_texture_filter_minmax = has_ext("GL_ARB_texture_filter_minmax");
	GLAD_GL_ARB_texture_float = has_ext("GL_ARB_texture_float");
	GLAD_GL_ARB_texture_gather = has_ext("GL_ARB_texture_gather");
	GLAD_GL_ARB_texture_mirror_clamp_to_edge = has_ext("GL_ARB_texture_mirror_clamp_to_edge");
	GLAD_GL_ARB_texture_mirrored_repeat = has_ext("GL_ARB_texture_mirrored_repeat");
	GLAD_GL_ARB_texture_multisample = has_ext("GL_ARB_texture_multisample");
	GLAD_GL_ARB_texture_non_power_of_two = has_ext("GL_ARB_texture_non_power_of_two");
	GLAD_GL_ARB_texture_query_levels = has_ext("GL_ARB_texture_query_levels");
	GLAD_GL_ARB_texture_query_lod = has_ext("GL_ARB_texture_query_lod");
	GLAD_GL_ARB_texture_rectangle = has_ext("GL_ARB_texture_rectangle");
	GLAD_GL_ARB_texture_rg = has_ext("GL_ARB_texture_rg");
	GLAD_GL_ARB_texture_rgb10_a2ui = has_ext("GL_ARB_texture_rgb10_a2ui");
	GLAD_GL_ARB_texture_stencil8 = has_ext("GL_ARB_texture_stencil8");
	GLAD_GL_ARB_texture_storage = has_ext("GL_ARB_texture_storage");
	GLAD_GL_ARB_texture_storage_multisample = has_ext("GL_ARB_texture_storage_multisample");
	GLAD_GL_ARB_texture_swizzle = has_ext("GL_ARB_texture_swizzle");
	GLAD_GL_ARB_texture_view = has_ext("GL_ARB_texture_view");
	GLAD_GL_ARB_timer_query = has_ext("GL_ARB_timer_query");
	GLAD_GL_ARB_transform_feedback2 = has_ext("GL_ARB_transform_feedback2");
	GLAD_GL_ARB_transform_feedback3 = has_ext("GL_ARB_transform_feedback3");
	GLAD_GL_ARB_transform_feedback_instanced = has_ext("GL_ARB_transform_feedback_instanced");
	GLAD_GL_ARB_transform_feedback_overflow_query = has_ext("GL_ARB_transform_feedback_overflow_query");
	GLAD_GL_ARB_transpose_matrix = has_ext("GL_ARB_transpose_matrix");
	GLAD_GL_ARB_uniform_buffer_object = has_ext("GL_ARB_uniform_buffer_object");
	GLAD_GL_ARB_vertex_array_bgra = has_ext("GL_ARB_vertex_array_bgra");
	GLAD_GL_ARB_vertex_array_object = has_ext("GL_ARB_vertex_array_object");
	GLAD_GL_ARB_vertex_attrib_64bit = has_ext("GL_ARB_vertex_attrib_64bit");
	GLAD_GL_ARB_vertex_attrib_binding = has_ext("GL_ARB_vertex_attrib_binding");
	GLAD_GL_ARB_vertex_blend = has_ext("GL_ARB_vertex_blend");
	GLAD_GL_ARB_vertex_buffer_object = has_ext("GL_ARB_vertex_buffer_object");
	GLAD_GL_ARB_vertex_program = has_ext("GL_ARB_vertex_program");
	GLAD_GL_ARB_vertex_shader = has_ext("GL_ARB_vertex_shader");
	GLAD_GL_ARB_vertex_type_10f_11f_11f_rev = has_ext("GL_ARB_vertex_type_10f_11f_11f_rev");
	GLAD_GL_ARB_vertex_type_2_10_10_10_rev = has_ext("GL_ARB_vertex_type_2_10_10_10_rev");
	GLAD_GL_ARB_viewport_array = has_ext("GL_ARB_viewport_array");
	GLAD_GL_ARB_window_pos = has_ext("GL_ARB_window_pos");
	GLAD_GL_ATI_draw_buffers = has_ext("GL_ATI_draw_buffers");
	GLAD_GL_ATI_element_array = has_ext("GL_ATI_element_array");
	GLAD_GL_ATI_envmap_bumpmap = has_ext("GL_ATI_envmap_bumpmap");
	GLAD_GL_ATI_fragment_shader = has_ext("GL_ATI_fragment_shader");
	GLAD_GL_ATI_map_object_buffer = has_ext("GL_ATI_map_object_buffer");
	GLAD_GL_ATI_meminfo = has_ext("GL_ATI_meminfo");
	GLAD_GL_ATI_pixel_format_float = has_ext("GL_ATI_pixel_format_float");
	GLAD_GL_ATI_pn_triangles = has_ext("GL_ATI_pn_triangles");
	GLAD_GL_ATI_separate_stencil = has_ext("GL_ATI_separate_stencil");
	GLAD_GL_ATI_text_fragment_shader = has_ext("GL_ATI_text_fragment_shader");
	GLAD_GL_ATI_texture_env_combine3 = has_ext("GL_ATI_texture_env_combine3");
	GLAD_GL_ATI_texture_float = has_ext("GL_ATI_texture_float");
	GLAD_GL_ATI_texture_mirror_once = has_ext("GL_ATI_texture_mirror_once");
	GLAD_GL_ATI_vertex_array_object = has_ext("GL_ATI_vertex_array_object");
	GLAD_GL_ATI_vertex_attrib_array_object = has_ext("GL_ATI_vertex_attrib_array_object");
	GLAD_GL_ATI_vertex_streams = has_ext("GL_ATI_vertex_streams");
	GLAD_GL_EXT_422_pixels = has_ext("GL_EXT_422_pixels");
	GLAD_GL_EXT_EGL_image_storage = has_ext("GL_EXT_EGL_image_storage");
	GLAD_GL_EXT_EGL_sync = has_ext("GL_EXT_EGL_sync");
	GLAD_GL_EXT_abgr = has_ext("GL_EXT_abgr");
	GLAD_GL_EXT_bgra = has_ext("GL_EXT_bgra");
	GLAD_GL_EXT_bindable_uniform = has_ext("GL_EXT_bindable_uniform");
	GLAD_GL_EXT_blend_color = has_ext("GL_EXT_blend_color");
	GLAD_GL_EXT_blend_equation_separate = has_ext("GL_EXT_blend_equation_separate");
	GLAD_GL_EXT_blend_func_separate = has_ext("GL_EXT_blend_func_separate");
	GLAD_GL_EXT_blend_logic_op = has_ext("GL_EXT_blend_logic_op");
	GLAD_GL_EXT_blend_minmax = has_ext("GL_EXT_blend_minmax");
	GLAD_GL_EXT_blend_subtract = has_ext("GL_EXT_blend_subtract");
	GLAD_GL_EXT_clip_volume_hint = has_ext("GL_EXT_clip_volume_hint");
	GLAD_GL_EXT_cmyka = has_ext("GL_EXT_cmyka");
	GLAD_GL_EXT_color_subtable = has_ext("GL_EXT_color_subtable");
	GLAD_GL_EXT_compiled_vertex_array = has_ext("GL_EXT_compiled_vertex_array");
	GLAD_GL_EXT_convolution = has_ext("GL_EXT_convolution");
	GLAD_GL_EXT_coordinate_frame = has_ext("GL_EXT_coordinate_frame");
	GLAD_GL_EXT_copy_texture = has_ext("GL_EXT_copy_texture");
	GLAD_GL_EXT_cull_vertex = has_ext("GL_EXT_cull_vertex");
	GLAD_GL_EXT_debug_label = has_ext("GL_EXT_debug_label");
	GLAD_GL_EXT_debug_marker = has_ext("GL_EXT_debug_marker");
	GLAD_GL_EXT_depth_bounds_test = has_ext("GL_EXT_depth_bounds_test");
	GLAD_GL_EXT_direct_state_access = has_ext("GL_EXT_direct_state_access");
	GLAD_GL_EXT_draw_buffers2 = has_ext("GL_EXT_draw_buffers2");
	GLAD_GL_EXT_draw_instanced = has_ext("GL_EXT_draw_instanced");
	GLAD_GL_EXT_draw_range_elements = has_ext("GL_EXT_draw_range_elements");
	GLAD_GL_EXT_external_buffer = has_ext("GL_EXT_external_buffer");
	GLAD_GL_EXT_fog_coord = has_ext("GL_EXT_fog_coord");
	GLAD_GL_EXT_framebuffer_blit = has_ext("GL_EXT_framebuffer_blit");
	GLAD_GL_EXT_framebuffer_blit_layers = has_ext("GL_EXT_framebuffer_blit_layers");
	GLAD_GL_EXT_framebuffer_multisample = has_ext("GL_EXT_framebuffer_multisample");
	GLAD_GL_EXT_framebuffer_multisample_blit_scaled = has_ext("GL_EXT_framebuffer_multisample_blit_scaled");
	GLAD_GL_EXT_framebuffer_object = has_ext("GL_EXT_framebuffer_object");
	GLAD_GL_EXT_framebuffer_sRGB = has_ext("GL_EXT_framebuffer_sRGB");
	GLAD_GL_EXT_geometry_shader4 = has_ext("GL_EXT_geometry_shader4");
	GLAD_GL_EXT_gpu_program_parameters = has_ext("GL_EXT_gpu_program_parameters");
	GLAD_GL_EXT_gpu_shader4 = has_ext("GL_EXT_gpu_shader4");
	GLAD_GL_EXT_histogram = has_ext("GL_EXT_histogram");
	GLAD_GL_EXT_index_array_formats = has_ext("GL_EXT_index_array_formats");
	GLAD_GL_EXT_index_func = has_ext("GL_EXT_index_func");
	GLAD_GL_EXT_index_material = has_ext("GL_EXT_index_material");
	GLAD_GL_EXT_index_texture = has_ext("GL_EXT_index_texture");
	GLAD_GL_EXT_light_texture = has_ext("GL_EXT_light_texture");
	GLAD_GL_EXT_memory_object = has_ext("GL_EXT_memory_object");
	GLAD_GL_EXT_memory_object_fd = has_ext("GL_EXT_memory_object_fd");
	GLAD_GL_EXT_memory_object_win32 = has_ext("GL_EXT_memory_object_win32");
	GLAD_GL_EXT_misc_attribute = has_ext("GL_EXT_misc_attribute");
	GLAD_GL_EXT_multi_draw_arrays = has_ext("GL_EXT_multi_draw_arrays");
	GLAD_GL_EXT_multisample = has_ext("GL_EXT_multisample");
	GLAD_GL_EXT_multiview_tessellation_geometry_shader = has_ext("GL_EXT_multiview_tessellation_geometry_shader");
	GLAD_GL_EXT_multiview_texture_multisample = has_ext("GL_EXT_multiview_texture_multisample");
	GLAD_GL_EXT_multiview_timer_query = has_ext("GL_EXT_multiview_timer_query");
	GLAD_GL_EXT_packed_depth_stencil = has_ext("GL_EXT_packed_depth_stencil");
	GLAD_GL_EXT_packed_float = has_ext("GL_EXT_packed_float");
	GLAD_GL_EXT_packed_pixels = has_ext("GL_EXT_packed_pixels");
	GLAD_GL_EXT_paletted_texture = has_ext("GL_EXT_paletted_texture");
	GLAD_GL_EXT_pixel_buffer_object = has_ext("GL_EXT_pixel_buffer_object");
	GLAD_GL_EXT_pixel_transform = has_ext("GL_EXT_pixel_transform");
	GLAD_GL_EXT_pixel_transform_color_table = has_ext("GL_EXT_pixel_transform_color_table");
	GLAD_GL_EXT_point_parameters = has_ext("GL_EXT_point_parameters");
	GLAD_GL_EXT_polygon_offset = has_ext("GL_EXT_polygon_offset");
	GLAD_GL_EXT_polygon_offset_clamp = has_ext("GL_EXT_polygon_offset_clamp");
	GLAD_GL_EXT_post_depth_coverage = has_ext("GL_EXT_post_depth_coverage");
	GLAD_GL_EXT_provoking_vertex = has_ext("GL_EXT_provoking_vertex");
	GLAD_GL_EXT_raster_multisample = has_ext("GL_EXT_raster_multisample");
	GLAD_GL_EXT_rescale_normal = has_ext("GL_EXT_rescale_normal");
	GLAD_GL_EXT_secondary_color = has_ext("GL_EXT_secondary_color");
	GLAD_GL_EXT_semaphore = has_ext("GL_EXT_semaphore");
	GLAD_GL_EXT_semaphore_fd = has_ext("GL_EXT_semaphore_fd");
	GLAD_GL_EXT_semaphore_win32 = has_ext("GL_EXT_semaphore_win32");
	GLAD_GL_EXT_separate_shader_objects = has_ext("GL_EXT_separate_shader_objects");
	GLAD_GL_EXT_separate_specular_color = has_ext("GL_EXT_separate_specular_color");
	GLAD_GL_EXT_shader_framebuffer_fetch = has_ext("GL_EXT_shader_framebuffer_fetch");
	GLAD_GL_EXT_shader_framebuffer_fetch_non_coherent = has_ext("GL_EXT_shader_framebuffer_fetch_non_coherent");
	GLAD_GL_EXT_shader_image_load_formatted = has_ext("GL_EXT_shader_image_load_formatted");
	GLAD_GL_EXT_shader_image_load_store = has_ext("GL_EXT_shader_image_load_store");
	GLAD_GL_EXT_shader_integer_mix = has_ext("GL_EXT_shader_integer_mix");
	GLAD_GL_EXT_shader_samples_identical = has_ext("GL_EXT_shader_samples_identical");
	GLAD_GL_EXT_shadow_funcs = has_ext("GL_EXT_shadow_funcs");
	GLAD_GL_EXT_shared_texture_palette = has_ext("GL_EXT_shared_texture_palette");
	GLAD_GL_EXT_sparse_texture2 = has_ext("GL_EXT_sparse_texture2");
	GLAD_GL_EXT_stencil_clear_tag = has_ext("GL_EXT_stencil_clear_tag");
	GLAD_GL_EXT_stencil_two_side = has_ext("GL_EXT_stencil_two_side");
	GLAD_GL_EXT_stencil_wrap = has_ext("GL_EXT_stencil_wrap");
	GLAD_GL_EXT_subtexture = has_ext("GL_EXT_subtexture");
	GLAD_GL_EXT_texture = has_ext("GL_EXT_texture");
	GLAD_GL_EXT_texture3D = has_ext("GL_EXT_texture3D");
	GLAD_GL_EXT_texture_array = has_ext("GL_EXT_texture_array");
	GLAD_GL_EXT_texture_buffer_object = has_ext("GL_EXT_texture_buffer_object");
	GLAD_GL_EXT_texture_compression_latc = has_ext("GL_EXT_texture_compression_latc");
	GLAD_GL_EXT_texture_compression_rgtc = has_ext("GL_EXT_texture_compression_rgtc");
	GLAD_GL_EXT_texture_compression_s3tc = has_ext("GL_EXT_texture_compression_s3tc");
	GLAD_GL_EXT_texture_cube_map = has_ext("GL_EXT_texture_cube_map");
	GLAD_GL_EXT_texture_env_add = has_ext("GL_EXT_texture_env_add");
	GLAD_GL_EXT_texture_env_combine = has_ext("GL_EXT_texture_env_combine");
	GLAD_GL_EXT_texture_env_dot3 = has_ext("GL_EXT_texture_env_dot3");
	GLAD_GL_EXT_texture_filter_anisotropic = has_ext("GL_EXT_texture_filter_anisotropic");
	GLAD_GL_EXT_texture_filter_minmax = has_ext("GL_EXT_texture_filter_minmax");
	GLAD_GL_EXT_texture_integer = has_ext("GL_EXT_texture_integer");
	GLAD_GL_EXT_texture_lod_bias = has_ext("GL_EXT_texture_lod_bias");
	GLAD_GL_EXT_texture_mirror_clamp = has_ext("GL_EXT_texture_mirror_clamp");
	GLAD_GL_EXT_texture_object = has_ext("GL_EXT_texture_object");
	GLAD_GL_EXT_texture_perturb_normal = has_ext("GL_EXT_texture_perturb_normal");
	GLAD_GL_EXT_texture_sRGB = has_ext("GL_EXT_texture_sRGB");
	GLAD_GL_EXT_texture_sRGB_R8 = has_ext("GL_EXT_texture_sRGB_R8");
	GLAD_GL_EXT_texture_sRGB_RG8 = has_ext("GL_EXT_texture_sRGB_RG8");
	GLAD_GL_EXT_texture_sRGB_decode = has_ext("GL_EXT_texture_sRGB_decode");
	GLAD_GL_EXT_texture_shadow_lod = has_ext("GL_EXT_texture_shadow_lod");
	GLAD_GL_EXT_texture_shared_exponent = has_ext("GL_EXT_texture_shared_exponent");
	GLAD_GL_EXT_texture_snorm = has_ext("GL_EXT_texture_snorm");
	GLAD_GL_EXT_texture_storage = has_ext("GL_EXT_texture_storage");
	GLAD_GL_EXT_texture_swizzle = has_ext("GL_EXT_texture_swizzle");
	GLAD_GL_EXT_timer_query = has_ext("GL_EXT_timer_query");
	GLAD_GL_EXT_transform_feedback = has_ext("GL_EXT_transform_feedback");
	GLAD_GL_EXT_vertex_array = has_ext("GL_EXT_vertex_array");
	GLAD_GL_EXT_vertex_array_bgra = has_ext("GL_EXT_vertex_array_bgra");
	GLAD_GL_EXT_vertex_attrib_64bit = has_ext("GL_EXT_vertex_attrib_64bit");
	GLAD_GL_EXT_vertex_shader = has_ext("GL_EXT_vertex_shader");
	GLAD_GL_EXT_vertex_weighting = has_ext("GL_EXT_vertex_weighting");
	GLAD_GL_EXT_win32_keyed_mutex = has_ext("GL_EXT_win32_keyed_mutex");
	GLAD_GL_EXT_window_rectangles = has_ext("GL_EXT_window_rectangles");
	GLAD_GL_EXT_x11_sync_object = has_ext("GL_EXT_x11_sync_object");
	GLAD_GL_GREMEDY_frame_terminator = has_ext("GL_GREMEDY_frame_terminator");
	GLAD_GL_GREMEDY_string_marker = has_ext("GL_GREMEDY_string_marker");
	GLAD_GL_HP_convolution_border_modes = has_ext("GL_HP_convolution_border_modes");
	GLAD_GL_HP_image_transform = has_ext("GL_HP_image_transform");
	GLAD_GL_HP_occlusion_test = has_ext("GL_HP_occlusion_test");
	GLAD_GL_HP_texture_lighting = has_ext("GL_HP_texture_lighting");
	GLAD_GL_IBM_cull_vertex = has_ext("GL_IBM_cull_vertex");
	GLAD_GL_IBM_multimode_draw_arrays = has_ext("GL_IBM_multimode_draw_arrays");
	GLAD_GL_IBM_rasterpos_clip = has_ext("GL_IBM_rasterpos_clip");
	GLAD_GL_IBM_static_data = has_ext("GL_IBM_static_data");
	GLAD_GL_IBM_texture_mirrored_repeat = has_ext("GL_IBM_texture_mirrored_repeat");
	GLAD_GL_IBM_vertex_array_lists = has_ext("GL_IBM_vertex_array_lists");
	GLAD_GL_INGR_blend_func_separate = has_ext("GL_INGR_blend_func_separate");
	GLAD_GL_INGR_color_clamp = has_ext("GL_INGR_color_clamp");
	GLAD_GL_INGR_interlace_read = has_ext("GL_INGR_interlace_read");
	GLAD_GL_INTEL_blackhole_render = has_ext("GL_INTEL_blackhole_render");
	GLAD_GL_INTEL_conservative_rasterization = has_ext("GL_INTEL_conservative_rasterization");
	GLAD_GL_INTEL_fragment_shader_ordering = has_ext("GL_INTEL_fragment_shader_ordering");
	GLAD_GL_INTEL_framebuffer_CMAA = has_ext("GL_INTEL_framebuffer_CMAA");
	GLAD_GL_INTEL_map_texture = has_ext("GL_INTEL_map_texture");
	GLAD_GL_INTEL_parallel_arrays = has_ext("GL_INTEL_parallel_arrays");
	GLAD_GL_INTEL_performance_query = has_ext("GL_INTEL_performance_query");
	GLAD_GL_KHR_blend_equation_advanced = has_ext("GL_KHR_blend_equation_advanced");
	GLAD_GL_KHR_blend_equation_advanced_coherent = has_ext("GL_KHR_blend_equation_advanced_coherent");
	GLAD_GL_KHR_context_flush_control = has_ext("GL_KHR_context_flush_control");
	GLAD_GL_KHR_debug = has_ext("GL_KHR_debug");
	GLAD_GL_KHR_no_error = has_ext("GL_KHR_no_error");
	GLAD_GL_KHR_parallel_shader_compile = has_ext("GL_KHR_parallel_shader_compile");
	GLAD_GL_KHR_robust_buffer_access_behavior = has_ext("GL_KHR_robust_buffer_access_behavior");
	GLAD_GL_KHR_robustness = has_ext("GL_KHR_robustness");
	GLAD_GL_KHR_shader_subgroup = has_ext("GL_KHR_shader_subgroup");
	GLAD_GL_KHR_texture_compression_astc_hdr = has_ext("GL_KHR_texture_compression_astc_hdr");
	GLAD_GL_KHR_texture_compression_astc_ldr = has_ext("GL_KHR_texture_compression_astc_ldr");
	GLAD_GL_KHR_texture_compression_astc_sliced_3d = has_ext("GL_KHR_texture_compression_astc_sliced_3d");
	GLAD_GL_MESAX_texture_stack = has_ext("GL_MESAX_texture_stack");
	GLAD_GL_MESA_framebuffer_flip_x = has_ext("GL_MESA_framebuffer_flip_x");
	GLAD_GL_MESA_framebuffer_flip_y = has_ext("GL_MESA_framebuffer_flip_y");
	GLAD_GL_MESA_framebuffer_swap_xy = has_ext("GL_MESA_framebuffer_swap_xy");
	GLAD_GL_MESA_pack_invert = has_ext("GL_MESA_pack_invert");
	GLAD_GL_MESA_program_binary_formats = has_ext("GL_MESA_program_binary_formats");
	GLAD_GL_MESA_resize_buffers = has_ext("GL_MESA_resize_buffers");
	GLAD_GL_MESA_shader_integer_functions = has_ext("GL_MESA_shader_integer_functions");
	GLAD_GL_MESA_tile_raster_order = has_ext("GL_MESA_tile_raster_order");
	GLAD_GL_MESA_window_pos = has_ext("GL_MESA_window_pos");
	GLAD_GL_MESA_ycbcr_texture = has_ext("GL_MESA_ycbcr_texture");
	GLAD_GL_NVX_blend_equation_advanced_multi_draw_buffers = has_ext("GL_NVX_blend_equation_advanced_multi_draw_buffers");
	GLAD_GL_NVX_conditional_render = has_ext("GL_NVX_conditional_render");
	GLAD_GL_NVX_gpu_memory_info = has_ext("GL_NVX_gpu_memory_info");
	GLAD_GL_NVX_gpu_multicast2 = has_ext("GL_NVX_gpu_multicast2");
	GLAD_GL_NVX_linked_gpu_multicast = has_ext("GL_NVX_linked_gpu_multicast");
	GLAD_GL_NVX_progress_fence = has_ext("GL_NVX_progress_fence");
	GLAD_GL_NV_alpha_to_coverage_dither_control = has_ext("GL_NV_alpha_to_coverage_dither_control");
	GLAD_GL_NV_bindless_multi_draw_indirect = has_ext("GL_NV_bindless_multi_draw_indirect");
	GLAD_GL_NV_bindless_multi_draw_indirect_count = has_ext("GL_NV_bindless_multi_draw_indirect_count");
	GLAD_GL_NV_bindless_texture = has_ext("GL_NV_bindless_texture");
	GLAD_GL_NV_blend_equation_advanced = has_ext("GL_NV_blend_equation_advanced");
	GLAD_GL_NV_blend_equation_advanced_coherent = has_ext("GL_NV_blend_equation_advanced_coherent");
	GLAD_GL_NV_blend_minmax_factor = has_ext("GL_NV_blend_minmax_factor");
	GLAD_GL_NV_blend_square = has_ext("GL_NV_blend_square");
	GLAD_GL_NV_clip_space_w_scaling = has_ext("GL_NV_clip_space_w_scaling");
	GLAD_GL_NV_command_list = has_ext("GL_NV_command_list");
	GLAD_GL_NV_compute_program5 = has_ext("GL_NV_compute_program5");
	GLAD_GL_NV_compute_shader_derivatives = has_ext("GL_NV_compute_shader_derivatives");
	GLAD_GL_NV_conditional_render = has_ext("GL_NV_conditional_render");
	GLAD_GL_NV_conservative_raster = has_ext("GL_NV_conservative_raster");
	GLAD_GL_NV_conservative_raster_dilate = has_ext("GL_NV_conservative_raster_dilate");
	GLAD_GL_NV_conservative_raster_pre_snap = has_ext("GL_NV_conservative_raster_pre_snap");
	GLAD_GL_NV_conservative_raster_pre_snap_triangles = has_ext("GL_NV_conservative_raster_pre_snap_triangles");
	GLAD_GL_NV_conservative_raster_underestimation = has_ext("GL_NV_conservative_raster_underestimation");
	GLAD_GL_NV_copy_depth_to_color = has_ext("GL_NV_copy_depth_to_color");
	GLAD_GL_NV_copy_image = has_ext("GL_NV_copy_image");
	GLAD_GL_NV_deep_texture3D = has_ext("GL_NV_deep_texture3D");
	GLAD_GL_NV_depth_buffer_float = has_ext("GL_NV_depth_buffer_float");
	GLAD_GL_NV_depth_clamp = has_ext("GL_NV_depth_clamp");
	GLAD_GL_NV_draw_texture = has_ext("GL_NV_draw_texture");
	GLAD_GL_NV_draw_vulkan_image = has_ext("GL_NV_draw_vulkan_image");
	GLAD_GL_NV_evaluators = has_ext("GL_NV_evaluators");
	GLAD_GL_NV_explicit_multisample = has_ext("GL_NV_explicit_multisample");
	GLAD_GL_NV_fence = has_ext("GL_NV_fence");
	GLAD_GL_NV_fill_rectangle = has_ext("GL_NV_fill_rectangle");
	GLAD_GL_NV_float_buffer = has_ext("GL_NV_float_buffer");
	GLAD_GL_NV_fog_distance = has_ext("GL_NV_fog_distance");
	GLAD_GL_NV_fragment_coverage_to_color = has_ext("GL_NV_fragment_coverage_to_color");
	GLAD_GL_NV_fragment_program = has_ext("GL_NV_fragment_program");
	GLAD_GL_NV_fragment_program2 = has_ext("GL_NV_fragment_program2");
	GLAD_GL_NV_fragment_program4 = has_ext("GL_NV_fragment_program4");
	GLAD_GL_NV_fragment_program_option = has_ext("GL_NV_fragment_program_option");
	GLAD_GL_NV_fragment_shader_barycentric = has_ext("GL_NV_fragment_shader_barycentric");
	GLAD_GL_NV_fragment_shader_interlock = has_ext("GL_NV_fragment_shader_interlock");
	GLAD_GL_NV_framebuffer_mixed_samples = has_ext("GL_NV_framebuffer_mixed_samples");
	GLAD_GL_NV_framebuffer_multisample_coverage = has_ext("GL_NV_framebuffer_multisample_coverage");
	GLAD_GL_NV_geometry_program4 = has_ext("GL_NV_geometry_program4");
	GLAD_GL_NV_geometry_shader4 = has_ext("GL_NV_geometry_shader4");
	GLAD_GL_NV_geometry_shader_passthrough = has_ext("GL_NV_geometry_shader_passthrough");
	GLAD_GL_NV_gpu_multicast = has_ext("GL_NV_gpu_multicast");
	GLAD_GL_NV_gpu_program4 = has_ext("GL_NV_gpu_program4");
	GLAD_GL_NV_gpu_program5 = has_ext("GL_NV_gpu_program5");
	GLAD_GL_NV_gpu_program5_mem_extended = has_ext("GL_NV_gpu_program5_mem_extended");
	GLAD_GL_NV_gpu_shader5 = has_ext("GL_NV_gpu_shader5");
	GLAD_GL_NV_half_float = has_ext("GL_NV_half_float");
	GLAD_GL_NV_internalformat_sample_query = has_ext("GL_NV_internalformat_sample_query");
	GLAD_GL_NV_light_max_exponent = has_ext("GL_NV_light_max_exponent");
	GLAD_GL_NV_memory_attachment = has_ext("GL_NV_memory_attachment");
	GLAD_GL_NV_memory_object_sparse = has_ext("GL_NV_memory_object_sparse");
	GLAD_GL_NV_mesh_shader = has_ext("GL_NV_mesh_shader");
	GLAD_GL_NV_multisample_coverage = has_ext("GL_NV_multisample_coverage");
	GLAD_GL_NV_multisample_filter_hint = has_ext("GL_NV_multisample_filter_hint");
	GLAD_GL_NV_occlusion_query = has_ext("GL_NV_occlusion_query");
	GLAD_GL_NV_packed_depth_stencil = has_ext("GL_NV_packed_depth_stencil");
	GLAD_GL_NV_parameter_buffer_object = has_ext("GL_NV_parameter_buffer_object");
	GLAD_GL_NV_parameter_buffer_object2 = has_ext("GL_NV_parameter_buffer_object2");
	GLAD_GL_NV_path_rendering = has_ext("GL_NV_path_rendering");
	GLAD_GL_NV_path_rendering_shared_edge = has_ext("GL_NV_path_rendering_shared_edge");
	GLAD_GL_NV_pixel_data_range = has_ext("GL_NV_pixel_data_range");
	GLAD_GL_NV_point_sprite = has_ext("GL_NV_point_sprite");
	GLAD_GL_NV_present_video = has_ext("GL_NV_present_video");
	GLAD_GL_NV_primitive_restart = has_ext("GL_NV_primitive_restart");
	GLAD_GL_NV_primitive_shading_rate = has_ext("GL_NV_primitive_shading_rate");
	GLAD_GL_NV_query_resource = has_ext("GL_NV_query_resource");
	GLAD_GL_NV_query_resource_tag = has_ext("GL_NV_query_resource_tag");
	GLAD_GL_NV_register_combiners = has_ext("GL_NV_register_combiners");
	GLAD_GL_NV_register_combiners2 = has_ext("GL_NV_register_combiners2");
	GLAD_GL_NV_representative_fragment_test = has_ext("GL_NV_representative_fragment_test");
	GLAD_GL_NV_robustness_video_memory_purge = has_ext("GL_NV_robustness_video_memory_purge");
	GLAD_GL_NV_sample_locations = has_ext("GL_NV_sample_locations");
	GLAD_GL_NV_sample_mask_override_coverage = has_ext("GL_NV_sample_mask_override_coverage");
	GLAD_GL_NV_scissor_exclusive = has_ext("GL_NV_scissor_exclusive");
	GLAD_GL_NV_shader_atomic_counters = has_ext("GL_NV_shader_atomic_counters");
	GLAD_GL_NV_shader_atomic_float = has_ext("GL_NV_shader_atomic_float");
	GLAD_GL_NV_shader_atomic_float64 = has_ext("GL_NV_shader_atomic_float64");
	GLAD_GL_NV_shader_atomic_fp16_vector = has_ext("GL_NV_shader_atomic_fp16_vector");
	GLAD_GL_NV_shader_atomic_int64 = has_ext("GL_NV_shader_atomic_int64");
	GLAD_GL_NV_shader_buffer_load = has_ext("GL_NV_shader_buffer_load");
	GLAD_GL_NV_shader_buffer_store = has_ext("GL_NV_shader_buffer_store");
	GLAD_GL_NV_shader_storage_buffer_object = has_ext("GL_NV_shader_storage_buffer_object");
	GLAD_GL_NV_shader_subgroup_partitioned = has_ext("GL_NV_shader_subgroup_partitioned");
	GLAD_GL_NV_shader_texture_footprint = has_ext("GL_NV_shader_texture_footprint");
	GLAD_GL_NV_shader_thread_group = has_ext("GL_NV_shader_thread_group");
	GLAD_GL_NV_shader_thread_shuffle = has_ext("GL_NV_shader_thread_shuffle");
	GLAD_GL_NV_shading_rate_image = has_ext("GL_NV_shading_rate_image");
	GLAD_GL_NV_stereo_view_rendering = has_ext("GL_NV_stereo_view_rendering");
	GLAD_GL_NV_tessellation_program5 = has_ext("GL_NV_tessellation_program5");
	GLAD_GL_NV_texgen_emboss = has_ext("GL_NV_texgen_emboss");
	GLAD_GL_NV_texgen_reflection = has_ext("GL_NV_texgen_reflection");
	GLAD_GL_NV_texture_barrier = has_ext("GL_NV_texture_barrier");
	GLAD_GL_NV_texture_compression_vtc = has_ext("GL_NV_texture_compression_vtc");
	GLAD_GL_NV_texture_env_combine4 = has_ext("GL_NV_texture_env_combine4");
	GLAD_GL_NV_texture_expand_normal = has_ext("GL_NV_texture_expand_normal");
	GLAD_GL_NV_texture_multisample = has_ext("GL_NV_texture_multisample");
	GLAD_GL_NV_texture_rectangle = has_ext("GL_NV_texture_rectangle");
	GLAD_GL_NV_texture_rectangle_compressed = has_ext("GL_NV_texture_rectangle_compressed");
	GLAD_GL_NV_texture_shader = has_ext("GL_NV_texture_shader");
	GLAD_GL_NV_texture_shader2 = has_ext("GL_NV_texture_shader2");
	GLAD_GL_NV_texture_shader3 = has_ext("GL_NV_texture_shader3");
	GLAD_GL_NV_timeline_semaphore = has_ext("GL_NV_timeline_semaphore");
	GLAD_GL_NV_transform_feedback = has_ext("GL_NV_transform_feedback");
	GLAD_GL_NV_transform_feedback2 = has_ext("GL_NV_transform_feedback2");
	GLAD_GL_NV_uniform_buffer_unified_memory = has_ext("GL_NV_uniform_buffer_unified_memory");
	GLAD_GL_NV_vdpau_interop = has_ext("GL_NV_vdpau_interop");
	GLAD_GL_NV_vdpau_interop2 = has_ext("GL_NV_vdpau_interop2");
	GLAD_GL_NV_vertex_array_range = has_ext("GL_NV_vertex_array_range");
	GLAD_GL_NV_vertex_array_range2 = has_ext("GL_NV_vertex_array_range2");
	GLAD_GL_NV_vertex_attrib_integer_64bit = has_ext("GL_NV_vertex_attrib_integer_64bit");
	GLAD_GL_NV_vertex_buffer_unified_memory = has_ext("GL_NV_vertex_buffer_unified_memory");
	GLAD_GL_NV_vertex_program = has_ext("GL_NV_vertex_program");
	GLAD_GL_NV_vertex_program1_1 = has_ext("GL_NV_vertex_program1_1");
	GLAD_GL_NV_vertex_program2 = has_ext("GL_NV_vertex_program2");
	GLAD_GL_NV_vertex_program2_option = has_ext("GL_NV_vertex_program2_option");
	GLAD_GL_NV_vertex_program3 = has_ext("GL_NV_vertex_program3");
	GLAD_GL_NV_vertex_program4 = has_ext("GL_NV_vertex_program4");
	GLAD_GL_NV_video_capture = has_ext("GL_NV_video_capture");
	GLAD_GL_NV_viewport_array2 = has_ext("GL_NV_viewport_array2");
	GLAD_GL_NV_viewport_swizzle = has_ext("GL_NV_viewport_swizzle");
	GLAD_GL_OES_byte_coordinates = has_ext("GL_OES_byte_coordinates");
	GLAD_GL_OES_compressed_paletted_texture = has_ext("GL_OES_compressed_paletted_texture");
	GLAD_GL_OES_fixed_point = has_ext("GL_OES_fixed_point");
	GLAD_GL_OES_query_matrix = has_ext("GL_OES_query_matrix");
	GLAD_GL_OES_read_format = has_ext("GL_OES_read_format");
	GLAD_GL_OES_single_precision = has_ext("GL_OES_single_precision");
	GLAD_GL_OML_interlace = has_ext("GL_OML_interlace");
	GLAD_GL_OML_resample = has_ext("GL_OML_resample");
	GLAD_GL_OML_subsample = has_ext("GL_OML_subsample");
	GLAD_GL_OVR_multiview = has_ext("GL_OVR_multiview");
	GLAD_GL_OVR_multiview2 = has_ext("GL_OVR_multiview2");
	GLAD_GL_PGI_misc_hints = has_ext("GL_PGI_misc_hints");
	GLAD_GL_PGI_vertex_hints = has_ext("GL_PGI_vertex_hints");
	GLAD_GL_REND_screen_coordinates = has_ext("GL_REND_screen_coordinates");
	GLAD_GL_S3_s3tc = has_ext("GL_S3_s3tc");
	GLAD_GL_SGIS_detail_texture = has_ext("GL_SGIS_detail_texture");
	GLAD_GL_SGIS_fog_function = has_ext("GL_SGIS_fog_function");
	GLAD_GL_SGIS_generate_mipmap = has_ext("GL_SGIS_generate_mipmap");
	GLAD_GL_SGIS_multisample = has_ext("GL_SGIS_multisample");
	GLAD_GL_SGIS_pixel_texture = has_ext("GL_SGIS_pixel_texture");
	GLAD_GL_SGIS_point_line_texgen = has_ext("GL_SGIS_point_line_texgen");
	GLAD_GL_SGIS_point_parameters = has_ext("GL_SGIS_point_parameters");
	GLAD_GL_SGIS_sharpen_texture = has_ext("GL_SGIS_sharpen_texture");
	GLAD_GL_SGIS_texture4D = has_ext("GL_SGIS_texture4D");
	GLAD_GL_SGIS_texture_border_clamp = has_ext("GL_SGIS_texture_border_clamp");
	GLAD_GL_SGIS_texture_color_mask = has_ext("GL_SGIS_texture_color_mask");
	GLAD_GL_SGIS_texture_edge_clamp = has_ext("GL_SGIS_texture_edge_clamp");
	GLAD_GL_SGIS_texture_filter4 = has_ext("GL_SGIS_texture_filter4");
	GLAD_GL_SGIS_texture_lod = has_ext("GL_SGIS_texture_lod");
	GLAD_GL_SGIS_texture_select = has_ext("GL_SGIS_texture_select");
	GLAD_GL_SGIX_async = has_ext("GL_SGIX_async");
	GLAD_GL_SGIX_async_histogram = has_ext("GL_SGIX_async_histogram");
	GLAD_GL_SGIX_async_pixel = has_ext("GL_SGIX_async_pixel");
	GLAD_GL_SGIX_blend_alpha_minmax = has_ext("GL_SGIX_blend_alpha_minmax");
	GLAD_GL_SGIX_calligraphic_fragment = has_ext("GL_SGIX_calligraphic_fragment");
	GLAD_GL_SGIX_clipmap = has_ext("GL_SGIX_clipmap");
	GLAD_GL_SGIX_convolution_accuracy = has_ext("GL_SGIX_convolution_accuracy");
	GLAD_GL_SGIX_depth_pass_instrument = has_ext("GL_SGIX_depth_pass_instrument");
	GLAD_GL_SGIX_depth_texture = has_ext("GL_SGIX_depth_texture");
	GLAD_GL_SGIX_flush_raster = has_ext("GL_SGIX_flush_raster");
	GLAD_GL_SGIX_fog_offset = has_ext("GL_SGIX_fog_offset");
	GLAD_GL_SGIX_fragment_lighting = has_ext("GL_SGIX_fragment_lighting");
	GLAD_GL_SGIX_framezoom = has_ext("GL_SGIX_framezoom");
	GLAD_GL_SGIX_igloo_interface = has_ext("GL_SGIX_igloo_interface");
	GLAD_GL_SGIX_instruments = has_ext("GL_SGIX_instruments");
	GLAD_GL_SGIX_interlace = has_ext("GL_SGIX_interlace");
	GLAD_GL_SGIX_ir_instrument1 = has_ext("GL_SGIX_ir_instrument1");
	GLAD_GL_SGIX_list_priority = has_ext("GL_SGIX_list_priority");
	GLAD_GL_SGIX_pixel_texture = has_ext("GL_SGIX_pixel_texture");
	GLAD_GL_SGIX_pixel_tiles = has_ext("GL_SGIX_pixel_tiles");
	GLAD_GL_SGIX_polynomial_ffd = has_ext("GL_SGIX_polynomial_ffd");
	GLAD_GL_SGIX_reference_plane = has_ext("GL_SGIX_reference_plane");
	GLAD_GL_SGIX_resample = has_ext("GL_SGIX_resample");
	GLAD_GL_SGIX_scalebias_hint = has_ext("GL_SGIX_scalebias_hint");
	GLAD_GL_SGIX_shadow = has_ext("GL_SGIX_shadow");
	GLAD_GL_SGIX_shadow_ambient = has_ext("GL_SGIX_shadow_ambient");
	GLAD_GL_SGIX_sprite = has_ext("GL_SGIX_sprite");
	GLAD_GL_SGIX_subsample = has_ext("GL_SGIX_subsample");
	GLAD_GL_SGIX_tag_sample_buffer = has_ext("GL_SGIX_tag_sample_buffer");
	GLAD_GL_SGIX_texture_add_env = has_ext("GL_SGIX_texture_add_env");
	GLAD_GL_SGIX_texture_coordinate_clamp = has_ext("GL_SGIX_texture_coordinate_clamp");
	GLAD_GL_SGIX_texture_lod_bias = has_ext("GL_SGIX_texture_lod_bias");
	GLAD_GL_SGIX_texture_multi_buffer = has_ext("GL_SGIX_texture_multi_buffer");
	GLAD_GL_SGIX_texture_scale_bias = has_ext("GL_SGIX_texture_scale_bias");
	GLAD_GL_SGIX_vertex_preclip = has_ext("GL_SGIX_vertex_preclip");
	GLAD_GL_SGIX_ycrcb = has_ext("GL_SGIX_ycrcb");
	GLAD_GL_SGIX_ycrcb_subsample = has_ext("GL_SGIX_ycrcb_subsample");
	GLAD_GL_SGIX_ycrcba = has_ext("GL_SGIX_ycrcba");
	GLAD_GL_SGI_color_matrix = has_ext("GL_SGI_color_matrix");
	GLAD_GL_SGI_color_table = has_ext("GL_SGI_color_table");
	GLAD_GL_SGI_texture_color_table = has_ext("GL_SGI_texture_color_table");
	GLAD_GL_SUNX_constant_data = has_ext("GL_SUNX_constant_data");
	GLAD_GL_SUN_convolution_border_modes = has_ext("GL_SUN_convolution_border_modes");
	GLAD_GL_SUN_global_alpha = has_ext("GL_SUN_global_alpha");
	GLAD_GL_SUN_mesh_array = has_ext("GL_SUN_mesh_array");
	GLAD_GL_SUN_slice_accum = has_ext("GL_SUN_slice_accum");
	GLAD_GL_SUN_triangle_list = has_ext("GL_SUN_triangle_list");
	GLAD_GL_SUN_vertex = has_ext("GL_SUN_vertex");
	GLAD_GL_WIN_phong_shading = has_ext("GL_WIN_phong_shading");
	GLAD_GL_WIN_specular_fog = has_ext("GL_WIN_specular_fog");
	free_exts();
	return 1;
}

static void find_coreGL(void) {

    /* Thank you @elmindreda
     * https://github.com/elmindreda/greg/blob/master/templates/greg.c.in#L176
     * https://github.com/glfw/glfw/blob/master/src/context.c#L36
     */
    int i, major, minor;

    const char* version;
    const char* prefixes[] = {
        "OpenGL ES-CM ",
        "OpenGL ES-CL ",
        "OpenGL ES ",
        NULL
    };

    version = (const char*) glGetString(GL_VERSION);
    if (!version) return;

    for (i = 0;  prefixes[i];  i++) {
        const size_t length = strlen(prefixes[i]);
        if (strncmp(version, prefixes[i], length) == 0) {
            version += length;
            break;
        }
    }

/* PR #18 */
#ifdef _MSC_VER
    sscanf_s(version, "%d.%d", &major, &minor);
#else
    sscanf(version, "%d.%d", &major, &minor);
#endif

    GLVersion.major = major; GLVersion.minor = minor;
    max_loaded_major = major; max_loaded_minor = minor;
	GLAD_GL_VERSION_1_0 = (major == 1 && minor >= 0) || major > 1;
	GLAD_GL_VERSION_1_1 = (major == 1 && minor >= 1) || major > 1;
	GLAD_GL_VERSION_1_2 = (major == 1 && minor >= 2) || major > 1;
	GLAD_GL_VERSION_1_3 = (major == 1 && minor >= 3) || major > 1;
	GLAD_GL_VERSION_1_4 = (major == 1 && minor >= 4) || major > 1;
	GLAD_GL_VERSION_1_5 = (major == 1 && minor >= 5) || major > 1;
	GLAD_GL_VERSION_2_0 = (major == 2 && minor >= 0) || major > 2;
	GLAD_GL_VERSION_2_1 = (major == 2 && minor >= 1) || major > 2;
	GLAD_GL_VERSION_3_0 = (major == 3 && minor >= 0) || major > 3;
	GLAD_GL_VERSION_3_1 = (major == 3 && minor >= 1) || major > 3;
	GLAD_GL_VERSION_3_2 = (major == 3 && minor >= 2) || major > 3;
	GLAD_GL_VERSION_3_3 = (major == 3 && minor >= 3) || major > 3;
	if (GLVersion.major > 3 || (GLVersion.major >= 3 && GLVersion.minor >= 3)) {
		max_loaded_major = 3;
		max_loaded_minor = 3;
	}
}

int gladLoadGLLoader(GLADloadproc load) {
	GLVersion.major = 0; GLVersion.minor = 0;
	glGetString = (PFNGLGETSTRINGPROC)load("glGetString");
	if(glGetString == NULL) return 0;
	if(glGetString(GL_VERSION) == NULL) return 0;
	find_coreGL();
	load_GL_VERSION_1_0(load);
	load_GL_VERSION_1_1(load);
	load_GL_VERSION_1_2(load);
	load_GL_VERSION_1_3(load);
	load_GL_VERSION_1_4(load);
	load_GL_VERSION_1_5(load);
	load_GL_VERSION_2_0(load);
	load_GL_VERSION_2_1(load);
	load_GL_VERSION_3_0(load);
	load_GL_VERSION_3_1(load);
	load_GL_VERSION_3_2(load);
	load_GL_VERSION_3_3(load);

	if (!find_extensionsGL()) return 0;
	load_GL_3DFX_tbuffer(load);
	load_GL_AMD_debug_output(load);
	load_GL_AMD_draw_buffers_blend(load);
	load_GL_AMD_framebuffer_multisample_advanced(load);
	load_GL_AMD_framebuffer_sample_positions(load);
	load_GL_AMD_gpu_shader_int64(load);
	load_GL_AMD_interleaved_elements(load);
	load_GL_AMD_multi_draw_indirect(load);
	load_GL_AMD_name_gen_delete(load);
	load_GL_AMD_occlusion_query_event(load);
	load_GL_AMD_performance_monitor(load);
	load_GL_AMD_sample_positions(load);
	load_GL_AMD_sparse_texture(load);
	load_GL_AMD_stencil_operation_extended(load);
	load_GL_AMD_vertex_shader_tessellator(load);
	load_GL_APPLE_element_array(load);
	load_GL_APPLE_fence(load);
	load_GL_APPLE_flush_buffer_range(load);
	load_GL_APPLE_object_purgeable(load);
	load_GL_APPLE_texture_range(load);
	load_GL_APPLE_vertex_array_object(load);
	load_GL_APPLE_vertex_array_range(load);
	load_GL_APPLE_vertex_program_evaluators(load);
	load_GL_ARB_ES2_compatibility(load);
	load_GL_ARB_ES3_1_compatibility(load);
	load_GL_ARB_ES3_2_compatibility(load);
	load_GL_ARB_base_instance(load);
	load_GL_ARB_bindless_texture(load);
	load_GL_ARB_blend_func_extended(load);
	load_GL_ARB_buffer_storage(load);
	load_GL_ARB_cl_event(load);
	load_GL_ARB_clear_buffer_object(load);
	load_GL_ARB_clear_texture(load);
	load_GL_ARB_clip_control(load);
	load_GL_ARB_color_buffer_float(load);
	load_GL_ARB_compute_shader(load);
	load_GL_ARB_compute_variable_group_size(load);
	load_GL_ARB_copy_buffer(load);
	load_GL_ARB_copy_image(load);
	load_GL_ARB_debug_output(load);
	load_GL_ARB_direct_state_access(load);
	load_GL_ARB_draw_buffers(load);
	load_GL_ARB_draw_buffers_blend(load);
	load_GL_ARB_draw_elements_base_vertex(load);
	load_GL_ARB_draw_indirect(load);
	load_GL_ARB_draw_instanced(load);
	load_GL_ARB_fragment_program(load);
	load_GL_ARB_framebuffer_no_attachments(load);
	load_GL_ARB_framebuffer_object(load);
	load_GL_ARB_geometry_shader4(load);
	load_GL_ARB_get_program_binary(load);
	load_GL_ARB_get_texture_sub_image(load);
	load_GL_ARB_gl_spirv(load);
	load_GL_ARB_gpu_shader_fp64(load);
	load_GL_ARB_gpu_shader_int64(load);
	load_GL_ARB_imaging(load);
	load_GL_ARB_indirect_parameters(load);
	load_GL_ARB_instanced_arrays(load);
	load_GL_ARB_internalformat_query(load);
	load_GL_ARB_internalformat_query2(load);
	load_GL_ARB_invalidate_subdata(load);
	load_GL_ARB_map_buffer_range(load);
	load_GL_ARB_matrix_palette(load);
	load_GL_ARB_multi_bind(load);
	load_GL_ARB_multi_draw_indirect(load);
	load_GL_ARB_multisample(load);
	load_GL_ARB_multitexture(load);
	load_GL_ARB_occlusion_query(load);
	load_GL_ARB_parallel_shader_compile(load);
	load_GL_ARB_point_parameters(load);
	load_GL_ARB_polygon_offset_clamp(load);
	load_GL_ARB_program_interface_query(load);
	load_GL_ARB_provoking_vertex(load);
	load_GL_ARB_robustness(load);
	load_GL_ARB_sample_locations(load);
	load_GL_ARB_sample_shading(load);
	load_GL_ARB_sampler_objects(load);
	load_GL_ARB_separate_shader_objects(load);
	load_GL_ARB_shader_atomic_counters(load);
	load_GL_ARB_shader_image_load_store(load);
	load_GL_ARB_shader_objects(load);
	load_GL_ARB_shader_storage_buffer_object(load);
	load_GL_ARB_shader_subroutine(load);
	load_GL_ARB_shading_language_include(load);
	load_GL_ARB_sparse_buffer(load);
	load_GL_ARB_sparse_texture(load);
	load_GL_ARB_sync(load);
	load_GL_ARB_tessellation_shader(load);
	load_GL_ARB_texture_barrier(load);
	load_GL_ARB_texture_buffer_object(load);
	load_GL_ARB_texture_buffer_range(load);
	load_GL_ARB_texture_compression(load);
	load_GL_ARB_texture_multisample(load);
	load_GL_ARB_texture_storage(load);
	load_GL_ARB_texture_storage_multisample(load);
	load_GL_ARB_texture_view(load);
	load_GL_ARB_timer_query(load);
	load_GL_ARB_transform_feedback2(load);
	load_GL_ARB_transform_feedback3(load);
	load_GL_ARB_transform_feedback_instanced(load);
	load_GL_ARB_transpose_matrix(load);
	load_GL_ARB_uniform_buffer_object(load);
	load_GL_ARB_vertex_array_object(load);
	load_GL_ARB_vertex_attrib_64bit(load);
	load_GL_ARB_vertex_attrib_binding(load);
	load_GL_ARB_vertex_blend(load);
	load_GL_ARB_vertex_buffer_object(load);
	load_GL_ARB_vertex_program(load);
	load_GL_ARB_vertex_shader(load);
	load_GL_ARB_vertex_type_2_10_10_10_rev(load);
	load_GL_ARB_viewport_array(load);
	load_GL_ARB_window_pos(load);
	load_GL_ATI_draw_buffers(load);
	load_GL_ATI_element_array(load);
	load_GL_ATI_envmap_bumpmap(load);
	load_GL_ATI_fragment_shader(load);
	load_GL_ATI_map_object_buffer(load);
	load_GL_ATI_pn_triangles(load);
	load_GL_ATI_separate_stencil(load);
	load_GL_ATI_vertex_array_object(load);
	load_GL_ATI_vertex_attrib_array_object(load);
	load_GL_ATI_vertex_streams(load);
	load_GL_EXT_EGL_image_storage(load);
	load_GL_EXT_bindable_uniform(load);
	load_GL_EXT_blend_color(load);
	load_GL_EXT_blend_equation_separate(load);
	load_GL_EXT_blend_func_separate(load);
	load_GL_EXT_blend_minmax(load);
	load_GL_EXT_color_subtable(load);
	load_GL_EXT_compiled_vertex_array(load);
	load_GL_EXT_convolution(load);
	load_GL_EXT_coordinate_frame(load);
	load_GL_EXT_copy_texture(load);
	load_GL_EXT_cull_vertex(load);
	load_GL_EXT_debug_label(load);
	load_GL_EXT_debug_marker(load);
	load_GL_EXT_depth_bounds_test(load);
	load_GL_EXT_direct_state_access(load);
	load_GL_EXT_draw_buffers2(load);
	load_GL_EXT_draw_instanced(load);
	load_GL_EXT_draw_range_elements(load);
	load_GL_EXT_external_buffer(load);
	load_GL_EXT_fog_coord(load);
	load_GL_EXT_framebuffer_blit(load);
	load_GL_EXT_framebuffer_blit_layers(load);
	load_GL_EXT_framebuffer_multisample(load);
	load_GL_EXT_framebuffer_object(load);
	load_GL_EXT_geometry_shader4(load);
	load_GL_EXT_gpu_program_parameters(load);
	load_GL_EXT_gpu_shader4(load);
	load_GL_EXT_histogram(load);
	load_GL_EXT_index_func(load);
	load_GL_EXT_index_material(load);
	load_GL_EXT_light_texture(load);
	load_GL_EXT_memory_object(load);
	load_GL_EXT_memory_object_fd(load);
	load_GL_EXT_memory_object_win32(load);
	load_GL_EXT_multi_draw_arrays(load);
	load_GL_EXT_multisample(load);
	load_GL_EXT_paletted_texture(load);
	load_GL_EXT_pixel_transform(load);
	load_GL_EXT_point_parameters(load);
	load_GL_EXT_polygon_offset(load);
	load_GL_EXT_polygon_offset_clamp(load);
	load_GL_EXT_provoking_vertex(load);
	load_GL_EXT_raster_multisample(load);
	load_GL_EXT_secondary_color(load);
	load_GL_EXT_semaphore(load);
	load_GL_EXT_semaphore_fd(load);
	load_GL_EXT_semaphore_win32(load);
	load_GL_EXT_separate_shader_objects(load);
	load_GL_EXT_shader_framebuffer_fetch_non_coherent(load);
	load_GL_EXT_shader_image_load_store(load);
	load_GL_EXT_stencil_clear_tag(load);
	load_GL_EXT_stencil_two_side(load);
	load_GL_EXT_subtexture(load);
	load_GL_EXT_texture3D(load);
	load_GL_EXT_texture_array(load);
	load_GL_EXT_texture_buffer_object(load);
	load_GL_EXT_texture_integer(load);
	load_GL_EXT_texture_object(load);
	load_GL_EXT_texture_perturb_normal(load);
	load_GL_EXT_texture_storage(load);
	load_GL_EXT_timer_query(load);
	load_GL_EXT_transform_feedback(load);
	load_GL_EXT_vertex_array(load);
	load_GL_EXT_vertex_attrib_64bit(load);
	load_GL_EXT_vertex_shader(load);
	load_GL_EXT_vertex_weighting(load);
	load_GL_EXT_win32_keyed_mutex(load);
	load_GL_EXT_window_rectangles(load);
	load_GL_EXT_x11_sync_object(load);
	load_GL_GREMEDY_frame_terminator(load);
	load_GL_GREMEDY_string_marker(load);
	load_GL_HP_image_transform(load);
	load_GL_IBM_multimode_draw_arrays(load);
	load_GL_IBM_static_data(load);
	load_GL_IBM_vertex_array_lists(load);
	load_GL_INGR_blend_func_separate(load);
	load_GL_INTEL_framebuffer_CMAA(load);
	load_GL_INTEL_map_texture(load);
	load_GL_INTEL_parallel_arrays(load);
	load_GL_INTEL_performance_query(load);
	load_GL_KHR_blend_equation_advanced(load);
	load_GL_KHR_debug(load);
	load_GL_KHR_parallel_shader_compile(load);
	load_GL_KHR_robustness(load);
	load_GL_MESA_framebuffer_flip_y(load);
	load_GL_MESA_resize_buffers(load);
	load_GL_MESA_window_pos(load);
	load_GL_NVX_conditional_render(load);
	load_GL_NVX_gpu_multicast2(load);
	load_GL_NVX_linked_gpu_multicast(load);
	load_GL_NVX_progress_fence(load);
	load_GL_NV_alpha_to_coverage_dither_control(load);
	load_GL_NV_bindless_multi_draw_indirect(load);
	load_GL_NV_bindless_multi_draw_indirect_count(load);
	load_GL_NV_bindless_texture(load);
	load_GL_NV_blend_equation_advanced(load);
	load_GL_NV_clip_space_w_scaling(load);
	load_GL_NV_command_list(load);
	load_GL_NV_conditional_render(load);
	load_GL_NV_conservative_raster(load);
	load_GL_NV_conservative_raster_dilate(load);
	load_GL_NV_conservative_raster_pre_snap_triangles(load);
	load_GL_NV_copy_image(load);
	load_GL_NV_depth_buffer_float(load);
	load_GL_NV_draw_texture(load);
	load_GL_NV_draw_vulkan_image(load);
	load_GL_NV_evaluators(load);
	load_GL_NV_explicit_multisample(load);
	load_GL_NV_fence(load);
	load_GL_NV_fragment_coverage_to_color(load);
	load_GL_NV_fragment_program(load);
	load_GL_NV_framebuffer_mixed_samples(load);
	load_GL_NV_framebuffer_multisample_coverage(load);
	load_GL_NV_geometry_program4(load);
	load_GL_NV_gpu_multicast(load);
	load_GL_NV_gpu_program4(load);
	load_GL_NV_gpu_program5(load);
	load_GL_NV_gpu_shader5(load);
	load_GL_NV_half_float(load);
	load_GL_NV_internalformat_sample_query(load);
	load_GL_NV_memory_attachment(load);
	load_GL_NV_memory_object_sparse(load);
	load_GL_NV_mesh_shader(load);
	load_GL_NV_occlusion_query(load);
	load_GL_NV_parameter_buffer_object(load);
	load_GL_NV_path_rendering(load);
	load_GL_NV_pixel_data_range(load);
	load_GL_NV_point_sprite(load);
	load_GL_NV_present_video(load);
	load_GL_NV_primitive_restart(load);
	load_GL_NV_query_resource(load);
	load_GL_NV_query_resource_tag(load);
	load_GL_NV_register_combiners(load);
	load_GL_NV_register_combiners2(load);
	load_GL_NV_sample_locations(load);
	load_GL_NV_scissor_exclusive(load);
	load_GL_NV_shader_buffer_load(load);
	load_GL_NV_shading_rate_image(load);
	load_GL_NV_texture_barrier(load);
	load_GL_NV_texture_multisample(load);
	load_GL_NV_timeline_semaphore(load);
	load_GL_NV_transform_feedback(load);
	load_GL_NV_transform_feedback2(load);
	load_GL_NV_vdpau_interop(load);
	load_GL_NV_vdpau_interop2(load);
	load_GL_NV_vertex_array_range(load);
	load_GL_NV_vertex_attrib_integer_64bit(load);
	load_GL_NV_vertex_buffer_unified_memory(load);
	load_GL_NV_vertex_program(load);
	load_GL_NV_vertex_program4(load);
	load_GL_NV_video_capture(load);
	load_GL_NV_viewport_swizzle(load);
	load_GL_OES_byte_coordinates(load);
	load_GL_OES_fixed_point(load);
	load_GL_OES_query_matrix(load);
	load_GL_OES_single_precision(load);
	load_GL_OVR_multiview(load);
	load_GL_PGI_misc_hints(load);
	load_GL_SGIS_detail_texture(load);
	load_GL_SGIS_fog_function(load);
	load_GL_SGIS_multisample(load);
	load_GL_SGIS_pixel_texture(load);
	load_GL_SGIS_point_parameters(load);
	load_GL_SGIS_sharpen_texture(load);
	load_GL_SGIS_texture4D(load);
	load_GL_SGIS_texture_color_mask(load);
	load_GL_SGIS_texture_filter4(load);
	load_GL_SGIX_async(load);
	load_GL_SGIX_flush_raster(load);
	load_GL_SGIX_fragment_lighting(load);
	load_GL_SGIX_framezoom(load);
	load_GL_SGIX_igloo_interface(load);
	load_GL_SGIX_instruments(load);
	load_GL_SGIX_list_priority(load);
	load_GL_SGIX_pixel_texture(load);
	load_GL_SGIX_polynomial_ffd(load);
	load_GL_SGIX_reference_plane(load);
	load_GL_SGIX_sprite(load);
	load_GL_SGIX_tag_sample_buffer(load);
	load_GL_SGI_color_table(load);
	load_GL_SUNX_constant_data(load);
	load_GL_SUN_global_alpha(load);
	load_GL_SUN_mesh_array(load);
	load_GL_SUN_triangle_list(load);
	load_GL_SUN_vertex(load);
	return GLVersion.major != 0 || GLVersion.minor != 0;
}

