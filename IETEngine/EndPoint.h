#ifndef EndPoint_h__
#define EndPoint_h__

namespace Physics
{
	class BoundingBox;

	struct EndPoint 
	{
		float			m_min_point;
		float			m_max_point;

		BoundingBox*	m_bounding_box;

		EndPoint(){}

		 
		bool operator<(const EndPoint& other_endpoint) const;
	};

	#include "BoundingBox.h"
 

	bool EndPoint::operator<(const EndPoint& other_endpoint) const
	{
		return m_min_point < other_endpoint.m_min_point;
	}

}

#endif // EndPoint_h__
