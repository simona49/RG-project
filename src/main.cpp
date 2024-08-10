#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <learnopengl/filesystem.h>
#include <learnopengl/model.h>
#include <learnopengl/shader.h>
#include <rg/Camera.h>
#include <rg/service_locator.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action,
		  int mods);

auto loadTexture(char const *path) -> unsigned int;

void renderQuad();

// settings
const unsigned int SCR_WIDTH = 1000;
const unsigned int SCR_HEIGHT = 800;

// camera

float lastX = SCR_WIDTH / 2.0F;
float lastY = SCR_HEIGHT / 2.0F;
bool firstMouse = true;

// timing
float deltaTime = 0.0F;
float lastFrame = 0.0F;

struct PointLight {
      glm::vec3 position;
      glm::vec3 ambient;
      glm::vec3 diffuse;
      glm::vec3 specular;

      float constant;
      float linear;
      float quadratic;
};

struct DirLight {
      glm::vec3 direction;

      glm::vec3 ambient;
      glm::vec3 diffuse;
      glm::vec3 specular;
};

struct ProgramState {
      glm::vec3 clearColor = glm::vec3(0.7F, 0.0F, 0.3F);
      bool ImGuiEnabled = true;
      Camera camera;
      bool CameraMouseMovementUpdateEnabled = true;

      // positions
      glm::vec3 plantPosition = glm::vec3(0.5F, 0.7F, 0.1F);
      glm::vec3 tablePosition = glm::vec3(-0.6F);
      glm::vec3 planePosition = glm::vec3(0.5F, 0.5F, 0.5F);

      // indicators
      bool Blinn = true;
      bool pointLightInd = true;
      bool grayScaleInd = false;

      float plantScale = 0.1F;
      float tableScale = 5.0F;
      float heightScale = 0.08F;

      PointLight pointLight;
      DirLight dirLight;
      ProgramState() : camera(glm::vec3(0.0F, 0.0F, 1.0F)) {}

      void SaveToFile(std::string filename) const;
      void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename) const
{
      std::ofstream out(filename);
      out << clearColor.r << '\n'
	  << clearColor.g << '\n'
	  << clearColor.b << '\n'
	  << ImGuiEnabled << '\n'
	  << plantPosition.x << '\n'
	  << plantPosition.y << '\n'
	  << plantPosition.z << '\n'
	  << plantScale << '\n'
	  << tablePosition.x << '\n'
	  << tablePosition.y << '\n'
	  << tablePosition.z << '\n'
	  << tableScale << '\n'
	  << planePosition.x << '\n'
	  << planePosition.y << '\n'
	  << planePosition.z << '\n'
	  << camera.Position.x << '\n'
	  << camera.Position.y << '\n'
	  << camera.Position.z << '\n'
	  << camera.Front.x << '\n'
	  << camera.Front.y << '\n'
	  << camera.Front.z << '\n';
}

void ProgramState::LoadFromFile(std::string filename)
{
      std::ifstream in(filename);
      if (in) {
	    in >> clearColor.r >> clearColor.g >> clearColor.b >>
		ImGuiEnabled >> plantPosition.x >> plantPosition.y >>
		plantPosition.z >> plantScale >> tablePosition.x >>
		tablePosition.y >> tablePosition.z >> tableScale >>
		planePosition.x >> planePosition.y >> planePosition.z >>
		camera.Position.x >> camera.Position.y >> camera.Position.z >>
		camera.Front.x >> camera.Front.y >> camera.Front.z;
      }
}

ProgramState *programState;

void DrawImGui(ProgramState *programState);

