#include "command/command.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <map>
#include "common/file_helper.h"
#include "common/math_helper.h"
#include "mesh/tri_mesh.h"
#include "mesh/level_set_helper.h"

namespace command {

static void CheckHausdorffCommandFileName(
  const std::string& input_mesh, const std::string& target_mesh) {
  if (!common::HasExtension(input_mesh, ".off")) {
    std::cerr << common::RedHead()
      << "command::CheckHausdorffCommandFileName: wrong mesh file: "
      << input_mesh << common::RedTail() << std::endl;
    exit(-1);
  }
  if (!common::HasExtension(target_mesh, ".off")) {
    std::cerr << common::RedHead()
      << "command::CheckHausdorffCommandFileName: wrong mesh file: "
      << target_mesh << common::RedTail() << std::endl;
    exit(-1);
  }
}

void HausdorffCommand(const std::string& input_mesh,
  const std::string& target_mesh, const double density,
  const bool help, const bool verbose) {
  if (help) {
    std::cout
      << "hausdorff           Compute the Hausdorff distance." << std::endl
      << "  -a <OFF file>     The first input mesh." << std::endl
      << "  -b <OFF file>     The second input mesh." << std::endl
      << "  -d <double>       The surface sampling density." << std::endl
      << "  -h                Print help information." << std::endl
      << "  -v                Verbose mode." << std::endl << std::endl
      << "This function computes sup_{y\in mesh A} inf_{x\in mesh B}d(y, x)\|"
      << std::endl
      << "In other words, for each point in A, we find the minimum distance to "
      << "the whole mesh B. We then pick the point in A that has the maximum "
      << "value. In general, A should be the input mesh, and B should be the "
      << "mesh generated by our CSG solution." << std::endl << std::endl;
    return;
  }

  // Check inputs.
  CheckHausdorffCommandFileName(input_mesh, target_mesh);
  if (density <= 0.0) {
    common::PrintError("command::HausdorffCommand: nonpositive density.");
  }

  // Load two meshes.
  Eigen::Matrix3Xd vertex_a, vertex_b;
  Eigen::Matrix3Xi face_a, face_b;
  common::ReadOffFile(input_mesh, vertex_a, face_a);
  common::ReadOffFile(target_mesh, vertex_b, face_b);

  // Loop over all faces in mesh A.
  const int face_a_num = static_cast<int>(face_a.cols());
  const int face_b_num = static_cast<int>(face_b.cols());
  std::vector<Eigen::Matrix3d> mesh_b_triangles(0);
  mesh_b_triangles.reserve(face_b_num);
  for (int i = 0; i < face_b_num; ++i) {
    Eigen::Matrix3d triangle;
    for (int j = 0; j < 3; ++j)
      triangle.col(j) = vertex_b.col(face_b(j, i));
    mesh_b_triangles.push_back(triangle);
  }
  double hausdorff_dist = -common::Infinity();
  int progress = 0;
  for (int i = 0; i < face_a_num; ++i) {
    if (i / (face_a_num / 10) > progress) {
      ++progress;
      if (verbose) {
        std::cout << progress * 10 << "% is done." << std::endl;
      }
    }
    Eigen::Matrix3d triangle;
    for (int j = 0; j < 3; ++j) {
      triangle.col(j) = vertex_a.col(face_a(j, i));
    }
    const Eigen::Vector3d v0 = triangle.col(0), v1 = triangle.col(1),
      v2 = triangle.col(2);
    const double area = (v1 - v0).cross(v2 - v1).norm() * 0.5;
    int sample_num = static_cast<int>(area * density);
    const Eigen::Matrix3Xd samples =
      common::UniformSampleFromTriangle(v0, v1, v2, sample_num);
    sample_num = static_cast<int>(samples.cols());
    for (int j = 0; j < sample_num; ++j) {
      const Eigen::Vector3d p = samples.col(j);
      // Compute the distance from p to mesh B.
      double dist = common::Infinity();
      for (const auto& t : mesh_b_triangles) {
        const double d = common::PointToTriangleDistance(p, t);
        if (d < dist) dist = d;
      }
      if (dist > hausdorff_dist) hausdorff_dist = dist;
    }
  }
  std::cout << "Hausdorff distance: " << hausdorff_dist << std::endl;
}

}