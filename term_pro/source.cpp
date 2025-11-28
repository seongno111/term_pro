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
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <fstream>
#include <sstream>
#include <string>
#include <cctype>
#include <unordered_map>
#include <cstdint>

GLuint g_textureID = 0;
GLuint loadTexture(const char* path);
//--- 아래 5개 함수는 사용자 정의 함수 임
void make_vertexShaders();
void make_fragmentShaders();
void make_shaderProgram_();
void InitBuffer();
GLvoid drawScene();
GLvoid Reshape(int w, int h);
GLvoid Keyboard(unsigned char key, int x, int y);
GLvoid KeyboardUp(unsigned char key, int x, int y);
void Motion(int x, int y);
void processKeyHolds(int v);

int mouse_control = 1;
static std::array<bool, 256> g_keyDown = {};

class shape {
public:
	std::vector<GLfloat> vertices;
	std::vector<GLfloat> colors;
	std::vector<GLfloat> normals;                // 추가: 정점 노멀
	std::vector<unsigned int> index;
	std::vector<GLfloat> texcoords;
	int mv_state;
	int shape_kind;
	int de;
	int at_mv;
	int valid;
	GLuint vao_shape, vbo_shape[4], ebo_shape;   // vbo 3개로 확장: 0=pos,1=normal,2=color
	GLuint textureID = 0;

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
void lever_action_1(int v);
void lever_action_2(int v);
void slot_action(int v);
void coin_insert_ready(int v);
void coin_insert(int v);
void coin_drop(int v);
void jack_pot_1(int v);
void jack_pot_2(int v);
void jack_pot_3(int v);
glm::vec3 getCameraWorldPos();

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

shape many_coins;
bool m_coins = false;
float m_coins_trans = 0.4f;

shape one_coin;
bool coin_protect = false;
bool coin_inserted = false;
float one_coin_angle = 90.0f;
float one_coin_translate[3] = { 0.8f, 2.0f, 1.2f };
float coin_move_value[2] = { 0.0f };
int coin_ready_total_steps = 60;
int coin_ready_steps_remaining = 0;
glm::vec3 coin_start_pos;
glm::vec3 coin_target_pos;
float coin_start_angle = 90.0f;
float coin_target_angle = 0.0f;
int camera_move_total_steps = 60;
int camera_move_steps_remaining = 0;
glm::vec2 camera_start_move;   // x, z
glm::vec2 camera_target_move;  // x, z
glm::vec2 camera_start_angle = glm::vec2(0.0f, 0.0f);   // pitch(x), yaw(y)
glm::vec2 camera_target_angle = glm::vec2(0.0f, 0.0f);

shape slot;
float slot_angle[3] = { 0.0f };
int slot_value[3] = { 0 };
int slot_target[3] = { 0,0,0 };
int slot_speed = 0;

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

bool lever_protect = true;

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

	//--- GLEW 초기화하기 (한 번만)
	glewExperimental = GL_TRUE;
	GLenum glewErr = glewInit();
	if (glewErr != GLEW_OK) {
		std::cerr << "ERROR: glewInit() failed: " << glewGetErrorString(glewErr) << std::endl;
		exit(EXIT_FAILURE);
	}
	// 이제 안전하게 OpenGL 상태를 설정
	glEnable(GL_DEPTH_TEST);
	/*glEnable(GL_CULL_FACE);*/

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

	loadModelToShape("coin.obj", one_coin);
	ensureNormals(one_coin);

	loadModelToShape("coins_L.obj", many_coins);
	ensureNormals(many_coins);

	InitBuffer();
	glutMotionFunc(Motion);
	// 마우스 클릭 없이 움직일 때도 카메라 회전 허용하려면 passive motion 콜백 등록
	glutPassiveMotionFunc(Motion);
	glutDisplayFunc(drawScene); //--- 출력 콜백 함수
	glutReshapeFunc(Reshape);
	glutKeyboardFunc(Keyboard);
	glutKeyboardUpFunc(KeyboardUp);
	glutIgnoreKeyRepeat(1);

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
	glutTimerFunc(16, processKeyHolds, 0);
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
	buffer(one_coin);
	buffer(many_coins);
	// 공통: 라이트 색상 초기화
	glUseProgram(shaderProgramID);
	GLint lightColorLocation = glGetUniformLocation(shaderProgramID, "lightColor");
	if (lightColorLocation >= 0) glUniform3f(lightColorLocation, 1.0f, 1.0f, 1.0f);

