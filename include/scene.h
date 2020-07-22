#pragma once
#include <pcl/point_cloud.h>
#include "bshot_bits.h"

class Scene {
public: 
	std::string identifier;
	pcl::PointCloud<pcl::PointXYZ>::Ptr cloud;
	pcl::PointCloud<pcl::Normal> normals;
	pcl::PointCloud<pcl::PointXYZ> keypoints;
	std::vector<bshot_descriptor> descriptors;
	float resolution = 0.0f;
	Scene(string name_, pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_)
	: identifier(name_), cloud(cloud_) {
	}


};