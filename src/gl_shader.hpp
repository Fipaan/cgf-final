#pragma once

#include "gl_first.hpp"
#include "gl_const.hpp"

namespace gl {

    bool verify_shader(std::string_view type, GLuint shader);
    bool verify_linking(GLuint program);

    // vertex array data
    struct vad_t {
        GLuint      index;
  	    vad_cpv_t   components_per_vertex;
  	    data_kind   type;
  	    GLboolean   normalized;
  	    GLsizei     stride;
  	    const void *offset;

        vad_t(GLuint index, vad_cpv_t components_per_vertex, GLsizei stride_count, size_t offset_count);

        bool define_for_current_vbo();
        bool enable_for_current_vbo();
        bool disable_for_current_vbo();
    };

    struct shader_t {
        // TODO: I don't think it should own
        std::optional<std::string> id;
        GLuint program;
        bool vert_present, frag_present, initialized;

        shader_t();
        shader_t(std::string id);
        shader_t(GLuint program);
        ~shader_t();

        bool from_src(const char *vertex_src, const char *fragment_src);
        bool from_files(const char *vertex_path, const char *fragment_path);
        bool use();

        GLuint uniform_loc(std::string name);

        bool uniform(std::string name, GLfloat v0);
        bool uniform(std::string name, GLfloat v0, GLfloat v1);
        bool uniform(std::string name, GLfloat v0, GLfloat v1, GLfloat v2);
        bool uniform(std::string name, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);

        bool uniform(std::string name, GLint v0);
        bool uniform(std::string name, GLint v0, GLint v1);
        bool uniform(std::string name, GLint v0, GLint v1, GLint v2);
        bool uniform(std::string name, GLint v0, GLint v1, GLint v2, GLint v3);

        bool uniform(std::string name, GLuint v0);
        bool uniform(std::string name, GLuint v0, GLuint v1);
        bool uniform(std::string name, GLuint v0, GLuint v1, GLuint v2);
        bool uniform(std::string name, GLuint v0, GLuint v1, GLuint v2, GLuint v3);

        bool uniform(std::string name, GLdouble x);
        bool uniform(std::string name, GLdouble x, GLdouble y);
        bool uniform(std::string name, GLdouble x, GLdouble y, GLdouble z);
        bool uniform(std::string name, GLdouble x, GLdouble y, GLdouble z, GLdouble w);

        bool uniform(v_type_t type, std::string name, GLsizei count, const GLfloat *value);
        bool uniform(v_type_t type, std::string name, GLsizei count, const GLint *value);
        bool uniform(v_type_t type, std::string name, GLsizei count, const GLdouble *value);

        bool uniform(m_type_t type, std::string name, GLsizei count, gl_bool_t transpose, const GLfloat *value);
        bool uniform(m_type_t type, std::string name, GLsizei count, gl_bool_t transpose, const GLdouble *value);

        bool uniform_block_binding(GLuint uniformBlockIndex, GLuint uniformBlockBinding);
        bool uniform_subroutines(GLenum shadertype, GLsizei count, const GLuint *indices);

        bool uniform_handle_arb(std::string name, GLuint64 value);
        bool uniform_handle_arb(std::string name, GLsizei count, const GLuint64 *value);

        shader_t(const shader_t&)            = delete;
        shader_t& operator=(const shader_t&) = delete;
        shader_t(shader_t&&)                 = delete;
        shader_t& operator=(shader_t&&)      = delete;
    };

    struct vao_t {
        GLuint arr;
        bool initialized;

        vao_t(bool generate = false);
        ~vao_t();

        void gen_array();
        bool bind();

        vao_t(const vao_t&)            = delete;
        vao_t& operator=(const vao_t&) = delete;
        vao_t(vao_t&&)                 = delete;
        vao_t& operator=(vao_t&&)      = delete;
    };

    // TODO: right now all operations bind VBO manually, seems redundant.
    struct vbo_t {
        GLuint buffer;
        buffer_kind kind;
        std::vector<vad_t> vads;
        bool initialized;
        void *cached;

        vbo_t(buffer_kind kind = buffer_kind::array, bool generate = false);
        ~vbo_t();
        
        void gen_buffer();
        bool bind();
        
        bool add_vad(vad_t vad);

        template<typename T>
        bool init_buffer(std::span<T> arr, dsu_t usage);

        vbo_t(const vbo_t&)            = delete;
        vbo_t& operator=(const vbo_t&) = delete;
        vbo_t(vbo_t&&)                 = delete;
        vbo_t& operator=(vbo_t&&)      = delete;
    };
}

