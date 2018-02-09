// View Frustum Culling base code for CPE 476 VFC workshop
// built off 471 P4 game camera - 2015 revise with glfw and obj and glm - ZJW
// Note data-structure NOT recommended for CPE 476 -
// object locations in arrays and estimated radii
// use your improved data structures
// note shaders using GLSL 1.2 (just did not bother to update)


#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <memory>
#include <cassert>
#include <cmath>
#include <stdio.h>
#include <time.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <tiny_obj_loader/tiny_obj_loader.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "GLSL.h"
#include "Program.h"
#include "WindowManager.h"


using namespace std;
using namespace glm;

class Application : public EventCallbacks
{

public:

	std::shared_ptr<Program> prog;

	void init(const std::string& resourceDirectory)
	{
		RESOURCE_DIR = resourceDirectory;

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		loadShapes(RESOURCE_DIR + "/Nefertiti-10k.obj", nefer);
		loadShapes(RESOURCE_DIR + "/sphere.obj", sphere);
		initGL();

		// Initialize the GLSL program.
		prog = make_shared<Program>();
		prog->setVerbose(true);
		prog->setShaderNames(
			resourceDirectory + "/vert.glsl",
			resourceDirectory + "/frag.glsl");
		if (! prog->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		prog->addUniform("uProjMatrix");
		prog->addUniform("uViewMatrix");
		prog->addUniform("uModelMatrix");

		prog->addUniform("uLightPos");
		prog->addUniform("UaColor");
		prog->addUniform("UdColor");
		prog->addUniform("UsColor");
		prog->addUniform("Ushine");

		prog->addAttribute("aPosition");
		prog->addAttribute("aNormal");

		glClearColor(0.6f, 0.6f, 0.8f, 1.0f);

		glGenVertexArrays(1, &VertexArrayID);
		glBindVertexArray(VertexArrayID);
	}

	WindowManager * windowManager = nullptr;

	string RESOURCE_DIR = ""; // Where the resources are loaded from

	GLuint VertexArrayID;

	//main geometry for program
	vector<tinyobj::shape_t> nefer;
	vector<tinyobj::material_t> materials;
	vector<tinyobj::shape_t> sphere;

	//global used to control culling or not for sub-window views
	int CULL = 1;

	glm::vec3 g_light = glm::vec3(2, 6, 6);
	float updateDir = 0.5;

	vec3 g_lookAt = vec3(0, 1, -1);

	//transforms on objects - ugly as its frankenstein of two example codes
	glm::vec3 g_transN[10];
	float g_scaleN[10];
	float g_rotN[10];
	vec3 g_transS[10];
	float g_scaleS[10];
	float g_rotS[10];
	int g_mat_ids[10];
	float g_ang[10];

	GLuint posBufObjB = 0;
	GLuint norBufObjB = 0;
	GLuint indBufObjB = 0;

	GLuint posBufObjS = 0;
	GLuint norBufObjS = 0;
	GLuint indBufObjS = 0;

	GLuint posBufObjG = 0;
	GLuint norBufObjG = 0;


	int printOglError(const char *file, int line) {
		/* Returns 1 if an OpenGL error occurred, 0 otherwise. */
		GLenum glErr;
		int    retCode = 0;

		glErr = glGetError();
		while (glErr != GL_NO_ERROR)
		{
			std::string error;
			switch (glErr) {
			case GL_INVALID_OPERATION:      error = "INVALID_OPERATION";      break;
			case GL_INVALID_ENUM:           error = "INVALID_ENUM";           break;
			case GL_INVALID_VALUE:          error = "INVALID_VALUE";          break;
			case GL_OUT_OF_MEMORY:          error = "OUT_OF_MEMORY";          break;
			case GL_INVALID_FRAMEBUFFER_OPERATION:  error = "INVALID_FRAMEBUFFER_OPERATION";  break;
			}

			printf("glError in file %s @ line %d: %s\n", file, line, error.c_str());
			retCode = 1;
			glErr = glGetError();
		}
		return retCode;
	}

	/* helper function to change material attributes */
	void SetMaterial(int i) {

		switch (i) {
		case 0: //shiny blue plastic
			glUniform3f(prog->getUniform("UaColor"), 0.02f, 0.04f, 0.2f);
			glUniform3f(prog->getUniform("UdColor"), 0.0f, 0.16f, 0.9f);
			glUniform3f(prog->getUniform("UsColor"), 0.14f, 0.2f, 0.8f);
			glUniform1f(prog->getUniform("Ushine"), 120.0f);
			break;
		case 1: // flat grey
			glUniform3f(prog->getUniform("UaColor"), 0.13f, 0.13f, 0.14f);
			glUniform3f(prog->getUniform("UdColor"), 0.3f, 0.3f, 0.4f);
			glUniform3f(prog->getUniform("UsColor"), 0.3f, 0.3f, 0.4f);
			glUniform1f(prog->getUniform("Ushine"), 4.0f);
			break;
		case 2: //brass
			glUniform3f(prog->getUniform("UaColor"), 0.3294f, 0.2235f, 0.02745f);
			glUniform3f(prog->getUniform("UdColor"), 0.7804f, 0.5686f, 0.11373f);
			glUniform3f(prog->getUniform("UsColor"), 0.9922f, 0.941176f, 0.80784f);
			glUniform1f(prog->getUniform("Ushine"), 27.9f);
			break;
		case 3: //copper
			glUniform3f(prog->getUniform("UaColor"), 0.1913f, 0.0735f, 0.0225f);
			glUniform3f(prog->getUniform("UdColor"), 0.7038f, 0.27048f, 0.0828f);
			glUniform3f(prog->getUniform("UsColor"), 0.257f, 0.1376f, 0.08601f);
			glUniform1f(prog->getUniform("Ushine"), 12.8f);
			break;
		case 4: // flat grey
			glUniform3f(prog->getUniform("UaColor"), 0.13f, 0.13f, 0.14f);
			glUniform3f(prog->getUniform("UdColor"), 0.3f, 0.3f, 0.4f);
			glUniform3f(prog->getUniform("UsColor"), 0.3f, 0.3f, 0.4f);
			glUniform1f(prog->getUniform("Ushine"), 4.0f);
			break;
		case 5: //shadow
			glUniform3f(prog->getUniform("UaColor"), 0.12f, 0.12f, 0.12f);
			glUniform3f(prog->getUniform("UdColor"), 0.0f, 0.0f, 0.0f);
			glUniform3f(prog->getUniform("UsColor"), 0.0f, 0.0f, 0.0f);
			glUniform1f(prog->getUniform("Ushine"), 0);
			break;
		case 6: //gold
			glUniform3f(prog->getUniform("UaColor"), 0.09f, 0.07f, 0.08f);
			glUniform3f(prog->getUniform("UdColor"), 0.91f, 0.2f, 0.91f);
			glUniform3f(prog->getUniform("UsColor"), 1.0f, 0.7f, 1.0f);
			glUniform1f(prog->getUniform("Ushine"), 100.0f);
			break;
		case 7: //green
			glUniform3f(prog->getUniform("UaColor"), 0.0f, 0.07f, 0.0f);
			glUniform3f(prog->getUniform("UdColor"), 0.1f, 0.91f, 0.3f);
			glUniform3f(prog->getUniform("UsColor"), 0, 0, 0);
			glUniform1f(prog->getUniform("Ushine"), 0.0f);
			break;
		}
	}

	/* projection matrix */
	mat4 SetProjectionMatrix() {
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);

		mat4 Projection = perspective(radians(50.0f), (float) width / height, 0.1f, 100.f);
		CHECKED_GL_CALL(glUniformMatrix4fv(prog->getUniform("uProjMatrix"), 1, GL_FALSE, glm::value_ptr(Projection)));
		return Projection;
	}

