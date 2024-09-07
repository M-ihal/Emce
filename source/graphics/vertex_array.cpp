#include "vertex_array.h"

#include <glew.h>
#include <stdio.h>

VertexArray::VertexArray(void) {
	GL_CHECK(glGenVertexArrays(1, &m_vao_id));
	if(!m_vao_id) {
		fprintf(stderr, "[error] VAO: Failed to generate vertex array id.\n");
	}

	fprintf(stdout, "[info] VAO: Created vertex array, ID: %d (%p)\n", m_vao_id, this);
}

VertexArray::~VertexArray(void) {
	if(m_vao_id) {
		glDeleteVertexArrays(1, &m_vao_id);
		m_vao_id = 0;

		fprintf(stdout, "[info] VAO: Deleted vertex array (%p)\n", this);
	}
}
