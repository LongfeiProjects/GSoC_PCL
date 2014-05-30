/**
 * @file SQ_fitter.h
 */
#pragma once

#include <pcl/pcl_base.h>

/**
 * @class SQ_fitter
 */
template <typename PointT>
class SQ_fitter {

 public:

    SQ_fitter();
    ~SQ_fitter();
    bool SQFitting( const pcl::PointCloud<PointT> &_cloud );
    
    bool initialization();

 private:


    bool fitting();

    double _smax; /**< Maximum voxel size */
    double _smin; /**< Minimum voxel size */
    int _N; /**< Number of scales */
    double _thresh; /**< Error threshold */

};


#include "impl/SQ_fitter.hpp"
