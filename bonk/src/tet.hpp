#pragma once
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/IO/polygon_mesh_io.h>
#include <CGAL/Mesh_complex_3_in_triangulation_3.h>
#include <CGAL/Mesh_criteria_3.h>
#include <CGAL/Mesh_triangulation_3.h>
#include <CGAL/Named_function_parameters.h>
#include <CGAL/Polygon_mesh_processing/IO/polygon_mesh_io.h>
#include <CGAL/Polygon_mesh_processing/bbox.h>
#include <CGAL/Polygon_mesh_processing/border.h>
#include <CGAL/Polygon_mesh_processing/orientation.h>
#include <CGAL/Polygon_mesh_processing/self_intersections.h>
#include <CGAL/Polygon_mesh_processing/triangulate_faces.h>
#include <CGAL/Polyhedral_mesh_domain_3.h>
#include <CGAL/Polyhedron_3.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/Surface_mesh/Surface_mesh.h>
#include <CGAL/Surface_mesh_simplification/Policies/Edge_collapse/Edge_count_stop_predicate.h>
#include <CGAL/Surface_mesh_simplification/edge_collapse.h>
#include <CGAL/boost/graph/copy_face_graph.h>
#include <CGAL/boost/graph/helpers.h>
#include <CGAL/boost/graph/properties.h>
#include <CGAL/bounding_box.h>
#include <CGAL/make_mesh_3.h>
#include <CGAL/number_utils.h>
#include <CGAL/refine_mesh_3.h>
#include <CGAL/tags.h>
#include <Spectra/MatOp/SparseCholesky.h>
#include <Spectra/MatOp/SparseSymMatProd.h>
#include <Spectra/SymGEigsSolver.h>
#include <Spectra/Util/GEigsMode.h>
#include <eigen3/Eigen/Core>
#include <eigen3/Eigen/Eigen>
#include <unordered_map>

class BonkInstance {
using Kernel = CGAL::Exact_predicates_inexact_constructions_kernel;
using Polyhedron = CGAL::Polyhedron_3<Kernel>;
using Domain = CGAL::Polyhedral_mesh_domain_3<Polyhedron, Kernel>;
using Triangulation = CGAL::Mesh_triangulation_3<Domain, CGAL::Default, CGAL::Parallel_if_available_tag>::type;
using MeshComplex = CGAL::Mesh_complex_3_in_triangulation_3<Triangulation>;
using Criteria = CGAL::Mesh_criteria_3<Triangulation>;
using Point = Kernel::Point_3;
using Mesh = CGAL::Surface_mesh<Point>;
using StopPredicate = CGAL::Surface_mesh_simplification::Edge_count_stop_predicate<Polyhedron>;
using SpMat = Eigen::SparseMatrix<double>;
using V = Eigen::VectorXd;
using T = Eigen::Triplet<double>;
public:
  enum class BonkResult {
    Success,
    FileOpenFailure,
    TetGenFailure,
    BadInvocation,
    ModalSetupFailure,
    ModalSimulationFailure,
    ModalCompleteExtinction
  };
  BonkInstance() = default; 
  BonkResult loadMesh(std::string filename);
  BonkResult prepareThree();
  bool threeReady() {return isThreeReady;}
  //size_t getVertCount() {return vert_count;}
  std::vector<int> getIndices() {return indices;}
  std::vector<double> getVertices() {return vertices;}
  BonkResult initModalContext(double density, double k, double dt, double damping = 0.05, double freqDamping = 0.01);
  BonkResult bonk(std::vector<int> indices, std::vector<double> weights, std::array<double, 3> normalizedForceDirection);
  BonkResult runModal(int count);
  std::vector<double> getResults() {
    return modalResults;
  }
private:
  bool detectAndFillHoles(Polyhedron poly);
  void compressModesAndCalcPhase(double damping, double freqDamping);
  void calcPhase(double damping, double freqDamping);
  bool isBonkable {false};
  bool isTetMesh {false};
  bool isThreeReady {false};
  MeshComplex complex;
  std::unordered_map<Triangulation::Vertex_handle, int> handles_to_inds {};
  std::vector<Triangulation::Vertex_handle> inds_to_handles {};
  std::unordered_map<int, int> three_to_local {};
  std::vector<int> indices {};
  std::vector<double> vertices {};
  std::vector<double> modalResults {};
  size_t vert_count {0};
  double density {1};
  double dt;
  // Modal
  static constexpr int MODES {50};
  V forces;
  V freq, phase_step, amp, phase, damp;
  Eigen::MatrixXd modes;
};
