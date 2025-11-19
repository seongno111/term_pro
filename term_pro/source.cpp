#define _CRT_SECURE_NO_WARNINGS //--- 프로그램 맨 앞에 선언할 것
#include <stdlib.h>
#include <iostream>
#include <stdio.h>
#include <array>
#include <vector>
#include <cmath>
#include <gl/glew.h>
#include <gl/freeglut.h>
#include <gl/freeglut_ext.h>
#include <gl/glm/glm.hpp>
#include <gl/glm/ext.hpp>
#include <gl/glm/gtc/matrix_transform.hpp>
#include "obj_header.h"

//--- 아래 5개 함수는 사용자 정의 함수 임
void make_vertexShaders();
void make_fragmentShaders();
void make_shaderProgram_();
void InitBuffer();
GLvoid drawScene();
GLvoid Reshape(int w, int h);
GLvoid Keyboard(unsigned char key, int x, int y);
void Motion(int x, int y);

int mouse_control = 1;

class shape {
public:
	std::vector<GLfloat> vertices;
	std::vector<GLfloat> colors;
	std::vector<GLfloat> normals;                // 추가: 정점 노멀
	std::vector<unsigned int> index;
	int mv_state;
	int shape_kind;
	int de;
	int at_mv;
	int valid;
	GLuint vao_shape, vbo_shape[3], ebo_shape;   // vbo 3개로 확장: 0=pos,1=normal,2=color

