#include "Editor.hpp"

#define GL_GLEXT_PROTOTYPES
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <GL/gl.h>

#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <libpx.hpp>

#include <memory>

#include <cstdio>
#include <cstdlib>

#include "Dialog.hpp"
#include "ImageIO.hpp"
#include "Mode.hpp"

#include "ExportDialog.hpp"

#include "DrawMode.hpp"
#include "EditMode.hpp"
#include "PoseMode.hpp"

namespace px {

namespace {

/// This is the GLSL source code for the vertex shader.
extern const char* vertexShaderSource;

/// This is the GLSL source code for the fragment shader.
extern const char* fragmentShaderSource;

} // namespace

/// Represents the graphical editor window.
class EditorImpl final
{
  friend Editor;
  /// The change in zoom at each zoom in/out operation.
  const float zoomFactor = 1.5f;
  /// The current zoom factor.
  float zoom = 1;
  /// The document being edited.
  Document* doc = nullptr;
  /// The image being rendered to.
  Image* image = nullptr;
  /// The current dialog (usually null).
  std::unique_ptr<Dialog> dialog;
  /// The current operation mode.
  std::unique_ptr<Mode> mode;
  /// The editor window instance.
  GLFWwindow* window = nullptr;
  /// The vertex array ID.
  GLuint vertexArray = 0;
  /// Stores indices for the two triangles.
  GLuint elementBuffer = 0;
  /// The quadrilateral vertex buffer.
  GLuint vertexBuffer = 0;
  /// The vertex shader ID.
  GLuint vertexShader = 0;
  /// The fragment shader ID.
  GLuint fragmentShader = 0;
  /// The ID of the texture that
  /// receives the painter output.
  GLuint texture = 0;
  /// The GL program ID.
  GLuint program = 0;
  /// The location of the image transformation.
  GLuint transformLocation = 0;
  /// Releases data allocated by the implementation data.
  ~EditorImpl();
  /// Calculates the transform from window
  /// to edit space.
  glm::mat4 calcTransform() noexcept;
};

EditorImpl::~EditorImpl()
{
  if (doc) {
    closeDoc(doc);
  }
  if (image) {
    closeImage(image);
  }
  if (window) {
    glfwDestroyWindow(window);
  }
}

glm::mat4 EditorImpl::calcTransform() noexcept
{
  int fbWidth = 0;
  int fbHeight = 0;
  glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

  auto docWidth  = getDocWidth(doc);
  auto docHeight = getDocHeight(doc);

  float fbAspect = float(fbWidth) / fbHeight;
  float docAspect = float(docWidth) / docHeight;

  float scaleX = zoom * (docAspect / fbAspect);
  float scaleY = zoom;

  return glm::scale(glm::vec3(scaleX, scaleY, 1.0f));
}

namespace {

/// Handles a callback from GLFW for a mouse button event.
/// Eventually passes control to @ref Editor::leftClick
/// or @ref Editor::rightClick
void mouseButtonCallback(GLFWwindow* window, int button, int action, int)
{
  Editor* editor = (Editor*) glfwGetWindowUserPointer(window);

  bool state = false;

  if (action == GLFW_PRESS) {
    state = true;
  } else if (action == GLFW_RELEASE) {
    state = false;
  } else {
    return;
  }

  if (button == GLFW_MOUSE_BUTTON_LEFT) {
    editor->leftClick(state);
  } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
    editor->rightClick(state);
  }
}

/// A callback function compatible with GLFW that passes mouse
/// movement info to @ref Editor::mouseMotion
void mouseMotionCallback(GLFWwindow* window, double x, double y)
{
  Editor* editor = (Editor*) glfwGetWindowUserPointer(window);

  editor->mouseMotion(x, y);
}

  /// Sets up a shader.
  ///
  /// @param name The name to give the shader in error reports.
  /// @param source A pointer to the shader source code.
  /// @param shaderType The type of shader being compiled.
  ///
  /// @return The ID of the shader.
GLuint setupShader(const char* name, const char* source, GLenum shaderType)
{
  GLuint id = glCreateShader(shaderType);
  if (!id) {
    return 0;
  }

  glShaderSource(id, 1, &source, nullptr);

  glCompileShader(id);

  GLint compileSuccess = GL_FALSE;

  glGetShaderiv(id, GL_COMPILE_STATUS, &compileSuccess);
  if (compileSuccess) {
    return id;
  }

  GLint logLength = 0;

  glGetShaderiv(id, GL_INFO_LOG_LENGTH, &logLength);

  char* msg = (char*) std::malloc(size_t(logLength));
  if (!msg) {
    return 0;
  }

  glGetShaderInfoLog(id, logLength, &logLength, msg);

  std::fprintf(stderr, "Error compiling '%s': %s", name, msg);

  std::free(msg);

  return id;
}

} // namespace