auto main() -> int
{
      // glfw: initialize and configure
      // ------------------------------
      glfwInit();
      glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
      glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
      glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
      // glfwWindowHint(GLFW_SAMPLES, 4);

#ifdef __APPLE__
      glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

      // glfw window creation
      // --------------------
      GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT,
					    "LearnOpenGL", nullptr, nullptr);
      if (window == nullptr) {
	    std::cout << "Failed to create GLFW window" << std::endl;
	    glfwTerminate();
	    return -1;
      }
      glfwMakeContextCurrent(window);
      glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
      glfwSetCursorPosCallback(window, mouse_callback);
      glfwSetScrollCallback(window, scroll_callback);
      glfwSetKeyCallback(window, key_callback);

      // tell GLFW to capture our mouse
      //    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

      // glad: load all OpenGL function pointers
      // ---------------------------------------
      if (gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) == 0) {
	    std::cout << "Failed to initialize GLAD" << std::endl;
	    return -1;
      }

      // tell stb_image.h to flip loaded texture's on the y-axis (before loading
      // model).
      stbi_set_flip_vertically_on_load(1);

      programState = new ProgramState;
      programState->LoadFromFile("resources/program_state.txt");
      if (programState->ImGuiEnabled) {
	    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
      }
      // Init Imgui
      IMGUI_CHECKVERSION();
      ImGui::CreateContext();
      ImGuiIO &io = ImGui::GetIO();
      (void)io;

      ImGui_ImplGlfw_InitForOpenGL(window, true);
      ImGui_ImplOpenGL3_Init("#version 330 core");

      // configure global opengl state
      // -----------------------------
      glEnable(GL_DEPTH_TEST);

      // build and compile shaders
      // -------------------------
      Shader ourShader("resources/shaders/2.model_lighting.vs",
		       "resources/shaders/2.model_lighting.fs");
      Shader tableShader("resources/shaders/table.vs",
			 "resources/shaders/table.fs");
      Shader cubeShader("resources/shaders/lightCube.vs",
			"resources/shaders/lightCube.fs");
      Shader shader("resources/shaders/blending.vs",
		    "resources/shaders/blending.fs");
      Shader screenShader("resources/shaders/screen.vs",
			  "resources/shaders/screen.fs");
      Shader planeShader("resources/shaders/plane.vs",
			 "resources/shaders/plane.fs");

      float cubeVertices[] = {
	  -0.5F, -0.5F, -0.5F, 0.5F,  -0.5F, -0.5F, 0.5F,  0.5F,  -0.5F,
	  0.5F,	 0.5F,	-0.5F, -0.5F, 0.5F,  -0.5F, -0.5F, -0.5F, -0.5F,

	  -0.5F, -0.5F, 0.5F,  0.5F,  0.5F,  0.5F,  0.5F,  -0.5F, 0.5F,
	  0.5F,	 0.5F,	0.5F,  -0.5F, -0.5F, 0.5F,  -0.5F, 0.5F,  0.5F,

	  -0.5F, 0.5F,	0.5F,  -0.5F, -0.5F, -0.5F, -0.5F, 0.5F,  -0.5F,
	  -0.5F, -0.5F, -0.5F, -0.5F, 0.5F,  0.5F,  -0.5F, -0.5F, 0.5F,

	  0.5F,	 0.5F,	0.5F,  0.5F,  0.5F,  -0.5F, 0.5F,  -0.5F, -0.5F,
	  0.5F,	 -0.5F, -0.5F, 0.5F,  -0.5F, 0.5F,  0.5F,  0.5F,  0.5F,

	  -0.5F, -0.5F, -0.5F, 0.5F,  -0.5F, 0.5F,  0.5F,  -0.5F, -0.5F,
	  0.5F,	 -0.5F, 0.5F,  -0.5F, -0.5F, -0.5F, -0.5F, -0.5F, 0.5F,

	  -0.5F, 0.5F,	-0.5F, 0.5F,  0.5F,  -0.5F, 0.5F,  0.5F,  0.5F,
	  0.5F,	 0.5F,	0.5F,  -0.5F, 0.5F,  0.5F,  -0.5F, 0.5F,  -0.5F};

      float transparentVertices[] = {
	  // positions                       // texture Coords
	  0.0F, 0.5F, 0.0F, 0.0F,  0.0F, 0.0F, -0.5F, 0.0F,
	  0.0F, 1.0F, 1.0F, -0.5F, 0.0F, 1.0F, 1.0F,

	  0.0F, 0.5F, 0.0F, 0.0F,  0.0F, 1.0F, -0.5F, 0.0F,
	  1.0F, 1.0F, 1.0F, 0.5F,  0.0F, 1.0F, 0.0F};

      float quadVertices[] = {// vertex attributes for a quad that fills the
			      // entire screen in Normalized Device Coordinates.
			      // positions            // texCoords
			      -1.0F, 1.0F, 0.0F, 1.0F,	-1.0F, -1.0F,
			      0.0F,  0.0F, 1.0F, -1.0F, 1.0F,  0.0F,

			      -1.0F, 1.0F, 0.0F, 1.0F,	1.0F,  -1.0F,
			      1.0F,  0.0F, 1.0F, 1.0F,	1.0F,  1.0F};

      // light cube
      unsigned int VAO;
      unsigned int VBO;
      glGenVertexArrays(1, &VAO);
      glGenBuffers(1, &VBO);
      glBindVertexArray(VAO);
      glBindBuffer(GL_ARRAY_BUFFER, VBO);
      glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), &cubeVertices,
		   GL_STATIC_DRAW);
      glEnableVertexAttribArray(0);
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
			    (void *)nullptr);

      // transparent VAO
      unsigned int transparentVAO;
      unsigned int transparentVBO;
      glGenVertexArrays(1, &transparentVAO);
      glGenBuffers(1, &transparentVBO);
      glBindVertexArray(transparentVAO);
      glBindBuffer(GL_ARRAY_BUFFER, transparentVBO);
      glBufferData(GL_ARRAY_BUFFER, sizeof(transparentVertices),
		   transparentVertices, GL_STATIC_DRAW);
      glEnableVertexAttribArray(0);
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
			    (void *)nullptr);
      glEnableVertexAttribArray(1);
      glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
			    (void *)(3 * sizeof(float)));
      glBindVertexArray(0);

      vector<glm::vec3> vegetation{
	  glm::vec3(-1.5F, -1.6F, -0.48F), glm::vec3(1.5F, -1.6F, 0.51F),
	  glm::vec3(-3.0F, -1.6F, 0.7F),   glm::vec3(-0.3F, -1.6F, -2.3F),
	  glm::vec3(5.5F, -1.6F, -0.6F),   glm::vec3(2.0F, -1.6F, 2.9F),
	  glm::vec3(2.5F, -1.6F, -2.0F),   glm::vec3(2.5F, -1.6F, -6.0F),
	  glm::vec3(0.0F, -1.6F, -6.0F),   glm::vec3(-2.5F, -1.6F, -6.0F),
	  glm::vec3(-5.0F, -1.6F, -7.0F),  glm::vec3(-8.0F, -1.6F, -8.0F),
	  glm::vec3(-7.5F, -1.6F, -4.0F),  glm::vec3(-5.0F, -1.6F, -2.0F),
	  glm::vec3(-8.5F, -1.6F, 0.0F),   glm::vec3(-6.0F, -1.6F, 2.0F),
	  glm::vec3(-4.5F, -1.6F, 5.0F),   glm::vec3(-8.5F, -1.6F, 3.0F),
	  glm::vec3(-2.5F, -1.6F, 3.0F),   glm::vec3(0.0F, -1.6F, 5.0F),
	  glm::vec3(0.0F, -1.6F, 2.5F),	   glm::vec3(6.0F, -1.6F, 5.5F),
	  glm::vec3(4.5F, -1.6F, 4.0F),	   glm::vec3(3.5F, -1.6F, 2.0F),
	  glm::vec3(4.5F, -1.6F, -3.0F),   glm::vec3(5.5F, -1.6F, -6.0F),
      };

      // loading textures
      unsigned int transparentTexture =
	  loadTexture("resources/textures/grass.png");

      unsigned int diffuseMap = loadTexture("resources/textures/ground.jpg");
      unsigned int normalMap =
	  loadTexture("resources/textures/ground_normal.jpg");
      unsigned int heightMap =
	  loadTexture("resources/textures/ground_disp.png");

      shader.use();
      shader.setInt("texture1", 0);
      planeShader.use();
      planeShader.setInt("diffuseMap", 0);
      planeShader.setInt("normalMap", 1);
      planeShader.setInt("depthMap", 2);

      // setup screen VAO
      unsigned int quadVAO;
      unsigned int quadVBO;
      glGenVertexArrays(1, &quadVAO);
      glGenBuffers(1, &quadVBO);
      glBindVertexArray(quadVAO);
      glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
      glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices,
		   GL_STATIC_DRAW);
      glEnableVertexAttribArray(0);
      glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
			    (void *)nullptr);
      glEnableVertexAttribArray(1);
      glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
			    (void *)(2 * sizeof(float)));

      // MSAA framebuffer
      unsigned int framebuffer;
      glGenFramebuffers(1, &framebuffer);
      glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

      unsigned int textureColorBufferMultiSampled;
      glGenTextures(1, &textureColorBufferMultiSampled);
      glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textureColorBufferMultiSampled);
      glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGB, SCR_WIDTH,
			      SCR_HEIGHT, GL_TRUE);
      glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			     GL_TEXTURE_2D_MULTISAMPLE,
			     textureColorBufferMultiSampled, 0);

      unsigned int rbo;
      glGenRenderbuffers(1, &rbo);
      glBindRenderbuffer(GL_RENDERBUFFER, rbo);
      glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8,
				       SCR_WIDTH, SCR_HEIGHT);
      glBindRenderbuffer(GL_RENDERBUFFER, 0);
      glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
				GL_RENDERBUFFER, rbo);

      // check if framebuffer is completed
      if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
	    cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << endl;
      }
      // deactivate framebuffer
      glBindFramebuffer(GL_FRAMEBUFFER, 0);

      // configure second post-processing framebuffer
      unsigned int intermediateFBO;
      glGenFramebuffers(1, &intermediateFBO);
      glBindFramebuffer(GL_FRAMEBUFFER, intermediateFBO);

      unsigned int screenTexture;
      glGenTextures(1, &screenTexture);
      glBindTexture(GL_TEXTURE_2D, screenTexture);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB,
		   GL_UNSIGNED_BYTE, nullptr);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			     GL_TEXTURE_2D, screenTexture, 0);

      if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
	    cout << "ERROR::FRAMEBUFFER:: Intermediate framebuffer is not "
		    "complete!"
		 << endl;
      }
      glBindFramebuffer(GL_FRAMEBUFFER, 0);

      // load models
      // -----------
      Model ourModel("resources/objects/rock/potted_plant_obj.obj");
      ourModel.SetShaderTextureNamePrefix("material.");

      Model tableModel("resources/objects/table/table.obj");
      tableModel.SetShaderTextureNamePrefix("material.");

      PointLight &pointLight = programState->pointLight;
      pointLight.ambient = glm::vec3(0.1, 0.1, 0.1);
      pointLight.diffuse = glm::vec3(0.8, 0.8, 0.8);
      pointLight.specular = glm::vec3(0.7, 0.7, 0.7);

      pointLight.constant = 0.5F;
      pointLight.linear = 0.09F;
      pointLight.quadratic = 0.032F;

      DirLight &dirLight = programState->dirLight;

      dirLight.direction = glm::vec3(-0.2F, -1.0F, -0.3F);
      dirLight.ambient = glm::vec3(0.5F, 0.5F, 0.5F);
      dirLight.diffuse = glm::vec3(0.2F, 0.2F, 0.2F);
      dirLight.specular = glm::vec3(0.3F, 0.3F, 0.3F);

      auto &initServiceLocator = rg::ServiceLocator::Get();
      // draw in wireframe
      // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

      // render loop
      // -----------
      // rg::ServiceLocator::Get().getEventController().subscribeToEvent(rg::EventType::MouseMoved,
      // &programState->camera);
      rg::ServiceLocator::Get().getEventController().subscribeToEvent(
	  rg::EventType::Keyboard, &programState->camera);

      while (glfwWindowShouldClose(window) == 0) {
	    // per-frame time logic
	    // --------------------
	    float currentFrame = glfwGetTime();
	    deltaTime = currentFrame - lastFrame;
	    lastFrame = currentFrame;

	    // input
	    // -----
	    // processInput(window);

	    rg::ServiceLocator::Get().getProcessController().update(deltaTime);

	    programState->camera.update(deltaTime);

	    // render
	    // ------
	    glClearColor(programState->clearColor.r, programState->clearColor.g,
			 programState->clearColor.b, 1.0F);
	    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	    glClearColor(programState->clearColor.r, programState->clearColor.g,
			 programState->clearColor.b, 1.0F);
	    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	    glEnable(GL_DEPTH_TEST);
	    // don't forget to enable shader before setting uniforms

	    glm::vec3 lightPos = glm::vec3(4.0 * cos(currentFrame), 4.0F,
					   4.0 * sin(currentFrame));
	    ourShader.use();
	    pointLight.position = lightPos;
	    ourShader.setVec3("pointLight.position", pointLight.position);
	    ourShader.setVec3("pointLight.ambient", pointLight.ambient);
	    ourShader.setVec3("pointLight.diffuse", pointLight.diffuse);
	    ourShader.setVec3("pointLight.specular", pointLight.specular);
	    ourShader.setFloat("pointLight.constant", pointLight.constant);
	    ourShader.setFloat("pointLight.linear", pointLight.linear);
	    ourShader.setFloat("pointLight.quadratic", pointLight.quadratic);

	    ourShader.setVec3("dirLight.direction", dirLight.direction);
	    ourShader.setVec3("dirLight.ambient", dirLight.ambient);
	    ourShader.setVec3("dirLight.diffuse", dirLight.diffuse);
	    ourShader.setVec3("dirLight.specular", dirLight.specular);

	    ourShader.setVec3("viewPosition", programState->camera.Position);
	    ourShader.setFloat("material.shininess", 32.0F);
	    ourShader.setBool("Blinn", programState->Blinn);

	    tableShader.use();
	    tableShader.setVec3("dirLight.ambient", dirLight.ambient);
	    tableShader.setVec3("dirLight.direction", dirLight.direction);
	    tableShader.setVec3("dirLight.diffuse", dirLight.diffuse);
	    tableShader.setVec3("dirLight.specular", dirLight.specular);
	    tableShader.setVec3("viewPosition", programState->camera.Position);
	    tableShader.setFloat("material.shininess", 32.0F);
	    tableShader.setBool("Blinn", programState->Blinn);

	    tableShader.setVec3("pointLight.position", pointLight.position);
	    tableShader.setVec3("pointLight.ambient", pointLight.ambient);
	    tableShader.setVec3("pointLight.diffuse", pointLight.diffuse);
	    tableShader.setVec3("pointLight.specular", pointLight.specular);
	    tableShader.setFloat("pointLight.constant", pointLight.constant);
	    tableShader.setFloat("pointLight.linear", pointLight.linear);
	    tableShader.setFloat("pointLight.quadratic", pointLight.quadratic);

	    // view/projection transformations

	    glm::mat4 projection = glm::perspective(
		glm::radians(80.0F), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1F,
		100.0F);
	    ourShader.setMat4("projection", projection);

	    glm::mat4 view = programState->camera.GetViewMatrix();
	    ourShader.setMat4("view", view);
	    glBindVertexArray(VAO);

	    glm::mat4 model3 = glm::mat4(1.0F);
	    model3 = glm::translate(model3, lightPos);
	    model3 = glm::scale(model3, glm::vec3(0.3F));
	    cubeShader.setMat4("model3", model3);
	    cubeShader.setMat4("view", view);
	    cubeShader.setMat4("projection", projection);

	    glEnable(GL_CULL_FACE);
	    glDrawArrays(GL_TRIANGLES, 0, 36);
	    glDisable(GL_CULL_FACE);

	    // render the loaded model
	    glm::mat4 model = glm::mat4(0.7F);
	    model = glm::translate(
		model,
		programState->plantPosition);  // translate it down so it's at
					       // the center of the scene
	    model = glm::scale(
		model,
		glm::vec3(
		    programState->plantScale));	 // it's a bit too big for our
						 // scene, so scale it down
	    ourShader.setMat4("model", model);
	    ourModel.Draw(ourShader);

	    glm::mat4 model2 = glm::mat4(3.0F);
	    model2 = glm::translate(model2, programState->tablePosition);
	    model2 = glm::scale(model2, glm::vec3(programState->tableScale));
	    tableShader.setMat4("model2", model2);
	    tableModel.Draw(tableShader);

	    // texture objects

	    // vegetation
	    shader.use();
	    glm::mat4 model7 = glm::mat4(1.0F);
	    shader.setMat4("projection", projection);
	    shader.setMat4("view", view);

	    glBindVertexArray(transparentVAO);
	    glBindTexture(GL_TEXTURE_2D, transparentTexture);
	    for (auto i : vegetation) {
		  model7 = glm::mat4(1.0F);
		  model7 = glm::translate(model7, i);
		  shader.setMat4("model7", model7);
		  glDrawArrays(GL_TRIANGLES, 0, 6);
	    }
	    // plane
	    planeShader.use();
	    planeShader.setMat4("projection", projection);
	    planeShader.setMat4("view", view);

	    model = glm::mat4(1.0F);
	    model = glm::translate(model, programState->planePosition);
	    model = glm::scale(model, glm::vec3(6.1F));
	    planeShader.setMat4("model", model);
	    planeShader.setVec3("viewPos", programState->camera.Position);
	    planeShader.setVec3("lightPos", lightPos);
	    planeShader.setFloat("heightScale", programState->heightScale);

	    glActiveTexture(GL_TEXTURE0);
	    glBindTexture(GL_TEXTURE_2D, diffuseMap);
	    glActiveTexture(GL_TEXTURE1);
	    glBindTexture(GL_TEXTURE_2D, normalMap);
	    glActiveTexture(GL_TEXTURE2);
	    glBindTexture(GL_TEXTURE_2D, heightMap);
	    renderQuad();

	    screenShader.use();
	    screenShader.setBool("grayScaleInd", programState->grayScaleInd);

	    // 2. now blit multisampled buffer(s) to normal colorbuffer of
	    // intermediate FBO. Image is stored in screenTexture
	    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
	    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, intermediateFBO);
	    glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH,
			      SCR_HEIGHT, GL_COLOR_BUFFER_BIT, GL_NEAREST);

	    // 3. now render quad with scene's visuals as its texture image
	    glBindFramebuffer(GL_FRAMEBUFFER, 0);
	    glClearColor(1.0F, 1.0F, 1.0F, 1.0F);
	    glClear(GL_COLOR_BUFFER_BIT);
	    glDisable(GL_DEPTH_TEST);

	    // draw Screen quad
	    screenShader.use();
	    glBindVertexArray(quadVAO);
	    glActiveTexture(GL_TEXTURE0);
	    glBindTexture(GL_TEXTURE_2D,
			  screenTexture);  // use the now resolved color
					   // attachment as the quad's texture
	    glDrawArrays(GL_TRIANGLES, 0, 6);

	    if (programState->ImGuiEnabled) {
		  DrawImGui(programState);
	    }

	    // glfw: swap buffers and poll IO events (keys pressed/released,
	    // mouse moved etc.)
	    // -------------------------------------------------------------------------------
	    glfwSwapBuffers(window);
	    glfwPollEvents();
	    rg::ServiceLocator::Get().getInputController().update(deltaTime);
      }

      programState->SaveToFile("resources/program_state.txt");
      delete programState;
      ImGui_ImplOpenGL3_Shutdown();
      ImGui_ImplGlfw_Shutdown();
      ImGui::DestroyContext();

      glDeleteVertexArrays(1, &VAO);
      glDeleteVertexArrays(1, &transparentVAO);
      glDeleteBuffers(1, &VBO);
      glDeleteBuffers(1, &transparentVBO);
      glDeleteVertexArrays(1, &quadVAO);
      glDeleteBuffers(1, &quadVBO);
      glDeleteFramebuffers(1, &framebuffer);
      glDeleteFramebuffers(1, &intermediateFBO);
      glDeleteRenderbuffers(1, &rbo);
      // glfw: terminate, clearing all previously allocated GLFW resources.
      // ------------------------------------------------------------------
      glfwTerminate();
      return 0;
}

