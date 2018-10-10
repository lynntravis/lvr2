/* Copyright (C) 2011 Uni Osnabrück
 * This file is part of the LAS VEGAS Reconstruction Toolkit,
 *
 * LAS VEGAS is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * LAS VEGAS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 *
 * DatIO.cpp
 *
 *  Created on: Aug 8, 2013
 *      Author: Thomas Wiemann
 */

#include <lvr2/io/DatIO.hpp>
#include <lvr2/io/Timestamp.hpp>
#include <lvr2/io/Progress.hpp>

#include <boost/filesystem.hpp>

#include <iostream>
#include <fstream>
#include <vector>
using std::vector;
using std::ifstream;
using std::ofstream;
using std::cout;
using std::endl;

namespace lvr2
{

DatIO::DatIO()
{

}

DatIO::~DatIO()
{

}

ModelPtr DatIO::read(string filename)
{
	return read(filename, 4);
}

ModelPtr DatIO::read(string filename, int n, int reduction)
{
	ModelPtr model( new Model);
	PointBufferPtr pointBuffer(new PointBuffer);

	// Allocate point buffer and read data from file
	int c = 0;
	ifstream in(filename.c_str(), std::ios::binary);


	int numPoints = 0;

	in.read((char*)&numPoints, sizeof(int));

	// Determine modulo value suitable for desired reduction
	int mod_filter = 1;
	if(reduction != 0 && numPoints > reduction)
	{
		mod_filter = (int)(numPoints / reduction);
		cout << timestamp << "Reduction mode. Reading every " << mod_filter << "th point." << endl;
		numPoints = reduction;
	}
	else
	{
		reduction = numPoints; // needed for progress estimation
	}



	float* pointArray = new float[3 * numPoints];
	unsigned char* colorArray = new unsigned char[3 * numPoints];
    floatArr intensityArray;

    if (n > 3)
    {
        intensityArray = floatArr( new float[numPoints] );
    }

	float* buffer = new float[n-1];
	int    reflect;

	string msg = timestamp.getElapsedTime() + "Loading points";
	ProgressBar progress(numPoints, msg);
	int d = 0;
	while(in.good())
	{
		memset(buffer, 0, (n - 1) * sizeof(float));

		in.read((char*)buffer, (n - 1) * sizeof(float));

        // should only be read if n > 3...
        if (n > 3)
        {
            in.read((char*)&reflect, sizeof(int));
        }

		if(c % mod_filter == 0 && d < numPoints)
		{
			// Copy point data to point array
			int pos = 3 * d;
			pointArray[pos] 	= buffer[0];
			pointArray[pos + 1] = buffer[1];
			pointArray[pos + 2] = buffer[2];

			if(n > 3)
			{
                intensityArray[d]   = (float) reflect;

				colorArray[pos] 	= (unsigned char)reflect;
				colorArray[pos + 1] = (unsigned char)reflect;
				colorArray[pos + 2] = (unsigned char)reflect;
			}
			else
			{
				colorArray[pos] 	= (unsigned char)0;
				colorArray[pos + 1] = (unsigned char)0;
				colorArray[pos + 2] = (unsigned char)0;
			}
			d++;
			++progress;
		}
		c++;
	}
	delete[] buffer;
	in.close();

	// Truncate arrays to actual size
	pointArray = (float*)realloc(pointArray, 3 * d * sizeof(float));
	colorArray = (unsigned char*)realloc(colorArray, 3 * d * sizeof(unsigned char));

	cout << timestamp << "Creating point buffer with " << d << "points." << endl;

	// Setup model pointer
	floatArr parr(pointArray);
	ucharArr carr(colorArray);
	pointBuffer->setPointArray(parr, d);
	pointBuffer->setColorArray(carr, d);
    pointBuffer->addFloatChannel(intensityArray, "intensities", d, 1);

	model->m_pointCloud = pointBuffer;
	return model;

}

void DatIO::save(ModelPtr ptr, string filename)
{
	m_model = ptr;
	save(filename);
}

void  DatIO::save(string filename)
{
	PointBufferPtr pointBuffer = m_model->m_pointCloud;
	float buffer[4];
	if(pointBuffer)
	{
		ofstream out(filename.c_str(), std::ios::binary);
		if(out.good())
		{
			size_t numPoints = pointBuffer->numPoints();
			size_t numIntensities;
            unsigned w_intensities;
			floatArr pointArray = pointBuffer->getPointArray();
			floatArr intensityArray = pointBuffer->getFloatArray("intensities", numIntensities, w_intensities);
			float buffer[4];
			cout << timestamp << "Writing " << numPoints << " to " << filename << endl;
			for(size_t i = 0; i < numPoints; i++)
			{
				memset(buffer, 0, 4 * sizeof(float));
				size_t pos = i * 3;
				buffer[0] = pointArray[pos];
				buffer[1] = pointArray[pos + 1];
				buffer[2] = pointArray[pos + 2];
				if(intensityArray)
				{
					buffer[3] = intensityArray[i * w_intensities];
				}
				out.write((char*)buffer, 4 * sizeof(float));
			}
			out.close();
		}
		else
		{
			cout << timestamp << "DatIO: Unable to open file " << filename << " for writing." << endl;
		}
	}
}

} // namespace lvr2