	void shape_s(GLfloat v[3][3], GLfloat c[9], int sh) {
		shape_kind = sh;
		mv_state = 0;
		de = rand() % 8;
		at_mv = 0;
		valid = 1;
		for (int j = 0; j < 3; j++) {
			for (int i = 0; i < 3; i++) {
				vertices.push_back(v[j][i]);
				colors.push_back(c[j * 3 + i]);
			}
			index.push_back(j * 3);
			index.push_back(j * 3 + 1);
			index.push_back(j * 3 + 2);
		}
		// 노멀은 단순한 면법선으로 채우지 않음(필요하면 추가)
	}
	void rec_s(GLfloat v[4][3], GLfloat c[12]) {
		// 사각형(4정점) 입력을 받아 2개의 삼각형(6 인덱스)으로 만드는 안전한 구현으로 교체
		shape_kind = 4;
		mv_state = 0;
		de = rand() % 8;
		at_mv = 0;
		valid = 1;

		// 현재 vertices 기준 오프셋
		unsigned int base = static_cast<unsigned int>(vertices.size() / 3);

		// 정점과 색상 추가
		for (int j = 0; j < 4; j++) {
			for (int i = 0; i < 3; i++) {
				vertices.push_back(v[j][i]);
				colors.push_back(c[j * 3 + i]);
			}
		}

		// 면 법선 계산 (평면 사각형이므로 하나의 법선을 모든 정점에 할당)
		glm::vec3 A(v[0][0], v[0][1], v[0][2]);
		glm::vec3 B(v[1][0], v[1][1], v[1][2]);
		glm::vec3 C(v[2][0], v[2][1], v[2][2]);
		glm::vec3 n = glm::cross(B - A, C - A);
		if (glm::length(n) > 1e-6f) n = glm::normalize(n);
		else n = glm::vec3(0.0f, 1.0f, 0.0f); // 안전한 기본값

		if (n.y < 0.0f) n = -n;

		// 정점별 노멀 추가
		for (int j = 0; j < 4; ++j) {
			normals.push_back(n.x);
			normals.push_back(n.y);
			normals.push_back(n.z);
		}

		// 인덱스: 2개의 삼각형으로 분할 (base 오프셋 적용)
		index.push_back(base + 0);
		index.push_back(base + 1);
		index.push_back(base + 2);

		index.push_back(base + 0);
		index.push_back(base + 2);
		index.push_back(base + 3);
	}
	void line_s(GLfloat v[6], GLfloat c[6]) {
		shape_kind = 2;
		mv_state = 0;
		de = 0;
		at_mv = 0;
		for (int j = 0; j < 3; j++) {
			for (int i = 0; i < 6; i++) {
				vertices.push_back(v[i]);
				colors.push_back(c[i]);
			}
			index.push_back(j * 3);
			index.push_back(j * 3 + 1);
			index.push_back(j * 3 + 2);
		}
	}
	void square_s(GLfloat v[6][3], GLfloat c[12]) {
		shape_kind = 1;
		mv_state = 0;
		de = 0;
		at_mv = 0;
		for (int j = 0; j < 6; j++) {
			for (int i = 0; i < 3; i++) {
				vertices.push_back(v[j][i]);
				colors.push_back(c[j * 3 + i]);
			}
			index.push_back(j * 4);
			index.push_back(j * 4 + 1);
			index.push_back(j * 4 + 2);
		}
	}
	void cube_s(GLfloat v[8][3], GLfloat c[24]) {
		shape_kind = 8;
		mv_state = 0;
		de = 5;
		at_mv = rand() % 2;
		if (at_mv == 0) {
			at_mv = -1;
			de = 0;
		}
		valid = 1;

		// 현재 vertices 기준 인덱스
		unsigned int base = static_cast<unsigned int>(vertices.size() / 3);

		// 정점/색 추가
		for (int j = 0; j < 8; j++) {
			for (int i = 0; i < 3; i++) {
				vertices.push_back(v[j][i]);
				colors.push_back(c[j * 3 + i]);
			}
		}

		// 로컬 삼각형 인덱스 (face 나누기)
		unsigned int faceIdx[] = {
			0,1,2, 0,2,3,   // back
			4,5,6, 4,6,7,   // front
			0,1,5, 0,5,4,   // bottom/side
			1,2,6, 1,6,5,
			2,3,7, 2,7,6,
			3,0,4, 3,4,7
		};

		// 각 삼각형에 대한 면법선 누적 (정점별 평균)
		const int nVerts = 8;
		std::vector<glm::vec3> acc(nVerts, glm::vec3(0.0f));
		for (size_t t = 0; t < sizeof(faceIdx) / sizeof(faceIdx[0]); t += 3) {
			unsigned int ia = faceIdx[t + 0];
			unsigned int ib = faceIdx[t + 1];
			unsigned int ic = faceIdx[t + 2];
			glm::vec3 A(vertices[(base + ia) * 3 + 0], vertices[(base + ia) * 3 + 1], vertices[(base + ia) * 3 + 2]);
			glm::vec3 B(vertices[(base + ib) * 3 + 0], vertices[(base + ib) * 3 + 1], vertices[(base + ib) * 3 + 2]);
			glm::vec3 C(vertices[(base + ic) * 3 + 0], vertices[(base + ic) * 3 + 1], vertices[(base + ic) * 3 + 2]);
			glm::vec3 n = glm::cross(B - A, C - A);
			if (glm::length(n) > 0.00001f) n = glm::normalize(n);
			acc[ia] += n;
			acc[ib] += n;
			acc[ic] += n;
		}
		// 정규화하여 normals에 저장
		for (int i = 0; i < nVerts; ++i) {
			glm::vec3 nn = glm::normalize(acc[i]);
			if (!glm::isnan(nn.x) && !glm::isnan(nn.y) && !glm::isnan(nn.z)) {
				normals.push_back(nn.x);
				normals.push_back(nn.y);
				normals.push_back(nn.z);
			}
			else {
				// 예비값
				normals.push_back(0.0f);
				normals.push_back(1.0f);
				normals.push_back(0.0f);
			}
		}

		// 인덱스는 base 오프셋 적용하여 추가
		for (size_t k = 0; k < sizeof(faceIdx) / sizeof(faceIdx[0]); ++k) {
			index.push_back(base + faceIdx[k]);
		}
	}