	if (slot.textureID == 0) {
		slot.textureID = loadTexture("slot.png");
		if (slot.textureID != 0) {
			glBindTexture(GL_TEXTURE_2D, slot.textureID);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
	}
	if (machine.textureID == 0) {
		machine.textureID = loadTexture("machine.png");
		if (machine.textureID != 0) {
			glBindTexture(GL_TEXTURE_2D, machine.textureID);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
	}
	if (one_coin.textureID == 0) {
		one_coin.textureID = loadTexture("coin.png");
		if (one_coin.textureID != 0) {
			glBindTexture(GL_TEXTURE_2D, one_coin.textureID);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
	}
	if (lever.textureID == 0) {
		lever.textureID = loadTexture("lever.png");
		if (lever.textureID != 0) {
			glBindTexture(GL_TEXTURE_2D, lever.textureID);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
	}
	if (many_coins.textureID == 0) {
		many_coins.textureID = loadTexture("many_coin.png");
		if (many_coins.textureID != 0) {
			glBindTexture(GL_TEXTURE_2D, many_coins.textureID);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
	}
	GLint texLoc = glGetUniformLocation(shaderProgramID, "texSampler");
	if (texLoc >= 0) {
		glUniform1i(texLoc, 0);
	}
	GLint useTexLoc = glGetUniformLocation(shaderProgramID, "useTexture");
	if (useTexLoc >= 0) glUniform1i(useTexLoc, g_textureID ? 1 : 0);
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
	glViewport(0, 0, width, height);
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
		if (machine.textureID != 0) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, machine.textureID);
			GLint useTexLoc = glGetUniformLocation(shaderProgramID, "useTexture");
			if (useTexLoc >= 0) glUniform1i(useTexLoc, 1);
		}
		glm::mat4 model_m = glm::mat4(1.0f);
		model_m = glm::translate(model_m, glm::vec3(0.0f, 2.0f, 0.0f));

		if (locModel >= 0) glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model_m));

		glBindVertexArray(machine.vao_shape);
		glDrawElements(GL_TRIANGLES, (GLsizei)machine.index.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		if (machine.textureID != 0) {
			glBindTexture(GL_TEXTURE_2D, 0);
			GLint useTexLoc = glGetUniformLocation(shaderProgramID, "useTexture");
			if (useTexLoc >= 0) glUniform1i(useTexLoc, 0);
		}
	}

	if (!lever.vertices.empty() && !lever.index.empty()) {
		if (lever.textureID != 0) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, lever.textureID);
			GLint useTexLoc = glGetUniformLocation(shaderProgramID, "useTexture");
			if (useTexLoc >= 0) glUniform1i(useTexLoc, 1);
		}
		glm::mat4 model_m = glm::mat4(1.0f);
		model_m = glm::translate(model_m, glm::vec3(1.14f, 2.7f, 0.0f));
		glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(lever_angle), glm::vec3(1.0f, 0.0f, 0.0f));

		glm::mat4 UNI = glm::mat4(1.0f);
		UNI = model_m * rotation;

		if (locModel >= 0) glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(UNI));

		glBindVertexArray(lever.vao_shape);
		glDrawElements(GL_TRIANGLES, (GLsizei)lever.index.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
		if (lever.textureID != 0) {
			glBindTexture(GL_TEXTURE_2D, 0);
			GLint useTexLoc = glGetUniformLocation(shaderProgramID, "useTexture");
			if (useTexLoc >= 0) glUniform1i(useTexLoc, 0);
		}
	}

	if (!slot.vertices.empty() && !slot.index.empty()) {
		// 텍스처 바인드
		if (slot.textureID != 0) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, slot.textureID);
			GLint useTexLoc = glGetUniformLocation(shaderProgramID, "useTexture");
			if (useTexLoc >= 0) glUniform1i(useTexLoc, 1);
		}
		for (int i = 0; i < 3; i++) {
			glm::mat4 model_m = glm::mat4(1.0f);
			model_m = glm::translate(model_m, glm::vec3(0.5f * i - 0.5f, 2.7f, 0.5f));
			glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(slot_angle[i]), glm::vec3(1.0f, 0.0f, 0.0f));

			glm::mat4 UNI = glm::mat4(1.0f);
			UNI = model_m * rotation;

			if (locModel >= 0) glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(UNI));

			glBindVertexArray(slot.vao_shape);
			glDrawElements(GL_TRIANGLES, (GLsizei)slot.index.size(), GL_UNSIGNED_INT, 0);
			glBindVertexArray(0);
		}
		// 텍스처 언바인드 및 플래그 끄기
		if (slot.textureID != 0) {
			glBindTexture(GL_TEXTURE_2D, 0);
			GLint useTexLoc = glGetUniformLocation(shaderProgramID, "useTexture");
			if (useTexLoc >= 0) glUniform1i(useTexLoc, 0);
		}
	}

	if (!one_coin.vertices.empty() && !one_coin.index.empty()) {
		if (one_coin.textureID != 0) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, one_coin.textureID);
			GLint useTexLoc = glGetUniformLocation(shaderProgramID, "useTexture");
			if (useTexLoc >= 0) glUniform1i(useTexLoc, 1);
		}
		glm::mat4 model_m = glm::mat4(1.0f);
		model_m = glm::translate(model_m, glm::vec3(one_coin_translate[0], one_coin_translate[1], one_coin_translate[2]));
		glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(one_coin_angle), glm::vec3(0.0f, 0.0f, 1.0f));

		glm::mat4 UNI = glm::mat4(1.0f);
		UNI = model_m * rotation;

		if (locModel >= 0) glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(UNI));

		glBindVertexArray(one_coin.vao_shape);
		glDrawElements(GL_TRIANGLES, (GLsizei)one_coin.index.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
		if (one_coin.textureID != 0) {
			glBindTexture(GL_TEXTURE_2D, 0);
			GLint useTexLoc = glGetUniformLocation(shaderProgramID, "useTexture");
			if (useTexLoc >= 0) glUniform1i(useTexLoc, 0);
		}
	}

	if (!many_coins.vertices.empty() && !many_coins.index.empty() && m_coins) {
		if (many_coins.textureID != 0) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, many_coins.textureID);
			GLint useTexLoc = glGetUniformLocation(shaderProgramID, "useTexture");
			if (useTexLoc >= 0) glUniform1i(useTexLoc, 1);
		}
		glm::mat4 model_m = glm::mat4(1.0f);
		model_m = glm::translate(model_m, glm::vec3(0.0f, 1.0f, m_coins_trans));
		glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));

		glm::mat4 UNI = glm::mat4(1.0f);
		UNI = model_m * rotation;

		if (locModel >= 0) glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(UNI));

		glBindVertexArray(many_coins.vao_shape);
		glDrawElements(GL_TRIANGLES, (GLsizei)many_coins.index.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
		if (many_coins.textureID != 0) {
			glBindTexture(GL_TEXTURE_2D, 0);
			GLint useTexLoc = glGetUniformLocation(shaderProgramID, "useTexture");
			if (useTexLoc >= 0) glUniform1i(useTexLoc, 0);
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
	// machine의 월드 위치 (drawScene에서 model_m = translate(0,2,0) 사용)
	glm::vec3 machinePos = glm::vec3(0.0f, 2.0f, 0.0f);
	// 카메라 월드 위치 계산
	glm::vec3 camPos = getCameraWorldPos();

	// 허용 범위: x 너비 3.0 (±1.5), z 거리 3.0 (±3.0)
	float halfWidthX = 1.5f;
	float maxZdist = 3.0f;

	float dx = fabs(camPos.x - machinePos.x);
	float dz = fabs(camPos.z - machinePos.z);
	unsigned char k = static_cast<unsigned char>(key);
	if (!g_keyDown[key]) {
		g_keyDown[key] = true;
	}
	switch (key)
	{
	case 'w':
		if (mouse_control == 1) {
			camera_rocate[2] += 0.1f;
		}
		break;
	case 's':
		if (mouse_control == 1) {
			camera_rocate[2] -= 0.1f;
		}
		break;
	case 'a':
		if (mouse_control == 1) {
			camera_rocate[0] += 0.1f;
		}
		break;
	case 'd':
		if (mouse_control == 1) {
			camera_rocate[0] -= 0.1f;
		}
		break;
	case 'q':
		if (!lever_protect && coin_inserted) {
			lever_protect = true;
			lever_action_1(0);
		}
		break;
	case 'e':
		if (dx <= halfWidthX && dz <= maxZdist) {
			if (!coin_protect && !coin_inserted) {
				coin_protect = true;
				coin_move_value[0] = 1.18f - one_coin_translate[0];
				coin_move_value[1] = 1.95f - one_coin_translate[1];
				coin_insert_ready(15);
			}
		}
		else {
			std::cerr << "Too far from machine to insert coin. camPos=(" << camPos.x << "," << camPos.y << "," << camPos.z << "), dx=" << dx << " dz=" << dz << std::endl;
		}
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
			if (coin_inserted) {
				one_coin_angle = 90.0f;
				one_coin_translate[0] = 0.8f;
				one_coin_translate[1] = 6.0f;
				one_coin_translate[2] = 1.2f;
				coin_drop(0);
			}
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

	glutPostRedisplay();
}
GLvoid KeyboardUp(unsigned char key, int x, int y)
{
	unsigned char k = static_cast<unsigned char>(key);
	switch (key)
	{
	case 'q':
		if (lever_protect && coin_inserted) {
			lever_action_2(15);
		}
		break;
	}

	if (g_keyDown[k]) g_keyDown[k] = false;

}

void processKeyHolds(int v)
{
	// 호출 주기: 약 16ms (60FPS). 필요한 경우 변경.
	const float camSpeed = 0.05f;    // 한 틱당 카메라 이동량
	const float leverSpeed = 6.0f;  // 한 틱당 레버 회전(도)

	// 이동 키(마우스 제어 모드일 때만)
	if (mouse_control == 1) {
		if (g_keyDown[(unsigned char)'w']) camera_rocate[2] += camSpeed;
		if (g_keyDown[(unsigned char)'s']) camera_rocate[2] -= camSpeed;
		if (g_keyDown[(unsigned char)'a']) camera_rocate[0] += camSpeed;
		if (g_keyDown[(unsigned char)'d']) camera_rocate[0] -= camSpeed;
	}


	if (g_keyDown[(unsigned char)'q']) {
		if (lever_protect) {
			if (slot_speed < 29) {
				slot_speed += 1;
			}
		}
	}

	glutPostRedisplay();
	glutTimerFunc(16, processKeyHolds, 0);
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
	glutPostRedisplay();
}

glm::vec3 getCameraWorldPos() {
	glm::mat4 view = glm::mat4(1.0f);
	view = glm::rotate(view, glm::radians(camera_angle[0]), glm::vec3(1.0f, 0.0f, 0.0f));
	view = glm::rotate(view, glm::radians(camera_angle[1]), glm::vec3(0.0f, 1.0f, 0.0f));
	// drawScene에서는 Y에 -3.0f를 사용하므로 동일하게 적용
	view = glm::translate(view, glm::vec3(camera_rocate[0], -3.0f, camera_rocate[2]));
	glm::vec4 camWorld = glm::inverse(view) * glm::vec4(0, 0, 0, 1);
	return glm::vec3(camWorld);
}

void coin_insert_ready(int v) {
	// 초기 호출(v==15)일 때 카메라/코인 보간 초기화
	if (v == 15) {
		// 마우스 델타 초기화 및 포인터 중앙화
		lastMouseX = -1;
		lastMouseY = -1;
		int cx = width / 2;
		int cy = height / 2;
		glutWarpPointer(cx, cy);

		// 코인 보간 초기화
		coin_ready_total_steps = 60;                // 총 프레임 수 (조절 가능)
		coin_ready_steps_remaining = coin_ready_total_steps;
		coin_start_pos = glm::vec3(one_coin_translate[0], one_coin_translate[1], one_coin_translate[2]);
		coin_target_pos = glm::vec3(1.18f, 1.95f, one_coin_translate[2]); // 준비 위치
		coin_start_angle = one_coin_angle;
		coin_target_angle = one_coin_angle - 90.0f;

		// 카메라 부드러운 이동 초기화 (x, z) 및 각도(pitch, yaw)
		camera_move_total_steps = 60; // 동일한 프레임 수로 동기화 (필요시 변경)
		camera_move_steps_remaining = camera_move_total_steps;
		camera_start_move = glm::vec2(camera_rocate[0], camera_rocate[2]);
		camera_target_move = glm::vec2(0.0f, -3.2f); // 목표 position (x, z)

		// 카메라 각도 목표 설정 (pitch, yaw)
		camera_start_angle = glm::vec2(camera_angle[0], camera_angle[1]);
		camera_target_angle = glm::vec2(10.0f, 0.0f); // 예: 살짝 내려다보는 각도, 정면

		// 수동 제어 잠금 (마우스로 더이상 조작하지 않음)
		mouse_control = 0;
	}

	// 보간이 남아있으면 한 프레임 분 만큼 선형 보간해서 코인/카메라 이동
	if (coin_ready_steps_remaining > 0) {
		// t: 0..1 보간 계수
		float stepIndex = static_cast<float>(coin_ready_total_steps - coin_ready_steps_remaining + 1);
		float t = stepIndex / static_cast<float>(coin_ready_total_steps);
		// 필요하면 easing 적용: t = 1 - pow(1 - t, 3); 등

		// 코인 위치/각도 보간 (선형)
		glm::vec3 pos = coin_start_pos * (1.0f - t) + coin_target_pos * t;
		one_coin_translate[0] = pos.x;
		one_coin_translate[1] = pos.y;
		one_coin_angle = coin_start_angle * (1.0f - t) + coin_target_angle * t;

		// 카메라 보간 (x, z) 및 각도 (pitch, yaw)
		if (camera_move_steps_remaining > 0) {
			float camStepIndex = static_cast<float>(camera_move_total_steps - camera_move_steps_remaining + 1);
			float ct = camStepIndex / static_cast<float>(camera_move_total_steps);
			// 위치 보간
			glm::vec2 camPos = camera_start_move * (1.0f - ct) + camera_target_move * ct;
			camera_rocate[0] = camPos.x;
			camera_rocate[2] = camPos.y;
			// 각도 보간 (pitch, yaw)
			camera_angle[0] = camera_start_angle.x * (1.0f - ct) + camera_target_angle.x * ct;
			camera_angle[1] = camera_start_angle.y * (1.0f - ct) + camera_target_angle.y * ct;

			camera_move_steps_remaining--;
		}
		coin_ready_steps_remaining--;
		glutPostRedisplay();
		glutTimerFunc(16, coin_insert_ready, v - 1);
		return;
	}

	// 준비 보간 완료 -> 실제 삽입 동작 시작
	coin_insert(0);
}

void coin_insert(int v) {
	if (one_coin_translate[2] <= 0.2f) { lever_protect = false; coin_inserted = true; return; }

	one_coin_translate[2] -= 0.2f / 30.0f;

	glutPostRedisplay();
	glutTimerFunc(16, coin_insert, v);
}

void coin_drop(int v) {
	if (one_coin_translate[1] <= 2.0f) {
		coin_protect = false;
		coin_inserted = false;
		return; 
	}

	one_coin_translate[1] -= 0.5f / 10.0f;

	glutPostRedisplay();
	glutTimerFunc(16, coin_drop, v);
}

void lever_action_1(int v) {
	if (v < 15) {
		lever_angle += 90.0f / 15.0f;
	}
	else {
		return;
	}
	glutPostRedisplay();
	glutTimerFunc(16, lever_action_1, v + 1);
}

void lever_action_2(int v) {
	if (v < 30) {
		lever_angle -= 90.0f / 15.0f;
	}
	else {
		lever_protect = false;
		int temp = rand() % 10;
		for (int i = 0; i < 3; i++) {
			slot_value[i] = 15 * (28+temp) + 15 * i;
		}

		// 각 슬롯 애니메이션 시작
		slot_action(0);
		slot_action(1);
		slot_action(2);
		return;
	}
	glutPostRedisplay();
	glutTimerFunc(16, lever_action_2, v + 1);
}

void slot_action(int v) {
	if (slot_value[v] == 0) {
		if (v == 2) {
			if (slot_angle[0] == 0.0f && slot_angle[1] == 0.0f && slot_angle[2] == 0.0f) {
				jack_pot_1(60);
			}
		}
		return;
	}

	slot_angle[v] += 360.f / 7 / 15;
	if (slot_angle[v] == 360.0f) slot_angle[v] = 0.0f;

	slot_value[v]--;
	glutPostRedisplay();
	glutTimerFunc(30 - slot_speed, slot_action, v);
}

glm::vec2 camera_saved_move = glm::vec2(0.0f, -10.0f); // x, z
glm::vec2 camera_saved_angle_vec = glm::vec2(0.0f, 0.0f); // pitch, yaw
bool camera_saved_flag = false;

void jack_pot_1(int v) {
	const int steps = 60;
	const float backDelta = -0.3f;
	const float pitchDelta = 8.0f;

	if (v == steps) {
		// 저장된 값이 없으면 시작 상태를 저장 (한 번만)
		if (!camera_saved_flag) {
			camera_saved_move = glm::vec2(camera_rocate[0], camera_rocate[2]);
			camera_saved_angle_vec = glm::vec2(camera_angle[0], camera_angle[1]);
			camera_saved_flag = true;
		}

		m_coins = true;
		camera_move_total_steps = steps;
		camera_move_steps_remaining = steps;
		camera_start_move = glm::vec2(camera_rocate[0], camera_rocate[2]);

		// 목표는 저장된 값 기준으로 계산 (상대 누적 방지)
		camera_target_move = glm::vec2(camera_saved_move.x, camera_saved_move.y + backDelta);

		camera_start_angle = glm::vec2(camera_angle[0], camera_angle[1]);
		camera_target_angle = glm::vec2(camera_start_angle.x + pitchDelta, camera_start_angle.y);

		// 사용자 입력 잠금
		mouse_control = 0;
	}

	// 보간 루틴(기존 코드 유지)...
	if (camera_move_steps_remaining > 0) {
		float stepIndex = static_cast<float>(camera_move_total_steps - camera_move_steps_remaining + 1);
		float t = stepIndex / static_cast<float>(camera_move_total_steps);

		glm::vec2 camPos = camera_start_move * (1.0f - t) + camera_target_move * t;
		camera_rocate[0] = camPos.x;
		camera_rocate[2] = camPos.y;

		camera_angle[0] = camera_start_angle.x * (1.0f - t) + camera_target_angle.x * t;
		camera_angle[1] = camera_start_angle.y * (1.0f - t) + camera_target_angle.y * t;

		camera_move_steps_remaining--;
		glutPostRedisplay();
		glutTimerFunc(16, jack_pot_1, v - 1);
		return;
	}
	jack_pot_2(0);
}

void jack_pot_2(int v) {
	if (v < 120) {
		m_coins_trans += 0.1f / 15.0f;
		glutPostRedisplay();
		glutTimerFunc(16, jack_pot_2, v + 1);
		return;
	}

	jack_pot_3(60);
	glutPostRedisplay();
}

void jack_pot_3(int v) {
	const int steps = 60;
	const float backDelta = 0.3f;
	const float pitchDelta = -8.0f;

	if (v == steps) {
		// 만약 아직 저장이 안 되어 있으면 보정용으로 저장 (안전)
		if (!camera_saved_flag) {
			camera_saved_move = glm::vec2(camera_rocate[0], camera_rocate[2]);
			camera_saved_angle_vec = glm::vec2(camera_angle[0], camera_angle[1]);
			camera_saved_flag = true;
		}

		m_coins = true;
		camera_move_total_steps = steps;
		camera_move_steps_remaining = steps;
		camera_start_move = glm::vec2(camera_rocate[0], camera_rocate[2]);

		// 목표는 저장된 값 기준으로 계산 (상대 누적 방지)
		camera_target_move = glm::vec2(camera_saved_move.x, camera_saved_move.y + backDelta);

		camera_start_angle = glm::vec2(camera_angle[0], camera_angle[1]);
		camera_target_angle = glm::vec2(camera_start_angle.x + pitchDelta, camera_start_angle.y);

		// 사용자 입력 잠금
		mouse_control = 0;
	}

	if (camera_move_steps_remaining > 0) {
		float stepIndex = static_cast<float>(camera_move_total_steps - camera_move_steps_remaining + 1);
		float t = stepIndex / static_cast<float>(camera_move_total_steps);

		glm::vec2 camPos = camera_start_move * (1.0f - t) + camera_target_move * t;
		camera_rocate[0] = camPos.x;
		camera_rocate[2] = camPos.y;

		camera_angle[0] = camera_start_angle.x * (1.0f - t) + camera_target_angle.x * t;
		camera_angle[1] = camera_start_angle.y * (1.0f - t) + camera_target_angle.y * t;

		camera_move_steps_remaining--;
		glutPostRedisplay();
		glutTimerFunc(16, jack_pot_3, v - 1);
		return;
	}

	// 정확히 원상복구: 저장된 값으로 복원
	camera_rocate[0] = camera_saved_move.x;
	camera_rocate[2] = camera_saved_move.y;
	camera_angle[0] = camera_saved_angle_vec.x;
	camera_angle[1] = camera_saved_angle_vec.y;

	// 상태 초기화 및 사용자 제어 복원
	camera_saved_flag = false;
	m_coins_trans = 0.4f;
	m_coins = false;
	slot_speed = 0;
	mouse_control = 1;

	glutPostRedisplay();
}

void loadModelToShape(const char* filename, shape& s) {
	// read basic model (positions + faces) via existing helper
	Model m = { nullptr, 0, nullptr, 0 };
	read_obj_file(filename, &m);
	if (m.vertex_count == 0 || m.face_count == 0) {
		std::cerr << "read_obj_file failed or empty: " << filename << std::endl;
		return;
	}

	// collect per-vertex colors and vt from file (if present)
	std::vector<glm::vec2> fileUVs;
	std::vector<float> fileColors;
	{
		std::ifstream ifs(filename);
		if (!ifs) {
			std::cerr << "Unable to re-open OBJ for UVs: " << filename << std::endl;
		}
		else {
			std::string line;
			size_t vcount = 0;
			while (std::getline(ifs, line)) {
				if (line.size() < 2) continue;
				if (line[0] == 'v' && std::isspace(static_cast<unsigned char>(line[1]))) {
					// v x y z [r g b]
					std::istringstream iss(line);
					std::string tag;
					float x, y, z;
					iss >> tag >> x >> y >> z;
					float r, g, b;
					if (iss >> r >> g >> b) {
						fileColors.push_back(r); fileColors.push_back(g); fileColors.push_back(b);
					}
					++vcount;
				}
				else if (line.rfind("vt ", 0) == 0) {
					std::istringstream iss(line);
					std::string tag;
					float u, v;
					iss >> tag >> u >> v;
					fileUVs.emplace_back(u, v);
				}
			}
			ifs.close();
		}
	}

	bool haveFileColors = (fileColors.size() == m.vertex_count * 3);
	if (haveFileColors) {
		for (size_t i = 0; i < fileColors.size(); i += 3) {
			if (fileColors[i] > 1.5f || fileColors[i + 1] > 1.5f || fileColors[i + 2] > 1.5f) {
				for (size_t j = 0; j < fileColors.size(); ++j) fileColors[j] /= 255.0f;
				break;
			}
		}
	}

	// compute averaged vertex normals from faces (per original vertex)
	std::vector<glm::vec3> accum(m.vertex_count, glm::vec3(0.0f));
	for (size_t f = 0; f < m.face_count; ++f) {
		unsigned int ia = m.faces[f].v1;
		unsigned int ib = m.faces[f].v2;
		unsigned int ic = m.faces[f].v3;
		glm::vec3 A(m.vertices[ia].x, m.vertices[ia].y, m.vertices[ia].z);
		glm::vec3 B(m.vertices[ib].x, m.vertices[ib].y, m.vertices[ib].z);
		glm::vec3 C(m.vertices[ic].x, m.vertices[ic].y, m.vertices[ic].z);
		glm::vec3 n = glm::cross(B - A, C - A);
		if (glm::length(n) > 1e-6f) n = glm::normalize(n);
		accum[ia] += n; accum[ib] += n; accum[ic] += n;
	}
	std::vector<glm::vec3> vertexNormals(m.vertex_count);
	for (size_t i = 0; i < m.vertex_count; ++i) {
		glm::vec3 nn = glm::normalize(accum[i]);
		if (glm::length(nn) < 1e-6f) nn = glm::vec3(0.0f, 1.0f, 0.0f);
		vertexNormals[i] = nn;
	}

	// Re-parse faces from file to preserve vt indices (supports v, v/vt, v//vn, v/vt/vn)
	std::ifstream ifs(filename);
	if (!ifs) {
		std::cerr << "Failed to open OBJ for face parsing: " << filename << std::endl;
		// fallback: copy positions/normals/colors without UVs
		s.vertices.clear(); s.colors.clear(); s.normals.clear(); s.index.clear(); s.texcoords.clear();
		for (size_t i = 0; i < m.vertex_count; ++i) {
			s.vertices.push_back(m.vertices[i].x);
			s.vertices.push_back(m.vertices[i].y);
			s.vertices.push_back(m.vertices[i].z);
			if (haveFileColors) {
				s.colors.push_back(fileColors[i * 3 + 0]); s.colors.push_back(fileColors[i * 3 + 1]); s.colors.push_back(fileColors[i * 3 + 2]);
			}
			else {
				s.colors.push_back(0.8f); s.colors.push_back(0.8f); s.colors.push_back(0.8f);
			}
			s.normals.push_back(vertexNormals[i].x); s.normals.push_back(vertexNormals[i].y); s.normals.push_back(vertexNormals[i].z);
			s.texcoords.push_back(0.0f); s.texcoords.push_back(0.0f);
		}
		for (size_t f = 0; f < m.face_count; ++f) {
			s.index.push_back(m.faces[f].v1);
			s.index.push_back(m.faces[f].v2);
			s.index.push_back(m.faces[f].v3);
		}
		s.valid = 1;
		if (m.vertices) free(m.vertices);
		if (m.faces) free(m.faces);
		return;
	}

	struct Key { unsigned int v, vt; };
	struct KeyHash {
		size_t operator()(Key const& k) const noexcept {
			// size_t가 32비트일 수 있으므로 64비트로 결합
			uint64_t a = static_cast<uint64_t>(k.v);
			uint64_t b = static_cast<uint64_t>(k.vt);
			return static_cast<size_t>((a << 32) ^ b);
		}
	};
	struct KeyEq { bool operator()(Key const& a, Key const& b) const noexcept { return a.v == b.v && a.vt == b.vt; } };
	std::unordered_map<Key, unsigned int, KeyHash, KeyEq> cache;

	std::vector<GLfloat> outPos; outPos.reserve(m.face_count * 9);
	std::vector<GLfloat> outNormal; outNormal.reserve(m.face_count * 9);
	std::vector<GLfloat> outColor; outColor.reserve(m.face_count * 9);
	std::vector<GLfloat> outUV; outUV.reserve(m.face_count * 6);
	std::vector<unsigned int> outIndex; outIndex.reserve(m.face_count * 3);

	std::string line;
	while (std::getline(ifs, line)) {
		if (line.size() < 2) continue;
		if (line[0] == 'f' && std::isspace(static_cast<unsigned char>(line[1]))) {
			std::istringstream iss(line.substr(2));
			std::string tok;
			std::vector<Key> faceKeys;
			while (iss >> tok) {
				unsigned int vi = 0, vti = UINT32_MAX;
				// try common patterns
				if (sscanf_s(tok.c_str(), "%u/%u/%*u", &vi, &vti) == 2) {
					// v/vt/vn
				}
				else if (sscanf_s(tok.c_str(), "%u/%u", &vi, &vti) == 2) {
					// v/vt
				}
				else if (sscanf_s(tok.c_str(), "%u//%*u", &vi) == 1) {
					// v//vn
				}
				else if (sscanf_s(tok.c_str(), "%u", &vi) == 1) {
					// v
				}
				else {
					// fallback split
					size_t pos = tok.find('/');
					if (pos != std::string::npos) {
						std::string sv = tok.substr(0, pos);
						vi = std::stoul(sv);
						size_t pos2 = tok.find('/', pos + 1);
						if (pos2 != std::string::npos && pos2 > pos + 1) {
							std::string svt = tok.substr(pos + 1, pos2 - pos - 1);
							if (!svt.empty()) vti = std::stoul(svt);
						}
						else {
							std::string svt = tok.substr(pos + 1);
							if (!svt.empty()) vti = std::stoul(svt);
						}
					}
				}
				if (vi > 0) {
					Key k; k.v = vi - 1;
					k.vt = (vti == UINT32_MAX) ? UINT32_MAX : (vti - 1);
					faceKeys.push_back(k);
				}
			}
			// triangulate polygon (fan)
			if (faceKeys.size() >= 3) {
				for (size_t k = 1; k + 1 < faceKeys.size(); ++k) {
					Key tri[3] = { faceKeys[0], faceKeys[k], faceKeys[k + 1] };
					for (int ti = 0; ti < 3; ++ti) {
						Key key = tri[ti];
						auto it = cache.find(key);
						if (it != cache.end()) {
							outIndex.push_back(it->second);
						}
						else {
							unsigned int newIdx = static_cast<unsigned int>(outPos.size() / 3);
							// position
							outPos.push_back(m.vertices[key.v].x);
							outPos.push_back(m.vertices[key.v].y);
							outPos.push_back(m.vertices[key.v].z);
							// normal (from averaged per-original vertex)
							outNormal.push_back(vertexNormals[key.v].x);
							outNormal.push_back(vertexNormals[key.v].y);
							outNormal.push_back(vertexNormals[key.v].z);
							// color
							if (haveFileColors) {
								outColor.push_back(fileColors[key.v * 3 + 0]);
								outColor.push_back(fileColors[key.v * 3 + 1]);
								outColor.push_back(fileColors[key.v * 3 + 2]);
							}
							else {
								outColor.push_back(0.8f); outColor.push_back(0.8f); outColor.push_back(0.8f);
							}
							// uv
							if (key.vt != UINT32_MAX && key.vt < fileUVs.size()) {
								outUV.push_back(fileUVs[key.vt].x);
								outUV.push_back(1.0f - fileUVs[key.vt].y);
							}
							else {
								outUV.push_back(0.0f); outUV.push_back(0.0f);
							}
							cache.emplace(key, newIdx);
							outIndex.push_back(newIdx);
						}
					}
				}
			}
		}
	}
	ifs.close();

	// transfer to shape
	s.vertices = std::move(outPos);
	s.normals = std::move(outNormal);
	s.colors = std::move(outColor);
	s.texcoords = std::move(outUV);
	s.index = std::move(outIndex);
	s.valid = 1;

	// UV / vertex count 검사: texcoords.size()/2 == vertices.size()/3
	size_t vCount = s.vertices.size() / 3;
	size_t uvCount = s.texcoords.size() / 2;
	if (uvCount != vCount) {
		std::cerr << "WARNING: UV count (" << uvCount << ") != vertex count (" << vCount << "). Auto-fixing UVs.\n";
		std::vector<GLfloat> fixed;
		fixed.reserve(vCount * 2);
		if (uvCount == 0) {
			// 모두 0,0 으로 채움
			for (size_t i = 0; i < vCount; ++i) { fixed.push_back(0.0f); fixed.push_back(0.0f); }
		}
		else {
			// uvCount < vCount: 반복 또는 0으로 채움 (간단한 보정)
			for (size_t i = 0; i < vCount; ++i) {
				size_t src = (i < uvCount) ? i : (uvCount - 1);
				fixed.push_back(s.texcoords[src * 2 + 0]);
				fixed.push_back(s.texcoords[src * 2 + 1]);
			}
		}
		s.texcoords.swap(fixed);
	}
	// cleanup
	if (m.vertices) free(m.vertices);
	if (m.faces) free(m.faces);
	
}

void buffer(shape& temp) {
	glGenVertexArrays(1, &temp.vao_shape);
	glBindVertexArray(temp.vao_shape);
	// 항상 4개의 버퍼 생성하되 사용여부는 이후에 판단
	glGenBuffers(4, temp.vbo_shape);

	// position
	glBindBuffer(GL_ARRAY_BUFFER, temp.vbo_shape[0]);
	glBufferData(GL_ARRAY_BUFFER, temp.vertices.size() * sizeof(GLfloat), temp.vertices.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	// normal
	glBindBuffer(GL_ARRAY_BUFFER, temp.vbo_shape[1]);
	glBufferData(GL_ARRAY_BUFFER, temp.normals.size() * sizeof(GLfloat), temp.normals.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);

	// color
	glBindBuffer(GL_ARRAY_BUFFER, temp.vbo_shape[2]);
	glBufferData(GL_ARRAY_BUFFER, temp.colors.size() * sizeof(GLfloat), temp.colors.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(2);

	// texcoord (u,v) — 데이터가 없으면 attribute 비활성화
	if (!temp.texcoords.empty()) {
		glBindBuffer(GL_ARRAY_BUFFER, temp.vbo_shape[3]);
		glBufferData(GL_ARRAY_BUFFER, temp.texcoords.size() * sizeof(GLfloat), temp.texcoords.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(3);
	} else {
		// 버퍼는 생성되어 있으나 attribute는 사용하지 않음
		glBindBuffer(GL_ARRAY_BUFFER, temp.vbo_shape[3]);
		glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_STATIC_DRAW);
		glDisableVertexAttribArray(3);
	}

	glGenBuffers(1, &temp.ebo_shape);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, temp.ebo_shape);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, temp.index.size() * sizeof(unsigned int), temp.index.data(), GL_STATIC_DRAW);

	glBindVertexArray(0);
	if (&temp == &slot) {
		std::cerr << "slot: verts=" << temp.vertices.size() / 3 << " uvs=" << temp.texcoords.size() / 2
			<< " idx=" << temp.index.size() << std::endl;
	}
	if (&temp == &machine) {
		std::cerr << "machine: verts=" << temp.vertices.size() / 3 << " uvs=" << temp.texcoords.size() / 2
			<< " idx=" << temp.index.size() << std::endl;
	}
}

GLuint loadTexture(const char* path)
{
	int w, h, n;
	unsigned char* data = stbi_load(path, &w, &h, &n, 4);
	if (!data) {
		std::cerr << "Failed to load texture: " << path << std::endl;
		return 0;
	}
	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	stbi_image_free(data);
	return tex;
}