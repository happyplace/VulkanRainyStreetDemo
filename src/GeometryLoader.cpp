#include "GeometryLoader.h"

#include <SDL_assert.h>

Vertex* geometry_get_vertex(Geometry* geometry)
{
    return reinterpret_cast<Vertex*>(&geometry->buffer[0]);
}

uint32_t* geometry_get_index(Geometry* geometry)
{
    return reinterpret_cast<uint32_t*>(&geometry->buffer[sizeof(Vertex)*geometry->vertex_count]);
}

void geometry_destroy(Geometry *geometry)
{
    SDL_assert(geometry);

    if (geometry->buffer != nullptr)
    {
        delete[] geometry->buffer;
    }

    delete geometry;
}

void geometry_load_vertex(Vertex& vertex, float px, float py, float pz, float nx, float ny, float nz, float tx, float ty, float tz, float uv_x, float uv_y)
{
    vertex.position.x = px;
    vertex.position.y = py;
    vertex.position.z = pz;

    vertex.normal.x = nx;
    vertex.normal.y = ny;
    vertex.normal.z = nz;

    vertex.tangent.x = tx;
    vertex.tangent.y = ty;
    vertex.tangent.z = tz;

    vertex.texture.x = uv_x;
    vertex.texture.y = uv_y;
}

Geometry* geometry_loader_load_cube()
{
    Geometry* geometry = new Geometry();

    geometry->vertex_count = 24;
    geometry->index_count = 36;

    geometry->buffer = new char[sizeof(Vertex) * geometry->vertex_count + sizeof(uint32_t) * geometry->index_count];

    float width_half = 0.5f;
    float height_half = 0.5f;
    float depth_half = 0.5f;

    Vertex* v = geometry_get_vertex(geometry);

    // front face
    geometry_load_vertex(v[0], -width_half, -height_half, -depth_half, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    geometry_load_vertex(v[1], -width_half, +height_half, -depth_half, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    geometry_load_vertex(v[2], +width_half, +height_half, -depth_half, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    geometry_load_vertex(v[3], +width_half, -height_half, -depth_half, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

    // back face
	geometry_load_vertex(v[4], -width_half, -height_half, +depth_half, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	geometry_load_vertex(v[5], +width_half, -height_half, +depth_half, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	geometry_load_vertex(v[6], +width_half, +height_half, +depth_half, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	geometry_load_vertex(v[7], -width_half, +height_half, +depth_half, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

	// top face
	geometry_load_vertex(v[8], -width_half, +height_half, -depth_half, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	geometry_load_vertex(v[9], -width_half, +height_half, +depth_half, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	geometry_load_vertex(v[10], +width_half, +height_half, +depth_half, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	geometry_load_vertex(v[11], +width_half, +height_half, -depth_half, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

	// bottom face
	geometry_load_vertex(v[12], -width_half, -height_half, -depth_half, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	geometry_load_vertex(v[13], +width_half, -height_half, -depth_half, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	geometry_load_vertex(v[14], +width_half, -height_half, +depth_half, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	geometry_load_vertex(v[15], -width_half, -height_half, +depth_half, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

	// left face
	geometry_load_vertex(v[16], -width_half, -height_half, +depth_half, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	geometry_load_vertex(v[17], -width_half, +height_half, +depth_half, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	geometry_load_vertex(v[18], -width_half, +height_half, -depth_half, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
	geometry_load_vertex(v[19], -width_half, -height_half, -depth_half, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);

	// right face
	geometry_load_vertex(v[20], +width_half, -height_half, -depth_half, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f);
	geometry_load_vertex(v[21], +width_half, +height_half, -depth_half, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
	geometry_load_vertex(v[22], +width_half, +height_half, +depth_half, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
	geometry_load_vertex(v[23], +width_half, -height_half, +depth_half, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);

	uint32_t* i = geometry_get_index(geometry);

	// front face
	i[0] = 0; i[1] = 1; i[2] = 2;
	i[3] = 0; i[4] = 2; i[5] = 3;

	// back face
	i[6] = 4; i[7]  = 5; i[8]  = 6;
	i[9] = 4; i[10] = 6; i[11] = 7;

	// top face
	i[12] = 8; i[13] =  9; i[14] = 10;
	i[15] = 8; i[16] = 10; i[17] = 11;

	// bottom face
	i[18] = 12; i[19] = 13; i[20] = 14;
	i[21] = 12; i[22] = 14; i[23] = 15;

	// left face
	i[24] = 16; i[25] = 17; i[26] = 18;
	i[27] = 16; i[28] = 18; i[29] = 19;

	// right face
	i[30] = 20; i[31] = 21; i[32] = 22;
	i[33] = 20; i[34] = 22; i[35] = 23;

    return geometry;
}