	void pyramid_s(GLfloat v[5][3], GLfloat c[15]) {
		shape_kind = 5;
		mv_state = 0;
		de = rand() % 8;
		at_mv = 0;
		valid = 1;

		unsigned int base = static_cast<unsigned int>(vertices.size() / 3);

		// 정점, 색상 추가 (base 4 + apex)
		for (int j = 0; j < 5; j++) {
			for (int i = 0; i < 3; i++) {
				vertices.push_back(v[j][i]);
				colors.push_back(c[j * 3 + i]);
			}
		}

		// 로컬 인덱스 정의
		unsigned int localIdx[] = {
			0,1,2, 0,2,3, // base
			0,4,1,
			1,4,2,
			2,4,3,
			3,4,0
		};

		// 노멀 누적 (5 정점)
		const int nVerts = 5;
		std::vector<glm::vec3> acc(nVerts, glm::vec3(0.0f));
		for (size_t t = 0; t < sizeof(localIdx) / sizeof(localIdx[0]); t += 3) {
			unsigned int ia = localIdx[t + 0];
			unsigned int ib = localIdx[t + 1];
			unsigned int ic = localIdx[t + 2];
			glm::vec3 A(vertices[(base + ia) * 3 + 0], vertices[(base + ia) * 3 + 1], vertices[(base + ia) * 3 + 2]);
			glm::vec3 B(vertices[(base + ib) * 3 + 0], vertices[(base + ib) * 3 + 1], vertices[(base + ib) * 3 + 2]);
			glm::vec3 C(vertices[(base + ic) * 3 + 0], vertices[(base + ic) * 3 + 1], vertices[(base + ic) * 3 + 2]);
			glm::vec3 n = glm::cross(B - A, C - A);
			if (glm::length(n) > 0.00001f) n = glm::normalize(n);
			acc[ia] += n;
			acc[ib] += n;
			acc[ic] += n;
		}
		for (int i = 0; i < nVerts; ++i) {
			glm::vec3 nn = glm::normalize(acc[i]);
			if (!glm::isnan(nn.x) && !glm::isnan(nn.y) && !glm::isnan(nn.z)) {
				normals.push_back(nn.x);
				normals.push_back(nn.y);
				normals.push_back(nn.z);
			}
			else {
				normals.push_back(0.0f); normals.push_back(1.0f); normals.push_back(0.0f);
			}
		}

		// 인덱스 추가 (base 오프셋)
		for (size_t k = 0; k < sizeof(localIdx) / sizeof(localIdx[0]); ++k) {
			index.push_back(base + localIdx[k]);
		}
	}
	shape() {

	}
};

void loadModelToShape(const char* filename, shape& s);
void buffer(shape& temp);
void lever_action(int v);
void slot_action(int v);

auto ensureNormals = [](shape& s) {
	size_t vcount = s.vertices.size() / 3;
	size_t ncount = s.normals.size() / 3;
	std::cerr << "shape verts=" << vcount << " cols=" << (s.colors.size() / 3)
		<< " norms=" << ncount << " idx=" << s.index.size() << std::endl;
	if (ncount == 0 && vcount > 0) {
		// 모두 위쪽 노멀로 초기화(간단한 보정)
		s.normals.reserve(vcount * 3);
		for (size_t i = 0; i < vcount; ++i) {
			s.normals.push_back(0.0f);
			s.normals.push_back(1.0f);
			s.normals.push_back(0.0f);
		}
		std::cerr << "  -> normals were empty: filled with (0,1,0) for " << vcount << " verts\n";
	}
	else if (ncount != vcount) {
		std::cerr << "  -> WARNING: normals count != vertex count\n";
	}
};

shape bottom;
shape machine;

shape slot;
float slot_angle[3] = { 0.0f };
int slot_value[3] = {0};
int slot_target[3] = { 0,0,0 };

shape lever;
float lever_angle = 0.0f;

//--- 필요한 변수 선언
GLint width, height;
GLchar* vertexSource, * fragmentSource;
GLuint vertexShader, fragmentShader;
GLuint shaderProgramID;

