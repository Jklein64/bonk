#include "tet.hpp"
#include <filesystem>
#include <numbers>

#define SURFACE_MESH_MAX_VERTICES 2500

BonkInstance::BonkResult BonkInstance::loadMesh(std::string filename) {
  std::filesystem::path p {filename};
  if (!std::filesystem::exists(p)) {
    return BonkResult::FileOpenFailure;
  }
  Polyhedron poly;
  Mesh m;
  CGAL::IO::read_polygon_mesh(filename, m);
  CGAL::copy_face_graph(m, poly);
  if (!CGAL::is_triangle_mesh(poly)) {
    CGAL::Polygon_mesh_processing::triangulate_faces(poly);
  }
  if (poly.size_of_vertices() > SURFACE_MESH_MAX_VERTICES) {
    StopPredicate stop(SURFACE_MESH_MAX_VERTICES);
    CGAL::Surface_mesh_simplification::edge_collapse(poly, stop, CGAL::parameters::vertex_index_map(get(CGAL::vertex_external_index, poly)).halfedge_index_map(get(CGAL::halfedge_external_index, poly)));
  }
  Domain domain(poly);
  Criteria criteria(
    CGAL::parameters::facet_angle(30),
    CGAL::parameters::facet_size(0.2),
    CGAL::parameters::facet_distance(0.01),
    CGAL::parameters::cell_radius_edge_ratio(2.0),
    CGAL::parameters::cell_size(0.2)
  );
  complex = CGAL::make_mesh_3<MeshComplex>(domain, criteria);
  complex.remove_isolated_vertices();
  int idx {0};
  for (auto cell = complex.cells_in_complex_begin(); cell != complex.cells_in_complex_end(); ++cell) {
    for (int i = 0; i < 4; i++) {
      auto v = cell->vertex(i);
      if (handles_to_inds.find(v) == handles_to_inds.end()) {
        handles_to_inds[v] = idx;
        inds_to_handles.push_back(v);
        idx++;
      }
    }
  }
  vert_count = static_cast<size_t>(idx);
  isTetMesh = true;
  return BonkResult::Success;
}

BonkInstance::BonkResult BonkInstance::prepareThree() {
  if (!isTetMesh) {return BonkResult::BadInvocation;}
  std::unordered_map<int, int> localToThree {};
  int threeIndex {0};
  for (auto facet = complex.facets_in_complex_begin(); facet != complex.facets_in_complex_end(); ++facet) {
    auto cell = facet->first;
    auto opposite_vertex_index = facet->second;
    // Gemini
    auto i0 = (opposite_vertex_index + 1) & 3;
    auto i1 = (opposite_vertex_index + 2) & 3;
    auto i2 = (opposite_vertex_index + 3) & 3;
    std::array<int, 3> faceIndices = {i0, i1, i2};
    if ((opposite_vertex_index % 2) == 1) {
      std::swap(faceIndices[0], faceIndices[1]);
    }
    // end
    for (int k = 0; k < 3; k++) {
      auto v = cell->vertex(faceIndices[k]);
      int local = handles_to_inds[v];
      if (localToThree.find(local) == localToThree.end()) {
        auto p = v->point();
        vertices.push_back(CGAL::to_double(p.x()));
        vertices.push_back(CGAL::to_double(p.y()));
        vertices.push_back(CGAL::to_double(p.z()));
        localToThree[local] = threeIndex;
        three_to_local[threeIndex] = local;
        threeIndex++;
      }
      indices.push_back(localToThree[local]);
    }
  }
  isThreeReady = true;
  return BonkResult::Success;
}

