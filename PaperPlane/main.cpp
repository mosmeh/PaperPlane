#include "stdafx.h"

constexpr int WIDTH = 800;
constexpr int HEIGHT = 600;

int main() {
	glfwSetErrorCallback([] (int code, const char* msg) {
		std::cerr << "GLFW: " << code << " " << msg << std::endl;
		exit(EXIT_FAILURE);
	});
	glfwInit();
	std::atexit([] {
		glfwTerminate();
	});
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	const auto window = glfwCreateWindow(WIDTH, HEIGHT, "PaperPlane", nullptr, nullptr);
	glfwMakeContextCurrent(window);
	if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
		std::cerr << "glad: failed to load" << std::endl;
		exit(EXIT_FAILURE);
	}

	while (!glfwWindowShouldClose(window)) {
		glClear(GL_COLOR_BUFFER_BIT);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	return EXIT_SUCCESS;
}