GLfloat bk_color[3] = { 0.3f,0.3f,0.3f };

float camera_rocate[3] = { 0.0f, 0.0f, -10.0f };
float camera_angle[2] = { 0.0f, 0.0f };

// 추가: 마우스 상태 저장 및 감도
int lastMouseX = -1;
int lastMouseY = -1;
float mouseSensitivity = 0.05f;

bool lever_protect = false;

// 추가: 커서 고정 상태 플래그
bool cursorLocked = false;

char* filetobuf(const char* file)
{
	FILE* fptr = fopen(file, "rb");
	if (!fptr) return NULL;
	fseek(fptr, 0, SEEK_END);
	long length = ftell(fptr);
	fseek(fptr, 0, SEEK_SET);
	char* buf = (char*)malloc(length + 1);
	if (!buf) { fclose(fptr); return NULL; }
	fread(buf, 1, length, fptr);
	fclose(fptr);

	// UTF-8 BOM 제거 (0xEF,0xBB,0xBF)
	if (length >= 3 &&
		(unsigned char)buf[0] == 0xEF &&
		(unsigned char)buf[1] == 0xBB &&
		(unsigned char)buf[2] == 0xBF)
	{
		// 앞의 3바이트 건너뛰고 데이터를 앞으로 당김
		for (long i = 3; i < length; ++i) buf[i - 3] = buf[i];
		length -= 3;
	}

	buf[length] = '\0';
	return buf;
}


//--- 메인 함수
void main(int argc, char** argv) //--- 윈도우 출력하고 콜백함수 설정
{
	srand((unsigned int)time(NULL));
	width = 1600;
	height = 1300;
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(width, height);
	glutCreateWindow("Example1");
	//--- GLEW 초기화하기
	glewExperimental = GL_TRUE;
	glEnable(GL_DEPTH_TEST);
	/*glEnable(GL_CULL_FACE);*/
	glewInit();
	make_shaderProgram_();
	GLfloat b_v[4][3] = {
		{-100.0f, 0.0f, -100.0f},
		{ 100.0f, 0.0f, -100.0f},
		{ 100.0f, 0.0f,  100.0f},
		{-100.0f, 0.0f,  100.0f}
	};
	GLfloat b_c[12] = {
		0.6f, 0.6f, 0.6f,
		0.6f, 0.6f, 0.6f,
		0.6f, 0.6f, 0.6f,
		0.6f, 0.6f, 0.6f
	};

	bottom.rec_s(b_v, b_c);
	ensureNormals(bottom);

	loadModelToShape("machine.obj", machine);
	ensureNormals(machine);

	loadModelToShape("lever.obj", lever);
	ensureNormals(lever);

	loadModelToShape("slot.obj", slot);
	ensureNormals(slot);

	InitBuffer();
	glutMotionFunc(Motion);
	// 마우스 클릭 없이 움직일 때도 카메라 회전 허용하려면 passive motion 콜백 등록
	glutPassiveMotionFunc(Motion);
	glutDisplayFunc(drawScene); //--- 출력 콜백 함수
	glutReshapeFunc(Reshape);
	glutKeyboardFunc(Keyboard);

	// 시작 시 mouse_control == 1이면 커서 고정(중앙)으로 설정
	if (mouse_control == 1) {
		cursorLocked = true;
		glutSetCursor(GLUT_CURSOR_NONE);
		int cx = width / 2;
		int cy = height / 2;
		glutWarpPointer(cx, cy);
		// lastMouse를 초기화하여 첫 프레임에서 델타가 크게 나오지 않도록 함
		lastMouseX = -1;
		lastMouseY = -1;
	}

	glutMainLoop();
}