Editor::Editor() : impl(new EditorImpl())
{
  impl->doc = createDoc();

  impl->image = createImage(getDocWidth(impl->doc), getDocHeight(impl->doc));

  impl->mode = std::unique_ptr<Mode>(createDrawMode(this));
}

Editor::~Editor()
{
  delete impl;
}

bool Editor::createWindow()
{
  impl->window = glfwCreateWindow(640, 480, "PX Editor", NULL, NULL);
  if (!impl->window) {
    return false;
  }

  glfwSetWindowUserPointer(impl->window, this);

  glfwSetCursorPosCallback(impl->window, &mouseMotionCallback);

  glfwSetMouseButtonCallback(impl->window, &mouseButtonCallback);

  glfwMakeContextCurrent(impl->window);

  if (glewInit() != GLEW_OK) {
    return false;
  }

  IMGUI_CHECKVERSION();

  ImGui::CreateContext();

  ImGui_ImplGlfw_InitForOpenGL(impl->window, true);

  ImGui_ImplOpenGL3_Init("#version 330");

  return true;
}

Document* Editor::getDocument() noexcept { return impl->doc; }

const Document* Editor::getDocument() const noexcept { return impl->doc; }

bool Editor::initGlData()
{
  glGenTextures(1, &impl->texture);

  glBindTexture(GL_TEXTURE_2D, impl->texture);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glGenVertexArrays(1, &impl->vertexArray);
  glBindVertexArray(impl->vertexArray);

  glGenBuffers(1, &impl->vertexBuffer);
  glGenBuffers(1, &impl->elementBuffer);

  glBindBuffer(GL_ARRAY_BUFFER, impl->vertexBuffer);

  const float vertices[] {
    // position      | texture coordinates
    1.0f,  1.0f, 0.0f, 1.0f, 0.0f,
    1.0f, -1.0f, 0.0f, 1.0f, 1.0f,
   -1.0f, -1.0f, 0.0f, 0.0f, 1.0f,
   -1.0f,  1.0f, 0.0f, 0.0f, 0.0f
  };

  const unsigned int indices[] {
    0, 1, 3,
    1, 2, 3
  };

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, impl->elementBuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  impl->vertexShader = setupShader("Vertex Shader", vertexShaderSource, GL_VERTEX_SHADER);
  if (!impl->vertexShader) {
    return false;
  }

  impl->fragmentShader = setupShader("Fragment Shader", fragmentShaderSource, GL_FRAGMENT_SHADER);
  if (!impl->fragmentShader) {
    return false;
  }

  impl->program = glCreateProgram();
  glAttachShader(impl->program, impl->vertexShader);
  glAttachShader(impl->program, impl->fragmentShader);
  glLinkProgram(impl->program);

  GLint linkSuccess = GL_FALSE;

  glGetProgramiv(impl->program, GL_LINK_STATUS, &linkSuccess);
  if (!linkSuccess) {
    return false;
  }
  // glDetachShader(programID, vertexShader);
  // glDetachShader(programID, fragmentShader);
  // glDeleteShader(vertexShader);
  // glDeleteShader(fragmentShader);

  glUseProgram(impl->program);

  impl->transformLocation = glGetUniformLocation(impl->program, "transform");

  return true;
}

void Editor::iterate()
{
  glfwWaitEventsTimeout(1000.0f / 24.0f);

  renderFrame();
}

void Editor::renderFrame()
{
  // Begin rendering.

  glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  renderDocument();
  renderUi();

  // Update framebuffer size.

  int w = 0;
  int h = 0;
  glfwGetFramebufferSize(impl->window, &w, &h);

  glViewport(0, 0, w, h);

  glfwSwapBuffers(impl->window);
}

void Editor::renderMenuBar()
{
  if (ImGui::BeginMainMenuBar()) {

    renderFileMenu();
    renderEditMenu();
    renderViewMenu();

    ImGui::EndMainMenuBar();
  }
}

void Editor::renderFileMenu()
{
  if (ImGui::BeginMenu("File", true)) {

    ImGui::MenuItem("Open...");
    ImGui::MenuItem("Save");
    ImGui::MenuItem("Save As...");

    if (ImGui::MenuItem("Export") && !impl->dialog) {
      impl->dialog = std::unique_ptr<Dialog>(createExportDialog());
    }

    if (ImGui::MenuItem("Quit")) {
      glfwSetWindowShouldClose(impl->window, GLFW_TRUE);
    }

    ImGui::EndMenu();
  }
}

