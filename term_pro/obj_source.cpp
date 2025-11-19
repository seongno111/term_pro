#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "obj_header.h"

void read_newline(char* str) {
	char* pos;
	if ((pos = strchr(str, '\n')) != NULL)
		*pos = '\0';
}

void read_obj_file(const char* filename, Model* model) {
	FILE* file = NULL;
	fopen_s(&file, filename, "r");
	if (!file) {
		perror("Error opening file");
		model->vertex_count = 0;
		model->face_count = 0;
		return;
	}
	char line[MAX_LINE_LENGTH];
	model->vertex_count = 0;
	model->face_count = 0;
	// 1차 패스: 정점과 face 라인 수 세기
	while (fgets(line, sizeof(line), file)) {
		read_newline(line);
		if (line[0] == 'v' && line[1] == ' ')
			model->vertex_count++;
		else if (line[0] == 'f' && line[1] == ' ')
			model->face_count++;
	}
	fseek(file, 0, SEEK_SET);

	// 메모리 할당
	model->vertices = (Vertex*)malloc(model->vertex_count * sizeof(Vertex));
	// face는 폴리곤을 삼각형으로 분해하므로 여유 있게 할당 (최대 face_count * (n-2))
	model->faces = (Face*)malloc((model->face_count * 3) * sizeof(Face));

	size_t vertex_index = 0;
	size_t face_index = 0;
	while (fgets(line, sizeof(line), file)) {
		read_newline(line);
		if (line[0] == 'v' && line[1] == ' ') {
			// "v x y z"
			if (vertex_index < model->vertex_count) {
				sscanf_s(line + 2, "%f %f %f",
					&model->vertices[vertex_index].x,
					&model->vertices[vertex_index].y,
					&model->vertices[vertex_index].z);
				vertex_index++;
			}
		}
		else if (line[0] == 'f' && line[1] == ' ') {
			// "f v1 v2 v3" 또는 "f v1/vt1/vn1 v2/vt2/vn2 v3/..." 또는 폴리곤
			char copy[MAX_LINE_LENGTH];
			strcpy_s(copy, sizeof(copy), line + 2);
			char* context = NULL;
			char* token = strtok_s(copy, " \t", &context);
			unsigned int idxs[64];
			int idxCount = 0;
			while (token && idxCount < 64) {
				unsigned int vi = 0;
				// 여러 포맷에 대해 앞 숫자(정점 인덱스)만 추출
				if (sscanf_s(token, "%u", &vi) == 1) {
					// "v"
				} else if (sscanf_s(token, "%u/%*u/%*u", &vi) == 1) {
					// "v/t/n"
				} else if (sscanf_s(token, "%u//%*u", &vi) == 1) {
					// "v//n"
				} else if (sscanf_s(token, "%u/%*u", &vi) == 1) {
					// "v/t"
				} else {
					// fallback: 슬래시가 있으면 앞부분을 분리
					char* slash = strchr(token, '/');
					if (slash) {
						*slash = '\0';
						vi = (unsigned int)atoi(token);
						*slash = '/';
					} else {
						vi = (unsigned int)atoi(token);
					}
				}
				if (vi > 0) {
					idxs[idxCount++] = vi;
				}
				token = strtok_s(NULL, " \t", &context);
			}
			// 삼각형 팬으로 분해
			if (idxCount >= 3) {
				for (int k = 1; k < idxCount - 1; ++k) {
					if (face_index < model->face_count * 3) {
						model->faces[face_index].v1 = idxs[0] - 1;
						model->faces[face_index].v2 = idxs[k] - 1;
						model->faces[face_index].v3 = idxs[k + 1] - 1;
						face_index++;
					}
				}
			}
		}
	}
	fclose(file);
	// 실제 face_count 업데이트
	model->face_count = face_index;
}