	/* top down views using ortho */
	mat4 SetOrthoMatrix(float const Size) {
		mat4 ortho = glm::ortho(-Size, Size, -Size, Size, 2.1f, 100.f);
		CHECKED_GL_CALL(glUniformMatrix4fv(prog->getUniform("uProjMatrix"), 1, GL_FALSE, glm::value_ptr(ortho)));
		return ortho;
	}

	/* camera controls - this is the camera for the top down view */
	mat4 SetTopView() {
		mat4 Cam = glm::lookAt(cameraPos + vec3(0, 8, 0), cameraPos, g_lookAt - cameraPos);
		CHECKED_GL_CALL(glUniformMatrix4fv(prog->getUniform("uViewMatrix"), 1, GL_FALSE, glm::value_ptr(Cam)));
		return Cam;
	}

	/*normal game camera */
	mat4 SetView()
	{
		mat4 Cam = glm::lookAt(cameraPos, g_lookAt, vec3(0, 1, 0));
		CHECKED_GL_CALL(glUniformMatrix4fv(prog->getUniform("uViewMatrix"), 1, GL_FALSE, glm::value_ptr(Cam)));
		return Cam;
	}

	/* model transforms - these are insane because they came from p2B and P4*/
	mat4 SetModel(vec3 trans, float rotY, float rotX, vec3 sc) {
		mat4 Trans = translate(glm::mat4(1.0f), trans);
		mat4 RotateY = rotate(glm::mat4(1.0f), rotY, glm::vec3(0.0f, 1, 0));
		mat4 RotateX = rotate(glm::mat4(1.0f), rotX, glm::vec3(1, 0, 0));
		mat4 Sc = scale(glm::mat4(1.0f), sc);
		mat4 com = Trans * RotateY*Sc*RotateX;
		CHECKED_GL_CALL(glUniformMatrix4fv(prog->getUniform("uModelMatrix"), 1, GL_FALSE, glm::value_ptr(com)));
		return com;
	}

