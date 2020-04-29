#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <stdio.h>
#include <math.h>

#include <GL/glew.h>
#include <GL/freeglut.h>

#include "maths_funcs.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>

#include "teapot.h"
#include "loader.h"

#include <windows.h> 
#include <mmsystem.h>

int width = 1000;
int height = 1000;

GLuint ObjectShaderProgramID;

GLuint objectVAO = 0;
GLuint objectVBO = 0;
GLuint objectVCO = 0;
GLuint objectloc1;
GLuint objectloc2;

// Object Translation and Rotation Variables
float objectLocationx = 0.06;
float objectLocationy = -0.25;
float objectLocationz = 0.0;

float objectAnglex = 0.27;
float objectAngley = -0.16;
float objectAnglez = 0.35;
float objectSize = 0.5;

float upper_arm_angle = 0.0;
float lower_arm_angle = 0.35;


// function variables
float upper_arm_length = 85.70;
float lower_arm_length = 109.0;


// Model Load Variables
LoadObj human_body("./Object/Human/body.obj");
LoadObj human_upper_arm("./Object/Human/upper_arm.obj");
LoadObj human_lower_arm("./Object/Human/lower_arm.obj");

// Simple IK Implementation
//reference: Class Slides
vec2 calculateAngles(vec2 dest)
{
	// Function Variables
	float angle1, angle2;
	float dist;
	float temp1, temp2;
	vec2 source = vec2(562, 231);

	dist = sqrt(pow(source.v[0] - dest.v[0], 2) + pow(source.v[1] - dest.v[1], 2));
	std::cout << "dist: " << dist <<std::endl;
	temp1 = atan2(dest.v[1] - source.v[1], dest.v[0] - source.v[0]);
	std::cout << "temp1: " << temp1 << std::endl;
	temp2 = acos((dist * dist + upper_arm_length * upper_arm_length - lower_arm_length * lower_arm_length) / (2 * dist * upper_arm_length));
	std::cout << "temp2: " << temp2 << std::endl;
	angle1 = temp1 + temp2;

	angle2 = acos((upper_arm_length * upper_arm_length + lower_arm_length * lower_arm_length - dist * dist) / (2 * upper_arm_length * lower_arm_length));

	std::cout << "cal function call" << angle1 << " : " << angle2;
	//new_upper_arm = theta_1;
	//new_lower_arm = theta_2;
	return vec2(angle1, angle2);
}

std::string readShaderSource(const std::string& fileName)
{
	std::ifstream file(fileName.c_str()); 
	if (file.fail()) {
		std::cerr << "Error loading shader called " << fileName << std::endl;
		exit(EXIT_FAILURE);
	}

	std::stringstream stream;
	stream << file.rdbuf();
	file.close();

	return stream.str();
}

static void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType) {
	GLuint ShaderObj = glCreateShader(ShaderType);
	if (ShaderObj == 0) {
		std::cerr << "Error creating shader type " << ShaderType << std::endl;
		exit(EXIT_FAILURE);
	}

	/* bind shader source code to shader object */
	std::string outShader = readShaderSource(pShaderText);
	const char* pShaderSource = outShader.c_str();
	glShaderSource(ShaderObj, 1, (const GLchar * *)& pShaderSource, NULL);

	/* compile the shader and check for errors */
	glCompileShader(ShaderObj);
	GLint success;
	glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
	if (!success) {
		GLchar InfoLog[1024];
		glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
		std::cerr << "Error compiling shader type " << ShaderType << ": " << InfoLog << std::endl;
		exit(EXIT_FAILURE);
	}
	glAttachShader(ShaderProgram, ShaderObj); /* attach compiled shader to shader programme */
}