void make_vertexShaders()
{
	//--- 버텍스 세이더 읽어 저장하고 컴파일 하기
	//--- filetobuf: 사용자정의 함수로 텍스트를 읽어서 문자열에 저장하는 함수
	vertexSource = filetobuf("vertex.glsl");
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, (const GLchar**)&vertexSource, 0);
	glCompileShader(vertexShader);
	GLint result;
	GLchar errorLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, errorLog);
		std::cerr << "ERROR: vertex shader 컴파일 실패\n" << errorLog << std::endl;
		return;
	}
}

void make_fragmentShaders()
{
	//--- 프래그먼트 세이더 읽어 저장하고 컴파일하기
	fragmentSource = filetobuf("fragment.glsl"); // 프래그세이더 읽어오기
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, (const GLchar**)&fragmentSource, 0);
	glCompileShader(fragmentShader);
	GLint result;
	GLchar errorLog[512];
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, errorLog);
		std::cerr << "ERROR: frag_shader 컴파일 실패\n" << errorLog << std::endl;
		return;
	}
}

void InitBuffer() {
	buffer(bottom);
	buffer(machine);
	buffer(lever);
	buffer(slot);
	// 공통: 라이트 색상 초기화
	glUseProgram(shaderProgramID);
	GLint lightColorLocation = glGetUniformLocation(shaderProgramID, "lightColor");
	if (lightColorLocation >= 0) glUniform3f(lightColorLocation, 1.0f, 1.0f, 1.0f);
}

void make_shaderProgram_() {
	make_vertexShaders();
	make_fragmentShaders();
	shaderProgramID = glCreateProgram();
	glAttachShader(shaderProgramID, vertexShader);
	glAttachShader(shaderProgramID, fragmentShader);
	glLinkProgram(shaderProgramID);

	GLint linked = 0;
	glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &linked);
	if (!linked) {
		GLchar progLog[1024];
		glGetProgramInfoLog(shaderProgramID, sizeof(progLog), NULL, progLog);
		std::cerr << "ERROR: shader program 링크 실패\n" << progLog << std::endl;
		exit(EXIT_FAILURE);
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	glUseProgram(shaderProgramID);
}
GLvoid drawScene() {
	glClearColor(bk_color[0], bk_color[1], bk_color[2], 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(shaderProgramID);

	// projection/view/model 설정 (기존 코드 유지)
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 200.0f);
	glm::mat4 view = glm::mat4(1.0f);
	view = glm::rotate(view, glm::radians(camera_angle[0]), glm::vec3(1.0f, 0.0f, 0.0f));
	view = glm::rotate(view, glm::radians(camera_angle[1]), glm::vec3(0.0f, 1.0f, 0.0f));
	view = glm::translate(view, glm::vec3(camera_rocate[0], -3.0f, camera_rocate[2]));

	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));

	GLint locProj = glGetUniformLocation(shaderProgramID, "projection");
	if (locProj >= 0) glUniformMatrix4fv(locProj, 1, GL_FALSE, glm::value_ptr(projection));
	GLint locView = glGetUniformLocation(shaderProgramID, "view");
	if (locView >= 0) glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(view));
	GLint locModel = glGetUniformLocation(shaderProgramID, "model");
	if (locModel >= 0) glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));

	// 라이트/뷰/오브젝트 컬러 유니폼 전달 (기존 코드 유지)
	glm::vec3 lightPos_world = glm::vec3(0.0f, 10.0f, 0.0f);
	GLint locLightPos = glGetUniformLocation(shaderProgramID, "lightPos");
	if (locLightPos >= 0) glUniform3f(locLightPos, lightPos_world.x, lightPos_world.y, lightPos_world.z);
	GLint locViewPos = glGetUniformLocation(shaderProgramID, "viewPos");
	glm::vec3 camPos = glm::vec3(glm::inverse(view) * glm::vec4(0, 0, 0, 1));
	if (locViewPos >= 0) glUniform3f(locViewPos, camPos.x, camPos.y, camPos.z);
	GLint locObjColor = glGetUniformLocation(shaderProgramID, "objectColor");
	if (locObjColor >= 0) glUniform3f(locObjColor, 0.6f, 0.6f, 0.6f);

	// bottom 그리기
	glBindVertexArray(bottom.vao_shape);
	glDrawElements(GL_TRIANGLES, (GLsizei)bottom.index.size(), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	// machine 그리기 (있을 때)
	if (!machine.vertices.empty() && !machine.index.empty()) {
		glm::mat4 model_m = glm::mat4(1.0f);
		model_m = glm::translate(model_m, glm::vec3(0.0f, 2.0f, 0.0f));

		if (locModel >= 0) glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model_m));

		glBindVertexArray(machine.vao_shape);
		glDrawElements(GL_TRIANGLES, (GLsizei)machine.index.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}

	if (!lever.vertices.empty() && !lever.index.empty()) {
		glm::mat4 model_m = glm::mat4(1.0f);
		model_m = glm::translate(model_m, glm::vec3(1.14f, 2.7f, 0.0f));
		glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(lever_angle), glm::vec3(1.0f, 0.0f, 0.0f));

		glm::mat4 UNI = glm::mat4(1.0f);
		UNI = model_m * rotation;

		if (locModel >= 0) glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(UNI));

		glBindVertexArray(lever.vao_shape);
		glDrawElements(GL_TRIANGLES, (GLsizei)lever.index.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}

	if (!slot.vertices.empty() && !slot.index.empty()) {
		for (int i = 0; i < 3; i++) {
			glm::mat4 model_m = glm::mat4(1.0f);
			model_m = glm::translate(model_m, glm::vec3(0.5f*i-0.5f, 2.7f, 0.5f));
			glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(slot_angle[i]), glm::vec3(1.0f, 0.0f, 0.0f));

			glm::mat4 UNI = glm::mat4(1.0f);
			UNI = model_m * rotation;

			if (locModel >= 0) glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(UNI));

			glBindVertexArray(slot.vao_shape);
			glDrawElements(GL_TRIANGLES, (GLsizei)slot.index.size(), GL_UNSIGNED_INT, 0);
			glBindVertexArray(0);
		}
	}

	
	GLenum err = glGetError();
	if (err != GL_NO_ERROR) {
		std::cerr << "GL error in drawScene(): 0x" << std::hex << err << std::dec << std::endl;
	}

	glutSwapBuffers();
}
//--- 다시그리기 콜백 함수
GLvoid Reshape(int w, int h)
{
	glViewport(0, 0, w, h);
}

