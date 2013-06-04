/* 
   This file is part of the Calibu Project.
   https://robotics.gwu.edu/git/calibu
   
   Copyright (C) 2013 George Washington University,
                      Steven Lovegrove,
                      Gabe Sibley
                      
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
   
   http://www.apache.org/licenses/LICENSE-2.0
   
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#pragma once

#include <algorithm>
#include <vector>
#include <sophus/se3.hpp>
#include <calibu/cam/CameraModel.h>
#include <calibu/utils/Range.h>

namespace calibu
{
    ///////////////////////////////////////////////////////////////////////////////
    /// This structure is used to build a LUT without requiring branching
    //  when rectifying images. The values out of the image to the top left
    //  pixels instead of having a test for out of bound access, the aim is
    //  to avoid a branch in the code.
    //
    //    int xt = (int) x;  /* top-left corner */
    //    int yt = (int) y;
    //    float ax = x - xt;
    //    float ay = y - yt;
    //    float *ptr = image + (width*yt) + xt;
    //
    //    return( (1-ax) * (1-ay) * *ptr +
    //            ( ax ) * (1-ay) * *(ptr+1) +
    //            (1-ax) * ( ay ) * *(ptr+(width)) +
    //            ( ax ) * ( ay ) * *(ptr+(width)+1) );
    struct BilinearLutPoint
    {
        int idx0; // index to pixel in src image 
        int idx1; // index to pixel + one row in src image 
        float w00; // top left weight for bilinear interpolation
        float w01; // top right weight for bilinear interpolation
        float w10; // bottom left weight ...
        float w11; // bottom right weight ...
    };

    ///////////////////////////////////////////////////////////////////////////////
    struct LookupTable
    {
        LookupTable(){};
        LookupTable( int nWidth, int nHeight )
        {
            m_vLutPixels.resize( nWidth*nHeight );
            m_nWidth = nWidth;
        }

        LookupTable( const LookupTable& rhs )
        {
            m_vLutPixels.resize( rhs.m_vLutPixels.size() );
            m_nWidth = rhs.m_nWidth;
            m_vLutPixels = rhs.m_vLutPixels;
        }

        unsigned int Width()
        {
            return m_nWidth;
        }

        unsigned int Height()
        {
            return m_vLutPixels.size() / m_nWidth;
        }

        void SetPoint( unsigned int nRow, unsigned int nCol, BilinearLutPoint& p )
        {
            m_vLutPixels[ nRow*m_nWidth + nCol ] = p;
        }

        std::vector<BilinearLutPoint> m_vLutPixels;
        int m_nWidth; // so m_nHeight = m_vPixels.size()/m_nWidth
    };


    /*
    void CreateLookupTable(
            const calibu::CameraModelInterface& cam_from,
            const Eigen::Matrix3d R_onK,
            Eigen::Matrix<Eigen::Vector2f, Eigen::Dynamic, Eigen::Dynamic>& lookup_warp
            );
*/

    /// Create lookup table which can be used to remap a general camera model
    /// 'cam_from' to a linear and potentially rotated model, 'R_onK'.
    /// R_onK is formed from the multiplication R_on (old form new) and the new
    /// camera intrinsics K.
    void CreateLookupTable(
            const calibu::CameraModelInterface& cam_from,
            const Eigen::Matrix3d R_onKinv,
            LookupTable& lut  
            );

    ///
    inline Range MinMaxRotatedCol( const calibu::CameraModelInterface& cam, const Eigen::Matrix3d& Rnl_l )
    {
        Range range = Range::Open();
        for(size_t row = 0; row < cam.Height(); ++row) {    
            const Eigen::Vector2d ln = Project(Eigen::Vector3d(Rnl_l*Unproject(cam.Unmap(Eigen::Vector2d(0,row)))));
            const Eigen::Vector2d rn = Project(Eigen::Vector3d(Rnl_l*Unproject(cam.Unmap(Eigen::Vector2d(cam.Width()-1,row)))));
            range.ExcludeLessThan(ln[0]);
            range.ExcludeGreaterThan(rn[0]);
        }
        return range;
    }

    ///
    inline Range MinMaxRotatedRow( const calibu::CameraModelInterface& cam, const Eigen::Matrix3d& Rnl_l )
    {
        Range range = Range::Open();
        for(size_t col = 0; col < cam.Width(); ++col) {    
            const Eigen::Vector2d tn = Project(Eigen::Vector3d(Rnl_l*cam.UnmapUnproject(Eigen::Vector2d(col,0)) ));
            const Eigen::Vector2d bn = Project(Eigen::Vector3d(Rnl_l*cam.UnmapUnproject(Eigen::Vector2d(col,cam.Height()-1)) ));
            range.ExcludeLessThan(tn[1]);
            range.ExcludeGreaterThan(bn[1]);
        }
        return range;
    }

    void Rectify(
            LookupTable& lut,
            unsigned char* pInputImageData,
            unsigned char* pOutputRectImageData,
            int w, int h
            );
/*
    void Rectify(
            Eigen::Matrix<Eigen::Vector2f,Eigen::Dynamic,Eigen::Dynamic>& lookup_warp,
            unsigned char* pInputImageData,
            unsigned char* pOutputRectImageData,
            int w, int h
            );
 */
}


