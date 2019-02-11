/*
 * Mesh3DGen.h
 *
 *  Created on: 22.11.2016
 *      Author: tzovas
 */
#pragma once

#include <scai/lama.hpp>
#include <scai/lama/matrix/all.hpp>
#include <scai/lama/Vector.hpp>
#include <scai/dmemo/Distribution.hpp>
#include <scai/dmemo/BlockDistribution.hpp>
#include <scai/common/Math.hpp>
#include <scai/lama/storage/MatrixStorage.hpp>
#include <scai/tracing.hpp>

#include <assert.h>
#include <cmath>
#include <set>
#include <climits>
#include <list>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <iterator>
#include <tuple>    
#include <random>

#include "quadtree/Point.h"
#include "quadtree/SpatialTree.h"
#include "quadtree/SpatialCell.h"
#include "quadtree/QuadTreeCartesianEuclid.h" 

#include "AuxiliaryFunctions.h"
#include "Settings.h"


using scai::lama::CSRSparseMatrix;
using scai::lama::DenseVector;


namespace ITI {
	template <typename IndexType, typename ValueType>
	class MeshGenerator{
            public:
                
                static void createOctaTreeMesh( scai::lama::CSRSparseMatrix<ValueType> &adjM,  std::vector<DenseVector<ValueType>> &coords, const IndexType numberOfPoints, const ValueType maxCoord);

                static void createOctaTreeMesh_2( scai::lama::CSRSparseMatrix<ValueType> &adjM,  std::vector<DenseVector<ValueType>> &coords, const IndexType numberOfPoints, const ValueType maxCoord);
                
                
                static void writeGraphStructured3DMesh_seq( std::vector<IndexType> numPoints, const std::string filename);
                /** Creates a structed 3D mesh, both the adjacency matrix and the coordinates vectors.
                 * 
                 * @param[out] adjM The adjacency matrix of the output graph. Dimensions are [numPoints[0] x numPoints[1] x numPoints[2]].
                 * @param[out] coords The coordinates of every graph node. coords.size()=2 and coords[i].size()=numPoints[i], so a point i(x,y,z) has coordinates (coords[0][i], coords[1][i], coords[2][i]).
                 * @param[in] maxCoord The maximum value a coordinate can have in each dimension, maxCoord.size()=3.
                 * @param[in] numPoints The number of points in every dimension, numPoints.size()=3.
                 */
                static void createStructured3DMesh_seq(CSRSparseMatrix<ValueType> &adjM, std::vector<DenseVector<ValueType>> &coords, const std::vector<ValueType> maxCoord, const std::vector<IndexType> numPoints);
                
                /** Creates the adjacency matrix and the coordinate vector for a 3D mesh in a distributed way.
                 */
                static void createStructured3DMesh_dist(CSRSparseMatrix<ValueType> &adjM, std::vector<DenseVector<ValueType>> &coords, const std::vector<ValueType> maxCoord, const std::vector<IndexType> numPoints);
                
                static void createStructured2DMesh_dist(CSRSparseMatrix<ValueType> &adjM, std::vector<DenseVector<ValueType>> &coords, const std::vector<ValueType> maxCoord, const std::vector<IndexType> numPoints);
                
                static void createRandomStructured3DMesh_dist(CSRSparseMatrix<ValueType> &adjM, std::vector<DenseVector<ValueType>> &coords, const std::vector<ValueType> maxCoord, const std::vector<IndexType> numPoints);
                
                /*Creates points in a cube of side maxCoord in dimensions and adds them in a quad tree.
                 * Adds more points in specific areas at random. 
                 */
                static void createQuadMesh( CSRSparseMatrix<ValueType> &adjM, std::vector<DenseVector<ValueType>> &coords,const int dimensions, const IndexType numberOfAreas, const IndexType pointsPerArea, const ValueType maxCoord, IndexType seed);

                static void graphFromQuadtree(CSRSparseMatrix<ValueType> &adjM, std::vector<DenseVector<ValueType>> &coords, const QuadTreeCartesianEuclid &quad);
                    
                /* Creates random points in the cube for the given dimension, points in [0,maxCoord]^dim.
                 */
                static std::vector<DenseVector<ValueType>> randomPoints(IndexType numberOfPoints, int dimensions, ValueType maxCoord);
                
                /* Calculates the 3D distance between two points.
                 */
                static ValueType dist3D(DenseVector<ValueType> p1, DenseVector<ValueType> p2);
                                
                static ValueType dist3DSquared(std::tuple<IndexType, IndexType, IndexType> p1, std::tuple<IndexType, IndexType, IndexType> p2);
                
                /*  Given a (global) index and the size for each dimension (numPpoints.size()=3) calculates the position
                 *  of the index in 3D. The return value is not the coordinates of the point!
                 */
                //static std::tuple<IndexType, IndexType, IndexType> index2_3DPoint(IndexType index,  std::vector<IndexType> numPoints);
        };//class MeshGenerator
        
}//namespace ITI