// mapping
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
      if (quadVAO == 0) {
	    // positions
	    glm::vec3 pos1(3.0F, -0.5F, 3.0F);
	    glm::vec3 pos2(-3.0F, -0.5F, 3.0F);
	    glm::vec3 pos3(-3.0F, -0.5F, -3.0F);
	    glm::vec3 pos4(3.0F, -0.5F, -3.0F);
	    // texture coordinates
	    glm::vec2 uv1(2.0F, 0.0F);
	    glm::vec2 uv2(0.0F, 0.0F);
	    glm::vec2 uv3(0.0F, 2.0F);
	    glm::vec2 uv4(2.0F, 2.0F);
	    // normal vector
	    glm::vec3 nm(0.0F, 0.0F, 1.0F);

	    // calculate tangent/bitangent vectors of both triangles
	    glm::vec3 tangent1;
	    glm::vec3 bitangent1;
	    glm::vec3 tangent2;
	    glm::vec3 bitangent2;
	    // triangle 1
	    glm::vec3 edge1 = pos2 - pos1;
	    glm::vec3 edge2 = pos3 - pos1;
	    glm::vec2 deltaUV1 = uv2 - uv1;
	    glm::vec2 deltaUV2 = uv3 - uv1;

	    float f =
		1.0F / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

	    tangent1.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
	    tangent1.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
	    tangent1.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
	    tangent1 = glm::normalize(tangent1);

	    bitangent1.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
	    bitangent1.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
	    bitangent1.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
	    bitangent1 = glm::normalize(bitangent1);

	    // triangle 2
	    edge1 = pos3 - pos1;
	    edge2 = pos4 - pos1;
	    deltaUV1 = uv3 - uv1;
	    deltaUV2 = uv4 - uv1;

	    f = 1.0F / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

	    tangent2.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
	    tangent2.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
	    tangent2.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
	    tangent2 = glm::normalize(tangent2);

	    bitangent2.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
	    bitangent2.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
	    bitangent2.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
	    bitangent2 = glm::normalize(bitangent2);

	    float quadVertices[] = {
		// positions            // normal         // texcoords  //
		// tangent                          // bitangent
		pos1.x,	      pos1.y,	    pos1.z,	  nm.x,
		nm.y,	      nm.z,	    uv1.x,	  uv1.y,
		tangent1.x,   tangent1.y,   tangent1.z,	  bitangent1.x,
		bitangent1.y, bitangent1.z, pos2.x,	  pos2.y,
		pos2.z,	      nm.x,	    nm.y,	  nm.z,
		uv2.x,	      uv2.y,	    tangent1.x,	  tangent1.y,
		tangent1.z,   bitangent1.x, bitangent1.y, bitangent1.z,
		pos3.x,	      pos3.y,	    pos3.z,	  nm.x,
		nm.y,	      nm.z,	    uv3.x,	  uv3.y,
		tangent1.x,   tangent1.y,   tangent1.z,	  bitangent1.x,
		bitangent1.y, bitangent1.z,

		pos1.x,	      pos1.y,	    pos1.z,	  nm.x,
		nm.y,	      nm.z,	    uv1.x,	  uv1.y,
		tangent2.x,   tangent2.y,   tangent2.z,	  bitangent2.x,
		bitangent2.y, bitangent2.z, pos3.x,	  pos3.y,
		pos3.z,	      nm.x,	    nm.y,	  nm.z,
		uv3.x,	      uv3.y,	    tangent2.x,	  tangent2.y,
		tangent2.z,   bitangent2.x, bitangent2.y, bitangent2.z,
		pos4.x,	      pos4.y,	    pos4.z,	  nm.x,
		nm.y,	      nm.z,	    uv4.x,	  uv4.y,
		tangent2.x,   tangent2.y,   tangent2.z,	  bitangent2.x,
		bitangent2.y, bitangent2.z};
	    // plane VAO
	    glGenVertexArrays(1, &quadVAO);
	    glGenBuffers(1, &quadVBO);
	    glBindVertexArray(quadVAO);
	    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices,
			 GL_STATIC_DRAW);
	    glEnableVertexAttribArray(0);
	    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float),
				  (void *)nullptr);
	    glEnableVertexAttribArray(1);
	    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float),
				  (void *)(3 * sizeof(float)));
	    glEnableVertexAttribArray(2);
	    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 14 * sizeof(float),
				  (void *)(6 * sizeof(float)));
	    glEnableVertexAttribArray(3);
	    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float),
				  (void *)(8 * sizeof(float)));
	    glEnableVertexAttribArray(4);
	    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float),
				  (void *)(11 * sizeof(float)));
      }
      glBindVertexArray(quadVAO);
      glDrawArrays(GL_TRIANGLES, 0, 6);
      glBindVertexArray(0);
}

