#include "gl_shader.hpp"

#include "basic.hpp"

bool gl::verify_shader(std::string_view type, GLuint shader)
{
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar desc[512];
        glGetShaderInfoLog(shader, sizeof(desc)/sizeof(desc[0]), NULL, desc);
        errorfn("{} compilation failed: {}", type, desc);
    }
    return success != 0;
}

bool gl::verify_linking(GLuint program)
{
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar desc[512];
        glGetProgramInfoLog(program, sizeof(desc)/sizeof(desc[0]), NULL, desc);
        errorfn("shader linking failed: {}", desc);
    }
    return success != 0;
}

gl::vad_t::vad_t(GLuint index, vad_cpv_t components_per_vertex, GLsizei stride_count, size_t offset_count)
{
    this->index      = index;
    this->components_per_vertex = components_per_vertex;
    this->type       = data_kind::gl_float;
    this->normalized = GL_TRUE;
    stride = stride_count * sizeof(GLfloat);
    offset = (const void*)(offset_count * sizeof(GLfloat));
}

bool gl::vad_t::define_for_current_vbo()
{
    glVertexAttribPointer(index, components_per_vertex, type, normalized, stride, offset);
    if (check_gl_error("glVertexAttribPointer")) return false;
    return true;
}

bool gl::vad_t::enable_for_current_vbo()
{
    glEnableVertexAttribArray(index);
    if (check_gl_error("glEnableVertexAttribArray")) return false;
    return true;
}

bool gl::vad_t::disable_for_current_vbo()
{
    glDisableVertexAttribArray(index);
    if (check_gl_error("glDisableVertexAttribArray")) return false;
    return true;
}

gl::shader_t::shader_t()
{
    initialized = false;
}

gl::shader_t::shader_t(std::string id)
{
    this->id = id;
    initialized = false;
}

gl::shader_t::shader_t(GLuint program)
{
    this->program = program;
    initialized  = true;
}

gl::shader_t::~shader_t()
{
    if (initialized) {
        glDeleteProgram(program);
        initialized = false;
    }
}

bool gl::shader_t::from_src(const char *vertex_src, const char *fragment_src)
{
    assert(!initialized);
    assert(vertex_src || fragment_src);

    GLuint vs = (GLuint)-1;
    if (vertex_src) {
        vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs, 1, &vertex_src, NULL);
        glCompileShader(vs);
        if (!verify_shader("vertex", vs)) return false;
        vert_present = true;
    }
    
    GLuint fs = (GLuint)-1;
    if (fragment_src) {
        fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, 1, &fragment_src, NULL);
        glCompileShader(fs);
        if (!verify_shader("fragment", fs)) return false;
        frag_present = true;
    }

    program = glCreateProgram();
    if (vertex_src)   glAttachShader(program, vs);
    if (fragment_src) glAttachShader(program, fs);
    glLinkProgram(program);
    if (!verify_linking(program)) return false;
    if (vertex_src)   glDeleteShader(vs);
    if (fragment_src) glDeleteShader(fs);

    initialized = true;

    return true;
}

bool gl::shader_t::from_files(const char *vertex_path, const char *fragment_path)
{
    assert(!initialized);
    std::string vertex_src, fragment_src;
    const char *vertex_data   = nullptr;
    const char *fragment_data = nullptr;
    if (vertex_path != nullptr) {
        if (!read_entire_file(vertex_src,vertex_path))   return false;
        vertex_data = vertex_src.data();
    }
    if (fragment_path != nullptr) {
        if (!read_entire_file(fragment_src, fragment_path)) return false;
        fragment_data = fragment_src.data();
    }
    return from_src(vertex_data, fragment_data);
}

bool gl::shader_t::use()
{
    assert(initialized);
    glUseProgram(program);
    if (check_gl_error("glUseProgram")) return false;
    return true;
}

GLuint gl::shader_t::uniform_loc(std::string name)
{
    return glGetUniformLocation(program, name.data());
}

bool gl::shader_t::uniform(std::string name, GLfloat v0)
{
    glUniform1f(uniform_loc(name), v0);
    return gl::check_gl_error("glUniform1f");
}

// bool gl::shader_t::uniform(std::string name, GLfloat v0, GLfloat v1);

bool gl::shader_t::uniform(std::string name, GLfloat v0, GLfloat v1, GLfloat v2)
{
    glUniform3f(uniform_loc(name), v0, v1, v2);
    return gl::check_gl_error("glUniform3f");
}

bool gl::shader_t::uniform(std::string name, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3)
{
    glUniform4f(uniform_loc(name), v0, v1, v2, v3);
    return gl::check_gl_error("glUniform4f");
}

bool gl::shader_t::uniform(std::string name, GLint v0)
{
    glUniform1i(uniform_loc(name), v0);
    return gl::check_gl_error("glUniform1i");
}

