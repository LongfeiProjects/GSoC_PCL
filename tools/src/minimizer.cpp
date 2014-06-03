/**
 * @file minimizer.cpp
 * @brief Mock minimizer for fi =a0 +a1*xi^2 + a2*yi + a3*zi^3 
 */
#include "minimizer.h"
#include "SQ_sampler.h"
#include <iostream>
#include <time.h>
#include <stdlib.h>
#include <pcl/visualization/pcl_visualizer.h>


/**
 * @function minimizer
 * @brief Constructor
 */
minimizer::minimizer() :
    mSamples( new pcl::PointCloud<pcl::PointXYZ> ) {
    srand( time(NULL) );

    mLambda = 0.1;
    mMaxIter = 1000;
    mMinThresh = 0.005;

    // 11 Parameters: a,b,c, e1,e2, px,py,pz, ra,pa,ya
    mNumParams = 11;
    mParams.resize(mNumParams);
}

/**
 * @function minimizer
 * @brief Destructor
 */
minimizer::~minimizer() {

}

/**
 * @function loadPoints
 */
bool minimizer::loadPoints( std::string _pcdFilename ) {
    
    if( pcl::io::loadPCDFile<pcl::PointXYZ>( _pcdFilename.c_str(), *mSamples ) == -1 ) {
	std::cout <<"\t [ERROR] Could not read file " << std::endl;
	return false;
    }

    std::cout << "\t [GOOD] Loaded "<< mSamples->points.size() << " points" << std::endl;
    return true;
    
}

/**
 * @function loadPoints
 */
bool minimizer::loadPoints( const pcl::PointCloud<pcl::PointXYZ>::Ptr &_cloud ) {
    
    mSamples = _cloud;
    std::cout << "\t [GOOD] Loaded "<< mSamples->points.size() << " points" << std::endl;
    return true;    
}


/**
 * @function visualizePoints
 */
void minimizer::visualizePoints() {

    std::cout << "\t [INFO] Visualizing input cloud to minimizer"<< std::endl;
    boost::shared_ptr<pcl::visualization::PCLVisualizer> viewer (new pcl::visualization::PCLVisualizer ("3D Viewer"));
    viewer->setBackgroundColor (0, 0, 0);
    viewer->addCoordinateSystem (1.0, 0);
    viewer->initCameraParameters ();
    
    // 2. Visualize the clouds
    pcl::visualization::PointCloudColorHandlerCustom<pcl::PointXYZ> color( mSamples, 255, 0, 0 );
    viewer->addPointCloud<pcl::PointXYZ> ( mSamples, color, "cloud" );	
       
    // 3. Run loop
    while (!viewer->wasStopped ()) {
	viewer->spinOnce (100);
	boost::this_thread::sleep (boost::posix_time::microseconds (100000));
    }    

}

/**
 * @function minimize
 */
bool minimizer::minimize( const SQ_params &_par_in,
			  SQ_params &_par_out ) {
    
    Eigen::MatrixXd H;  Eigen::MatrixXd J;
    Eigen::MatrixXd dH = Eigen::MatrixXd::Identity( mNumParams, mNumParams );
    double error;

    // Initialize mA with some value
    int iter = 0;
    Eigen::VectorXd oldParams;
    
    mParams << params2Vec( _par_in );

    std::cout << "Initial guess for coefficients: "<< mParams.transpose() << std::endl;

    do {
	oldParams = mParams;
	H = ddf( mParams );
	J = df( mParams );

	for( int i = 0; i < mNumParams; ++i ) { dH(i,i) = H(i,i); }
	mParams = mParams - (H + mLambda*dH).inverse()*J;
	iter++;
	error = (mParams - oldParams).norm();

	std::cout << "Iter: "<< iter << " with error: "<< error << std::endl;

    } while( iter < mMaxIter && error > mMinThresh );
    
    vec2Param( mParams, _par_out );
    
    
    if( iter >= mMaxIter ) {
	std::cout << "[BAD] Crab, we did not converge after "<<iter<<" iterations"<< std::endl;
	std::cout << "Final coefficients: "<< mParams.transpose() << std::endl;
	return false;
    } else {
	std::cout << "[GOOD] Yes, we did converge in "<< iter << std::endl;
	std::cout << "Final coefficients: "<< mParams.transpose() << std::endl;
	std::cout << "J: \n"<< J << std::endl; 
	std::cout << "H: \n" << H << std::endl;
	return true;
    }
    

    return true;
}

