/* Capture file format shared by tools/mac_gl_record_interpose.c and
   tools/mac_gl_replay.c.  Native byte order (the files are made and replayed
   on PowerPC Macs).

   File: glr_header, then records.  Record: glr_rec (16 bytes), the nw words
   padded to a multiple of 16 bytes, then bloblen bytes padded to a multiple
   of 16 bytes.  The first nops_pre records rebuild the state before the
   captured frame (objects, their data, state calls; no draws); the next
   nops_frame records are the frame, ending with glFlush. */
#ifndef MAC_GL_REPLAY_FORMAT_H
#define MAC_GL_REPLAY_FORMAT_H
#define GLR_MAGIC 0x474C5250u
#define GLR_VERSION 1u
typedef struct { unsigned magic, version, nops_pre, nops_frame, flags, reserved[3]; } glr_header;
typedef struct { unsigned op, nw, bloblen, pad; } glr_rec;

/* ops with their own code in the recorder and the replay */
#define GLR_SPECIAL_OPS(X) \
    X(glGenTextures) X(glGenBuffers) X(glGenProgramsARB) X(glGenFramebuffersEXT) \
    X(glGenRenderbuffersEXT) X(glGenQueriesARB) \
    X(glDeleteTextures) X(glDeleteBuffers) X(glDeleteProgramsARB) X(glDeleteFramebuffersEXT) \
    X(glDeleteRenderbuffersEXT) X(glDeleteQueriesARB) X(glDeleteShader) X(glDeleteProgram) \
    X(glBindTexture) X(glBindBuffer) X(glBindProgramARB) X(glBindFramebufferEXT) \
    X(glBindRenderbufferEXT) X(glUseProgram) \
    X(glFramebufferTexture2DEXT) X(glFramebufferRenderbufferEXT) \
    X(glCreateShader) X(glCreateProgram) X(glShaderSource) X(glCompileShader) \
    X(glAttachShader) X(glLinkProgram) X(glBindAttribLocation) X(glGetUniformLocation) \
    X(glGetAttribLocation) X(glUniform1f) X(glUniform1i) X(glUniform4fv) \
    X(glBeginQueryARB) X(glGetQueryObjectuivARB) \
    X(glProgramStringARB) X(glProgramEnvParameter4fvARB) X(glVertexAttrib4fv) X(glTexParameterfv) \
    X(glBufferData) X(glBufferSubData) X(glTexImage2D) X(glTexSubImage2D) X(glTexImage3D) \
    X(glCompressedTexImage2D) \
    X(glVertexAttribPointer) X(glVertexAttribPointerClient) X(glClientBlob) X(glClientArray) \
    X(glDrawElements) X(glDrawRangeElements) X(glDrawArrays) \
    X(glFlush) X(glGetError) X(glGetIntegerv) X(glGetFloatv) X(glGetBooleanv) X(glIsEnabled)

enum {
    OP_NONE = 0,
#define X(n) OP_##n,
    GLR_SPECIAL_OPS(X)
#undef X
#define GLR_GEN_ENUM
#include "gl_simple.inc"
#undef GLR_GEN_ENUM
    OP_COUNT
};

#define GLR_MAX_WORDS 12
#endif
