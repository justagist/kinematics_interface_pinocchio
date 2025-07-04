// Copyright (c) 2024, Saif Sidhik.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
/// \author: Saif Sidhik

#include "kinematics_interface/kinematics_interface.hpp"
#include "pluginlib/class_loader.hpp"
#include "rclcpp_lifecycle/lifecycle_node.hpp"
#include "ros2_control_test_assets/descriptions.hpp"
#include <gmock/gmock.h>
#include <memory>

class TestPinocchioPlugin : public ::testing::Test
{
public:
    std::shared_ptr<pluginlib::ClassLoader<kinematics_interface::KinematicsInterface>> ik_loader_;
    std::shared_ptr<kinematics_interface::KinematicsInterface> ik_;
    std::shared_ptr<rclcpp_lifecycle::LifecycleNode> node_;
    std::string end_effector_ = "link2";
    std::string urdf_ =
        std::string(ros2_control_test_assets::urdf_head) + std::string(ros2_control_test_assets::urdf_tail);

    void SetUp()
    {
        // init ros
        rclcpp::init(0, nullptr);
        node_ = std::make_shared<rclcpp_lifecycle::LifecycleNode>("test_node");
        std::string plugin_name = "kinematics_interface_pinocchio/KinematicsInterfacePinocchio";
        ik_loader_ = std::make_shared<pluginlib::ClassLoader<kinematics_interface::KinematicsInterface>>(
            "kinematics_interface", "kinematics_interface::KinematicsInterface"
        );
        ik_ = std::unique_ptr<kinematics_interface::KinematicsInterface>(ik_loader_->createUnmanagedInstance(plugin_name
        ));
    }

    void TearDown()
    {
        // shutdown ros
        rclcpp::shutdown();
    }

    void loadURDFParameter()
    {
        auto urdf = std::string(ros2_control_test_assets::urdf_head) + std::string(ros2_control_test_assets::urdf_tail);
        rclcpp::Parameter param("robot_description", urdf);
        node_->declare_parameter("robot_description", "");
        node_->set_parameter(param);
    }

    void loadAlphaParameter()
    {
        rclcpp::Parameter param("alpha", 0.005);
        node_->declare_parameter("alpha", 0.005);
        node_->set_parameter(param);
    }
};

TEST_F(TestPinocchioPlugin, Pinocchio_plugin_function)
{
    // load robot description and alpha to parameter server
    loadURDFParameter();
    loadAlphaParameter();

    // initialize the  plugin
    ASSERT_TRUE(ik_->initialize(urdf_, node_->get_node_parameters_interface(), ""));

    // calculate end effector transform
    Eigen::Matrix<double, Eigen::Dynamic, 1> pos = Eigen::Matrix<double, 3, 1>::Zero();
    Eigen::Isometry3d end_effector_transform;
    ASSERT_TRUE(ik_->calculate_link_transform(pos, end_effector_, end_effector_transform));

    // convert cartesian delta to joint delta
    Eigen::Matrix<double, 6, 1> delta_x = Eigen::Matrix<double, 6, 1>::Zero();
    delta_x[2] = 1;
    Eigen::Matrix<double, Eigen::Dynamic, 1> delta_theta = Eigen::Matrix<double, 3, 1>::Zero();
    ASSERT_TRUE(ik_->convert_cartesian_deltas_to_joint_deltas(pos, delta_x, end_effector_, delta_theta));

    // convert joint delta to cartesian delta
    Eigen::Matrix<double, 6, 1> delta_x_est;
    ASSERT_TRUE(ik_->convert_joint_deltas_to_cartesian_deltas(pos, delta_theta, end_effector_, delta_x_est));

    // Ensure kinematics math is correct
    for (size_t i = 0; i < static_cast<size_t>(delta_x.size()); ++i)
    {
        ASSERT_NEAR(delta_x[i], delta_x_est[i], 0.02);
    }

    // calculate jacobian
    Eigen::Matrix<double, 6, Eigen::Dynamic> jacobian = Eigen::Matrix<double, 6, 3>::Zero();
    ASSERT_TRUE(ik_->calculate_jacobian(pos, end_effector_, jacobian));

    // calculate jacobian inverse
    Eigen::Matrix<double, Eigen::Dynamic, 6> jacobian_inverse =
        jacobian.completeOrthogonalDecomposition().pseudoInverse();
    Eigen::Matrix<double, Eigen::Dynamic, 6> jacobian_inverse_est = Eigen::Matrix<double, 3, 6>::Zero();
    ASSERT_TRUE(ik_->calculate_jacobian_inverse(pos, end_effector_, jacobian_inverse_est));

    // ensure jacobian inverse math is correct
    for (Eigen::Index i = 0; i < jacobian_inverse.rows(); ++i)
    {
        for (Eigen::Index j = 0; j < jacobian_inverse.cols(); ++j)
        {
            ASSERT_NEAR(jacobian_inverse(i, j), jacobian_inverse_est(i, j), 0.02);
        }
    }
}

