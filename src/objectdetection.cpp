//Includes
#include <iostream>
#include <pcl/filters/median_filter.h>
#include <pcl/filters/statistical_outlier_removal.h>
#include <pcl/keypoints/sift_keypoint.h>
#include "../include/keypointdetector.h"
#include "../include/matching.h"
#include "../include/scene.h"
#include "../include/filehandler.h"
#include "../include/cloud_creator.h"
#include <pcl/segmentation/extract_clusters.h>
#include <pcl/registration/icp.h>
#include <pcl/surface/mls.h>
#include <pcl/filters/shadowpoints.h>
#include <random>
#include "../include/menu/base_menu.h"
#include "../include/menu/main_menu.h"
#include "../include/menu/creation_menu.h"
#include "../include/menu/detection_menu.h"
#include "../include/menu/filter_menu.h"
#include "../include/menu/merge_menu.h"
#include "../include/registrator.h"
#include "../include/parameter_handler.h"
#include "../include/measure.h"
#include "../include/pose_estimator.h"
#include "objectdetection.h"

//Namespaces
namespace fs = boost::filesystem;
using namespace std;
using namespace pcl;

#ifndef isshot
#define isshot 1	//For SHOT-Descriptor use 1 for FPFH 0
#endif
/*
______
| ___ \
| |_/ /__ _  _ __  __ _  _ __ ___   ___
|  __// _` || '__|/ _` || '_ ` _ \ / __|
| |  | (_| || |  | (_| || | | | | |\__ \
\_|   \__,_||_|   \__,_||_| |_| |_||___/
Params used during cloudgen & objectdetection
*/
std::vector<tuple<bshot_descriptor, int>> learned_descriptors;
std::vector<tuple<string, vector<double>>> angles;

float shotRadius_ = 30;
float fpfhRadius_ = 20;
const int rows = 240, columns = 320;
bool query_learning = false;

//Paths used during objectdetection
string query_directory = "../../../../clouds/query_clouds";
string target_directory = "../../../../clouds/target_clouds";
string pr_root = "../../../../PR/Buch";
string stats_root = "../../../../stats";
string pose_estimation_root = "../../../../pose_estimation";
vector<fs::path> query_names;
vector<fs::path> target_names;
//Paths used during merging
string origin_directory = "../../../../clouds/merging/origin";
string permutation_directory = "../../../../clouds/merging/permutation";
vector<fs::path> origin_names;
vector<fs::path> permutation_names;
//Paths used during learning
string query_learning_directory = "../../../../clouds/learning";
vector<fs::path> query_learning_names;
//Paths used during processing
string processing_directory = "../../../../clouds/processing";
string processed_directory = "../../../../clouds/processing/processed";
vector<fs::path> processing_names;

#if isshot
const float supportRadius_ = shotRadius_;
string descriptor = "B_SHOT";
#else
const float supportRadius_ = fpfhRadius_;
string descriptor = "FPFH";
#endif

std::string get_identifier(string filename, int name_pos, int extension_pos) {
	return filename.substr(name_pos, (extension_pos - name_pos));
}

std::string get_input() {
	std::cout << "enter value: " << std::endl;
	string input;
	cin >> input;
	return input;
}

int nthOccurrence(const std::string& str, const std::string& findMe, int nth)
{
	size_t  pos = 0;
	int     cnt = 0;

	while (cnt != nth)
	{
		pos += 1;
		pos = str.find(findMe, pos);
		if (pos == std::string::npos)
			return -1;
		cnt++;
	}
	return pos;
}

int value_between_seperator(string& str, string seperator, int pos) {
	int temp;
	//Case 1 digit
	if (str.substr(pos + 2, 1).compare(seperator) == 0) {
		temp = std::stoi(str.substr(pos + 1, 1));
	}
	// Case 2 digits
	else if (str.substr(pos + 3, 1).compare(seperator) == 0) {
		temp = std::stoi(str.substr(pos + 1, 2));
	}
	//Case 3 digits
	else {
		temp = std::stoi(str.substr(pos + 1, 3));
	}
	return temp;
}