// bool gl::shader_t::uniform(std::string name, GLint v0, GLint v1);
// bool gl::shader_t::uniform(std::string name, GLint v0, GLint v1, GLint v2);
// bool gl::shader_t::uniform(std::string name, GLint v0, GLint v1, GLint v2, GLint v3);

// bool gl::shader_t::uniform(std::string name, GLuint v0);
// bool gl::shader_t::uniform(std::string name, GLuint v0, GLuint v1);
// bool gl::shader_t::uniform(std::string name, GLuint v0, GLuint v1, GLuint v2);
// bool gl::shader_t::uniform(std::string name, GLuint v0, GLuint v1, GLuint v2, GLuint v3);

// bool gl::shader_t::uniform(std::string name, GLdouble x);
// bool gl::shader_t::uniform(std::string name, GLdouble x, GLdouble y);
// bool gl::shader_t::uniform(std::string name, GLdouble x, GLdouble y, GLdouble z);
// bool gl::shader_t::uniform(std::string name, GLdouble x, GLdouble y, GLdouble z, GLdouble w);

// bool gl::shader_t::uniform(v_type_t type, std::string name, GLsizei count, const GLfloat *value);
// bool gl::shader_t::uniform(v_type_t type, std::string name, GLsizei count, const GLint *value);
// bool gl::shader_t::uniform(v_type_t type, std::string name, GLsizei count, const GLdouble *value);


bool gl::shader_t::uniform(m_type_t type, std::string name, GLsizei count, gl_bool_t transpose, const GLfloat *value)
{
    void (*glUniformCallback)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) = nullptr;
    std::string_view glUniformCallbackName = "";

#define __glUniformCallbackSetup(suffix)               \
    glUniformCallback     = glUniformMatrix##suffix;   \
    glUniformCallbackName = "glUniformMatrix" #suffix; \
    break;

    switch (type) {
    case gl::m_type_t::m2x2: __glUniformCallbackSetup(2fv);
    case gl::m_type_t::m2x3: __glUniformCallbackSetup(2x3fv);
    case gl::m_type_t::m2x4: __glUniformCallbackSetup(2x4fv);
    case gl::m_type_t::m3x2: __glUniformCallbackSetup(3x2fv);
    case gl::m_type_t::m3x3: __glUniformCallbackSetup(3fv);
    case gl::m_type_t::m3x4: __glUniformCallbackSetup(3x4fv);
    case gl::m_type_t::m4x2: __glUniformCallbackSetup(4x2fv);
    case gl::m_type_t::m4x3: __glUniformCallbackSetup(4x3fv);
    case gl::m_type_t::m4x4: __glUniformCallbackSetup(4fv);
    }

#undef __glUniformCallbackName
#undef __glUniformCallbackStringify
#undef __glUniformCallbackSetup

    glUniformCallback(uniform_loc(name), count, transpose, value);
    return gl::check_gl_error(glUniformCallbackName);
}

// bool gl::shader_t::uniform(m_type_t type, std::string name, GLsizei count, gl_bool_t transpose, const GLdouble *value);

// bool gl::shader_t::uniform_block_binding(GLuint uniformBlockIndex, GLuint uniformBlockBinding);
// bool gl::shader_t::uniform_subroutines(GLenum shadertype, GLsizei count, const GLuint *indices);

// bool gl::shader_t::uniform_handle_arb(std::string name, GLuint64 value);
// bool gl::shader_t::uniform_handle_arb(std::string name, GLsizei count, const GLuint64 *value);

gl::vao_t::vao_t(bool generate)
{
    initialized = false;
    if (generate) gen_array();
}

void gl::vao_t::gen_array()
{
    assert(!initialized);
    glGenVertexArrays(1, &arr);
    initialized = true;
}

gl::vao_t::~vao_t()
{
    if (initialized) {
        glDeleteVertexArrays(1, &arr);
        initialized = false;
    }
}

bool gl::vao_t::bind()
{
    assert(initialized);
    glBindVertexArray(arr);
    if (check_gl_error()) return false;
    return true;
}

// TODO: right now all operations bind VBO manually, seems redundant.

gl::vbo_t::vbo_t(buffer_kind kind, bool generate)
{
    initialized = false;
    if (generate) gen_buffer();
    this->kind = kind;
}

void gl::vbo_t::gen_buffer()
{
    assert(!initialized);
    glGenBuffers(1, &buffer);
    initialized = true;
}

gl::vbo_t::~vbo_t()
{
    if (initialized) {
        glDeleteBuffers(1, &buffer);
        initialized = false;
    }
}

bool gl::vbo_t::bind()
{
    assert(initialized);
    glBindBuffer(kind, buffer);
    if (check_gl_error()) return false;
    return true;
}

bool gl::vbo_t::add_vad(vad_t vad)
{
    if (!bind()) return false;

    if (!vad.define_for_current_vbo()) return false;
    if (!vad.enable_for_current_vbo()) return false;

    vads.push_back(vad);

    return true;
}
