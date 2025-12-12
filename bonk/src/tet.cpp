#include "tet.hpp"
#include <filesystem>
#include <iterator>
#include <numbers>

#define SURFACE_MESH_MAX_VERTICES 2500

bool BonkInstance::detectAndFillHoles(Polyhedron poly) {
  std::vector<boost::graph_traits<Polyhedron>::halfedge_descriptor> border_cycles {};
  CGAL::Polygon_mesh_processing::extract_boundary_cycles(poly, std::back_inserter(border_cycles));
  if (!border_cycles.size()) {
    return true;
  }
  for (auto h : border_cycles) {
    if (!std::get<0>(CGAL::Polygon_mesh_processing::triangulate_refine_and_fair_hole(poly, h))) {
      return false;
    }
  }
  return true;
}

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
  auto noHoles = detectAndFillHoles(poly);
  if (!noHoles) {
    return BonkResult::FileOpenFailure;
  }
  if (!CGAL::Polygon_mesh_processing::is_outward_oriented(poly)) {
    CGAL::Polygon_mesh_processing::orient(poly);
  }
  if (poly.size_of_vertices() > SURFACE_MESH_MAX_VERTICES) {
    StopPredicate stop(SURFACE_MESH_MAX_VERTICES);
    CGAL::Surface_mesh_simplification::edge_collapse(poly, stop, CGAL::parameters::vertex_index_map(get(CGAL::vertex_external_index, poly)).halfedge_index_map(get(CGAL::halfedge_external_index, poly)));
  }
  if (CGAL::Polygon_mesh_processing::does_self_intersect<CGAL::Parallel_if_available_tag>(poly, CGAL::parameters::vertex_point_map(get(CGAL::vertex_point, poly)))) {
    return BonkResult::FileOpenFailure;
  }
  Domain domain(poly);
  auto bb = CGAL::Polygon_mesh_processing::bbox(poly);
  auto bb_size = std::sqrt(std::pow(bb.x_span(), 2) + std::pow(bb.y_span(), 2) + std::pow(bb.z_span(), 2));
  auto max_facet_size = 0.05 * bb_size;
  Criteria criteria(
    CGAL::parameters::facet_angle(30),
    CGAL::parameters::facet_size(max_facet_size),
    CGAL::parameters::facet_distance(max_facet_size * 0.1),
    CGAL::parameters::cell_radius_edge_ratio(2.0),
    CGAL::parameters::cell_size(max_facet_size)
  );
  complex = CGAL::make_mesh_3<MeshComplex>(domain, criteria);
  complex.remove_isolated_vertices();
  if (complex.number_of_cells_in_complex() == 0) {
    return BonkResult::FileOpenFailure;
  }
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

double getJustNoticableDifference(double freq) {
  if (freq < 250) {return 1.0;}
  if (freq < 500) {return 1.25;}
  if (freq < 1000) {return 2.5;}
  if (freq < 2000) {return 4.0;}
  if (freq < 4000) {return 20.0;}
  if (freq < 8000) {return 88.0;}
  return freq / 100.0;
}

void BonkInstance::calcPhase(double damping, double freqDamping) {
  phase.resizeLike(freq);  phase.setZero();
  damp.resizeLike(freq);  damp.setZero();
  phase_step.resizeLike(freq);  phase_step.setZero();
  amp.resizeLike(freq);  amp.setZero();
  for (int i = 0; i < freq.size(); i++) {
    phase_step[i] = freq[i] * dt;
    double d = damping + (freq[i] * freqDamping);
    damp[i] = std::exp(-d * dt);
  }
}

void BonkInstance::compressModesAndCalcPhase(double damping, double freqDamping) {
  std::vector<double> temp_freq {};
  std::vector<V> temp_modes {};
  V mode_sum = modes.col(0);
  double freq_sum = freq[0];
  int count {1};
  auto start_f = freq[0];
  for (int i = 1; i < freq.size(); ++i) {
    auto f = freq[i];
    auto f_prev = freq[i-1];
    auto limit = getJustNoticableDifference(f_prev) * 2;
    if (std::abs(f - f_prev) < limit && std::abs(f - start_f) < limit * 2) {
      mode_sum += modes.col(i);
      freq_sum += f;
      count++;
    } else {
      temp_freq.push_back(freq_sum / count);
      temp_modes.push_back(mode_sum / std::sqrt(count));
      mode_sum = modes.col(i);
      freq_sum = f;
      count = 1;
      start_f = freq[i];
    }
  }
  temp_freq.push_back(freq_sum / count);
  temp_modes.push_back(mode_sum / std::sqrt(count));
  auto n_modes = temp_freq.size();
  freq.resize(n_modes);
  modes.resize(3*vert_count, n_modes);
  for (int i = 0; i < n_modes; ++i) {
    freq[i] = temp_freq[i];
    modes.col(i) = temp_modes[i];
  }
  calcPhase(damping, freqDamping);
}

BonkInstance::BonkResult BonkInstance::initModalContext(double density, double k, double dt, double damping, double freqDamping) {
  if (!isTetMesh) {return BonkResult::BadInvocation;}
  this->density = density;
  this->dt = dt;
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
  int nconv = eigs.compute(Spectra::SortRule::LargestMagn);
  if (eigs.info() != Spectra::CompInfo::Successful) {
    return BonkResult::ModalSetupFailure;
  }

  freq = eigs.eigenvalues();
  modes = eigs.eigenvectors();
  for (int i = 0; i < static_cast<int>(freq.size()); i++) {
    auto f = freq[i] > 0 ? std::sqrt(freq[i]) : 0.0;
    freq[i] = f / (2.0 * std::numbers::pi);
  }

  compressModesAndCalcPhase(damping, freqDamping);
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
