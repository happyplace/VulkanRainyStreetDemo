#ifndef VRSD_GeometryLoader_h_
#define VRSD_GeometryLoader_h_

#include "RenderDefines.h"

struct Geometry
{
    char* buffer = nullptr;
    int vertex_count = 0;
    int index_count = 0;
};

Vertex* geometry_get_vertex(Geometry* geometry);
uint32_t* geometry_get_index(Geometry* geometry);

void geometry_destroy(Geometry* geometry);

Geometry* geometry_loader_load_cube();

#endif // VRSD_GeometryLoader_h_
