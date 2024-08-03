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

    geometry->vertex_count = 8;
    geometry->index_count = 36;

    geometry->buffer = new char[sizeof(Vertex) * geometry->vertex_count + sizeof(uint32_t) * geometry->index_count];

    float width_half = 0.5f;
    float height_half = 0.5f;
    float depth_half = 0.5f;

    Vertex* v = geometry_get_vertex(geometry);

    geometry_load_vertex(v[0], -width_half, -height_half, -depth_half, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    geometry_load_vertex(v[1], +width_half, -height_half, -depth_half, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    geometry_load_vertex(v[2], -width_half, +height_half, -depth_half, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f);
    geometry_load_vertex(v[3], +width_half, +height_half, -depth_half, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	geometry_load_vertex(v[4], -width_half, -height_half, +depth_half, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	geometry_load_vertex(v[5], +width_half, -height_half, +depth_half, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	geometry_load_vertex(v[6], -width_half, +height_half, +depth_half, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	geometry_load_vertex(v[7], +width_half, +height_half, +depth_half, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f);

	uint32_t* i = geometry_get_index(geometry);

	i[0]  = 0;  i[1]  = 2;  i[2]  = 3;
	i[3]  = 0;  i[4]  = 3;  i[5]  = 1;
	i[6]  = 4;  i[7]  = 5;  i[8]  = 7;
	i[9]  = 4;  i[10] = 7;  i[11] = 6;
	i[12] = 1;  i[13] = 3; i[14] = 7;
	i[15] = 1;  i[16] = 7; i[17] = 5;
	i[18] = 0; i[19] = 4; i[20] = 6;
	i[21] = 0; i[22] = 6; i[23] = 2;
	i[24] = 2; i[25] = 6; i[26] = 7;
	i[27] = 2; i[28] = 7; i[29] = 3;
	i[30] = 0; i[31] = 1; i[32] = 5;
	i[33] = 0; i[34] = 5; i[35] = 4;

    return geometry;
}