/**
 * @function df
 * @brief Returns the derivative of f w.r.t. params
 */
Eigen::VectorXd minimizer::df( Eigen::VectorXd _params ) {
    
    Eigen::VectorXd df(mNumParams); df = Eigen::VectorXd::Zero(mNumParams);
    double x, y, z;
    
    pcl::PointCloud<pcl::PointXYZ>::iterator it;
    int i;
    double jac[mNumParams];

    for( int n = 0; n < mNumParams; ++n ) {
	df(n) = 0;	
    }

    for( it = mSamples->begin(), i = 0; 
	 it != mSamples->end(); 
	 ++it, ++i ) {

	double a1, a2, a3, e1, e2;
	double px, py, pz, ra, pa, ya;
	double x, y, z;
	a1 = _params(0); 
	a2 = _params(1);
	a3 = _params(2);
	e1 = _params(3);
	e2 = _params(4);
	px = _params(5);
	py = _params(6);
	pz = _params(7);
	ra = _params(8);
	pa = _params(9);
	ya = _params(10);

	x = it->x; y = it->y; z = it->z;


	jac_( &a1, &a2, &a3, &e1, &e2,
	      &px, &py, &pz, &ra, &pa, &ya,
	      &x,&y, &z, jac );

	for( int n = 0; n < mNumParams; ++n ) {
		if( jac[n] != jac[n] ) {
		    std::cout << "[Jacobian] NAN value in ("<<n<<")"<< std::endl;
		} else {
	    	df(n) += jac[n];	
		}
	}
    }
    
    return df;
    
}

/**
 * @function ddf
 * @brief Calculates the Hessian
 */
Eigen::MatrixXd minimizer::ddf( Eigen::VectorXd _params ) {

    Eigen::MatrixXd ddf = Eigen::MatrixXd::Zero(mNumParams,mNumParams);
    
    pcl::PointCloud<pcl::PointXYZ>::iterator it;
    int i;
    double hes[mNumParams*mNumParams];

    for( int m = 0; m < mNumParams; ++m ) {
	for( int n = 0; n < mNumParams; ++n ) {
	    ddf(m,n) = 0;
	}
    }

	double a1, a2, a3, e1, e2;
	double px, py, pz, ra, pa, ya;
	double x, y, z;
	a1 = _params(0); 
	a2 = _params(1);
	a3 = _params(2);
	e1 = _params(3);
	e2 = _params(4);
	px = _params(5);
	py = _params(6);
	pz = _params(7);
	ra = _params(8);
	pa = _params(9);
	ya = _params(10);

        
    for( it = mSamples->begin(), i = 0; 
	 it != mSamples->end(); 
	 ++it, ++i ) {

	x = it->x; y = it->y; z = it->z;
	hessian_( &a1, &a2, &a3, &e1, &e2,
		  &px, &py, &pz, &ra, &pa, &ya,
		  &x, &y, &z, hes );
	
	int ind = 0;
	for( int m = 0; m < mNumParams; ++m ) {
	    for( int n = 0; n < mNumParams; ++n ) {
		if( hes[ind] != hes[ind] ) {
		    std::cout << "[Hessian] NAN value in ("<< m<<", "<<n <<")"<< std::endl;
		} else {
		    ddf(m,n) += hes[ind];
		}
		ind++;
	    }
	}

    }


    return ddf;
}

/**
 * @function getRand
 */
double getRand( double _min,
		double _max ) {
    return _min + (_max - _min)*( (double) rand() / (double) RAND_MAX );   
}