BonkInstance::BonkResult BonkInstance::initModalContext(double density, double k, double dt, double damping, double freqDamping) {
  if (!isTetMesh) {return BonkResult::BadInvocation;}
  density = density;
  dt = dt;

  auto K = SpMat(3*vert_count, 3*vert_count);
  auto M = SpMat(3*vert_count, 3*vert_count);
  std::vector<T> kTriplets {};
  std::vector<T> mTriplets {};
  auto triangulation = complex.triangulation();
  for (auto cell = complex.cells_in_complex_begin(); cell != complex.cells_in_complex_end(); ++cell) {
    if (complex.subdomain_index(cell) == 0) {continue;} // Gemini
    double vol = std::abs(triangulation.tetrahedron(cell).volume());
    auto m = density * vol / 4;
    for (int i = 0; i < 4; i++){
      int u = handles_to_inds[cell->vertex(i)];
      mTriplets.push_back(T(3*u+0, 3*u+0, m)); 
      mTriplets.push_back(T(3*u+1, 3*u+1, m)); 
      mTriplets.push_back(T(3*u+2, 3*u+2, m)); 
      for (int j = i + 1; j < 4; j++) {
        int v = handles_to_inds[cell->vertex(j)];
        for (int k_ = 0; k_ < 3; k_++) {
          int u_ = 3*u+k_;
          int v_ = 3*v+k_;
          kTriplets.push_back(T(u_, u_, k));
          kTriplets.push_back(T(v_, v_, k));

          kTriplets.push_back(T(u_, v_, -k));
          kTriplets.push_back(T(v_, u_, -k));
        }
      }
    }
  }
  K.setFromTriplets(kTriplets.begin(), kTriplets.end());
  M.setFromTriplets(mTriplets.begin(), mTriplets.end());
  Spectra::SparseSymMatProd<double> opK(K);
  Spectra::SparseCholesky<double> opM(M);
  auto desired_modes = std::min(MODES, static_cast<int>(vert_count));
  auto ncv = std::min(desired_modes * 2 + 1, 3*static_cast<int>(vert_count)); // Gemini
  Spectra::SymGEigsSolver<Spectra::SparseSymMatProd<double>, Spectra::SparseCholesky<double>, Spectra::GEigsMode::Cholesky> eigs(opK, opM, desired_modes, ncv);

  eigs.init();
  int nconv = eigs.compute(Spectra::SortRule::SmallestMagn);
  if (eigs.info() != Spectra::CompInfo::Successful) {
    return BonkResult::ModalSetupFailure;
  }

  freq = eigs.eigenvalues();
  modes = eigs.eigenvectors();
  phase_step.resizeLike(freq);
  phase_step.setZero();
  amp.resizeLike(freq);
  amp.setZero();
  phase.resizeLike(freq);
  phase.setZero();
  damp.resizeLike(freq);
  damp.setZero();
  
  for (int i = 0; i < static_cast<int>(freq.size()); i++) {
    auto f = freq[i] > 0 ? std::sqrt(freq[i]) : 0.0;
    freq[i] = f / (2.0 * std::numbers::pi);
    phase_step[i] = f * dt;
    double d = damping + (f * freqDamping);
    damp[i] = std::exp(-d * dt);
  }
  isBonkable = true;
  return BonkResult::Success;
}

/* Arg 1: vector of all vertices to apply force to and normalized weight of force at that point (determined by e^-dist(v, center of force)), Arg 2: force direction */
BonkInstance::BonkResult BonkInstance::bonk(std::vector<int> indices, std::vector<double> weights, std::array<double, 3> normalizedForceDirection) {
  if (!isBonkable) {return BonkResult::BadInvocation;}
  forces.resize(3*vert_count);
  forces.setZero();
  for (size_t i = 0; i < indices.size(); i++) {
    auto trueIndex = three_to_local[indices[i]];
    forces[3*trueIndex] = normalizedForceDirection[0] * weights[i];
    forces[3*trueIndex+1] = normalizedForceDirection[1] * weights[i];
    forces[3*trueIndex+2] = normalizedForceDirection[2] * weights[i];
  }
  amp = modes.transpose() * forces;
  return BonkResult::Success;
}

BonkInstance::BonkResult BonkInstance::runModal(int count) {
  modalResults.assign(count, 0);
  // Gemini
  for (int i = 0; i < count; i++) {
    for (int j = 0; j < static_cast<int>(freq.size()); j++) {
      modalResults[i] += amp[j] * std::sin(phase[j]);
      phase[j] += phase_step[j];
      amp[j] *= damp[j];
      if (amp[j] <= 1e-8) {
        amp[j] = 0.0;
      }
    }
  }
  // end
  for (int i = 0; i < static_cast<int>(amp.size()); i++) {
    if (amp[i] != 0.0) {
      return BonkResult::Success;
    }
  }
  return BonkResult::ModalCompleteExtinction;
}