#pragma once
typedef struct {
	float x, y, z;
} Vertex;

typedef struct {
	unsigned int v1, v2, v3;
} Face;

typedef struct {
	Vertex* vertices;
	size_t vertex_count;
	Face* faces;
	size_t face_count;
} Model;
#define MAX_LINE_LENGTH 128

void read_newline(char* str);
void read_obj_file(const char* filename, Model* model);