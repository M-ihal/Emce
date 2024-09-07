#pragma once

#include "common.h"

class VertexArray {
public:
	CLASS_COPY_DISABLE(VertexArray);

	VertexArray(void);

	~VertexArray(void);

private:
	uint32_t m_vao_id;
};