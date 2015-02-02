#ifndef BoundingSphere_h__
#define BoundingSphere_h__

#include "glm\glm.hpp"
#include <vector>
#include "Vertex.h"

namespace Physics
{
	using namespace std;

	struct BoundingSphere
	{
		 
		glm::vec3	center;
		glm::vec3	color;
		float		radius;

		BoundingSphere(const vector<Vertex>& vertices)
			:radius(0.0f)
		{
			calculate(vertices);
		}
		

		bool BoundingSphere::Overlaps(const BoundingSphere *other) const
		{
			float distanceSquared = glm::distance2(center,other->center);
			return distanceSquared < (radius+other->radius)*(radius+other->radius);
		}

		inline void Change_Color(glm::vec3 color);

	private:
		void calculate(const vector<Vertex>& vertices) 
		{

			glm::vec3  min_coordinate;
			glm::vec3  max_coordinate;

			for (auto point : vertices)
			{
				if(point.Position.x < min_coordinate.x) min_coordinate.x = point.Position.x;
				if(point.Position.x > max_coordinate.x) max_coordinate.x = point.Position.x;
				if(point.Position.y < min_coordinate.y) min_coordinate.y = point.Position.y;
				if(point.Position.y > max_coordinate.y) max_coordinate.y = point.Position.y;
				if(point.Position.z < min_coordinate.z) min_coordinate.z = point.Position.z;
				if(point.Position.z > max_coordinate.z) max_coordinate.z = point.Position.z;
			}								

			float distance = glm::distance(min_coordinate, max_coordinate); 

			radius = distance  * 0.5f;
			center = (min_coordinate + max_coordinate) * 0.5f;


		}
	};

	void BoundingSphere::Change_Color(glm::vec3 color)
	{
		this->color= color;
	}

}

#endif // BoundingSphere_h__