GLvoid Keyboard(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 'w':
		camera_rocate[2] += 0.1f;
		break;
	case 's':
		camera_rocate[2] -= 0.1f;
		break;
	case 'a':
		camera_rocate[0] += 0.1f;
		break;
	case 'd':
		camera_rocate[0] -= 0.1f;
		break;
	case 'q':
		if (!lever_protect) {
			lever_protect = true;
			lever_action(0);
		}
		break;
	case 'e':
		
		break;
	case 'r':
		// 토글: 마우스 제어 모드 전환 및 커서 고정/해제 처리
		if (mouse_control != 1) {
			mouse_control = 1;
			// 고정 활성화
			cursorLocked = true;
			glutSetCursor(GLUT_CURSOR_NONE); // 커서 숨김
			int cx = width / 2;
			int cy = height / 2;
			glutWarpPointer(cx, cy); // 중앙으로 이동
			lastMouseX = -1;
			lastMouseY = -1;
		}
		else {
			mouse_control = 0;
			// 고정 해제
			cursorLocked = false;
			glutSetCursor(GLUT_CURSOR_LEFT_ARROW); // 커서 보임(기본 화살표)
			// 선택: 해제 시 포인터를 중앙에서 그대로 두거나 필요하면 이동 가능
			// lastMouse 초기화
			lastMouseX = -1;
			lastMouseY = -1;
		}
		break;
	}
	InitBuffer();
	glutPostRedisplay();
}

