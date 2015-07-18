/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include <cassert>
#include <cstdio>

#include "el/graphics/bitmap_fragment.h"
#include "el/util/debug.h"
#include "el/util/math.h"
#include "testbed/platform/gl2_renderer.h"

namespace testbed {
namespace platform {

GL2Renderer::GL2Bitmap::GL2Bitmap(GL2Renderer* renderer)
    : renderer_(renderer) {}

GL2Renderer::GL2Bitmap::~GL2Bitmap() {
  // Must flush and unbind before we delete the texture
  renderer_->FlushBitmap(this);
  if (handle_ == renderer_->current_texture_) {
    renderer_->BindBitmap(nullptr);
  }
  glDeleteTextures(1, &handle_);
}

bool GL2Renderer::GL2Bitmap::Init(int width, int height, uint32_t* data) {
  assert(width == el::util::GetNearestPowerOfTwo(width));
  assert(height == el::util::GetNearestPowerOfTwo(height));

  width_ = width;
  height_ = height;

  glGenTextures(1, &handle_);
  renderer_->BindBitmap(this);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  set_data(data);

  return true;
}

void GL2Renderer::GL2Bitmap::set_data(uint32_t* data) {
  renderer_->FlushBitmap(this);
  renderer_->BindBitmap(this);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width_, height_, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, data);
  ++renderer_->bitmap_validations_;
}

GL2Renderer::GL2Renderer() {
  const std::string vertex_shader_source =
      "\
    uniform mat4 projection_matrix;\n\
    attribute vec2 in_pos;\n\
    attribute vec2 in_uv;\n\
    attribute vec4 in_color;\n\
    varying vec4 vtx_color;\n\
    varying vec2 vtx_uv;\n\
    void main() {\n\
      gl_Position = projection_matrix * vec4(in_pos.xy, 0.0, 1.0);\n\
      vtx_color = in_color;\n\
      vtx_uv = in_uv;\n\
    }\n\
    ";
  const std::string fragment_shader_source =
      "\
    uniform sampler2D texture_sampler;\n\
    uniform float texture_mix;\n\
    varying vec4 vtx_color;\n\
    varying vec2 vtx_uv;\n\
    void main() {\n\
      gl_FragColor = vtx_color;\n\
      if (texture_mix > 0.0) {\n\
        gl_FragColor *= texture2D(texture_sampler, vtx_uv).rgba;\n\
      }\n\
    }\n\
    ";

  GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  const char* vertex_shader_source_ptr = vertex_shader_source.c_str();
  GLint vertex_shader_source_length = GLint(vertex_shader_source.size());
  glShaderSource(vertex_shader, 1, &vertex_shader_source_ptr,
                 &vertex_shader_source_length);
  glCompileShader(vertex_shader);
  char log[1024];
  GLsizei s;
  glGetShaderInfoLog(vertex_shader, 1024, &s, log);

  GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  const char* fragment_shader_source_ptr = fragment_shader_source.c_str();
  GLint fragment_shader_source_length = GLint(fragment_shader_source.size());
  glShaderSource(fragment_shader, 1, &fragment_shader_source_ptr,
                 &fragment_shader_source_length);
  glCompileShader(fragment_shader);
  glGetShaderInfoLog(fragment_shader, 1024, &s, log);

  program_ = glCreateProgram();
  glAttachShader(program_, vertex_shader);
  glAttachShader(program_, fragment_shader);
  glLinkProgram(program_);
  glGetProgramInfoLog(program_, 1024, &s, log);
  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);

  projection_matrix_loc_ = glGetUniformLocation(program_, "projection_matrix");
  texture_sampler_loc_ = glGetUniformLocation(program_, "texture_sampler");
  texture_mix_loc_ = glGetUniformLocation(program_, "texture_mix");
}

GL2Renderer::~GL2Renderer() { glDeleteProgram(program_); }

void GL2Renderer::BeginPaint(int render_target_w, int render_target_h) {
  bitmap_validations_ = 0;

  Renderer::BeginPaint(render_target_w, render_target_h);

  current_texture_ = 0;
  batch_.vertices = vertices_;

  // Ortho2D(0, (GLfloat)render_target_w, (GLfloat)render_target_h, 0);
  glViewport(0, 0, render_target_w, render_target_h);
  glScissor(0, 0, render_target_w, render_target_h);

  glEnable(GL_BLEND);
  glEnable(GL_TEXTURE_2D);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_SCISSOR_TEST);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glUseProgram(program_);

  float left = 0.0f;
  float right = float(render_target_w);
  float bottom = float(render_target_h);
  float top = 0.0f;
  float z_near = -1.0f;
  float z_far = 1.0f;
  float projection[16] = {0};
  projection[0] = 2.0f / (right - left);
  projection[5] = 2.0f / (top - bottom);
  projection[10] = -2.0f / (z_far - z_near);
  projection[12] = -(right + left) / (right - left);
  projection[13] = -(top + bottom) / (top - bottom);
  projection[14] = -(z_far + z_near) / (z_far - z_near);
  projection[15] = 1.0f;
  glUniformMatrix4fv(projection_matrix_loc_, 1, GL_FALSE, projection);
  glUniform1f(texture_mix_loc_, 0.0f);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        &vertices_[0].x);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        &vertices_[0].u);
  glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex),
                        &vertices_[0].color);
}

void GL2Renderer::EndPaint() {
  Renderer::EndPaint();

  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  glDisableVertexAttribArray(2);

  glUseProgram(0);

#ifdef EL_RUNTIME_DEBUG_INFO
  if (EL_DEBUG_SETTING(el::util::DebugInfo::Setting::kDrawRenderBatches)) {
    TBDebugOut("Frame caused %dll bitmap validations.\n", bitmap_validations_);
  }
#endif  // EL_RUNTIME_DEBUG_INFO
}

std::unique_ptr<el::graphics::Bitmap> GL2Renderer::CreateBitmap(
    int width, int height, uint32_t* data) {
  auto bitmap = std::make_unique<GL2Bitmap>(this);
  if (!bitmap->Init(width, height, data)) {
    return nullptr;
  }
  return std::unique_ptr<el::graphics::Bitmap>(std::move(bitmap));
}

void GL2Renderer::RenderBatch(Batch* batch) {
  if (current_texture_ && !batch->bitmap) {
    glUniform1f(texture_mix_loc_, 0.0f);
  } else if (!current_texture_ && batch->bitmap) {
    glUniform1f(texture_mix_loc_, 1.0f);
  }
  BindBitmap(batch->bitmap);
  glDrawArrays(GL_TRIANGLES, 0, uint32_t(batch->vertex_count));
}

void GL2Renderer::set_clip_rect(const el::Rect& rect) {
  glScissor(clip_rect_.x, screen_rect_.h - (clip_rect_.y + clip_rect_.h),
            clip_rect_.w, clip_rect_.h);
}

void GL2Renderer::BindBitmap(el::graphics::Bitmap* bitmap) {
  GLuint texture = bitmap ? static_cast<GL2Bitmap*>(bitmap)->handle_ : 0;
  if (texture != current_texture_) {
    current_texture_ = texture;
    glBindTexture(GL_TEXTURE_2D, current_texture_);
  }
}

}  // namespace platform
}  // namespace testbed