int main(int argc, char* argv[])
{
	FileHandler FileHandler;
	CloudCreator CloudCreator;
	Measure PerformanceTester;
	ParameterHandler PH;
	ParameterHandler* ParameterHandler = &PH;
	BaseMenu* CurrentMenu = new MainMenu(ParameterHandler);

	bool quit = false;
	bool execute = false;
	while (!quit && !execute)
	{
		CurrentMenu->clearScreen();
		CurrentMenu->printLogo();
		CurrentMenu->printFlavorText();
		CurrentMenu->printText();

		char choice = 'z';
		cin >> choice;

		BaseMenu* NewMenuPointer = CurrentMenu->getNextMenu(toupper(choice), quit, execute);

		if (NewMenuPointer)
		{
			//Delete previous menue if new window is no subwindow of it (prevent memory leaks)
			if (!NewMenuPointer->child) {
				delete CurrentMenu;
			}
			CurrentMenu = NewMenuPointer;
		}
	}

	if (ParameterHandler->get_exec_param_state("cloudcreation")) {
		//Paths used during cloudgen
		string depth_directory = "../../../../datasets/" + ParameterHandler->get_dataset() + "/raw_depth/" + ParameterHandler->get_object();
		string depth_extension = ".txt";
		string cloud_directory = "../../../../clouds/" + ParameterHandler->get_dataset() + "/" + ParameterHandler->get_object();
		vector<fs::path> depth_names;
		vector<fs::path> cloud_names;
		//Clouds used during cloud generation
		pcl::PointCloud<pcl::PointXYZ> query_cloud;
		pcl::PointCloud<pcl::PointXYZ> background_cloud;
		pcl::PointCloud<pcl::PointXYZ> filtered_query;
		pcl::PointCloud<pcl::PointXYZ> filtered_background;
		pcl::PointCloud<pcl::PointXYZ> final_cloud;

		FileHandler.get_all_file_names(depth_directory, depth_extension, depth_names);
		std::string bkgr_depth_filename = depth_directory + "/" + depth_names[0].string();
		std::vector<std::vector<float>> background_distance_array = FileHandler.melexis_txt_to_distance_array(bkgr_depth_filename, rows, columns);
		background_cloud = CloudCreator.distance_array_to_cloud(background_distance_array, 6.0f, 0.015f, 0.015f);
		if (ParameterHandler->get_filter_state("median")) {
			background_cloud = CloudCreator.median_filter(background_cloud, ParameterHandler->get_median_window_size());
		}
		//Pop first element (background without query)
		depth_names.erase(depth_names.begin());
		for (int i = 0; i < depth_names.size(); ++i) {
			vector<tuple<string, float>> creation_stats;
			std::string query_depth_filename = depth_directory + "/" + depth_names[i].string();
			PerformanceTester.time_meas();
			std::vector<std::vector<float>> query_distance_array = FileHandler.melexis_txt_to_distance_array(query_depth_filename, rows, columns);
			PerformanceTester.time_meas("time_textToDistance");
			PerformanceTester.time_meas();
			query_cloud = CloudCreator.distance_array_to_cloud(query_distance_array, 6.0f, 0.015f, 0.015f);
			PerformanceTester.time_meas("time_distanceToCloud");
			if (ParameterHandler->get_filter_state("median")) {
				PerformanceTester.time_meas();
				query_cloud = CloudCreator.median_filter(query_cloud, ParameterHandler->get_median_window_size());
				PerformanceTester.time_meas("time_medianfilter");
			}
			PerformanceTester.time_meas();
			final_cloud = CloudCreator.remove_background(query_cloud, background_cloud, 0.005f);
			PerformanceTester.time_meas("time_backgroundRemoval");
			if (ParameterHandler->get_filter_state("roi")) {
				PerformanceTester.time_meas();
				final_cloud = CloudCreator.roi_filter(final_cloud, "x", -(ParameterHandler->get_roi_x_limit()), ParameterHandler->get_roi_x_limit());
				final_cloud = CloudCreator.roi_filter(final_cloud, "y", -(ParameterHandler->get_roi_y_limit()), ParameterHandler->get_roi_y_limit());
				final_cloud = CloudCreator.roi_filter(final_cloud, "z", -(ParameterHandler->get_roi_z_limit()), ParameterHandler->get_roi_z_limit());
				PerformanceTester.time_meas("time_ROIfilter");
			}
			if (ParameterHandler->get_filter_state("sor")) {
				PerformanceTester.time_meas();
				final_cloud = CloudCreator.remove_outliers(final_cloud.makeShared(), ParameterHandler->get_sor_neighbor_count());
				PerformanceTester.time_meas("time_SORfilter");
			}
			string filename = cloud_directory + "/" + depth_names[i].string();
			string substr = filename.substr(0, (filename.length() - 4)) + ".ply";
			pcl::io::savePLYFileASCII(substr, final_cloud);
			if (ParameterHandler->get_cloudgen_stats_state()) {
				string statistics = PerformanceTester.get_stats();
				PerformanceTester.clear_stats();
				string stats_filename = filename.substr(0, (filename.length() - 4)) + "_stats.csv";
				FileHandler.writeToFile(statistics, stats_filename);
			}
		}
	}

	if (ParameterHandler->get_exec_param_state("merging")) {
		Normals QueryNormalEstimator;
		KeypointDetector QueryKeypointDetector;
		Descriptor QueryDescriber;
		std::vector<pcl::PointCloud<pcl::PointXYZ>::Ptr> clouds;
		std::vector<pcl::PointCloud<pcl::PointXYZ>::Ptr> transformed_clouds;
		std::vector<tuple<Eigen::Matrix4f, Eigen::Matrix4f>> transformation_matrices;
		pcl::PointCloud<pcl::PointXYZ>::Ptr origin_cloud(new pcl::PointCloud<pcl::PointXYZ>);

		//Load origin cloud
		FileHandler.get_all_file_names(origin_directory, ".ply", origin_names);
		string origin_filename = origin_directory + "/" + origin_names[0].string();
		FileHandler.load_ply_file(origin_filename, origin_cloud);
		float queryResolution = CloudCreator.compute_cloud_resolution(origin_cloud);
		Scene Query("query", origin_cloud);
		Query.resolution = queryResolution;
		//Estimate query normals
		QueryNormalEstimator.set_scalefactor(ParameterHandler->get_ne_scalefactor());
		QueryNormalEstimator.calculateNormals(Query.resolution, Query.cloud);
		QueryNormalEstimator.removeNaNNormals(Query.cloud);
		Query.normals = QueryNormalEstimator.normals;
		//Calculate query keypoints
		QueryKeypointDetector.set_threshold(0.7f);
		QueryKeypointDetector.set_neighbor_count(5);
		QueryKeypointDetector.calculateIssKeypoints(Query);
		Query.keypoints = QueryKeypointDetector.keypoints;
		//Calculate query descriptors
		QueryDescriber.calculateDescriptor(Query);
		Query.descriptors = QueryDescriber.descriptors;

		//Load all permutation clouds into vector
		FileHandler.get_all_file_names(permutation_directory, ".ply", permutation_names);
		for (int cloud = 0; cloud < permutation_names.size(); ++cloud) {
			pcl::PointCloud<pcl::PointXYZ>::Ptr permutation_cloud(new pcl::PointCloud<pcl::PointXYZ>);
			string cloud_filename = permutation_directory + "/" + permutation_names[cloud].string();
			FileHandler.load_ply_file(cloud_filename, permutation_cloud);
			clouds.push_back(permutation_cloud);
		}
		for (int cloud = 0; cloud < clouds.size(); ++cloud) {
			Normals NormalEstimator;
			KeypointDetector KeypointDetector;
			Descriptor Describer;
			Matching Matcher;

			float targetResolution = CloudCreator.compute_cloud_resolution(clouds[cloud]);
			Scene Target("target", clouds[cloud]);
			Target.resolution = targetResolution;
			//Estimate Normals
			NormalEstimator.set_scalefactor(ParameterHandler->get_ne_scalefactor());
			NormalEstimator.calculateNormals(targetResolution, Target.cloud);
			NormalEstimator.removeNaNNormals(Target.cloud);
			Target.normals = NormalEstimator.normals;
			//Detect keypoints
			KeypointDetector.set_threshold(0.7f);
			KeypointDetector.set_neighbor_count(5);
			KeypointDetector.calculateIssKeypoints(Target);
			Target.keypoints = KeypointDetector.keypoints;
			//Calculate descriptor for each keypoint
			Describer.calculateDescriptor(Target);
			Target.descriptors = Describer.descriptors;
			//Matching
			Matcher.queryDescriptor_ = Query.descriptors;
			Matcher.targetDescriptor_ = Target.descriptors;
			Matcher.calculateCorrespondences(ParameterHandler->get_matcher_distance_threshold());
			//Registration
			Registrator Registrator(Query, Target, Matcher.corresp);
			Registrator.set_distance_threshold(supportRadius_* Query.resolution / 2);
			Registrator.init_RANSAC();
			Eigen::Matrix4f ransac_transformation = Registrator.do_ransac();
			Eigen::Matrix4f icp_transformation = Registrator.do_icp();
			//Add inverse of transformation matrices to corresponding vector to enable target-to-source transformation
			transformation_matrices.push_back(make_tuple(ransac_transformation.inverse(), icp_transformation.inverse()));
		}
		if (ParameterHandler->get_visualization_state()) {
			for (int i = 0; i < clouds.size(); ++i) {
				pcl::PointCloud<pcl::PointXYZ>::Ptr transformed_cloud(new pcl::PointCloud<pcl::PointXYZ>);
				boost::shared_ptr<pcl::visualization::PCLVisualizer> viewer(new pcl::visualization::PCLVisualizer("3D Viewer"));
				viewer->setBackgroundColor(0, 0, 0);
				viewer->initCameraParameters();
				//Add origin points to visualizer
				pcl::visualization::PointCloudColorHandlerCustom<pcl::PointXYZ> blue(origin_cloud, 0, 0, 200);
				viewer->addPointCloud<pcl::PointXYZ>(origin_cloud, blue, "sample cloud1");
				viewer->setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_POINT_SIZE, 1, "sample cloud1");
				//add unaligned points to visualizer
				pcl::visualization::PointCloudColorHandlerCustom<pcl::PointXYZ> red(clouds[i], 200, 0, 0);
				viewer->addPointCloud<pcl::PointXYZ>(clouds[i], red, "sample cloud2");
				viewer->setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_POINT_SIZE, 1, "sample cloud2");
				//transform unaligned points and add to visualizer
				Eigen::Matrix4f rt = get<0>(transformation_matrices[i]);
				Eigen::Matrix4f it = get<1>(transformation_matrices[i]);
				pcl::transformPointCloud(*clouds[i], *transformed_cloud, it);
				pcl::transformPointCloud(*transformed_cloud, *transformed_cloud, rt);
				transformed_clouds.push_back(transformed_cloud);
				//Add aligned points to visualizer
				viewer->addPointCloud<pcl::PointXYZ>(transformed_cloud, "sample cloud3");
				viewer->setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_POINT_SIZE, 1, "sample cloud3");
				viewer->setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_COLOR, 0.0, 1.0, 0.0, "sample cloud3");
				while (!viewer->wasStopped())
				{
					viewer->spinOnce(100);
					std::this_thread::sleep_for(100ms);
				}
			}
		}
		pcl::PointCloud<pcl::PointXYZ>::Ptr master_cloud(new pcl::PointCloud<pcl::PointXYZ>);
		*master_cloud = *origin_cloud;
		for (int i = 0; i < transformed_clouds.size(); ++i) {
			*master_cloud += *transformed_clouds[i];
		}
		pcl::io::savePLYFileASCII("../../../../clouds/merging/merged/mastercloud.ply", *master_cloud);
		if (ParameterHandler->get_visualization_state()) {
			CloudCreator.show_cloud(master_cloud);
		}
	}

	if (ParameterHandler->get_exec_param_state("detection")) {
		pcl::PointCloud<pcl::PointXYZ>::Ptr query(new pcl::PointCloud<pcl::PointXYZ>);
		pcl::PointCloud<pcl::PointXYZ>::Ptr target(new pcl::PointCloud<pcl::PointXYZ>);
		FileHandler.get_all_file_names(query_directory, ".ply", query_names);
		FileHandler.get_all_file_names(target_directory, ".ply", target_names);

		for (int neighbor = 0; neighbor < ParameterHandler->sizeof_detector_nn(); ++neighbor)
		{
			for (int threshold = 0; threshold < ParameterHandler->sizeof_detector_threshold(); ++threshold)
			{
				for (int query_number = 0; query_number < query_names.size(); ++query_number)
				{
					Normals QueryNormalEstimator;
					KeypointDetector QueryKeypointDetector;
					Descriptor QueryDescriber;

					std::string query_filename = query_directory + "/" + query_names[query_number].string();
					int name_pos_query = query_filename.find((query_names[query_number].string()), 0);
					int ext_pos_query = query_filename.find(".ply", 0);
					std::string query_identifier = get_identifier(query_filename, name_pos_query, ext_pos_query);

					FileHandler.load_ply_file(query_filename, query);
					Scene Query(query_identifier, query);
					//Convert [mm] cloud to [m] cloud
					for (int p = 0; p < Query.cloud->points.size(); ++p) {
						Query.cloud->points[p].x = Query.cloud->points[p].x * 0.001;
						Query.cloud->points[p].y = Query.cloud->points[p].y * 0.001;
						Query.cloud->points[p].z = Query.cloud->points[p].z * 0.001;
					}
					Query.resolution = CloudCreator.compute_cloud_resolution(Query.cloud);
					//Query Normal Estimation
					QueryNormalEstimator.set_scalefactor(ParameterHandler->get_ne_scalefactor());
					QueryNormalEstimator.calculateNormals(Query.resolution, Query.cloud);
					QueryNormalEstimator.removeNaNNormals(Query.cloud);
					Query.normals = QueryNormalEstimator.normals;
					//Query Keypoint Detection
					QueryKeypointDetector.set_threshold(ParameterHandler->get_detector_threshold_at(threshold));
					QueryKeypointDetector.set_neighbor_count(ParameterHandler->get_detector_nn_at(neighbor));
					QueryKeypointDetector.calculateIssKeypoints(Query);
					Query.keypoints = QueryKeypointDetector.keypoints;
					//Query Description
					QueryDescriber.set_support_radius(supportRadius_);
					QueryDescriber.calculateDescriptor(Query);
					Query.descriptors = QueryDescriber.descriptors;
					if (ParameterHandler->get_fpfh_state()) {
						QueryDescriber.calculate_fpfh_descriptor(Query);
						Query.fpfh_descriptors = QueryDescriber.fpfh_descriptors;
					}
					for (int target_number = 0; target_number < target_names.size(); ++target_number)
					{
						Normals NormalEstimator;
						KeypointDetector KeypointDetector;
						Descriptor Describer;
						Matching Matcher;
						vector<tuple<string, float>> processing_times, stats;

						string target_filename = target_directory + "/" + target_names[target_number].string();
						int name_pos = target_filename.find((target_names[target_number].string()), 0);
						int extension_pos = target_filename.find(".ply", 0);
						string target_identifier = get_identifier(target_filename, name_pos, extension_pos);
						
						FileHandler.load_ply_file(target_filename, target);
						Scene Target(target_identifier, target);

						// Necessary if undistorted clouds are processed (since they are in mm but pcl works on m-basis)
						for (int p = 0; p < Target.cloud->points.size(); ++p) {
							Target.cloud->points[p].x = Target.cloud->points[p].x * 0.001;
							Target.cloud->points[p].y = Target.cloud->points[p].y * 0.001;
							Target.cloud->points[p].z = Target.cloud->points[p].z * 0.001;
						}
						Target.resolution = CloudCreator.compute_cloud_resolution(Target.cloud);
						//Target Normal Estimation
						PerformanceTester.time_meas();
						NormalEstimator.calculateNormals(Target.resolution, Target.cloud);
						NormalEstimator.removeNaNNormals(Target.cloud);
						Target.normals = NormalEstimator.normals;
						PerformanceTester.time_meas("normal estimation");
						//Target Keypoint Detection
						PerformanceTester.time_meas();
						KeypointDetector.set_threshold(ParameterHandler->get_detector_threshold_at(threshold));
						KeypointDetector.set_neighbor_count(ParameterHandler->get_detector_nn_at(neighbor));
						KeypointDetector.calculateIssKeypoints(Target);
						Target.keypoints = KeypointDetector.keypoints;
						PerformanceTester.time_meas("detecting keypoints");
						//Target Description
						PerformanceTester.time_meas();
						Describer.set_support_radius(supportRadius_);
						Describer.calculateDescriptor(Target);
						Target.descriptors = Describer.descriptors;
						PerformanceTester.time_meas("calculating descriptor");
						//Matching
						PerformanceTester.time_meas();
						Matcher.queryDescriptor_ = Query.descriptors;
						Matcher.targetDescriptor_ = Target.descriptors;
						Matcher.calculateCorrespondences(ParameterHandler->get_matcher_distance_threshold());
						PerformanceTester.time_meas("matching");
						// Registration
						Registrator Registrator(Query, Target, Matcher.corresp);
						Registrator.set_distance_threshold(supportRadius_ * Query.resolution / 2);
						Registrator.init_RANSAC();
						Registrator.do_ransac();
						Registrator.do_icp();
						Registrator.print_precision_recall();
						std::string result = Registrator.get_result();
						string pr_filename = pr_root
							+ "/" + ParameterHandler->get_dataset()
							+ "/" + ParameterHandler->get_object()
							+ "/" + ParameterHandler->get_preprocessor_mode()
							+ "/" + descriptor
							+ "/" + Query.identifier + "_to_" + Target.identifier + ".csv";
						FileHandler.writeToFile(result, pr_filename);
						//Output preparation based on CLI input
						if (ParameterHandler->get_pose_estimation_state()) {
							PoseEstimator PoseEstimator;
							std::string estimates;
							string filename;
							if (ParameterHandler->get_iss_frame_based_pose_estimation_state()) {
								//Pose Estimation based on ISS Frames
								pcl::Correspondences corresp = Registrator.get_RANSAC_correspondences();
								estimates = PoseEstimator.get_ISS_pose_estimation(Query, Target, corresp);
								filename = pose_estimation_root
									+ "/" + ParameterHandler->get_dataset()
									+ "/" + ParameterHandler->get_object()
									+ "/" + ParameterHandler->get_preprocessor_mode()
									+ "/" + descriptor
									+ "/" + Query.identifier + "_to_" + Target.identifier + "_GNR_angles.csv";
								FileHandler.writeToFile(estimates, filename);
							}
							if (ParameterHandler->get_transformation_matrices_based_pose_estimation_state()) {
								//Pose Estimation based on RANSAC and ICP transformation matrices
								Eigen::Matrix4f transformation = Registrator.get_transformation();
								estimates = PoseEstimator.get_RANSAC_pose_estimation(transformation);
								filename = pose_estimation_root
									+ "/" + ParameterHandler->get_dataset()
									+ "/" + ParameterHandler->get_object()
									+ "/" + ParameterHandler->get_preprocessor_mode()
									+ "/" + descriptor
									+ "/" + Query.identifier + "_to_" + Target.identifier + "_RANSAC_GNR_angles.csv";
								FileHandler.writeToFile(estimates, filename);
							}
						}
						if (ParameterHandler->get_fpfh_state()) {
							Describer.calculate_fpfh_descriptor(Target);
							Target.fpfh_descriptors = Describer.fpfh_descriptors;
							Matcher.fpfh_query_descriptors = Query.fpfh_descriptors;
							Matcher.fpfh_target_descriptors = Target.fpfh_descriptors;
							Matcher.calculate_fpfh_Correspondences(0.7f);
							Registrator.set_input_correspondences(Matcher.corresp);
							Registrator.set_distance_threshold(20 * Query.resolution / 2);
							Registrator.init_RANSAC();
							Registrator.do_ransac();
							Registrator.do_icp();
							Registrator.print_precision_recall();
							result = Registrator.get_result();
							pr_filename = pr_root
								+ "/" + ParameterHandler->get_dataset()
								+ "/" + ParameterHandler->get_object()
								+ "/" + ParameterHandler->get_preprocessor_mode()
								+ "/fpfh"
								+ "/" + Query.identifier + "_to_" + Target.identifier + ".csv";
							FileHandler.writeToFile(result, pr_filename);
						}
						if (ParameterHandler->get_match_retrieval_state()) {
							std::string matches;
							std::string match_log_filename = pr_root + "/"
								+ ParameterHandler->get_dataset() + "/"
								+ ParameterHandler->get_object() + "/"
								+ ParameterHandler->get_preprocessor_mode() + "/"
								+ descriptor + "/match_log.csv";
							if (ParameterHandler->get_ransac_state()) {
								matches = Query.identifier + "_to_" + Target.identifier + "," + std::to_string(Registrator.get_number_of_matches()) + "\n";
							}
							else {
								matches = Query.identifier + "_to_" + Target.identifier + "," + std::to_string(Matcher.corresp.size()) + "\n";
							}
							FileHandler.writeToFile(matches, match_log_filename);
						}
						if (ParameterHandler->get_detection_stats_state()) {
							string stats_filename = stats_root + "/" 
								+ ParameterHandler->get_dataset() + "/" 
								+ ParameterHandler->get_object() + "/" 
								+ ParameterHandler->get_preprocessor_mode() 
								+ "/" + descriptor
								+ "/" + Query.identifier + "_to_" + Target.identifier + "_stats.csv";
							std::string stats = PerformanceTester.get_stats();
							PerformanceTester.clear_stats();
							FileHandler.writeToFile(stats, stats_filename);
						}
						if (ParameterHandler->get_detection_logging_state()) {
							std::string log_filename = pr_root
								+ "/" + ParameterHandler->get_dataset()
								+ "/" + ParameterHandler->get_object()
								+ "/" + ParameterHandler->get_preprocessor_mode()
								+ "/" + descriptor + "/log.csv";
							std::string log = Query.identifier + "_to_" + Target.identifier + "," + std::to_string((Registrator.get_number_of_tp() >= ParameterHandler->get_detection_threshold())) + "\n";
							FileHandler.writeToFile(log, log_filename);
						}
						if (ParameterHandler->get_visualization_state()) {
							boost::shared_ptr<pcl::visualization::PCLVisualizer> viewer(new pcl::visualization::PCLVisualizer("3D Viewer"));
							viewer->setBackgroundColor(0.15, 0.15, 0.15);
							viewer->initCameraParameters();
							//Move query so that it is separated from the target to see correspondences
							Eigen::Matrix4f t;
							t << 1, 0, 0, Query.resolution * 80,
								0, 1, 0, 0,
								0, 0, 1, 0,
								0, 0, 0, 1;
							pcl::PointCloud<pcl::PointXYZ> visu_query;
							pcl::PointCloud<pcl::PointXYZ> visu_query_keypoints;
							if (Registrator.is_query_aligned()) {
								pcl::transformPointCloud(*Registrator.get_icp_aligned_keypoints(), visu_query_keypoints, t);
								pcl::transformPointCloud(*Registrator.get_icp_aligned_cloud(), visu_query, t);
							}
							else {
								pcl::transformPointCloud(Query.keypoints, visu_query_keypoints, t);
								pcl::transformPointCloud(*Query.cloud, visu_query, t);
							}
							pcl::Correspondences Correspondences = Registrator.get_RANSAC_correspondences();
							if (Correspondences.size() >= RANSAC_MIN_MATCHES) {
							}
							else {
								Correspondences = Matcher.corresp;
							}
							//Add query keypoints to visualizer
							pcl::visualization::PointCloudColorHandlerCustom<pcl::PointXYZ> red(Query.keypoints.makeShared(), 200, 0, 0);
							viewer->addPointCloud<pcl::PointXYZ>(visu_query_keypoints.makeShared(), red, "sample cloud1");
							viewer->setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_POINT_SIZE, 4, "sample cloud1");
							//add target keypoints to visualizer
							pcl::visualization::PointCloudColorHandlerCustom<pcl::PointXYZ> green(Target.keypoints.makeShared(), 0, 255, 0);
							viewer->addPointCloud<pcl::PointXYZ>(Target.keypoints.makeShared(), green, "sample cloud2");
							viewer->setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_POINT_SIZE, 4, "sample cloud2");
							//add lines between the correspondences
							viewer->addCorrespondences<pcl::PointXYZ>(visu_query_keypoints.makeShared(), Target.keypoints.makeShared(), Correspondences, "correspondences");
							//add query points to visualizer
							viewer->addPointCloud<pcl::PointXYZ>(visu_query.makeShared(), "sample cloud3");
							viewer->setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_POINT_SIZE, 1, "sample cloud3");
							viewer->setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_COLOR, 1.0, 0.0, 0.0, "sample cloud3");
							//add target points to visualizer
							viewer->addPointCloud<pcl::PointXYZ>(Target.cloud, "sample cloud4");
							viewer->setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_POINT_SIZE, 1, "sample cloud4");
							viewer->setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_COLOR, 0.0, 1.0, 0.0, "sample cloud4");
							while (!viewer->wasStopped())
							{
								viewer->spinOnce(100);
								std::this_thread::sleep_for(100ms);
							}
						}
					}
				}
			}
		}
	}

	if (ParameterHandler->get_exec_param_state("processing")) {
		FileHandler.get_all_file_names(processing_directory, ".ply", processing_names);
		if (ParameterHandler->get_background_removal_state()) {
			//If background has to be removed first file contains background information -> load it into cloud
			string background_filename = processing_directory + "/" + processing_names[0].string();
			pcl::PointCloud<pcl::PointXYZ>::Ptr background(new pcl::PointCloud<pcl::PointXYZ>);
			FileHandler.load_ply_file(background_filename, background);
			//Pop first element (background without query)
			processing_names.erase(processing_names.begin());
			//Remove background from all scenes
			for (int i = 0; i < processing_names.size(); ++i) {
				string filename = processing_directory + "/" + processing_names[i].string();
				pcl::PointCloud<pcl::PointXYZ>::Ptr scene(new pcl::PointCloud<pcl::PointXYZ>);
				FileHandler.load_ply_file(filename, scene);
				pcl::PointCloud<pcl::PointXYZ> processed_cloud = CloudCreator.remove_background(*scene, *background, ParameterHandler->get_background_removal_threshold());
				filename = processed_directory + "/" + processing_names[i].string();
				string substr = filename.substr(0, (filename.length() - 4)) + ".ply";
				pcl::io::savePLYFileASCII(substr, processed_cloud);
			}
		}
		if (ParameterHandler->get_filtering_state()) {
			for (int i = 0; i < processing_names.size(); ++i) {
				string filename = processing_directory + "/" + processing_names[i].string();
				pcl::PointCloud<pcl::PointXYZ>::Ptr scene(new pcl::PointCloud<pcl::PointXYZ>);
				FileHandler.load_ply_file(filename, scene);
				if (ParameterHandler->get_filter_state("median")) {
					vector<vector<int>> neighbor_indices;
					vector<vector<float>> neighbor_distances;
					CloudCreator.get_neighbors(scene, neighbor_indices, neighbor_distances);
					scene = CloudCreator.median_filter_unordered_cloud(scene, neighbor_indices, neighbor_distances, ParameterHandler->get_median_window_size());
				}
				if (ParameterHandler->get_filter_state("roi")) {
					*scene = CloudCreator.roi_filter(*scene, "x", -(ParameterHandler->get_roi_x_limit()), ParameterHandler->get_roi_x_limit());
					*scene = CloudCreator.roi_filter(*scene, "y", -(ParameterHandler->get_roi_y_limit()), ParameterHandler->get_roi_y_limit());
					*scene = CloudCreator.roi_filter(*scene, "z", -(ParameterHandler->get_roi_z_limit()), ParameterHandler->get_roi_z_limit());
				}
				if (ParameterHandler->get_filter_state("sor")) {
					*scene = CloudCreator.remove_outliers(scene, ParameterHandler->get_sor_neighbor_count());
				}
				filename = processed_directory + "/" + processing_names[i].string();
				string substr = filename.substr(0, (filename.length() - 4)) + ".ply";
				pcl::io::savePLYFileASCII(substr, *scene);
			}
		}
		if (ParameterHandler->get_downsample_state()) {
			pcl::PointCloud<pcl::PointXYZ> downsampled_cloud;
			for (int i = 0; i < processing_names.size(); ++i){
				string filename = processing_directory + "/" + processing_names[i].string();
				pcl::PointCloud<pcl::PointXYZ>::Ptr scene(new pcl::PointCloud<pcl::PointXYZ>);
				FileHandler.load_ply_file(filename, scene);
				downsampled_cloud = CloudCreator.downsample_cloud(scene, ParameterHandler->get_leaf_size());
				filename = processed_directory + "/" + processing_names[i].string();
				string substr = filename.substr(0, (filename.length() - 4)) + ".ply";
				pcl::io::savePLYFileASCII(substr, downsampled_cloud);
			}
		}
	}

	return 0;
}