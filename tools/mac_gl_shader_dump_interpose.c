/* DYLD_INSERT_LIBRARIES interposer: dumps every GLSL shader source the
   renderer compiles and counts the draws made with each fragment shader.
   Output directory: $SHADER_DUMP_DIR (shader_NNNN.frag / .vert, summary.txt).
   Build: gcc-4.0 -arch ppc -dynamiclib -framework OpenGL -o x.dylib this.c */
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXS 4096
static GLuint shader_id[MAXS], shader_type[MAXS], shader_serial[MAXS];
static unsigned shader_count, serial;
static GLuint prog_id[MAXS], prog_frag[MAXS];
static unsigned long prog_draws[MAXS], prog_verts[MAXS];
static unsigned prog_count;
static unsigned find_prog(GLuint id, int make);
static GLuint current;
/* samples that pass the depth test per program, from GL_SAMPLES_PASSED
   queries around each draw (skipped inside the game's own query; draws made
   with the depth test off count 0) */
static unsigned long prog_samples[MAXS];
#define PEND 512
static GLuint pend_q[PEND]; static unsigned pend_p[PEND]; static unsigned pend_head, pend_tail;
static int query_on = -1;
static void drain(int all)
{
    while (pend_head != pend_tail) {
        GLuint avail = 0, r = 0, q = pend_q[pend_head % PEND];
        glGetQueryObjectuivARB(q, GL_QUERY_RESULT_AVAILABLE_ARB, &avail);
        if (!avail && !all) break;
        glGetQueryObjectuivARB(q, GL_QUERY_RESULT_ARB, &r);
        if (pend_p[pend_head % PEND] != (unsigned)-1) prog_samples[pend_p[pend_head % PEND]] += r;
        glDeleteQueriesARB(1, &q);
        pend_head++;
    }
}
static GLuint begin_query(void)
{
    GLint cur = 0; GLuint q = 0; unsigned p;
    if (query_on < 0) query_on = getenv("SHADER_DUMP_QUERIES") != NULL;
    if (!query_on) return 0;
    glGetQueryivARB(GL_SAMPLES_PASSED_ARB, GL_CURRENT_QUERY_ARB, &cur);
    if (cur) return 0;
    drain(0);
    if (pend_tail - pend_head >= PEND) drain(1);
    glGenQueriesARB(1, &q);
    glBeginQueryARB(GL_SAMPLES_PASSED_ARB, q);
    p = find_prog(current, 0);
    pend_q[pend_tail % PEND] = q; pend_p[pend_tail % PEND] = p; pend_tail++;
    return q;
}
static void end_query(GLuint q) { if (q) glEndQueryARB(GL_SAMPLES_PASSED_ARB); }
static unsigned long total_draws, total_verts;
static int registered;

static unsigned find_shader(GLuint id)
{ unsigned i; for (i = shader_count; i-- > 0;) if (shader_id[i] == id) return i; return (unsigned)-1; }
static unsigned find_prog(GLuint id, int make)
{ unsigned i; for (i = 0; i < prog_count; i++) if (prog_id[i] == id) return i;
  if (!make || prog_count >= MAXS) return (unsigned)-1;
  prog_id[prog_count] = id; prog_frag[prog_count] = 0; return prog_count++; }

static void summary(void)
{
    if (query_on > 0) drain(0);
    char path[512]; const char *dir = getenv("SHADER_DUMP_DIR"); FILE *f; unsigned i;
    if (!dir) return;
    sprintf(path, "%s/summary.txt", dir);
    f = fopen(path, "w"); if (!f) return;
    fprintf(f, "total_draws %lu total_vertices %lu\n", total_draws, total_verts);
    for (i = 0; i < prog_count; i++)
        fprintf(f, "program %u fragment_serial %u draws %lu vertices %lu samples %lu\n", prog_id[i], prog_frag[i], prog_draws[i], prog_verts[i], prog_samples[i]);
    fclose(f);
}
static void note_draw(GLsizei n)
{
    unsigned p = find_prog(current, 0);
    total_draws++; total_verts += (unsigned long)n;
    if (p != (unsigned)-1) { prog_draws[p]++; prog_verts[p] += (unsigned long)n; }
    if (!registered) { registered = 1; atexit(summary); }
    if ((total_draws & 255) == 0) summary();
}

static GLuint my_glCreateShader(GLenum type)
{
    GLuint id = glCreateShader(type);
    if (shader_count < MAXS) { shader_id[shader_count] = id; shader_type[shader_count] = type; shader_serial[shader_count] = 0; shader_count++; }
    return id;
}
static void my_glShaderSource(GLuint shader, GLsizei count, const GLchar **string, const GLint *length)
{
    const char *dir = getenv("SHADER_DUMP_DIR"); unsigned s = find_shader(shader);
    if (dir && s != (unsigned)-1 && count == 1) {
        char path[512]; FILE *f; GLint len = length ? length[0] : (GLint)strlen(string[0]);
        shader_serial[s] = ++serial;
        sprintf(path, "%s/shader_%04u.%s", dir, shader_serial[s], shader_type[s] == GL_FRAGMENT_SHADER ? "frag" : "vert");
        f = fopen(path, "w"); if (f) { fwrite(string[0], 1, (size_t)len, f); fclose(f); }
    }
    glShaderSource(shader, count, string, length);
}
static void my_glAttachShader(GLuint program, GLuint shader)
{
    unsigned s = find_shader(shader), p = find_prog(program, 1);
    if (s != (unsigned)-1 && p != (unsigned)-1 && shader_type[s] == GL_FRAGMENT_SHADER) prog_frag[p] = shader_serial[s];
    glAttachShader(program, shader);
}
static void my_glUseProgram(GLuint program) { current = program; glUseProgram(program); }
static void my_glDrawElements(GLenum m, GLsizei n, GLenum t, const GLvoid *i) { GLuint q; note_draw(n); q = begin_query(); glDrawElements(m, n, t, i); end_query(q); }
static void my_glDrawRangeElements(GLenum m, GLuint a, GLuint b, GLsizei n, GLenum t, const GLvoid *i) { GLuint q; note_draw(n); q = begin_query(); glDrawRangeElements(m, a, b, n, t, i); end_query(q); }
static void my_glDrawArrays(GLenum m, GLint a, GLsizei n) { GLuint q; note_draw(n); q = begin_query(); glDrawArrays(m, a, n); end_query(q); }

#define INTERPOSE(r, o) { (const void *)(unsigned long)&r, (const void *)(unsigned long)&o }
typedef struct { const void *replacement; const void *original; } interpose_t;
__attribute__((used)) static const interpose_t interposers[] __attribute__((section("__DATA,__interpose"))) = {
    INTERPOSE(my_glCreateShader, glCreateShader),
    INTERPOSE(my_glShaderSource, glShaderSource),
    INTERPOSE(my_glAttachShader, glAttachShader),
    INTERPOSE(my_glUseProgram, glUseProgram),
    INTERPOSE(my_glDrawElements, glDrawElements),
    INTERPOSE(my_glDrawRangeElements, glDrawRangeElements),
    INTERPOSE(my_glDrawArrays, glDrawArrays),
};
