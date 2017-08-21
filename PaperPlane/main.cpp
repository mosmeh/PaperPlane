#include "stdafx.h"

constexpr int WINDOW_SIZE = 600;
const float HOLE_WIDTH = 0.25f;
const float SIDE_WALL_WIDTH = 0.1f;
const float BARRIER_INTERVAL = 0.5f;
const float INITIAL_BARRIER_HEIGHT = 0.1f;
const float PLANE_POS_Y = 0.2f;
const unsigned int MAX_DIRECTION = 2;

GLuint compileShader(const char* filename, GLenum type) {
	const GLuint shader = glCreateShader(type);
	std::ifstream ifs(filename);
	if (!ifs) {
		std::cerr << "shader: could not open file" << std::endl;
		exit(EXIT_FAILURE);
	}

	std::ostringstream ss;
	ss << ifs.rdbuf();
	
	const auto code = ss.str();
	const auto str = code.c_str();
	const GLint size = code.size();
	glShaderSource(shader, 1, &str, &size);
	glCompileShader(shader);

	GLint infoLogLength;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);

	if (infoLogLength > 0) {
		std::vector<char> infoLog(infoLogLength);
		glGetShaderInfoLog(shader, infoLogLength, nullptr, infoLog.data());

		std::cerr << "shader: " << infoLog.data() << std::endl;
	}

	GLint compileStatus;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
	if (compileStatus != GL_TRUE) {
		std::cerr << "shader: compile failed" << std::endl;
		exit(EXIT_FAILURE);
	}

	return shader;
}

GLuint linkProgram(const std::vector<GLuint>& shaders) {
	const GLuint program = glCreateProgram();
	for (const auto shader : shaders) {
		glAttachShader(program, shader);
	}
	glLinkProgram(program);
	for (const auto shader : shaders) {
		glDetachShader(program, shader);
	}
	GLint infoLogLength;
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);

	if (infoLogLength > 0) {
		std::vector<char> infoLog(infoLogLength);
		glGetProgramInfoLog(program, infoLogLength, nullptr, infoLog.data());

		std::cerr << "program: " << infoLog.data() << std::endl;
	}

	GLint linkStatus;
	glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
	if (linkStatus != GL_TRUE) {
		std::cerr << "program: link failed" << std::endl;
		exit(EXIT_FAILURE);
	}

	return program;
}

struct Barrier {
	enum class Type : uint32_t {
		Left = 0,
		Right = 1,
		Slit = 2
	};

	const Type type;
	const float leftToHole, height;
	float yPos;

	bool hit(float x) const {
		switch (type) {
		case Type::Left:
			if (x > leftToHole) {
				return false;
			}
			break;
		case Type::Right:
			if (x < leftToHole) {
				return false;
			}
			break;
		case Type::Slit:
			if (leftToHole < x && x < leftToHole + HOLE_WIDTH) {
				return false;
			}
			break;
		}
		return yPos < PLANE_POS_Y && PLANE_POS_Y < yPos + height;
	}

	bool isVisible() const {
		return yPos > -height;
	}
};

class Level {
public:
	Level() :
		TYPE_DIST(0, 2),
		POS_DIST(0.4f, 0.6f),
		mileage_(0.f) {

		std::random_device randomDevice;
		randomEngine_ = std::mt19937(randomDevice());

		const GLuint barrierVert = compileShader("barrier.vert", GL_VERTEX_SHADER);
		const GLuint barrierGeom = compileShader("barrier.geom", GL_GEOMETRY_SHADER);
		const GLuint barrierFrag = compileShader("barrier.frag", GL_FRAGMENT_SHADER);
		barrierProgram_ = linkProgram({barrierVert, barrierGeom, barrierFrag});
		glDeleteShader(barrierVert);
		glDeleteShader(barrierGeom);
		glDeleteShader(barrierFrag);

		const GLuint fullVert = compileShader("fullscreen.vert", GL_VERTEX_SHADER);
		const GLuint backFrag = compileShader("background.frag", GL_FRAGMENT_SHADER);
		backgroundProgram_ = linkProgram({fullVert, backFrag});
		glDeleteShader(fullVert);
		glDeleteShader(backFrag);

		glGenVertexArrays(1, &vertexArray_);
		glBindVertexArray(vertexArray_);

		glGenBuffers(1, &vertexBuffer_);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);

		glEnableVertexAttribArray(0);
		glVertexAttribIPointer(0, 1, GL_UNSIGNED_INT, sizeof(Barrier), nullptr);

		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(Barrier), reinterpret_cast<GLvoid*>(offsetof(Barrier, leftToHole)));

		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Barrier), reinterpret_cast<GLvoid*>(offsetof(Barrier, height)));

		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Barrier), reinterpret_cast<GLvoid*>(offsetof(Barrier, yPos)));

		float height = 0;
		while (height < 1.f) {
			addBarrier();
			height += BARRIER_INTERVAL;
		}
	}

	virtual ~Level() {
		glDeleteProgram(barrierProgram_);
		glDeleteProgram(backgroundProgram_);
		glDeleteBuffers(1, &vertexBuffer_);
		glDeleteVertexArrays(1, &vertexArray_);
	}


	void update(float speedY) {
		mileage_ += speedY;

		for (auto& barrier : barriers_) {
			barrier.yPos -= speedY;
		}
		if (!barriers_.front().isVisible()) {
			barriers_.pop_front();
			addBarrier();
		}
	}

	void draw(float planePosX) const {
		const std::vector<Barrier> buf(barriers_.begin(), barriers_.end());
		glBufferData(GL_ARRAY_BUFFER, buf.size() * sizeof(Barrier), buf.data(), GL_DYNAMIC_DRAW);

		glUseProgram(backgroundProgram_);
		glUniform1f(glGetUniformLocation(backgroundProgram_, "x"), planePosX);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		glUseProgram(barrierProgram_);
		glDrawArrays(GL_POINTS, 0, barriers_.size());
	}

	bool hit(float posX) const {
		if (posX < SIDE_WALL_WIDTH || posX > 1.f - SIDE_WALL_WIDTH) {
			return true;
		}
		for (const auto& barrier : barriers_) {
			if (barrier.hit(posX)) {
				return true;
			}
		}
		return false;
	}

	float getMileage() const {
		return mileage_;
	}