GLuint CompileShaders(const char* pVShaderText, const char* pFShaderText)
{
	//Start the process of setting up our shaders by creating a program ID
	//Note: we will link all the shaders together into this ID
	GLuint ShaderProgramID = glCreateProgram();
	if (ShaderProgramID == 0) {
		std::cerr << "Error creating shader program" << std::endl;
		exit(EXIT_FAILURE);
	}

	// Create two shader objects, one for the vertex, and one for the fragment shader
	AddShader(ShaderProgramID, pVShaderText, GL_VERTEX_SHADER);
	AddShader(ShaderProgramID, pFShaderText, GL_FRAGMENT_SHADER);

	GLint Success = 0;
	GLchar ErrorLog[1024] = { 0 };


	// After compiling all shader objects and attaching them to the program, we can finally link it
	glLinkProgram(ShaderProgramID);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(ShaderProgramID, GL_LINK_STATUS, &Success);
	if (Success == 0) {
		glGetProgramInfoLog(ShaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		std::cerr << "Error linking shader program: " << ErrorLog << std::endl;
		exit(EXIT_FAILURE);
	}

	// program has been successfully linked but needs to be validated to check whether the program can execute given the current pipeline state
	glValidateProgram(ShaderProgramID);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(ShaderProgramID, GL_VALIDATE_STATUS, &Success);
	if (!Success) {
		glGetProgramInfoLog(ShaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		std::cerr << "Invalid shader program: " << ErrorLog << std::endl;
		exit(EXIT_FAILURE);
	}
	return ShaderProgramID;
}

void generateObjectBuffer(GLuint TempObjectShaderProgramID)
{
	objectloc1 = glGetAttribLocation(TempObjectShaderProgramID, "vertex_position");
	objectloc2 = glGetAttribLocation(TempObjectShaderProgramID, "vertex_normals");

	// Body
	GLuint body_vp_vbo = 0;
	glGenBuffers(1, &body_vp_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, body_vp_vbo);
	glBufferData(GL_ARRAY_BUFFER, 3 * human_body.getNumVertices() * sizeof(float), human_body.getVertices(), GL_STATIC_DRAW);

	GLuint body_vn_vbo = 0;
	glGenBuffers(1, &body_vn_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, body_vn_vbo);
	glBufferData(GL_ARRAY_BUFFER, 3 * human_body.getNumVertices() * sizeof(float), human_body.getNormals(), GL_STATIC_DRAW);

	glGenVertexArrays(1, &objectVAO);
	glBindVertexArray(objectVAO);

	glEnableVertexAttribArray(objectloc1);
	glBindBuffer(GL_ARRAY_BUFFER, body_vp_vbo);
	glVertexAttribPointer(objectloc1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(objectloc2);
	glBindBuffer(GL_ARRAY_BUFFER, body_vn_vbo);
	glVertexAttribPointer(objectloc2, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	// Upper Arm
	GLuint upper_arm_vp_vbo = 0;
	glGenBuffers(1, &upper_arm_vp_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, upper_arm_vp_vbo);
	glBufferData(GL_ARRAY_BUFFER, 3 * human_upper_arm.getNumVertices() * sizeof(float), human_upper_arm.getVertices(), GL_STATIC_DRAW);

	GLuint upper_arm_vn_vbo = 0;
	glGenBuffers(1, &upper_arm_vn_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, upper_arm_vn_vbo);
	glBufferData(GL_ARRAY_BUFFER, 3 * human_upper_arm.getNumVertices() * sizeof(float), human_upper_arm.getNormals(), GL_STATIC_DRAW);

	glGenVertexArrays(1, &objectVBO);
	glBindVertexArray(objectVBO);

	glEnableVertexAttribArray(objectloc1);
	glBindBuffer(GL_ARRAY_BUFFER, upper_arm_vp_vbo);
	glVertexAttribPointer(objectloc1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(objectloc2);
	glBindBuffer(GL_ARRAY_BUFFER, upper_arm_vn_vbo);
	glVertexAttribPointer(objectloc2, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	// Lower Arm
	GLuint lower_arm_vp_vbo = 0;
	glGenBuffers(1, &lower_arm_vp_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, lower_arm_vp_vbo);
	glBufferData(GL_ARRAY_BUFFER, 3 * human_lower_arm.getNumVertices() * sizeof(float), human_lower_arm.getVertices(), GL_STATIC_DRAW);

	GLuint lower_arm_vn_vbo = 0;
	glGenBuffers(1, &lower_arm_vn_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, lower_arm_vn_vbo);
	glBufferData(GL_ARRAY_BUFFER, 3 * human_lower_arm.getNumVertices() * sizeof(float), human_lower_arm.getNormals(), GL_STATIC_DRAW);

	glGenVertexArrays(1, &objectVCO);
	glBindVertexArray(objectVCO);

	glEnableVertexAttribArray(objectloc1);
	glBindBuffer(GL_ARRAY_BUFFER, lower_arm_vp_vbo);
	glVertexAttribPointer(objectloc1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(objectloc2);
	glBindBuffer(GL_ARRAY_BUFFER, lower_arm_vn_vbo);
	glVertexAttribPointer(objectloc2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
}

void init(void) {

	// Toon Reflectance Model
	ObjectShaderProgramID = CompileShaders("./Shaders/Vertex Shader.vert",
		"./Shaders/Fragment Shader.frag");
	generateObjectBuffer(ObjectShaderProgramID);
}

void display() {
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(ObjectShaderProgramID);

	int matrix_location = glGetUniformLocation(ObjectShaderProgramID, "model");
	int view_mat_location = glGetUniformLocation(ObjectShaderProgramID, "view");
	int proj_mat_location = glGetUniformLocation(ObjectShaderProgramID, "proj");

	// Body
	glViewport(0, 0, width, height);
	glm::mat4 body_view = glm::lookAt(glm::vec3(7.0f, -0.2f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 body_persp_proj = glm::perspective(objectSize, (float)(width) / (float)height, 0.1f, 100.0f);
	glm::mat4 body_model = glm::mat4(1.0);
	body_model = glm::translate(body_model, glm::vec3(0.0,-1.0,0.0));
	body_model = glm::rotate(body_model, 0.0f, glm::vec3(1, 0, 0));
	body_model = glm::rotate(body_model, -11.0f, glm::vec3(0, 1, 0));
	body_model = glm::rotate(body_model, 0.0f, glm::vec3(0, 0, 1));

	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, &body_persp_proj[0][0]);
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, &body_view[0][0]);
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, &body_model[0][0]);

	glBindVertexArray(objectVAO);
	glDrawArrays(GL_TRIANGLES, 0, human_body.getNumVertices());

	// Upper Arm
	glm::mat4 upper_arm_view = glm::lookAt(glm::vec3(7.0f, -0.2f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 upper_arm_persp_proj = glm::perspective(objectSize, (float)(width) / (float)height, 0.1f, 100.0f);
	glm::mat4 upper_arm_model = glm::mat4(1.0);
	upper_arm_model = glm::translate(upper_arm_model, glm::vec3(0.0, 0.4, -0.22));
	upper_arm_model = glm::rotate(upper_arm_model, 0.0f, glm::vec3(1, 0, 0));
	upper_arm_model = glm::rotate(upper_arm_model, -4.5f, glm::vec3(0, 1, 0));
	upper_arm_model = glm::rotate(upper_arm_model, upper_arm_angle, glm::vec3(0, 0, 1));

	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, &upper_arm_persp_proj[0][0]);
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, &upper_arm_view[0][0]);
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, &upper_arm_model[0][0]);
	 
	glBindVertexArray(objectVBO);
	glDrawArrays(GL_TRIANGLES, 0, human_upper_arm.getNumVertices());

	// Lower Arm
	glm::mat4 lower_arm_view = glm::lookAt(glm::vec3(7.0f, -0.2f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 lower_arm_persp_proj = glm::perspective(objectSize, (float)(width) / (float)height, 0.1f, 100.0f);
	glm::mat4 lower_arm_model = glm::mat4(1.0);
	lower_arm_model = glm::translate(lower_arm_model, glm::vec3(0.06, -0.25, 0.0));
	lower_arm_model = glm::rotate(lower_arm_model, 0.27f, glm::vec3(1, 0, 0));
	lower_arm_model = glm::rotate(lower_arm_model, -0.16f, glm::vec3(0, 1, 0));
	lower_arm_model = glm::rotate(lower_arm_model, lower_arm_angle, glm::vec3(0, 0, 1));

	lower_arm_model = upper_arm_model * lower_arm_model;

	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, &lower_arm_persp_proj[0][0]);
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, &lower_arm_view[0][0]);
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, &lower_arm_model[0][0]);

	glBindVertexArray(objectVCO);
	glDrawArrays(GL_TRIANGLES, 0, human_lower_arm.getNumVertices());


	glutSwapBuffers();
}

void keyPress(unsigned char key, int xmouse, int ymouse) {
	switch (key) {
	case('q'):
		objectSize += 0.5;
		std::cout << objectSize << "Keypress: " << key << std::endl;
		break;
	case('a'):
		objectSize -= 0.5;
		std::cout << objectSize << "Keypress: " << key << std::endl;
		break;
	case ('w'):
		objectLocationx += 0.01;
		std::cout << objectLocationx << "Keypress: " << key << std::endl;
		break;
	case ('s'):
		objectLocationx -= 0.01;
		std::cout << objectLocationx << "Keypress: " << key << std::endl;
		break;
	case ('e'):
		objectLocationy += 0.01;
		std::cout << objectLocationy << "Keypress: " << key << std::endl;
		break;
	case ('d'):
		objectLocationy -= 0.01;
		std::cout << objectLocationy << "Keypress: " << key << std::endl;
		break;
	case ('r'):
		objectLocationz += 0.01;
		std::cout << objectLocationz << "Keypress: " << key << std::endl;
		break;
	case ('f'):
		objectLocationz -= 0.01;
		std::cout << objectLocationz << "Keypress: " << key << std::endl;
		break;
	case ('7'):
		objectAnglex += 0.01;
		std::cout << objectAnglex << "," << "7" << std::endl;
		break;
	case ('4'):
		objectAnglex -= 0.01;
		std::cout << objectAnglex << "," << "4" << std::endl;
		break;
	case ('8'):
		objectAngley += 0.01;
		std::cout << objectAngley << "," << "8" << std::endl;
		break;
	case ('5'):
		objectAngley -= 0.01;
		std::cout << objectAngley << "," << "5" << std::endl;
		break;
	case ('9'):
		objectAnglez += 0.01;
		std::cout << objectAnglez << "," << "9" << std::endl;
		break;
	case ('6'):
		objectAnglez -= 0.01;
		std::cout << objectAnglez << "," << "6" << std::endl;
		break;
	case ('m'):
		objectLocationx = 0.0;
		objectLocationy = 0.0;
		objectLocationz = 0.0;
		objectAnglex = 0.0;
		objectAngley = 0.0;
		objectAnglez = 0.0;
		//objectSize = 0.5;
		break;
	case ('u'):
		upper_arm_angle += 0.01;
		std::cout << "upper_arm_angle: " << upper_arm_angle << std::endl;
		break;
	case ('j'):
		upper_arm_angle -= 0.01;
		std::cout << "upper_arm_angle: " << upper_arm_angle << std::endl;
		break;
	case ('i'):
		lower_arm_angle += 0.01;
		std::cout << "lower_arm_angle: " << lower_arm_angle << std::endl;
		break;
	case ('k'):
		lower_arm_angle -= 0.01;
		std::cout << "lower_arm_angle: " << lower_arm_angle << std::endl;
		break;
	}
};

void mouse_button_callback(int button, int state, int x, int y)
{
	if (button == 0) {
		std::cout << "Mouse Func" << ": " << x << ", " << y << std::endl;
		vec2 temp = calculateAngles(vec2(x, y));
		if (!isnan(temp.v[0]) && !isnan(temp.v[1])) {
			upper_arm_angle = 1.86 - temp.v[0];
			lower_arm_angle = 3.25 - temp.v[1];
		}
	}
}



void updateScene() {

	//rotatez += 0.5f;
	// Draw the next frame
	glutPostRedisplay();
}

int main(int argc, char** argv) {

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(width, height);
	glutCreateWindow(argv[1]);

	glutDisplayFunc(display);

	glutIdleFunc(updateScene);
	glutKeyboardFunc(keyPress);
	glutMouseFunc(mouse_button_callback);
	GLenum res = glewInit();
	if (res != GLEW_OK) {
		std::cerr << "Error: " << glewGetErrorString(res) << std::endl;
		return EXIT_FAILURE;
	}

	init();
	glutMainLoop();
	return 0;
}