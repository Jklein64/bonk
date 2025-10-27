#include <gmsh.h>
#include <iostream>
#include <vector>

void remesh(std::string mesh_file) {
    gmsh::initialize();
    gmsh::model::add("tetmesh");
    try {
        gmsh::merge(mesh_file);
    }
    catch (...) {
        std::cout << "Could not open or merge file \"" << mesh_file << "\"" << std::endl;
        gmsh::finalize();
        throw;
    }
    std::vector<std::pair<int, int>> surfaces;
    gmsh::model::getEntities(surfaces, 2);
    if (surfaces.empty()) {
        std::cout << "No surfaces found in file!" << std::endl;
        gmsh::finalize();
    }
    std::vector<int> surface_tags;
    for (const auto& s : surfaces) {
        surface_tags.push_back(s.second);
    }
    int surface_loop_tag = gmsh::model::geo::addSurfaceLoop(surface_tags);
    int volume_tag = gmsh::model::geo::addVolume({ surface_loop_tag });
    gmsh::model::geo::synchronize();
    gmsh::option::setNumber("Mesh.Algorithm3D", 1);
    gmsh::model::mesh::generate(3);
    std::vector<int> elemTypes;
    std::vector<std::vector<std::size_t>> elemTags, elemNodeTags;
    gmsh::write(mesh_file + ".vtk");
    gmsh::finalize();
}
