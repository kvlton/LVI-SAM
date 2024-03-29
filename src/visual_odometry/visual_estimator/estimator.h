#pragma once

#include "parameters.h"
#include "feature_manager.h"
#include "utility/utility.h"
#include "utility/tic_toc.h"
#include "initial/solve_5pts.h"
#include "initial/initial_sfm.h"
#include "initial/initial_alignment.h"
#include "initial/initial_ex_rotation.h"
#include <std_msgs/Header.h>
#include <std_msgs/Float32.h>

#include <ceres/ceres.h>
#include "factor/imu_factor.h"
#include "factor/pose_local_parameterization.h"
#include "factor/projection_factor.h"
#include "factor/projection_td_factor.h"
#include "factor/marginalization_factor.h"

#include <unordered_map>
#include <queue>
#include <opencv2/core/eigen.hpp>


class Estimator
{
  public:
    Estimator();

    void setParameter();

    // interface
    void processIMU(double t, const Vector3d &linear_acceleration, const Vector3d &angular_velocity);
    void processImage(const map<int, vector<pair<int, Eigen::Matrix<double, 8, 1>>>> &image, 
                      const vector<float> &lidar_initialization_info,
                      const std_msgs::Header &header);

    // internal
    void clearState();
    bool initialStructure();
    bool visualInitialAlign();
    // void visualInitialAlignWithDepth();
    bool relativePose(Matrix3d &relative_R, Vector3d &relative_T, int &l);
    void slideWindow();
    void solveOdometry();
    void slideWindowNew();
    void slideWindowOld();
    void optimization();
    void vector2double();
    void double2vector();
    bool failureDetection();


    enum SolverFlag
    {
        INITIAL,   // 初始化
        NON_LINEAR // 非线性优化
    };

    enum MarginalizationFlag
    {
        MARGIN_OLD = 0,       // 边缘化最老帧
        MARGIN_SECOND_NEW = 1 // 边缘化次新帧
    };

    SolverFlag solver_flag;
    MarginalizationFlag  marginalization_flag;
    Vector3d g;
    MatrixXd Ap[2], backup_A;
    VectorXd bp[2], backup_b;

    // camera to imu
    Matrix3d ric[NUM_OF_CAM]; // 单目的话数量为1
    Vector3d tic[NUM_OF_CAM];

    // 运动模型 PVQ
    Vector3d Ps[(WINDOW_SIZE + 1)];
    Vector3d Vs[(WINDOW_SIZE + 1)];
    Matrix3d Rs[(WINDOW_SIZE + 1)];
    Vector3d Bas[(WINDOW_SIZE + 1)]; // 加速度的bias
    Vector3d Bgs[(WINDOW_SIZE + 1)]; // 角速度的bias
    double td;

    // 滑动窗口
    Matrix3d back_R0, last_R, last_R0;
    Vector3d back_P0, last_P, last_P0;
    std_msgs::Header Headers[(WINDOW_SIZE + 1)]; // 关键帧的时间戳

    // 预积分
    IntegrationBase *pre_integrations[(WINDOW_SIZE + 1)]; // 两帧之间的预积分 (关键帧之间)
    Vector3d acc_0, gyr_0; // 上一时刻的加速度a和角速度w

    // imu数据
    vector<double> dt_buf[(WINDOW_SIZE + 1)];
    vector<Vector3d> linear_acceleration_buf[(WINDOW_SIZE + 1)];
    vector<Vector3d> angular_velocity_buf[(WINDOW_SIZE + 1)];

    int frame_count; // 滑窗内的关键帧个数
    int sum_of_outlier, sum_of_back, sum_of_front, sum_of_invalid;

    FeatureManager f_manager;
    MotionEstimator m_estimator;
    InitialEXRotation initial_ex_rotation; // 外参标定 (camera to imu 旋转)

    bool first_imu;
    bool is_valid, is_key;
    bool failure_occur;

    vector<Vector3d> point_cloud;
    vector<Vector3d> margin_cloud;
    vector<Vector3d> key_poses;
    double initial_timestamp;

    // ceres的参数
    double para_Pose[WINDOW_SIZE + 1][SIZE_POSE];           // P, Q组合成pose
    double para_SpeedBias[WINDOW_SIZE + 1][SIZE_SPEEDBIAS]; // V, Ba, Bg
    double para_Feature[NUM_OF_F][SIZE_FEATURE];            // 特征点的深度
    double para_Ex_Pose[NUM_OF_CAM][SIZE_POSE];             // camera to imu外参
    double para_Retrive_Pose[SIZE_POSE];
    double para_Td[1][1]; // 滑窗内第一个时刻相机到IMU时钟差 
    double para_Tr[1][1];

    int loop_window_index;

    MarginalizationInfo *last_marginalization_info;
    vector<double *> last_marginalization_parameter_blocks;

    map<double, ImageFrame> all_image_frame; // 所有普通帧 <时间戳, ImageFrame>
    IntegrationBase *tmp_pre_integration; // 两普通帧之间的预积分

    int failureCount;
};