void Editor::renderEditMenu()
{
  if (ImGui::BeginMenu("Edit", true)) {

    ImGui::MenuItem("Undo");
    ImGui::MenuItem("Redo");

    if (ImGui::BeginMenu("Mode")) {

      if (ImGui::MenuItem("Draw")) {
        impl->mode = std::unique_ptr<Mode>(createDrawMode(this));
      }

      if (ImGui::MenuItem("Edit")) {
        impl->mode = std::unique_ptr<Mode>(createEditMode(this));
      }

      if (ImGui::MenuItem("Pose")) {
        impl->mode = std::unique_ptr<Mode>(createPoseMode(this));
      }

      ImGui::EndMenu();
    }

    ImGui::EndMenu();
  }
}

void Editor::renderViewMenu()
{
  if (ImGui::BeginMenu("View", true)) {

    if (ImGui::MenuItem("Zoom In")) {
      impl->zoom *= impl->zoomFactor;
    }

    if (ImGui::MenuItem("Zoom Out")) {
      impl->zoom /= impl->zoomFactor;
    }

    if (ImGui::BeginMenu("Color Theme")) {

      if (ImGui::MenuItem("Light")) {
        lightMode();
      }

      if (ImGui::MenuItem("Dark")) {
        darkMode();
      }

      ImGui::EndMenu();
    }

    ImGui::EndMenu();
  }
}

void Editor::renderUi()
{
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  renderMenuBar();

  if (impl->dialog) {
    if (!impl->dialog->render(this)) {
      impl->dialog.reset();
    }
  }

  if (impl->mode) {
    impl->mode->render();
  }

  ImGui::Render();

  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Editor::renderDocument()
{
  /* Get metrics */

  auto docWidth  = getDocWidth(impl->doc);
  auto docHeight = getDocHeight(impl->doc);

  /* Render document to image */

  resizeImage(impl->image, docWidth, docHeight);

  render(impl->doc, impl->image);

  /* Draw onto window */

  auto transform = impl->calcTransform();

  glUniformMatrix4fv(impl->transformLocation, 1, GL_FALSE /* no transpose */, glm::value_ptr(transform));

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, impl->texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, docWidth, docHeight, 0, GL_RGBA, GL_FLOAT, getColorBuffer(impl->image));
  glGenerateMipmap(GL_TEXTURE_2D);

  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);

  glBindBuffer(GL_ARRAY_BUFFER, impl->vertexBuffer);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*) 0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*) (sizeof(float) * 3));

  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

  glDisableVertexAttribArray(1);
  glDisableVertexAttribArray(0);
}

bool Editor::shouldExit()
{
  return !!glfwWindowShouldClose(impl->window);
}

void Editor::mouseMotion(double x, double y)
{
  if (ImGui::GetIO().WantCaptureMouse) {
    return;
  } else if (!impl->mode) {
    return;
  }

  int w = 0;
  int h = 0;
  glfwGetFramebufferSize(impl->window, &w, &h);

  auto transform = impl->calcTransform();

  glm::vec4 size = transform * glm::vec4(w, h, 0, 1);

  glm::vec2 offset((float(w) - size.x) * 0.5f,
                   (float(h) - size.y) * 0.5f);

  glm::vec2 min(offset.x, offset.y);

  glm::vec2 max = min + glm::vec2(size.x, size.y);

  if ((x < min.x)
   || (y < min.y)
   || (x >= max.x)
   || (y >= max.y)) {
    return;
  }

  x = ((x - min.x) / size.x) * float(getDocWidth(impl->doc));
  y = ((y - min.y) / size.y) * float(getDocHeight(impl->doc));

  impl->mode->mouseMotion(unsigned(x), unsigned(y));
}

void Editor::leftClick(bool state)
{
  if (ImGui::GetIO().WantCaptureMouse) {
    return;
  }

  if (impl->mode) {
    impl->mode->leftClick(state);
  }
}

void Editor::rightClick(bool state)
{
  if (ImGui::GetIO().WantCaptureMouse) {
    return;
  }

  if (impl->mode) {
    impl->mode->rightClick(state);
  }
}

bool Editor::savePNG(const char* filename) const noexcept
{
  return px::savePNG(filename, impl->image);
}

void Editor::lightMode()
{
  ImGui::StyleColorsLight();
}

void Editor::darkMode()
{
  ImGui::StyleColorsDark();
}

namespace {

const char* vertexShaderSource = R"(
#version 330 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 texCoord;

uniform mat4 transform = mat4(1.0);

out vec2 texCoord2;

void main() {
  gl_Position = transform * vec4(pos, 1);
  texCoord2 = texCoord;
}
)";

const char* fragmentShaderSource = R"(
#version 330 core

out vec4 color;

in vec2 texCoord2;

uniform sampler2D imageTexture;

void main() {
  color = texture(imageTexture, texCoord2);
}
)";

} // namespace

} // namespace px