///// IMPLEMENTATION ///////////////////////////////////////////////////////////

// TODO: type-check according to kind
template<typename T>
bool gl::vbo_t::init_buffer(std::span<T> arr, dsu_t usage)
{
    if (!bind()) return false;
    glBufferData(kind, arr.size() * sizeof(arr[0]), arr.data(), usage);
    if (check_gl_error()) return false;
    return true;
}

/*
    TODO: Signatures not finished
bool gl::shader_t::uniform1i64ARB (std::string name, GLint64 x);
bool gl::shader_t::uniform2i64ARB (std::string name, GLint64 x, GLint64 y);
bool gl::shader_t::uniform3i64ARB (std::string name, GLint64 x, GLint64 y, GLint64 z);
bool gl::shader_t::uniform4i64ARB (std::string name, GLint64 x, GLint64 y, GLint64 z, GLint64 w);
bool gl::shader_t::uniform1i64vARB (std::string name, GLsizei count, const GLint64 *value);
bool gl::shader_t::uniform2i64vARB (std::string name, GLsizei count, const GLint64 *value);
bool gl::shader_t::uniform3i64vARB (std::string name, GLsizei count, const GLint64 *value);
bool gl::shader_t::uniform4i64vARB (std::string name, GLsizei count, const GLint64 *value);
bool gl::shader_t::uniform1ui64ARB (std::string name, GLuint64 x);
bool gl::shader_t::uniform2ui64ARB (std::string name, GLuint64 x, GLuint64 y);
bool gl::shader_t::uniform3ui64ARB (std::string name, GLuint64 x, GLuint64 y, GLuint64 z);
bool gl::shader_t::uniform4ui64ARB (std::string name, GLuint64 x, GLuint64 y, GLuint64 z, GLuint64 w);
bool gl::shader_t::uniform1ui64vARB (std::string name, GLsizei count, const GLuint64 *value);
bool gl::shader_t::uniform2ui64vARB (std::string name, GLsizei count, const GLuint64 *value);
bool gl::shader_t::uniform3ui64vARB (std::string name, GLsizei count, const GLuint64 *value);
bool gl::shader_t::uniform4ui64vARB (std::string name, GLsizei count, const GLuint64 *value);

bool gl::shader_t::uniformHandleui64NV (std::string name, GLuint64 value);
bool gl::shader_t::uniformHandleui64vNV (std::string name, GLsizei count, const GLuint64 *value);

bool gl::shader_t::uniform1i64NV (std::string name, GLint64EXT x);
bool gl::shader_t::uniform2i64NV (std::string name, GLint64EXT x, GLint64EXT y);
bool gl::shader_t::uniform3i64NV (std::string name, GLint64EXT x, GLint64EXT y, GLint64EXT z);
bool gl::shader_t::uniform4i64NV (std::string name, GLint64EXT x, GLint64EXT y, GLint64EXT z, GLint64EXT w);
bool gl::shader_t::uniform1i64vNV (std::string name, GLsizei count, const GLint64EXT *value);
bool gl::shader_t::uniform2i64vNV (std::string name, GLsizei count, const GLint64EXT *value);
bool gl::shader_t::uniform3i64vNV (std::string name, GLsizei count, const GLint64EXT *value);
bool gl::shader_t::uniform4i64vNV (std::string name, GLsizei count, const GLint64EXT *value);
bool gl::shader_t::uniform1ui64NV (std::string name, GLuint64EXT x);
bool gl::shader_t::uniform2ui64NV (std::string name, GLuint64EXT x, GLuint64EXT y);
bool gl::shader_t::uniform3ui64NV (std::string name, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z);
bool gl::shader_t::uniform4ui64NV (std::string name, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z, GLuint64EXT w);
bool gl::shader_t::uniform1ui64vNV (std::string name, GLsizei count, const GLuint64EXT *value);
bool gl::shader_t::uniform2ui64vNV (std::string name, GLsizei count, const GLuint64EXT *value);
bool gl::shader_t::uniform3ui64vNV (std::string name, GLsizei count, const GLuint64EXT *value);
bool gl::shader_t::uniform4ui64vNV (std::string name, GLsizei count, const GLuint64EXT *value);

bool gl::shader_t::uniformui64NV (std::string name, GLuint64EXT value);
bool gl::shader_t::uniformui64vNV (std::string name, GLsizei count, const GLuint64EXT *value);
*/