void Motion(int x, int y) {

	// mouse_control == 1 인 경우: 마우스 이동으로 카메라 회전 처리
	if (mouse_control == 1) {
		// 커서가 중앙에 고정된 모드
		if (cursorLocked) {
			int cx = width / 2;
			int cy = height / 2;

			// 중심 기준 델타 계산
			int dx = x - cx;
			int dy = y - cy;

			// 델타가 0이면 변화 없음 (워프에서 오는 이벤트 등 무시)
			if (dx != 0 || dy != 0) {
				camera_angle[1] += dx * mouseSensitivity; // yaw (Y축 회전)
				camera_angle[0] += dy * mouseSensitivity; // pitch (X축 회전)

				// 피치 제한(너무 돌아가지 않게)
				if (camera_angle[0] > 89.0f) camera_angle[0] = 89.0f;
				if (camera_angle[0] < -89.0f) camera_angle[0] = -89.0f;
			}

			// 포인터를 다시 중앙으로 옮겨 계속 고정 (워프는 또 이벤트를 발생시킴 — 델타가 0이라 처리됨)
			glutWarpPointer(cx, cy);

			// 화면 갱신
			glutPostRedisplay();
			return;
		}

		// 커서가 고정되지 않은 일반 모드: 기존 델타 기반 처리
		// 초기값 설정: 첫 입력은 이동으로 처리하지 않음
		if (lastMouseX < 0 || lastMouseY < 0) {
			lastMouseX = x;
			lastMouseY = y;
			return;
		}

		int dx = x - lastMouseX;
		int dy = y - lastMouseY;
		lastMouseX = x;
		lastMouseY = y;

		// 민감도 적용: dx -> yaw, dy -> pitch
		camera_angle[1] += dx * mouseSensitivity; // yaw (Y축 회전)
		camera_angle[0] += dy * mouseSensitivity; // pitch (X축 회전)

		// 피치 제한(너무 돌아가지 않게)
		if (camera_angle[0] > 89.0f) camera_angle[0] = 89.0f;
		if (camera_angle[0] < -89.0f) camera_angle[0] = -89.0f;

		// 화면 갱신
		glutPostRedisplay();
		return;
	}

	// mouse_control != 1 일때는 기존 동작 유지
	lastMouseX = -1;
	lastMouseY = -1;
	InitBuffer();
	glutPostRedisplay();
}

void lever_action(int v) {
	if (v < 15) {
		lever_angle += 90.0f / 15.0f;
	}
	else if (v < 30) {
		lever_angle -= 90.0f / 15.0f;
	}
	else {
		lever_protect = false;

		for (int i = 0; i < 3; i++) {
			slot_value[i] = 15*28+15*i;
		}

		// 각 슬롯 애니메이션 시작
		slot_action(0);
		slot_action(1);
		slot_action(2);
		return;
	}
	glutTimerFunc(16, lever_action, v + 1);
}

void slot_action(int v) {
	if (slot_value[v] == 0) return;

	slot_angle[v] += 360.f/7/15;
	if (slot_angle[v] == 360.0f) slot_angle[v] = 0.0f;
	
	slot_value[v]--;
	glutPostRedisplay();
	glutTimerFunc(16, slot_action, v);
	return;
}