TEST_F(TestPinocchioPlugin, Pinocchio_plugin_function_std_vector)
{
    // load robot description and alpha to parameter server
    loadURDFParameter();
    loadAlphaParameter();

    // initialize the  plugin
    ASSERT_TRUE(ik_->initialize(urdf_, node_->get_node_parameters_interface(), ""));

    // calculate end effector transform
    std::vector<double> pos = {0, 0, 0};
    Eigen::Isometry3d end_effector_transform;
    ASSERT_TRUE(ik_->calculate_link_transform(pos, end_effector_, end_effector_transform));

    // convert cartesian delta to joint delta
    std::vector<double> delta_x = {0, 0, 0, 0, 0, 0};
    delta_x[2] = 1;
    std::vector<double> delta_theta = {0, 0, 0};
    ASSERT_TRUE(ik_->convert_cartesian_deltas_to_joint_deltas(pos, delta_x, end_effector_, delta_theta));

    // convert joint delta to cartesian delta
    std::vector<double> delta_x_est(6);
    ASSERT_TRUE(ik_->convert_joint_deltas_to_cartesian_deltas(pos, delta_theta, end_effector_, delta_x_est));

    // Ensure kinematics math is correct
    for (size_t i = 0; i < static_cast<size_t>(delta_x.size()); ++i)
    {
        ASSERT_NEAR(delta_x[i], delta_x_est[i], 0.02);
    }

    // calculate jacobian
    Eigen::Matrix<double, 6, Eigen::Dynamic> jacobian = Eigen::Matrix<double, 6, 3>::Zero();
    ASSERT_TRUE(ik_->calculate_jacobian(pos, end_effector_, jacobian));

    // calculate jacobian inverse
    Eigen::Matrix<double, Eigen::Dynamic, 6> jacobian_inverse =
        jacobian.completeOrthogonalDecomposition().pseudoInverse();
    Eigen::Matrix<double, Eigen::Dynamic, 6> jacobian_inverse_est = Eigen::Matrix<double, 3, 6>::Zero();
    ASSERT_TRUE(ik_->calculate_jacobian_inverse(pos, end_effector_, jacobian_inverse_est));

    // ensure jacobian inverse math is correct
    for (Eigen::Index i = 0; i < jacobian_inverse.rows(); ++i)
    {
        for (Eigen::Index j = 0; j < jacobian_inverse.cols(); ++j)
        {
            ASSERT_NEAR(jacobian_inverse(i, j), jacobian_inverse_est(i, j), 0.02);
        }
    }
}

TEST_F(TestPinocchioPlugin, incorrect_input_sizes)
{
    // load robot description and alpha to parameter server
    loadURDFParameter();
    loadAlphaParameter();

    // initialize the  plugin
    ASSERT_TRUE(ik_->initialize(urdf_, node_->get_node_parameters_interface(), ""));

    // define correct values
    Eigen::Matrix<double, Eigen::Dynamic, 1> pos = Eigen::Matrix<double, 2, 1>::Zero();
    Eigen::Isometry3d end_effector_transform;
    Eigen::Matrix<double, 6, 1> delta_x = Eigen::Matrix<double, 6, 1>::Zero();
    delta_x[2] = 1;
    Eigen::Matrix<double, Eigen::Dynamic, 1> delta_theta = Eigen::Matrix<double, 2, 1>::Zero();
    Eigen::Matrix<double, 6, 1> delta_x_est;
    Eigen::Matrix<double, Eigen::Dynamic, 6> jacobian = Eigen::Matrix<double, 2, 6>::Zero();

    // wrong size input vector
    Eigen::Matrix<double, Eigen::Dynamic, 1> vec_5 = Eigen::Matrix<double, 5, 1>::Zero();
    // wrong size input jacobian
    Eigen::Matrix<double, Eigen::Dynamic, 6> mat_5_6 = Eigen::Matrix<double, 5, 6>::Zero();

    // calculate transform
    ASSERT_FALSE(ik_->calculate_link_transform(vec_5, end_effector_, end_effector_transform));
    ASSERT_FALSE(ik_->calculate_link_transform(pos, "link_not_in_model", end_effector_transform));

    // convert cartesian delta to joint delta
    ASSERT_FALSE(ik_->convert_cartesian_deltas_to_joint_deltas(vec_5, delta_x, end_effector_, delta_theta));
    ASSERT_FALSE(ik_->convert_cartesian_deltas_to_joint_deltas(pos, delta_x, "link_not_in_model", delta_theta));
    ASSERT_FALSE(ik_->convert_cartesian_deltas_to_joint_deltas(pos, delta_x, end_effector_, vec_5));

    // convert joint delta to cartesian delta
    ASSERT_FALSE(ik_->convert_joint_deltas_to_cartesian_deltas(vec_5, delta_theta, end_effector_, delta_x_est));
    ASSERT_FALSE(ik_->convert_joint_deltas_to_cartesian_deltas(pos, vec_5, end_effector_, delta_x_est));
    ASSERT_FALSE(ik_->convert_joint_deltas_to_cartesian_deltas(pos, delta_theta, "link_not_in_model", delta_x_est));

    // calculate jacobian inverse
    ASSERT_FALSE(ik_->calculate_jacobian_inverse(vec_5, end_effector_, jacobian));
    ASSERT_FALSE(ik_->calculate_jacobian_inverse(pos, end_effector_, mat_5_6));
    ASSERT_FALSE(ik_->calculate_jacobian_inverse(pos, "link_not_in_model", jacobian));
}

TEST_F(TestPinocchioPlugin, Pinocchio_plugin_no_robot_description)
{
    // load alpha to parameter server
    loadAlphaParameter();
    ASSERT_FALSE(ik_->initialize("", node_->get_node_parameters_interface(), ""));
}
