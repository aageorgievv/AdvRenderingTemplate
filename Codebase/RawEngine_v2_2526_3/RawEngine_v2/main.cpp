#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <algorithm>
#include <vector>
#include <chrono>
#include <random>

//#define MAC_CLION
#define VSTUDIO

#ifdef MAC_CLION
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#endif

#ifdef VSTUDIO
// Note: install imgui with:
//     ./vcpkg.exe install imgui[glfw-binding,opengl3-binding]
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#endif

int g_width = 1280;
int g_height = 720;

void processInput(GLFWwindow* window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
}

void framebufferSizeCallback(GLFWwindow* window,
	int width, int height) {
	g_width = width;
	g_height = height;
	glViewport(0, 0, width, height);
}

//My code -------------------------------------------------------------------------------------------

struct Body2D {
	glm::vec2 position;
	glm::vec2 halfSize;
};

struct CollisionStats {
	int candidatePairs = 0;
	int collisionPairs = 0;
};


bool Overlaps(const Body2D& a, const Body2D& b) {
	float aMinX = a.position.x - a.halfSize.x;
	float aMaxX = a.position.x + a.halfSize.x;
	float aMinY = a.position.y - a.halfSize.y;
	float aMaxY = a.position.y + a.halfSize.y;

	float bMinX = b.position.x - b.halfSize.x;
	float bMaxX = b.position.x + b.halfSize.x;
	float bMinY = b.position.y - b.halfSize.y;
	float bMaxY = b.position.y + b.halfSize.y;

	return aMinX <= bMaxX && aMaxX >= bMinX &&
		aMinY <= bMaxY && aMaxY >= bMinY;
}

CollisionStats RunBruteForce(const std::vector<Body2D>& bodies) {

	CollisionStats stats;

	for (int i = 0; i < (int)bodies.size(); ++i) 
	{
		for (int j = i + 1; j < bodies.size(); ++j) 
		{
			stats.candidatePairs++;
			if (Overlaps(bodies[i], bodies[j])) {
				stats.collisionPairs++;
			}
		}
	}

	return stats;
}

std::vector<Body2D> GenerateBodiesGrid(int count) {

	std::vector<Body2D> bodies;
	bodies.reserve(count);

	for (int i = 0; i < count; i++) 
	{
		float x = -5.0f + (i % 20) * 0.5f;
		float y = -5.0f + (i / 20) * 0.5f;

		Body2D body;
		body.position = glm::vec2(x, y);
		body.halfSize = glm::vec2(0.2f, 0.2f);

		bodies.push_back(body);
	}

	return bodies;
}

std::vector<Body2D> GenerateBodiesRandom(int count, unsigned int seed) {
	std::vector<Body2D> bodies;
	bodies.reserve(count);

	std::mt19937 rng(seed);
	std::uniform_real_distribution<float> xDist(-5.0f, 5.0f);
	std::uniform_real_distribution<float> yDist(-5.0f, 5.0f);

	for (int i = 0; i < count; i++)
	{
		Body2D body;
		body.position = glm::vec2(xDist(rng), yDist(rng));
		body.halfSize = glm::vec2(0.2f, 0.2f);
		bodies.push_back(body);
	}

	return bodies;
}

std::vector<Body2D> GenerateBodiesClustered(int count, unsigned int seed) {
	std::vector<Body2D> bodies;
	bodies.reserve(count);

	std::mt19937 rng(seed);
	std::uniform_real_distribution<float> centerDist(-4.0f, 4.0f);
	std::uniform_real_distribution<float> offsetDist(-0.8f, 0.8f);

	const int clusterCount = 4;
	std::vector<glm::vec2> centers;
	centers.reserve(clusterCount);

	for (int i = 0; i < clusterCount; ++i) 
	{
		centers.push_back(glm::vec2(centerDist(rng), centerDist(rng)));
	}

	for (int i = 0; i < count; ++i) 
	{
		int clusterIndex = i % clusterCount;

		Body2D body;
		body.position = centers[clusterIndex] + glm::vec2(offsetDist(rng), offsetDist(rng));
		body.halfSize = glm::vec2(0.2f, 0.2f);

		bodies.push_back(body);
	}
	return bodies;
}


int main() {
	glfwInit();
	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	GLFWwindow* window = glfwCreateWindow(g_width, g_height, "LearnOpenGL", NULL, NULL);

	if (window == NULL) {
		printf("Failed to create GLFW window\n");
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);

	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		printf("Failed to initialize GLAD\n");
		return -1;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	//Setup platforms
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 400");

	glEnable(GL_DEPTH_TEST);
	glFrontFace(GL_CCW);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glm::vec4 clearColor = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
	glClearColor(clearColor.r,
		clearColor.g, clearColor.b, clearColor.a);

	//-------------------------------------

	int distributionMode = 0; // 0 = grid , 1 = random
	unsigned int seed = 44;
	int bodyCount = 100;

	std::vector<Body2D> bodies = GenerateBodiesGrid(bodyCount);

	while (!glfwWindowShouldClose(window)) {

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();

		ImGui::NewFrame();

		auto start = std::chrono::high_resolution_clock::now();
		CollisionStats stats = RunBruteForce(bodies);
		auto end = std::chrono::high_resolution_clock::now();
		double bruteForceMs = std::chrono::duration<double, std::milli >(end - start).count();

		ImGui::Begin("Raw Engine v2");
		ImGui::Text("Collision Test");
		const char* distributionItems[] = { "Grid", "Random", "Clustered"};
		ImGui::Combo("Distribution", &distributionMode, distributionItems, 3);
		ImGui::InputInt("Random Seed", (int*)&seed);
		ImGui::SliderInt("Body Count", &bodyCount, 1, 5000);

		if (ImGui::Button("Regenerate Bodies")) {
			if (distributionMode == 0) {
				bodies = GenerateBodiesGrid(bodyCount);
			}
			else if (distributionMode == 1) {
				bodies = GenerateBodiesRandom(bodyCount, seed);
			}
			else if (distributionMode == 2) {
				bodies = GenerateBodiesClustered(bodyCount, seed);
			}
		}

		ImGui::Text("Candidate pairs: %d", stats.candidatePairs);
		ImGui::Text("Collision pairs: %d", stats.collisionPairs);
		ImGui::Separator();
		ImGui::Text("Brute force time: %.4f ms", bruteForceMs);
		ImGui::Separator();

		int previewCount = std::min((int)bodies.size(), 5);
		for (int i = 0; i < previewCount; ++i) {
			ImGui::Text("Body %d: pos(%.2f, %.2f) halfSize(%.2f, %.2f)",
				i,
				bodies[i].position.x,
				bodies[i].position.y,
				bodies[i].halfSize.x,
				bodies[i].halfSize.y);
		}

		ImGui::End();

		processInput(window);

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwTerminate();
	return 0;
}