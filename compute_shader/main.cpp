#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <vector>

// --- Utility: Compile a shader and check errors ---
GLuint compileShader(const char *source, GLenum type) {
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, nullptr);
  glCompileShader(shader);

  GLint success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    char log[512];
    glGetShaderInfoLog(shader, 512, nullptr, log);
    std::cerr << "Shader Compilation Error:\n" << log << std::endl;
  }
  return shader;
}

// --- Utility: Create a compute program ---
GLuint createComputeProgram(const char *computeSrc) {
  GLuint shader = compileShader(computeSrc, GL_COMPUTE_SHADER);
  GLuint program = glCreateProgram();
  glAttachShader(program, shader);
  glLinkProgram(program);

  GLint success;
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success) {
    char log[512];
    glGetProgramInfoLog(program, 512, nullptr, log);
    std::cerr << "Program Linking Error:\n" << log << std::endl;
  }

  glDeleteShader(shader);
  return program;
}

// --- GLSL Compute Shader Source ---
const char *computeShaderSrc = R"(
#version 430

layout(local_size_x = 256) in; // 256 threads per work group

layout(std430, binding = 0) buffer InputA { float A[]; };
layout(std430, binding = 1) buffer InputB { float B[]; };
layout(std430, binding = 2) buffer OutputC { float C[]; };

void main() {
    uint i = gl_GlobalInvocationID.x;
    C[i] = A[i] + B[i];
}
)";

int main() {
  // --- Init GLFW ---
  if (!glfwInit()) {
    std::cerr << "Failed to init GLFW\n";
    return -1;
  }
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // invisible window (compute only)
  GLFWwindow *window =
      glfwCreateWindow(1, 1, "Compute Shader", nullptr, nullptr);
  glfwMakeContextCurrent(window);

  // --- Init GLEW ---
  if (glewInit() != GLEW_OK) {
    std::cerr << "Failed to init GLEW\n";
    return -1;
  }

  // --- Example Data ---
  const int N = 1024;
  std::vector<float> A(N), B(N), C(N);
  for (int i = 0; i < N; i++) {
    A[i] = i * 1.0f;
    B[i] = (N - i) * 0.5f;
  }

  // --- Create GPU Buffers ---
  GLuint ssbo[3];
  glGenBuffers(3, ssbo);

  // A
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo[0]);
  glBufferData(GL_SHADER_STORAGE_BUFFER, N * sizeof(float), A.data(),
               GL_STATIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo[0]);

  // B
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo[1]);
  glBufferData(GL_SHADER_STORAGE_BUFFER, N * sizeof(float), B.data(),
               GL_STATIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo[1]);

  // C (output)
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo[2]);
  glBufferData(GL_SHADER_STORAGE_BUFFER, N * sizeof(float), nullptr,
               GL_STATIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo[2]);

  // --- Create and Use Compute Program ---
  GLuint program = createComputeProgram(computeShaderSrc);
  glUseProgram(program);

  // Dispatch compute shader (N threads total, grouped in 256s)
  int groups = (N + 255) / 256; // ceil(N/256)
  glDispatchCompute(groups, 1, 1);

  // Make sure GPU has finished writing to the buffer
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

  // --- Read Back Results ---
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo[2]);
  float *ptr = (float *)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
  if (ptr) {
    for (int i = 0; i < 10; i++) { // print first 10 results
      std::cout << "C[" << i << "] = " << ptr[i] << "\n";
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
  }

  // --- Cleanup ---
  glDeleteProgram(program);
  glDeleteBuffers(3, ssbo);
  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