// process all input: query GLFW whether relevant keys are pressed/released this
// frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
#if 0
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);
#else

#endif
}

// glfw: whenever the window size changed (by OS or user resize) this callback
// function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
      // make sure the viewport matches the new window dimensions; note that
      // width and height will be significantly larger than specified on retina
      // displays.
      // glViewport(0, 0, width, height);
      glViewport(0, 0, 800, 600);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
      rg::ServiceLocator::Get()
	  .getInputController()
	  .processMouseMovementCallback(xpos, ypos);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
#if 0
    programState->camera.ProcessMouseScroll(yoffset);
#else
      rg::ServiceLocator::Get().getInputController().processMouseScrollCallback(
	  yoffset);
#endif
}

void DrawImGui(ProgramState *programState)
{
      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();

      {
	    static float f = 0.0F;
	    ImGui::Begin("Hello window");
	    ImGui::Text("Hello text");
	    ImGui::SliderFloat("Float slider", &f, 0.0, 1.0);
	    ImGui::ColorEdit3("Background color",
			      (float *)&programState->clearColor);
	    ImGui::InputFloat3("Plant position",
			       (float *)&programState->plantPosition, nullptr);
	    ImGui::InputFloat("Plant scale", &programState->plantScale, 0.03F,
			      0.2F);

	    ImGui::InputFloat3("Table position",
			       (float *)&programState->tablePosition, nullptr);
	    ImGui::InputFloat("Table scale", (float *)&programState->tableScale,
			      0.03F, 0.2);

	    ImGui::InputFloat3("Plane position",
			       (float *)&programState->planePosition, nullptr);
	    ImGui::InputFloat3("Camera position",
			       (float *)&programState->camera.Position);

	    ImGui::DragFloat("pointLight.constant",
			     &programState->pointLight.constant, 0.05, 0.0,
			     1.0);
	    ImGui::DragFloat("pointLight.linear",
			     &programState->pointLight.linear, 0.05, 0.0, 1.0);
	    ImGui::DragFloat("pointLight.quadratic",
			     &programState->pointLight.quadratic, 0.05, 0.0,
			     1.0);
	    ImGui::End();
      }

      {
	    ImGui::Begin("Camera info");
	    const Camera &c = programState->camera;
	    ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x,
			c.Position.y, c.Position.z);
	    ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
	    ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y,
			c.Front.z);
	    ImGui::Checkbox("Camera mouse update",
			    &programState->CameraMouseMovementUpdateEnabled);
	    ImGui::End();
      }

      ImGui::Render();
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action,
		  int mods)
{
      if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
	    programState->ImGuiEnabled = !programState->ImGuiEnabled;
	    if (programState->ImGuiEnabled) {
		  programState->CameraMouseMovementUpdateEnabled = false;
		  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	    } else {
		  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	    }
      }

      if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
	    programState->Blinn = true;
      } else if (glfwGetKey(window, GLFW_KEY_B) == GLFW_RELEASE) {
	    programState->Blinn = false;
      }

      if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
	    PointLight &pointLight = programState->pointLight;
	    if (programState->pointLightInd) {
		  // disable point light
		  pointLight.ambient = glm::vec3(0.0, 0.0, 0.0);
		  pointLight.diffuse = glm::vec3(0.0, 0.0, 0.0);
		  pointLight.specular = glm::vec3(0.0, 0.0, 0.0);
		  programState->pointLightInd = false;
	    } else {
		  pointLight.ambient = glm::vec3(0.1, 0.1, 0.1);
		  pointLight.diffuse = glm::vec3(0.8, 0.8, 0.8);
		  pointLight.specular = glm::vec3(1.0, 1.0, 1.0);
		  programState->pointLightInd = true;
	    }
      }
      if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
	    // enable or disable gray scale
	    programState->grayScaleInd = !programState->grayScaleInd;
      }

      if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
	    programState->camera.ProcessKeyboard(Direction::FORWARD, deltaTime);
      }

      if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
	    programState->camera.ProcessKeyboard(Direction::BACKWARD,
						 deltaTime);
      }

      if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
	    programState->camera.ProcessKeyboard(Direction::RIGHT, deltaTime);
      }

      if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
	    programState->camera.ProcessKeyboard(Direction::LEFT, deltaTime);
      }
}

auto loadTexture(char const *path) -> unsigned int
{
      unsigned int textureID;
      glGenTextures(1, &textureID);

      int width;
      int height;
      int nrComponents;
      unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
      if (data != nullptr) {
	    GLenum format = GL_RGBA;
	    if (nrComponents == 1) {
		  format = GL_RED;
	    } else if (nrComponents == 3) {
		  format = GL_RGB;
	    } else if (nrComponents == 4) {
		  format = GL_RGBA;
	    }

	    glBindTexture(GL_TEXTURE_2D, textureID);
	    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
			 GL_UNSIGNED_BYTE, data);
	    glGenerateMipmap(GL_TEXTURE_2D);

	    glTexParameteri(
		GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
		format == GL_RGBA
		    ? GL_CLAMP_TO_EDGE
		    : GL_REPEAT);  // for this tutorial: use GL_CLAMP_TO_EDGE to
				   // prevent semi-transparent borders. Due to
				   // interpolation it takes texels from next
				   // repeat
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
			    format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
			    GL_LINEAR_MIPMAP_LINEAR);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	    stbi_image_free(data);
      } else {
	    std::cout << "Texture failed to load at path: " << path
		      << std::endl;
	    stbi_image_free(data);
      }

      return textureID;
}
