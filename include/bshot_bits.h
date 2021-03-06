/*
Parts of this code were taken from:
https://github.com/saimanoj18/iros_bshot
@INPROCEEDINGS{bshot_iros2015,
author={S. M. Prakhya and Bingbing Liu and Weisi Lin},
booktitle={Intelligent Robots and Systems (IROS), 2015 IEEE/RSJ International Conference on},
title={B-SHOT: A binary feature descriptor for fast and efficient keypoint matching on 3D point clouds},
year={2015},
pages={1929-1934},
doi={10.1109/IROS.2015.7353630},
month={Sept},}

additional parts were written by Jo�l Carlen, Student at the Lucerne University of Applied Sciences and Arts
*/


#include "bshot_headers_bits.h"
#ifndef BSHOT_BITS_H
#define BSHOT_BITS_H

//Get index of the nearest neighbor
template< typename T >
T minVect(const T *v, int n, int *ind=NULL)
{
    assert(n > 0);

    T min = v[0];
    if (ind != NULL) *ind = 0;
    for (int i=1; i<n; i++)
        if (v[i] < min) {
            min = v[i];
            if (ind != NULL) *ind=i;
        }

    return min;
}
//Get index of second nearest neighbor
template< typename T >
T secondMinVect(const T* v, int n, int val, int* ind = NULL)
{
	assert(n > 0);
	T min = v[0];
	if (ind != NULL)* ind = 0;
	for (int i = 1; i < n; i++)
	{
		if (v[i] < min && v[i] > val) {
			min = v[i];
			if (ind != NULL)* ind = i;
		}
	}
	return min;
}

class bshot_descriptor
{
public:
    std::bitset< 352 > bits;
};


class bshot
{
public :
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud;
    pcl::PointCloud<pcl::Normal> normals;
    pcl::PointCloud<pcl::PointXYZ> keypoints;
    pcl::PointCloud<pcl::SHOT352> shot;
    std::vector<bshot_descriptor> bshot;

    //void  calculate_voxel_grid_keypoints ( float leaf_size )
    //{
    //    // Find Keypoints on the input cloud
    //    pcl::VoxelGrid<pcl::PointXYZ> voxel_grid;
    //    voxel_grid.setLeafSize(leaf_size, leaf_size, leaf_size);
    //    voxel_grid.setInputCloud(cloud1.makeShared());
    //    voxel_grid.filter(cloud1_keypoints);
    //    voxel_grid.setInputCloud(cloud2.makeShared());
    //    voxel_grid.filter(cloud2_keypoints);
    //}
    
    void calculate_SHOT(float radius) {
        // SHOT estimation object.
        pcl::SHOTEstimation<pcl::PointXYZ, pcl::Normal, pcl::SHOT352> shot_calculator;
        shot_calculator.setRadiusSearch(radius);
        //shot.setNumberOfThreads(30);
        pcl::search::KdTree<pcl::PointXYZ>::Ptr kdtree(new pcl::search::KdTree<pcl::PointXYZ>);
        shot_calculator.setSearchMethod(kdtree);
        shot_calculator.setInputCloud(keypoints.makeShared());
        shot_calculator.setSearchSurface(cloud);
        shot_calculator.setInputNormals(normals.makeShared());
        shot_calculator.compute(shot);
    }

    void compute_bshot() {
        compute_bshot_from_SHOT(shot, bshot);
    }

    void compute_bshot_from_SHOT(pcl::PointCloud<pcl::SHOT352>& shot_descriptors_here, std::vector<bshot_descriptor>& bshot_descriptors)
    {
        bshot_descriptors.resize(shot_descriptors_here.size());
        for (int i = 0; i < (int)shot_descriptors_here.size(); i++)
        {
            std::bitset < 352 > temp;
            temp.reset();

            for (int j = 0 ; j < 88 ; j++)
            {
                float vec[4] = { 0 };
                for (int k = 0 ; k < 4 ; k++)
                {
                    vec[k] = shot_descriptors_here[i].descriptor[ j*4 + k ];

                }

                std::bitset< 4 > bit;
                bit.reset();

                float sum = vec[0]+vec[1]+vec[2]+vec[3];

                if (vec[0] == 0 && vec [1] == 0 && vec[2] == 0 && vec[3] == 0)
                {
                    //bin[0] = bin[1] = bin[2] = bin[3] = 0;
                    // by default , they are all ZEROS
                }
                else if ( vec[0] > (0.9 * (sum) ) )
                {
                    bit.set(0);
                }
                else if ( vec[1] > (0.9 * (sum) ) )
                {
                    bit.set(1);
                }
                else if ( vec[2] > (0.9 * (sum) ) )
                {
                    bit.set(2);
                }
                else if ( vec[3] > (0.9 * (sum) ) )
                {

                    bit.set(3);
                }
                else if ( (vec[0]+vec[1]) > (0.9 * (sum))  )
                {

                    bit.set(0);
                    bit.set(1);
                }
                else if ( (vec[1]+vec[2]) > (0.9 * (sum)) )
                {

                    bit.set(1);
                    bit.set(2);
                }

                else if ( (vec[2]+vec[3]) > (0.9 * (sum)) )
                {
                    ;
                    bit.set(2);
                    bit.set(3);
                }
                else if ( (vec[0]+vec[3]) > (0.9 * (sum)) )
                {

                    bit.set(0);
                    bit.set(3);
                }
                else if ( (vec[1]+vec[3]) > (0.9 * (sum)) )
                {

                    bit.set(1);
                    bit.set(3);
                }
                else if ( (vec[0]+vec[2]) > (0.9 * (sum)) )
                {

                    bit.set(0);
                    bit.set(2);
                }
                else if ( (vec[0]+ vec[1] +vec[2]) > (0.9 * (sum)) )
                {

                    bit.set(0);
                    bit.set(1);
                    bit.set(2);
                }
                else if ( (vec[1]+ vec[2] +vec[3]) > (0.9 * (sum)) )
                {

                    bit.set(1);
                    bit.set(2);
                    bit.set(3);
                }
                else if ( (vec[0]+ vec[2] +vec[3]) > (0.9 * (sum)) )
                {

                    bit.set(0);
                    bit.set(2);
                    bit.set(3);
                }
                else if ( (vec[0]+ vec[1] +vec[3]) > (0.9 * (sum)) )
                {

                    bit.set(0);
                    bit.set(1);
                    bit.set(3);
                }
                else
                {

                    bit.set(0);
                    bit.set(1);
                    bit.set(2);
                    bit.set(3);
                }

                if (bit.test(0))
                    temp.set(j*4);

                if (bit.test(1))
                    temp.set(j*4 + 1);

                if (bit.test(2))
                    temp.set(j*4 + 2);

                if (bit.test(3))
                    temp.set(j*4 + 3);

            }

            bshot_descriptors[i].bits = temp;
        }
    }

};
#endif // !BSHOT_BITS_H



