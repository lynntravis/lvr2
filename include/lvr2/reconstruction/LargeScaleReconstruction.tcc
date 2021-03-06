/**
 * Copyright (c) 2018, University Osnabrück
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University Osnabrück nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL University Osnabrück BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <iostream>
#include <lvr2/types/ScanTypes.hpp>
#include <lvr2/io/GHDF5IO.hpp>
#include <lvr2/io/hdf5/ChannelIO.hpp>
#include <lvr2/io/hdf5/ArrayIO.hpp>
#include <lvr2/io/hdf5/VariantChannelIO.hpp>
#include <lvr2/io/hdf5/MeshIO.hpp>
#include <lvr2/reconstruction/BigGrid.hpp>
#include <lvr2/reconstruction/VirtualGrid.hpp>
#include <lvr2/reconstruction/BigGridKdTree.hpp>
#include <lvr2/reconstruction/AdaptiveKSearchSurface.hpp>
#include <lvr2/reconstruction/PointsetGrid.hpp>
#include <lvr2/reconstruction/FastBox.hpp>
#include <lvr2/reconstruction/FastReconstruction.hpp>
#include "lvr2/algorithm/CleanupAlgorithms.hpp"
#include <lvr2/algorithm/NormalAlgorithms.hpp>
#include "lvr2/algorithm/Tesselator.hpp"


#include "LargeScaleReconstruction.hpp"

namespace lvr2
{
    using LSRWriter = lvr2::Hdf5IO<lvr2::hdf5features::ArrayIO,
            lvr2::hdf5features::ChannelIO,
            lvr2::hdf5features::VariantChannelIO,
            lvr2::hdf5features::MeshIO>;

    using Vec = lvr2::BaseVector<float>;

    //TODO: write set Methods for certain variables

    template <typename BaseVecT>
    LargeScaleReconstruction<BaseVecT>::LargeScaleReconstruction(string h5File)
    : m_filePath(h5File), m_voxelSize(0.1), m_bgVoxelSize(10), m_scale(1), m_chunkSize(100),m_nodeSize(1000000), m_partMethod(1),
    m_Ki(10), m_Kd(5), m_Kn(10), m_useRansac(false), m_extrude(false), m_removeDanglingArtifacts(0), m_cleanContours(0),
    m_fillHoles(0), m_optimizePlanes(false), m_getNormalThreshold(0.85), m_planeIterations(3), m_MinPlaneSize(7), m_SmallRegionThreshold(0),
    m_retesselate(false), m_LineFusionThreshold(0.01)
    {
        std::cout << "Reconstruction Instance generated..." << std::endl;
    }

    template<typename BaseVecT>
    LargeScaleReconstruction<BaseVecT>::LargeScaleReconstruction(std::string h5File, float voxelSize, float bgVoxelSize,
                                                                 float scale, size_t chunkSize, uint nodeSize,
                                                                 int partMethod, int ki, int kd, int kn, bool useRansac,
                                                                 bool extrude, int removeDanglingArtifacts,
                                                                 int cleanContours, int fillHoles, bool optimizePlanes,
                                                                 float getNormalThreshold, int planeIterations,
                                                                 int minPlaneSize, int smallRegionThreshold,
                                                                 bool retesselate, float lineFusionThreshold)
            : m_filePath(h5File), m_voxelSize(voxelSize), m_bgVoxelSize(bgVoxelSize),
              m_scale(scale), m_chunkSize(chunkSize),m_nodeSize(nodeSize),
              m_partMethod(partMethod), m_Ki(ki), m_Kd(kd), m_Kn(kn), m_useRansac(useRansac),
              m_extrude(extrude), m_removeDanglingArtifacts(removeDanglingArtifacts),
              m_cleanContours(cleanContours), m_fillHoles(fillHoles), m_optimizePlanes(optimizePlanes),
              m_getNormalThreshold(getNormalThreshold), m_planeIterations(planeIterations),
              m_MinPlaneSize(minPlaneSize), m_SmallRegionThreshold(smallRegionThreshold),
              m_retesselate(retesselate), m_LineFusionThreshold(lineFusionThreshold)
    {
    }

    template<typename BaseVecT>
    LargeScaleReconstruction<BaseVecT>::LargeScaleReconstruction(LargeScaleOptions::Options options) : LargeScaleReconstruction<BaseVecT>::LargeScaleReconstruction(options.getInputFileName()[0], options.getVoxelsize(), options.getBGVoxelsize(), options.getScaling(), options.getGridSize(),
                                                                                                                                                                    options.getNodeSize(), options.getVGrid(), options.getKi(), options.getKd(), options.getKn(), options.useRansac(), options.extrude(),
                                                                                                                                                                    options.getDanglingArtifacts(), options.getCleanContourIterations(), options.getFillHoles(), options.optimizePlanes(),
                                                                                                                                                                    options.getNormalThreshold(), options.getPlaneIterations(), options.getMinPlaneSize(), options.getSmallRegionThreshold(),
                                                                                                                                                                    options.retesselate(), options.getLineFusionThreshold())
    {
        std::cout << "Reconstruction Instance generated..." << std::endl;
    }



    template <typename BaseVecT>
    int LargeScaleReconstruction<BaseVecT>::mpiChunkAndReconstruct(ScanProjectEditMarkPtr project)
    {

        if(project->positions.size() != project->changed.size())
        {
            cout << "Inconsistency between number of given scans and diff-vector (scans to consider)! exit..." << endl;
            return 0;
        }

        //do more or less the same stuff as the executable
        cout << lvr2::timestamp << "Starting grid" << endl;

        //TODO: replace with new incremental Constructor later
        //BigGrid<BaseVecT> bg(m_filePath, m_bgVoxelSize, m_scale);
        BigGrid<BaseVecT> bg( m_bgVoxelSize ,project, m_scale);

        cout << lvr2::timestamp << "grid finished " << endl;
        BoundingBox<BaseVecT> bb = bg.getBB();
        cout << bb << endl;

        vector<BoundingBox<BaseVecT>> partitionBoxes;

        cout << lvr2::timestamp << "making tree" << endl;
        if (m_partMethod == 1)
        {
            VirtualGrid<BaseVecT> vGrid(
                    bg.getpartialBB(), m_chunkSize, m_bgVoxelSize);
            std::vector<shared_ptr<BoundingBox<BaseVecT>>> boxes;
            vGrid.calculateBoxes();
            ofstream partBoxOfs("BoundingBoxes.ser");
            for (size_t i = 0; i < vGrid.getBoxes().size(); i++)
            {
                BoundingBox<BaseVecT> partBB = *vGrid.getBoxes().at(i).get();
                partitionBoxes.push_back(partBB);
                partBoxOfs << partBB.getMin()[0] << " " << partBB.getMin()[1] << " "
                           << partBB.getMin()[2] << " " << partBB.getMax()[0] << " "
                           << partBB.getMax()[1] << " " << partBB.getMax()[2] << std::endl;
            }
            cout << lvr2::timestamp << "finished vGrid" << endl;
            std::cout << lvr2::timestamp << "got: " << partitionBoxes.size() << " Chunks"
                      << std::endl;
        }
        else
        {
            BigGridKdTree<BaseVecT> gridKd(bg.getBB(), m_nodeSize, &bg, m_bgVoxelSize);
            gridKd.insert(bg.pointSize(), bg.getBB().getCentroid());
            ofstream partBoxOfs("KdTree.ser");
            for (size_t i = 0; i < gridKd.getLeafs().size(); i++)
            {
                BoundingBox<BaseVecT> partBB = gridKd.getLeafs()[i]->getBB();
                partitionBoxes.push_back(partBB);
                partBoxOfs << partBB.getMin()[0] << " " << partBB.getMin()[1] << " "
                           << partBB.getMin()[2] << " " << partBB.getMax()[0] << " "
                           << partBB.getMax()[1] << " " << partBB.getMax()[2] << std::endl;
            }

            cout << lvr2::timestamp << "finished tree" << endl;
            std::cout << lvr2::timestamp << "got: " << partitionBoxes.size() << " leafs, saving leafs"
                      << std::endl;
        }

        BaseVecT bb_min(bb.getMin().x, bb.getMin().y, bb.getMin().z);
        BaseVecT bb_max(bb.getMax().x, bb.getMax().y, bb.getMax().z);
        BoundingBox<BaseVecT> cbb(bb_min, bb_max);

        vector<string> grid_files;
        unordered_set<string> meshes;
        // vector to save the new chunk names - which chunks have to be reconstructed
        vector<string> newChunks = vector<string>();

        uint partitionBoxesSkipped = 0;

        //create chunks
        for (int i = 0; i < partitionBoxes.size(); i++)
        {
            string name_id;
            if (m_partMethod == 1)
            {
                name_id =
                        std::to_string(
                                (int)floor(partitionBoxes.at(i).getMin().x / m_chunkSize)) +
                        "_" +
                        std::to_string(
                                (int)floor(partitionBoxes.at(i).getMin().y / m_chunkSize)) +
                        "_" +
                        std::to_string((int)floor(partitionBoxes.at(i).getMin().z / m_chunkSize));
            }
            else
            {
                name_id = std::to_string(i);
            }

            size_t numPoints;

            // todo: okay?
            floatArr points = bg.points(partitionBoxes[i].getMin().x,
                                        partitionBoxes[i].getMin().y,
                                        partitionBoxes[i].getMin().z,
                                        partitionBoxes[i].getMax().x,
                                        partitionBoxes[i].getMax().y,
                                        partitionBoxes[i].getMax().z,
                                        numPoints);

            // remove chunks with less than 50 points
            if (numPoints <= 50)
            {
                partitionBoxesSkipped++;
                continue;
            }

            BaseVecT gridbb_min(partitionBoxes[i].getMin().x,
                                partitionBoxes[i].getMin().y,
                                partitionBoxes[i].getMin().z);
            BaseVecT gridbb_max(partitionBoxes[i].getMax().x,
                                partitionBoxes[i].getMax().y,
                                partitionBoxes[i].getMax().z);
            BoundingBox<BaseVecT> gridbb(gridbb_min, gridbb_max);

            cout << "grid: " << i << "/" << partitionBoxes.size() - 1 << endl;
            cout << "grid has " << numPoints << " points" << endl;
            cout << "kn=" << m_Kn << endl;
            cout << "ki=" << m_Ki << endl;
            cout << "kd=" << m_Kd << endl;
            cout << gridbb << endl;

            lvr2::PointBufferPtr p_loader(new lvr2::PointBuffer);
            p_loader->setPointArray(points, numPoints);

            if (bg.hasNormals())
            {
                size_t numNormals;
                lvr2::floatArr normals = bg.normals(partitionBoxes[i].getMin().x,
                                                    partitionBoxes[i].getMin().y,
                                                    partitionBoxes[i].getMin().z,
                                                    partitionBoxes[i].getMax().x,
                                                    partitionBoxes[i].getMax().y,
                                                    partitionBoxes[i].getMax().z,
                                                    numNormals);

                p_loader->setNormalArray(normals, numNormals);
                cout << "got " << numNormals << " normals" << endl;
            }

            lvr2::PointsetSurfacePtr<Vec> surface;
            surface = make_shared<lvr2::AdaptiveKSearchSurface<Vec>>(p_loader,
                                                                     "FLANN",
                                                                     m_Kn,
                                                                     m_Ki,
                                                                     m_Kd,
                                                                     m_useRansac);
            //calculate important stuff for reconstruction
            if (!bg.hasNormals())
                surface->calculateSurfaceNormals();

            auto ps_grid = std::make_shared<lvr2::PointsetGrid<Vec, lvr2::FastBox<Vec>>>(
                    m_voxelSize, surface, gridbb, true, m_extrude);

            ps_grid->setBB(gridbb);
            ps_grid->calcIndices();
            ps_grid->calcDistanceValues();

            //TODO: is this even used? if not:remove it
            auto reconstruction =
                   make_unique<lvr2::FastReconstruction<Vec, lvr2::FastBox<Vec>>>(ps_grid);

            // save in HDF5
            ps_grid->saveCellsHDF5(m_filePath, name_id);
            // also save the name that is added to the hdf5
            newChunks.push_back(name_id);

            // save the mesh of the chunk
            // additionally for debug: Save the mesh as a ply todo delete later
            lvr2::HalfEdgeMesh<Vec> mesh;
            reconstruction->getMesh(mesh);
            lvr2::SimpleFinalizer<Vec> finalize;
            auto meshBuffer = MeshBufferPtr(finalize.apply(mesh));
            LSRWriter hdfWrite;
            hdfWrite.open(m_filePath);
            hdfWrite.save("chunks/" + name_id, meshBuffer);
            auto m = ModelPtr(new Model(meshBuffer));
            ModelFactory::saveModel(m, name_id + ".ply");

            // TODO: can be removed
//            // replaced creating .ser files to creating chunk in HDF5
//            std::stringstream ss2;
//            ss2 << name_id << ".ser";
//            ps_grid->saveCells(ss2.str());
//            meshes.insert(ss2.str());
        }

        // TODO: can be removed
//        //TODO: replace reading from .ser file to reading from ChunkManager
//        ifstream old_mesh("VGrid.ser");
//        if (m_partMethod == 1 && old_mesh.is_open())
//        {
//            while (old_mesh.good())
//            {
//                string mesh;
//                getline(old_mesh, mesh);
//                cout << "Old Mesh: " << mesh << endl;
//                if (!mesh.empty())
//                {
//                    meshes.insert(mesh);
//                }
//            }
//        }

        std::cout << "Skipped PartitionBoxes: " << partitionBoxesSkipped << std::endl;
        std::cout << "Generated Meshes: " << meshes.size() << std::endl;

        //TODO: remove this too
//        ofstream vGrid_ser;
//        vGrid_ser.open("VGrid.ser", ofstream::out | ofstream::trunc);
//        unordered_set<string>::iterator itr;
//
//        //TODO: potentially remove mesh-combine or leave it as a debug output
//        for (itr = meshes.begin(); itr != meshes.end(); itr++)
//        {
//            vGrid_ser << *itr << std::endl;
//            grid_files.push_back(*itr);
//        }
//
//        vGrid_ser.close();

        cout << lvr2::timestamp << "finished" << endl;

        //combine chunks
        auto vmax = cbb.getMax();
        auto vmin = cbb.getMin();
        vmin.x -= m_bgVoxelSize * 2;
        vmin.y -= m_bgVoxelSize * 2;
        vmin.z -= m_bgVoxelSize * 2;
        vmax.x += m_bgVoxelSize * 2;
        vmax.y += m_bgVoxelSize * 2;
        vmax.z += m_bgVoxelSize * 2;
        cbb.expand(vmin);
        cbb.expand(vmax);

        // auto hg = std::make_shared<HashGrid<BaseVecT, lvr2::FastBox<Vec>>>(grid_files, cbb, m_voxelSize);
        auto hg = std::make_shared<HashGrid<BaseVecT, lvr2::FastBox<Vec>>>(m_filePath, newChunks, cbb);

        auto reconstruction = make_unique<lvr2::FastReconstruction<Vec, lvr2::FastBox<Vec>>>(hg);

        lvr2::HalfEdgeMesh<Vec> mesh;

        reconstruction->getMesh(mesh);

        if (m_removeDanglingArtifacts)
        {
            cout << timestamp << "Removing dangling artifacts" << endl;
            removeDanglingCluster(mesh, static_cast<size_t>(m_removeDanglingArtifacts));
        }

        // Magic number from lvr1 `cleanContours`...
        cleanContours(mesh, m_cleanContours, 0.0001);

        // Fill small holes if requested
        if (m_fillHoles)
        {
            naiveFillSmallHoles(mesh, m_fillHoles, false);
        }

        // Calculate normals for vertices
        auto faceNormals = calcFaceNormals(mesh);

        ClusterBiMap<FaceHandle> clusterBiMap;
        if (m_optimizePlanes)
        {
            clusterBiMap = iterativePlanarClusterGrowing(mesh,
                                                         faceNormals,
                                                         m_getNormalThreshold,
                                                         m_planeIterations,
                                                         m_MinPlaneSize);

            if (m_SmallRegionThreshold > 0)
            {
                deleteSmallPlanarCluster(
                        mesh, clusterBiMap, static_cast<size_t>(m_SmallRegionThreshold));
            }

            double end_s = lvr2::timestamp.getElapsedTimeInS();

            if (m_retesselate)
            {
                Tesselator<Vec>::apply(
                        mesh, clusterBiMap, faceNormals, m_LineFusionThreshold);
            }
        }
        else
        {
            clusterBiMap = planarClusterGrowing(mesh, faceNormals, m_getNormalThreshold);
        }



        // Finalize mesh
        lvr2::SimpleFinalizer<Vec> finalize;
        auto meshBuffer = finalize.apply(mesh);

        // save mesh depending on input file type
        boost::filesystem::path selectedFile(m_filePath);

        std::time_t result = std::time(nullptr);
        std::string largeScale = (string) std::asctime(std::localtime(&result)) + ".ply";

        if (selectedFile.extension().string() == ".h5")
        {
            //MeshBufferPtr newMesh = MeshBufferPtr(meshBuffer);
            //LSRWriter hdfWrite;
            //hdfWrite.open(m_filePath);
            //hdfWrite.save("mesh", newMesh);

            auto m = ModelPtr(new Model(meshBuffer));
            ModelFactory::saveModel(m, largeScale);
        }
        else
        {
            auto m = ModelPtr(new Model(meshBuffer));
            ModelFactory::saveModel(m, largeScale);
        }

        return 1;
    }
    template <typename BaseVecT>
    int LargeScaleReconstruction<BaseVecT>::resetEditMark(ScanProjectEditMarkPtr project)
    {
        for(auto mark : project->changed)
        {
            mark = false;
        }

        return 1;
    }

}