void loadModelToShape(const char* filename, shape& s) {
	// Model 구조체는 obj_header.h에 정의되어 있음
	Model m = { nullptr, 0, nullptr, 0 };
	read_obj_file(filename, &m);
	if (m.vertex_count == 0 || m.face_count == 0) {
		std::cerr << "read_obj_file failed or empty: " << filename << std::endl;
		return;
	}

	// 초기화
	s.vertices.clear();
	s.colors.clear();
	s.normals.clear();
	s.index.clear();

	s.vertices.reserve(m.vertex_count * 3);
	s.colors.reserve(m.vertex_count * 3);

	// 정점 + 기본 색상 복사
	for (size_t i = 0; i < m.vertex_count; ++i) {
		s.vertices.push_back(m.vertices[i].x);
		s.vertices.push_back(m.vertices[i].y);
		s.vertices.push_back(m.vertices[i].z);
		// 기본 색상: 연한 회색 (원하면 변경)
		s.colors.push_back(0.8f);
		s.colors.push_back(0.8f);
		s.colors.push_back(0.8f);
	}

	// faces의 인덱스가 1-based인지 0-based인지 감지
	unsigned int maxIdx = 0;
	for (size_t f = 0; f < m.face_count; ++f) {
		maxIdx = std::max(maxIdx, m.faces[f].v1);
		maxIdx = std::max(maxIdx, m.faces[f].v2);
		maxIdx = std::max(maxIdx, m.faces[f].v3);
	}
	bool oneBased = (maxIdx == m.vertex_count);

	// 법선 누적용 버퍼 생성
	std::vector<glm::vec3> accum(m.vertex_count, glm::vec3(0.0f));

	// 인덱스 및 면법선 누적
	for (size_t f = 0; f < m.face_count; ++f) {
		unsigned int ia = m.faces[f].v1;
		unsigned int ib = m.faces[f].v2;
		unsigned int ic = m.faces[f].v3;
		if (oneBased) { if (ia > 0) --ia; if (ib > 0) --ib; if (ic > 0) --ic; }

		// 안전 범위 검사
		if (ia >= m.vertex_count || ib >= m.vertex_count || ic >= m.vertex_count) continue;

		glm::vec3 A(s.vertices[ia * 3 + 0], s.vertices[ia * 3 + 1], s.vertices[ia * 3 + 2]);
		glm::vec3 B(s.vertices[ib * 3 + 0], s.vertices[ib * 3 + 1], s.vertices[ib * 3 + 2]);
		glm::vec3 C(s.vertices[ic * 3 + 0], s.vertices[ic * 3 + 1], s.vertices[ic * 3 + 2]);

		glm::vec3 n = glm::cross(B - A, C - A);
		if (glm::length(n) > 1e-6f) n = glm::normalize(n);

		accum[ia] += n;
		accum[ib] += n;
		accum[ic] += n;

		s.index.push_back(static_cast<unsigned int>(ia));
		s.index.push_back(static_cast<unsigned int>(ib));
		s.index.push_back(static_cast<unsigned int>(ic));
	}

	// 정규화된 정점 노멀 생성
	s.normals.reserve(m.vertex_count * 3);
	for (size_t i = 0; i < m.vertex_count; ++i) {
		glm::vec3 nn = glm::normalize(accum[i]);
		if (glm::length(nn) < 1e-6f) nn = glm::vec3(0.0f, 1.0f, 0.0f); // 안전값
		s.normals.push_back(nn.x);
		s.normals.push_back(nn.y);
		s.normals.push_back(nn.z);
	}

	s.valid = 1;

	// read_obj_file이 어떤 할당 방식을 썼는지 확실치 않으므로 free 사용 (헤더가 C 스타일)
	if (m.vertices) free(m.vertices);
	if (m.faces) free(m.faces);
}

void buffer(shape& temp) {
	glGenVertexArrays(1, &temp.vao_shape);
	glBindVertexArray(temp.vao_shape);
	glGenBuffers(3, temp.vbo_shape);
	glBindBuffer(GL_ARRAY_BUFFER, temp.vbo_shape[0]);
	glBufferData(GL_ARRAY_BUFFER, temp.vertices.size() * sizeof(GLfloat), temp.vertices.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, temp.vbo_shape[1]);
	glBufferData(GL_ARRAY_BUFFER, temp.normals.size() * sizeof(GLfloat), temp.normals.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, temp.vbo_shape[2]);
	glBufferData(GL_ARRAY_BUFFER, temp.colors.size() * sizeof(GLfloat), temp.colors.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(2);
	glGenBuffers(1, &temp.ebo_shape);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, temp.ebo_shape);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, temp.index.size() * sizeof(unsigned int), temp.index.data(), GL_STATIC_DRAW);
}