private:
	const std::uniform_int_distribution<uint32_t> TYPE_DIST;
	const std::uniform_real_distribution<float> POS_DIST;
	std::mt19937 randomEngine_;
	GLuint backgroundProgram_, barrierProgram_, vertexArray_, vertexBuffer_;
	std::deque<Barrier> barriers_ = {{Barrier::Type::Slit, 0.5f - HOLE_WIDTH / 2, INITIAL_BARRIER_HEIGHT, 1.f}};
	float mileage_;

	void addBarrier() {
		auto type = Barrier::Type::Left;
		float leftToHole = 0.f;

		switch (barriers_.back().type) {
		case Barrier::Type::Left:
			type = std::bernoulli_distribution()(randomEngine_) ? Barrier::Type::Right : Barrier::Type::Slit;
			leftToHole = POS_DIST(randomEngine_);
			break;
		case Barrier::Type::Right:
			type = std::bernoulli_distribution()(randomEngine_) ? Barrier::Type::Left : Barrier::Type::Slit;
			leftToHole = 1.f - POS_DIST(randomEngine_);
			break;
		case Barrier::Type::Slit:
			type = Barrier::Type(TYPE_DIST(randomEngine_));
			leftToHole = POS_DIST(randomEngine_) - HOLE_WIDTH / 2;
			break;
		}

		const float factor = std::min(mileage_ / 100.f, 1.f);
		const float height = INITIAL_BARRIER_HEIGHT + factor * (0.3f - INITIAL_BARRIER_HEIGHT);

		barriers_.emplace_back<Barrier>({type, leftToHole, height, barriers_.back().yPos + BARRIER_INTERVAL});
	}
};

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
	const auto window = glfwCreateWindow(WINDOW_SIZE, WINDOW_SIZE, "PaperPlane", nullptr, nullptr);
	glfwSetWindowAspectRatio(window, 1, 1);
	glfwMakeContextCurrent(window);
	if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
		std::cerr << "glad: failed to load" << std::endl;
		exit(EXIT_FAILURE);
	}

#ifdef _DEBUG
	glad_set_post_callback([](const char* name, void*, int, ...) {
		const GLenum errorCode = glad_glGetError();
		if (errorCode == GL_NO_ERROR) {
			return;
		}

#define GL_STACK_OVERFLOW 0x0503
#define GL_STACK_UNDERFLOW 0x0504
#define LOG_GL_ERROR(code) case code: std::cerr << name << ": " << #code << std::endl; return;

		switch (errorCode) {
			LOG_GL_ERROR(GL_INVALID_ENUM);
			LOG_GL_ERROR(GL_INVALID_VALUE);
			LOG_GL_ERROR(GL_INVALID_OPERATION);
			LOG_GL_ERROR(GL_STACK_OVERFLOW);
			LOG_GL_ERROR(GL_STACK_UNDERFLOW);
			LOG_GL_ERROR(GL_OUT_OF_MEMORY);
			LOG_GL_ERROR(GL_INVALID_FRAMEBUFFER_OPERATION);
		default:
			std::cerr << name << ": unknown error (" << errorCode << ")" << std::endl;
			return;
		}

#undef GL_STACK_OVERFLOW
#undef GL_STACK_UNDERFLOW
#undef LOG_GL_ERROR
	});
#endif

	glfwSetFramebufferSizeCallback(window, [](GLFWwindow*, int width, int height) {
		glViewport(0, 0, width, height);
	});

	float planePosX = 0.5f;
	int direction = 0;
	Level level;

	glfwSwapInterval(1); // FIXME
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_TRUE);

	bool leftRepeated = false, rightRepeated = false;
	while (!glfwWindowShouldClose(window)) {
		if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
			if (!leftRepeated) {
				direction = std::max(direction - 1, -static_cast<int>(MAX_DIRECTION));
				leftRepeated = true;
			}
		} else {
			leftRepeated = false;
		}
		if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
			if (!rightRepeated) {
				direction = std::min(direction + 1, static_cast<int>(MAX_DIRECTION));
				rightRepeated = true;
			}
		} else {
			rightRepeated = false;
		}

		static constexpr float PI = 3.141593f;
		static constexpr std::array<float, MAX_DIRECTION + 1> ANGLES = {
			0.f, PI / 6.f, PI / 3.f
		};
		const float angle = (direction > 0 ? 1 : -1) * ANGLES[std::abs(direction)];

		const float planeSpeed = 2e-4f * level.getMileage() + 5e-3f;
		planePosX += std::sin(angle) * planeSpeed;

		if (level.hit(planePosX)) {
			exit(0);
		}

		level.update(std::cos(angle) * planeSpeed);

		const auto score = static_cast<unsigned int>(level.getMileage() * 3.f);
		std::stringstream ss;
		ss << "Paper Plane / Score: " << score;
		glfwSetWindowTitle(window, ss.str().c_str());

		glClear(GL_COLOR_BUFFER_BIT);
		level.draw(planePosX);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	return EXIT_SUCCESS;
}