	void SetModel(mat4 m) {
		CHECKED_GL_CALL(glUniformMatrix4fv(prog->getUniform("uModelMatrix"), 1, GL_FALSE, glm::value_ptr(m)));
	}

	//Given a vector of shapes which has already been read from an obj file
	// resize all vertices to the range [-1, 1]
	void resize_obj(std::vector<tinyobj::shape_t> &shapes) {
		float minX, minY, minZ;
		float maxX, maxY, maxZ;
		float scaleX, scaleY, scaleZ;
		float shiftX, shiftY, shiftZ;
		float epsilon = 0.001;

		minX = minY = minZ = 1.1754E+38F;
		maxX = maxY = maxZ = -1.1754E+38F;

		//Go through all vertices to determine min and max of each dimension
		for (size_t i = 0; i < shapes.size(); i++) {
			for (size_t v = 0; v < shapes[i].mesh.positions.size() / 3; v++) {
				if (shapes[i].mesh.positions[3 * v + 0] < minX) minX = shapes[i].mesh.positions[3 * v + 0];
				if (shapes[i].mesh.positions[3 * v + 0] > maxX) maxX = shapes[i].mesh.positions[3 * v + 0];

				if (shapes[i].mesh.positions[3 * v + 1] < minY) minY = shapes[i].mesh.positions[3 * v + 1];
				if (shapes[i].mesh.positions[3 * v + 1] > maxY) maxY = shapes[i].mesh.positions[3 * v + 1];

				if (shapes[i].mesh.positions[3 * v + 2] < minZ) minZ = shapes[i].mesh.positions[3 * v + 2];
				if (shapes[i].mesh.positions[3 * v + 2] > maxZ) maxZ = shapes[i].mesh.positions[3 * v + 2];
			}
		}
		//From min and max compute necessary scale and shift for each dimension
		float maxExtent, xExtent, yExtent, zExtent;
		xExtent = maxX - minX;
		yExtent = maxY - minY;
		zExtent = maxZ - minZ;
		if (xExtent >= yExtent && xExtent >= zExtent) {
			maxExtent = xExtent;
		}
		if (yExtent >= xExtent && yExtent >= zExtent) {
			maxExtent = yExtent;
		}
		if (zExtent >= xExtent && zExtent >= yExtent) {
			maxExtent = zExtent;
		}
		scaleX = 2.0 / maxExtent;
		shiftX = minX + (xExtent / 2.0);
		scaleY = 2.0 / maxExtent;
		shiftY = minY + (yExtent / 2.0);
		scaleZ = 2.0 / maxExtent;
		shiftZ = minZ + (zExtent) / 2.0;

		//Go through all verticies shift and scale them
		for (size_t i = 0; i < shapes.size(); i++) {
			for (size_t v = 0; v < shapes[i].mesh.positions.size() / 3; v++) {
				shapes[i].mesh.positions[3 * v + 0] = (shapes[i].mesh.positions[3 * v + 0] - shiftX) * scaleX;
				assert(shapes[i].mesh.positions[3 * v + 0] >= -1.0 - epsilon);
				assert(shapes[i].mesh.positions[3 * v + 0] <= 1.0 + epsilon);
				shapes[i].mesh.positions[3 * v + 1] = (shapes[i].mesh.positions[3 * v + 1] - shiftY) * scaleY;
				assert(shapes[i].mesh.positions[3 * v + 1] >= -1.0 - epsilon);
				assert(shapes[i].mesh.positions[3 * v + 1] <= 1.0 + epsilon);
				shapes[i].mesh.positions[3 * v + 2] = (shapes[i].mesh.positions[3 * v + 2] - shiftZ) * scaleZ;
				assert(shapes[i].mesh.positions[3 * v + 2] >= -1.0 - epsilon);
				assert(shapes[i].mesh.positions[3 * v + 2] <= 1.0 + epsilon);
			}
		}
	}

	/* draw a snowman */
	void drawSnowman(mat4 moveModel, int i) {

		// Enable and bind position array for drawing
		GLSL::enableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, posBufObjS);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

		// Enable and bind normal array for drawing
		GLSL::enableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, norBufObjS);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

		// Bind index array for drawing
		int nIndices = (int) sphere[0].mesh.indices.size();
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indBufObjS);

		SetMaterial(5);
		//shadow
		mat4 t = translate(mat4(1.0), vec3(0.2, -1.4, 0.2));
		mat4 s = scale(mat4(1.0), vec3(1, 0.01, 1));
		SetModel(moveModel*t*s);
		glDrawElements(GL_TRIANGLES, nIndices, GL_UNSIGNED_INT, 0);

		if (i % 2 == 0)
			SetMaterial(0);
		else
			SetMaterial(1);
		//body?
		t = translate(mat4(1.0), vec3(0, -0.5, 0));
		SetModel(moveModel*t);
		glDrawElements(GL_TRIANGLES, nIndices, GL_UNSIGNED_INT, 0);

		t = translate(mat4(1.0), vec3(0., 0.72, 0));
		s = scale(mat4(1.0), vec3(.75, .75, .75));
		mat4 com = t * s;
		SetModel(moveModel*com);
		glDrawElements(GL_TRIANGLES, nIndices, GL_UNSIGNED_INT, 0);

		t = translate(mat4(1.0), vec3(0, 1.75, 0));
		s = scale(mat4(1.0), vec3(0.55, 0.55, 0.55));
		com = t * s;
		SetModel(moveModel*com);
		glDrawElements(GL_TRIANGLES, nIndices, GL_UNSIGNED_INT, 0);

		//switch the shading to greyscale
		SetMaterial(4);
		//the right arm
		t = translate(mat4(1.0), vec3(.37, 0.75, .5));
		mat4 r = rotate(mat4(1.0), radians(g_ang[i]), vec3(0, 0, 1));
		mat4 t1 = translate(mat4(1.0), vec3(.37, 0.0, .0));
		s = scale(mat4(1.0), vec3(0.75, 0.05, 0.05));
		com = t * r*t1*s;
		SetModel(moveModel*com);
		glDrawElements(GL_TRIANGLES, nIndices, GL_UNSIGNED_INT, 0);

		//update animation on arm
		if (g_ang[i] > 18)
			updateDir = -0.5;
		else if (g_ang[i] < -20)
			updateDir = 0.5;
		g_ang[i] += updateDir;

		//the left arm
		t = translate(mat4(1.0), vec3(-.75, 0.75, .5));
		s = scale(mat4(1.0), vec3(0.75, 0.05, 0.05));
		com = t * s;
		SetModel(moveModel*com);
		glDrawElements(GL_TRIANGLES, nIndices, GL_UNSIGNED_INT, 0);

		//eyes
		t = translate(mat4(1.0), vec3(-.35, 1.75, .38));
		s = scale(mat4(1.0), vec3(0.05, 0.05, 0.05));
		com = t * s;
		SetModel(moveModel*com);
		glDrawElements(GL_TRIANGLES, nIndices, GL_UNSIGNED_INT, 0);

		t = translate(mat4(1.0), vec3(.35, 1.75, .38));
		s = scale(mat4(1.0), vec3(0.05, 0.05, 0.05));
		com = t * s;
		SetModel(moveModel*com);
		CHECKED_GL_CALL(glDrawElements(GL_TRIANGLES, nIndices, GL_UNSIGNED_INT, 0));

		GLSL::disableVertexAttribArray(0);
		GLSL::disableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	}


	void loadShapes(const string &objFile, std::vector<tinyobj::shape_t>& shapes)
	{
		string err;
		bool rc = tinyobj::LoadObj(shapes, materials, err, objFile.c_str());
		if (! rc) {
			cerr << err << endl;
		}
		resize_obj(shapes);
	}


	/*code to set up Nefertiti mesh */
	void initNefer(std::vector<tinyobj::shape_t>& shape) {

		if (! shape.size())
		{
			cerr << "Nefertiti mesh failed to load!" << endl;
			return;
		}

		// Send the position array to the GPU
		const vector<float> &posBuf = shape[0].mesh.positions;
		glGenBuffers(1, &posBufObjB);
		glBindBuffer(GL_ARRAY_BUFFER, posBufObjB);
		glBufferData(GL_ARRAY_BUFFER, posBuf.size() * sizeof(float), &posBuf[0], GL_STATIC_DRAW);

		// Send the normal array to the GPU
		vector<float> norBuf;
		glm::vec3 v1, v2, v3;
		glm::vec3 edge1, edge2, norm;
		int idx1, idx2, idx3;
		//for every vertex initialize the vertex normal to 0
		for (size_t j = 0; j < shape[0].mesh.positions.size() / 3; j++) {
			norBuf.push_back(0);
			norBuf.push_back(0);
			norBuf.push_back(0);
		}
		//process the mesh and compute the normals - for every face
		//add its normal to its associated vertex
		for (size_t i = 0; i < shape[0].mesh.indices.size() / 3; i++) {
			idx1 = shape[0].mesh.indices[3 * i + 0];
			idx2 = shape[0].mesh.indices[3 * i + 1];
			idx3 = shape[0].mesh.indices[3 * i + 2];
			v1 = glm::vec3(shape[0].mesh.positions[3 * idx1 + 0], shape[0].mesh.positions[3 * idx1 + 1], shape[0].mesh.positions[3 * idx1 + 2]);
			v2 = glm::vec3(shape[0].mesh.positions[3 * idx2 + 0], shape[0].mesh.positions[3 * idx2 + 1], shape[0].mesh.positions[3 * idx2 + 2]);
			v3 = glm::vec3(shape[0].mesh.positions[3 * idx3 + 0], shape[0].mesh.positions[3 * idx3 + 1], shape[0].mesh.positions[3 * idx3 + 2]);
			if (0) {
				std::cout << "v1 " << v1.x << " " << v1.y << " " << v1.z << std::endl;
				std::cout << "v2 " << v1.x << " " << v2.y << " " << v2.z << std::endl;
				std::cout << "v3 " << v3.x << " " << v3.y << " " << v3.z << std::endl;
			}
			edge1 = v2 - v1;
			edge2 = v3 - v1;
			norm = glm::cross(edge1, edge2);
			norBuf[3 * idx1 + 0] += (norm.x);
			norBuf[3 * idx1 + 1] += (norm.y);
			norBuf[3 * idx1 + 2] += (norm.z);
			norBuf[3 * idx2 + 0] += (norm.x);
			norBuf[3 * idx2 + 1] += (norm.y);
			norBuf[3 * idx2 + 2] += (norm.z);
			norBuf[3 * idx3 + 0] += (norm.x);
			norBuf[3 * idx3 + 1] += (norm.y);
			norBuf[3 * idx3 + 2] += (norm.z);
		}
		glGenBuffers(1, &norBufObjB);
		glBindBuffer(GL_ARRAY_BUFFER, norBufObjB);
		glBufferData(GL_ARRAY_BUFFER, norBuf.size() * sizeof(float), &norBuf[0], GL_STATIC_DRAW);

		// Send the index array to the GPU
		const vector<unsigned int> &indBuf = shape[0].mesh.indices;
		glGenBuffers(1, &indBufObjB);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indBufObjB);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indBuf.size() * sizeof(unsigned int), &indBuf[0], GL_STATIC_DRAW);

		// Unbind the arrays
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		GLSL::checkVersion();
		assert(glGetError() == GL_NO_ERROR);
	}

	/*send snowman data to GPU */
	void initSnow(std::vector<tinyobj::shape_t>& shape) {

		if (! shape.size())
		{
			cerr << "snowman model failed to load!" << endl;
			return;
		}

		// Send the position array to the GPU
		const vector<float> &posBuf = shape[0].mesh.positions;
		glGenBuffers(1, &posBufObjS);
		glBindBuffer(GL_ARRAY_BUFFER, posBufObjS);
		glBufferData(GL_ARRAY_BUFFER, posBuf.size() * sizeof(float), &posBuf[0], GL_STATIC_DRAW);

		// Send the normal array to the GPU
		vector<float> norBuf = shape[0].mesh.normals;;
		glGenBuffers(1, &norBufObjS);
		glBindBuffer(GL_ARRAY_BUFFER, norBufObjS);
		glBufferData(GL_ARRAY_BUFFER, norBuf.size() * sizeof(float), &norBuf[0], GL_STATIC_DRAW);

		// Send the index array to the GPU
		const vector<unsigned int> &indBuf = shape[0].mesh.indices;
		glGenBuffers(1, &indBufObjS);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indBufObjS);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indBuf.size() * sizeof(unsigned int), &indBuf[0], GL_STATIC_DRAW);

		// Unbind the arrays
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		GLSL::checkVersion();
		assert(glGetError() == GL_NO_ERROR);
	}

	/* ground plane data to GPU */
	void initGround() {

		float G_edge = 20;
		GLfloat g_backgnd_data[] = {
			  -G_edge, -1.0f, -G_edge,
				-G_edge,  -1.0f, G_edge,
				G_edge, -1.0f, -G_edge,
				-G_edge,  -1.0f, G_edge,
				G_edge, -1.0f, -G_edge,
				G_edge, -1.0f, G_edge,
		};


		GLfloat nor_Buf_G[] = {
			  0.0f, 1.0f, 0.0f,
			  0.0f, 1.0f, 0.0f,
			  0.0f, 1.0f, 0.0f,
			  0.0f, 1.0f, 0.0f,
			  0.0f, 1.0f, 0.0f,
			  0.0f, 1.0f, 0.0f,
		};

		glGenBuffers(1, &posBufObjG);
		glBindBuffer(GL_ARRAY_BUFFER, posBufObjG);
		glBufferData(GL_ARRAY_BUFFER, sizeof(g_backgnd_data), g_backgnd_data, GL_STATIC_DRAW);

		glGenBuffers(1, &norBufObjG);
		glBindBuffer(GL_ARRAY_BUFFER, norBufObjG);
		glBufferData(GL_ARRAY_BUFFER, sizeof(nor_Buf_G), nor_Buf_G, GL_STATIC_DRAW);

	}

	void initGL()
	{
		// Set the background color
		glClearColor(0.6f, 0.6f, 0.8f, 1.0f);
		// Enable Z-buffer test
		glEnable(GL_DEPTH_TEST);
		glPointSize(18);

		float tx, tz, r;
		float Wscale = 18.0;
		srand(1234);
		//allocate the transforms for the different models
		for (int i = 0; i < 10; i++) {
			tx = 0.2f + Wscale * (static_cast <float> (rand()) / static_cast <float> (RAND_MAX)) - Wscale / 1.0f;
			tz = 0.2f + Wscale * (static_cast <float> (rand()) / static_cast <float> (RAND_MAX)) - Wscale / 1.0f;
			r = 360 * (static_cast <float> (rand()) / static_cast <float> (RAND_MAX));
			g_transN[i] = vec3(tx, 0, tz);
			g_scaleN[i] = 1.0;
			g_rotN[i] = r;
			tx = 0.1f + Wscale * (static_cast <float> (rand()) / static_cast <float> (RAND_MAX)) - Wscale / 2.0f;
			tz = 0.1f + Wscale * (static_cast <float> (rand()) / static_cast <float> (RAND_MAX)) - Wscale / 2.0f;
			r = 360.f * (static_cast <float> (rand()) / static_cast <float> (RAND_MAX));
			g_transS[i] = vec3(tx, 0, tz);
			g_scaleS[i] = 1.0f;
			g_rotS[i] = r;
			g_mat_ids[i] = i % 4;
			g_ang[i] = 0;
		}

		initNefer(nefer);
		initSnow(sphere);
		initGround();
	}

	/* VFC code starts here TODO - start here and fill in these functions!!!*/
	vec4 Left, Right, Bottom, Top, Near, Far;
	vec4 planes[6];

	void ExtractVFPlanes(mat4 P, mat4 V) {

		/* composite matrix */
		mat4 comp = P * V;
		vec3 n; //use to pull out normal
		float l; //length of normal for plane normalization

		Left.x = 0; // see handout to fill in with values from comp
		Left.y = 0; // see handout to fill in with values from comp
		Left.z = 0; // see handout to fill in with values from comp
		Left.w = 0; // see handout to fill in with values from comp
		planes[0] = Left;
		cout << "Left' " << Left.x << " " << Left.y << " " << Left.z << " " << Left.w << endl;

		Right.x = 0; // see handout to fill in with values from comp
		Right.y = 0; // see handout to fill in with values from comp
		Right.z = 0; // see handout to fill in with values from comp
		Right.w = 0; // see handout to fill in with values from comp
		planes[1] = Right;
		cout << "Right " << Right.x << " " << Right.y << " " << Right.z << " " << Right.w << endl;

		Bottom.x = 0; // see handout to fill in with values from comp
		Bottom.y = 0; // see handout to fill in with values from comp
		Bottom.z = 0; // see handout to fill in with values from comp
		Bottom.w = 0; // see handout to fill in with values from comp
		planes[2] = Bottom;
		cout << "Bottom " << Bottom.x << " " << Bottom.y << " " << Bottom.z << " " << Bottom.w << endl;

		Top.x = 0; // see handout to fill in with values from comp
		Top.y = 0; // see handout to fill in with values from comp
		Top.z = 0; // see handout to fill in with values from comp
		Top.w = 0; // see handout to fill in with values from comp
		planes[3] = Top;
		cout << "Top " << Top.x << " " << Top.y << " " << Top.z << " " << Top.w << endl;

		Near.x = 0; // see handout to fill in with values from comp
		Near.y = 0; // see handout to fill in with values from comp
		Near.z = 0; // see handout to fill in with values from comp
		Near.w = 0; // see handout to fill in with values from comp
		planes[4] = Near;
		cout << "Near " << Near.x << " " << Near.y << " " << Near.z << " " << Near.w << endl;

		Far.x = 0; // see handout to fill in with values from comp
		Far.y = 0; // see handout to fill in with values from comp
		Far.z = 0; // see handout to fill in with values from comp
		Far.w = 0; // see handout to fill in with values from comp
		planes[5] = Far;
		cout << "Far " << Far.x << " " << Far.y << " " << Far.z << " " << Far.w << endl;
	}


	/* helper function to compute distance to the plane */
	/* TODO: fill in */
	float DistToPlane(float A, float B, float C, float D, vec3 point)
	{
		return 0.f;
	}

	/* Actual cull on planes */
	//returns 1 to CULL
	int ViewFrustCull(vec3 center, float radius) {

		float dist;

		if (CULL)
		{
			cout << "testing against all planes" << endl;
			for (int i = 0; i < 6; i++)
			{
				dist = DistToPlane(planes[i].x, planes[i].y, planes[i].z, planes[i].w, center);
				//test against each plane


			}
			return 0;
		}
		else
		{
			return 0;
		}
	}


	/* code to draw the scene */
	void drawScene(int PmatID) {

		int nIndices;

		for (int i = 0; i < 10; i++) {

			if (!ViewFrustCull(g_transN[i], -1.25)) {
				//draw the mesh
				// Enable and bind position array for drawing
				GLSL::enableVertexAttribArray(0);
				CHECKED_GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, posBufObjB));
				CHECKED_GL_CALL(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0));
				// Enable and bind normal array for drawing
				GLSL::enableVertexAttribArray(1);
				CHECKED_GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, norBufObjB));
				CHECKED_GL_CALL(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0));
				// Bind index array for drawing
				nIndices = (int) nefer[0].mesh.indices.size();
				CHECKED_GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indBufObjB));
				//set the color
				if (i % 2 == 0) {
					SetMaterial(2);
				}
				else {
					SetMaterial(3);
				}
				SetModel(g_transN[i], radians(g_rotN[i]), radians(-90.0f), vec3(1));

				//draw the mesh
				CHECKED_GL_CALL(glDrawElements(GL_TRIANGLES, nIndices, GL_UNSIGNED_INT, 0));
				//draw the shadow
				SetMaterial(5);
				SetModel(vec3(g_transN[i].x + 0.2, g_transN[i].y - 1, g_transN[i].z + 0.2), radians(g_rotN[i]), radians(-90.0f), vec3(1, .01, 1));
				CHECKED_GL_CALL(glDrawElements(GL_TRIANGLES, nIndices, GL_UNSIGNED_INT, 0));

				GLSL::disableVertexAttribArray(0);
				GLSL::disableVertexAttribArray(1);
				glBindBuffer(GL_ARRAY_BUFFER, 0);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			}
			//now test the snowmen
			if (!ViewFrustCull(g_transS[i], -1.5)) {
				/*now draw the snowmen */
				mat4 Trans = translate(mat4(1.0f), vec3(g_transS[i].x, g_transS[i].y + 0.4, g_transS[i].z));
				mat4 RotateY = rotate(mat4(1.0f), radians(g_rotS[i]), glm::vec3(0.0f, 1, 0));
				mat4 Sc = scale(glm::mat4(1.0f), vec3(g_scaleS[i]));
				mat4 com = Trans * RotateY*Sc;
				drawSnowman(com, i);
			}
		}

		//always draw the ground
		SetMaterial(PmatID);
		SetModel(vec3(0), radians(0.0f), radians(0.0f), vec3(1));

		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, posBufObjG);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*) 0);
		GLSL::enableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, norBufObjG);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

		CHECKED_GL_CALL(glDrawArrays(GL_TRIANGLES, 0, 6));

		GLSL::disableVertexAttribArray(0);
		GLSL::disableVertexAttribArray(1);
		CHECKED_GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, 0));

	}

	float t0 = 0;

	void render()
	{

		float t1 = (float) glfwGetTime();

		float const dT = (t1 - t0);
		t0 = t1;

		glm::vec3 up = glm::vec3(0, 1, 0);
		glm::vec3 forward = glm::vec3(cos(cTheta) * cos(cPhi), sin(cPhi), sin(cTheta) * cos(cPhi));
		glm::vec3 right = glm::cross(forward, up);

		if (moveForward)
			cameraPos += forward * cameraMoveSpeed * dT;
		if (moveBack)
			cameraPos -= forward * cameraMoveSpeed * dT;
		if (moveLeft)
			cameraPos -= right * cameraMoveSpeed * dT;
		if (moveRight)
			cameraPos += right * cameraMoveSpeed * dT;
		if (moveDown)
			topCameraSize -= 2.5f * dT;
		if (moveUp)
			topCameraSize += 2.5f * dT;

		g_lookAt = cameraPos + forward;

		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		glViewport(0, 0, width, height);

		// Clear the screen
		CHECKED_GL_CALL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

		// Use our GLSL program
		prog->bind();
		glUniform3f(prog->getUniform("uLightPos"), g_light.x, g_light.y, g_light.z);

		//draw the scene from the game camera with culling enabled
		mat4 P = SetProjectionMatrix();
		mat4 V = SetView();
		ExtractVFPlanes(P, V);
		CULL = 1;
		drawScene(0);

		/* draw the complete scene from a top down camera */
		CHECKED_GL_CALL(glClear(GL_DEPTH_BUFFER_BIT));
		glViewport(0, 0, 300, 300);
		SetOrthoMatrix(topCameraSize);
		SetTopView();
		CULL = 0;
		drawScene(7);

		/* draw the culled scene from a top down camera */
		CHECKED_GL_CALL(glClear(GL_DEPTH_BUFFER_BIT));
		glViewport(0, height - 300, 300, 300);
		SetOrthoMatrix(topCameraSize);
		SetTopView();
		CULL = 1;
		drawScene(7);

		prog->unbind();
	}


	void resizeCallback(GLFWwindow* window, int w, int h)
	{

	}

	float cTheta = 0;
	float cPhi = 0;
	bool mouseDown = false;

	double lastX = 0;
	double lastY = 0;
	float cameraRotateSpeed = 0.005f;

	void mouseCallback(GLFWwindow* window, int but, int action, int mods)
	{
		if (action == GLFW_PRESS)
		{
			mouseDown = true;
		}

		if (action == GLFW_RELEASE)
		{
			mouseDown = false;
		}
	}

	void cursorPosCallback(GLFWwindow* window, double xpos, double ypos)
	{
		if (mouseDown)
		{
			cTheta += (float) (xpos - lastX) * cameraRotateSpeed;
			cPhi -= (float) (ypos - lastY) * cameraRotateSpeed;
		}

		lastX = xpos;
		lastY = ypos;
	}

	void scrollCallback(GLFWwindow* window, double deltaX, double deltaY) {
	}

	bool moveForward = false;
	bool moveBack = false;
	bool moveLeft = false;
	bool moveRight = false;
	bool moveUp = false;
	bool moveDown = false;
	glm::vec3 cameraPos;
	float cameraMoveSpeed = 12.0f;
	float topCameraSize = 15.f;

	void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		switch (key)
		{
		case GLFW_KEY_W:
			moveForward = (action != GLFW_RELEASE);
			break;
		case GLFW_KEY_S:
			moveBack = (action != GLFW_RELEASE);
			break;
		case GLFW_KEY_A:
			moveLeft = (action != GLFW_RELEASE);
			break;
		case GLFW_KEY_D:
			moveRight = (action != GLFW_RELEASE);
			break;
		case GLFW_KEY_Q:
			moveUp = (action != GLFW_RELEASE);
			break;
		case GLFW_KEY_E:
			moveDown = (action != GLFW_RELEASE);
			break;

		case GLFW_KEY_1:
			cameraMoveSpeed = 1.f;
			break;
		case GLFW_KEY_2:
			cameraMoveSpeed = 3.f;
			break;
		case GLFW_KEY_3:
			cameraMoveSpeed = 6.f;
			break;
		case GLFW_KEY_4:
			cameraMoveSpeed = 12.f;
			break;
		case GLFW_KEY_5:
			cameraMoveSpeed = 24.f;
			break;
		}

		if (key == GLFW_KEY_Q && action == GLFW_PRESS)
			g_light.x += 0.25;
		if (key == GLFW_KEY_E && action == GLFW_PRESS)
			g_light.x -= 0.25;
	}

};

int main(int argc, char **argv)
{
	// Where the resources are loaded from
	std::string resourceDir = "resources";

	if (argc >= 2)
	{
		resourceDir = argv[1];
	}

	Application *application = new Application();

	// Your main will always include a similar set up to establish your window
	// and GL context, etc.

	WindowManager *windowManager = new WindowManager();
	windowManager->init(1024, 768);
	windowManager->setEventCallbacks(application);
	application->windowManager = windowManager;

	// This is the code that will likely change program to program as you
	// may need to initialize or set up different data and state

	application->init(resourceDir);

	// Loop until the user closes the window.
	while (! glfwWindowShouldClose(windowManager->getHandle()))
	{
		// Render scene.
		application->render();

		// Swap front and back buffers.
		glfwSwapBuffers(windowManager->getHandle());
		// Poll for and process events.
		glfwPollEvents();
	}

	// Quit program.
	windowManager->shutdown();
	return 0;
}
