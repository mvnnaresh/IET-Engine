#ifndef GJK_h__
#define GJK_h__

#include <glm/glm.hpp>
#include <vector>
#include "Vertex.h"
#include "ClosestPoint.h"

#define MAX_COUNT 30


using namespace std;

namespace Physics
{
	class GJK
	{
	private:
		vector<Vertex>&	d_vertices_shape_1;
		vector<Vertex>&	d_vertices_shape_2; 
		glm::vec3		d_initial_direction;
	public:
		GJK(vector<Vertex>&	vertices_shape_1,vector<Vertex>& vertices_shape_2,glm::vec3 centroid_shape1,glm::vec3 centroid_shape2);

		bool Intersect(Shader& shader);
	protected:
	private:
		glm::vec3	support(glm::vec3 direction, const vector<Vertex>& vertices);

		bool		do_simplex(vector<glm::vec3>& simplex, glm::vec3& direction);

		bool		is_same_direction(glm::vec3 a, glm::vec3 b);
	};



	glm::vec3 GJK::support(glm::vec3 direction, const vector<Vertex>& vertices)
	{
		size_t length = vertices.size();
		float max_dot_product = FLT_MIN;
		int   index_max_d_product= 0;
		float local_dot_product;
		for (int i = 0; i < length; i++)
		{
			local_dot_product = glm::dot(direction,vertices[i].Position);
			if (local_dot_product <= max_dot_product) continue;

			max_dot_product = local_dot_product;
			index_max_d_product = i;
		}

		return vertices[index_max_d_product].Position;

	}

	GJK::GJK(vector<Vertex>& vertices_shape_1,vector<Vertex>& vertices_shape_2,glm::vec3 centroid_shape1,glm::vec3 centroid_shape2)
		: 
		d_vertices_shape_1(vertices_shape_1),
		d_vertices_shape_2(vertices_shape_2)
	{	
		d_initial_direction = centroid_shape2 - centroid_shape1;
	}

	/*
	* For this GJK I take up to 4 points to initialize the simplex. 
	* I calculate one direction given the two centers of mass and then I pick one random direction to retrieve other two points
	* In this manner the simplex it is initialized with 4 points

	*/
	bool GJK::Intersect(Shader& shader)
	{
		vector<glm::vec3> simplex;
		bool intersect = false;
		d_initial_direction = glm::vec3(1.0f,0.0f,0.f);
		auto direction = glm::normalize(d_initial_direction);

		// Minkowski difference --> called support
		auto point1 = support(direction, d_vertices_shape_1);
		auto point2 = support(-direction, d_vertices_shape_2);

		auto point3 = point1 - point2;

		simplex.push_back(point3);

		auto D = direction;

		shader.Use();
		 
		{


			Point p(point3,glm::vec4(1.0f,0.0f,0.0f,1.0f));
			shader.SetUniform("shape_color",glm::vec3(1.0f,0.0f,0.0f));
			p.Draw();

			Point p1(point1,glm::vec4(1.0f,0.0f,0.0f,1.0f));
			Point p2(point2,glm::vec4(1.0f,0.0f,0.0f,1.0f)); 
			Point p3(glm::vec3(0.0f),glm::vec4(1.0f,0.0f,0.0f,1.0f)); 


			p1.Draw();
			p2.Draw();
			p3.Draw();
		}
		for (int counter = 0; counter < MAX_COUNT; counter++ )
		{
			if (do_simplex(simplex,D))
			{
				intersect = true;
				break;
			}

			auto newD = glm::normalize(D);
			auto dotD = glm::dot(newD,-D);

			auto A = support(newD, d_vertices_shape_1) - support(-newD, d_vertices_shape_2);
			auto AnewD= glm::dot(A,newD);


			if(AnewD <= dotD + 0.05f){
				intersect = false;
				break;
			}

			simplex.push_back(A);

			
		}

		for (int i = 0; i < simplex.size()-1; i++)
		{
			Line l(simplex[i],simplex[i+1]);
			l.Draw();
		}

		return intersect;
	}

	bool GJK::do_simplex(vector<glm::vec3>& simplex, glm::vec3& direction)
	{
		size_t s_size = simplex.size();

		switch (s_size)
		{
#pragma region 1-Simplex

		case 1:

			direction = -simplex[0];
			return false;

			break;
#pragma endregion 
#pragma region 2-Simplex 
		case 2:					// 1-Edge
			{
				auto A = simplex[1]; //A is the last inserted point. which is the direction we are looking at
				auto B = simplex[0]; 

				auto AO = -A;		//It is: O - A --> given the O is the origin thus 0,0,0. 
				auto AB = B - A;    //AB is the edge

				if (is_same_direction(AO,AB))
				{
					direction = glm::cross(glm::cross(AO,AB),AB); // This is to find the perpendicular to AB
					simplex.clear();
					simplex.push_back(B);
					simplex.push_back(A);

				}
				else
				{
					direction = AO;
					simplex.clear();
					simplex.push_back(A); // A is the closest to the origin.
				}

			}
			return false;		  // it's a edge so we return false as there is no intersection
			break;
#pragma endregion 
#pragma region 3-Simplex
		case 3:				      // 1-Triangle
			{
				auto A = simplex[simplex.size() - 1]; //A is still the last inserted point. which is the direction we are looking at
				auto B = simplex[simplex.size() - 2];
				auto C = simplex[simplex.size() - 3];

				auto AO = -A;
				auto AB = B - A;
				auto AC = C-A;

				auto AC_perp = glm::cross(glm::cross(AB,AC),AC);

				//Let's begin our tests
				if (is_same_direction(AC_perp,AO))
				{
					if (is_same_direction(AC,AO))
					{

						simplex.clear();
						simplex.push_back(C);
						simplex.push_back(A);
						direction = glm::cross(glm::cross(AC,AO),AC);
					}
					else
					{
						if (is_same_direction(AB,AO))
						{
							simplex.clear();
							simplex.push_back(B);
							simplex.push_back(A);
							direction = glm::cross(glm::cross(AB,AO),AB);
						}
						else
						{
							simplex.clear();
							simplex.push_back(A);
							direction = AO;
						}
					}
				}
				else
				{
					auto AB_perp = glm::cross(glm::cross(AB,AC),AB);
					if (is_same_direction(AB_perp,AO))
					{
						//this is the same block as above
						if (is_same_direction(AB,AO))
						{
							simplex.clear();
							simplex.push_back(B);
							simplex.push_back(A);
							direction = glm::cross(glm::cross(AB,AO),AB);
						}
						else
						{
							simplex.clear();
							simplex.push_back(A);
							direction = AO;
						}
					}
					else
					{
						auto ABC = glm::cross(AB,AC);
						simplex.clear();
						if (is_same_direction(ABC,AO))
						{
							simplex.push_back(C);
							simplex.push_back(B);
							simplex.push_back(A);
							direction = ABC;
						}
						else
						{
							simplex.push_back(B);
							simplex.push_back(C);
							simplex.push_back(A);
							direction = -ABC;
						}
					}
				}
			}
			return false;					//Still we don't have a tetrahedron
			break;
#pragma endregion  
#pragma region 4-Simplex
		case 4:
			{
				//Check the vertex first. Given we have arrived to the last point added is A it is the only one I check...hoping my assumption is correct
				auto A = simplex[simplex.size() - 1];
				auto B = simplex[simplex.size() - 2];
				auto C = simplex[simplex.size() - 3];
				auto D = simplex[simplex.size() - 4];

				auto AO = -A;

				auto p = ClosestPoint::Tetrahedron(glm::vec3(0.0f),A,B,C,D);

				if (p.point == glm::vec3(0.0f)) //The origin is inside the tetrahedron
					return true;

				simplex.clear();

				simplex.push_back(p.plane.c);
				simplex.push_back(p.plane.b);
				simplex.push_back(p.plane.a);

				D = p.plane.d;

				return false;
			}
			break;				// 1-Tetrahedron
#pragma endregion  
		}


	}

	bool GJK::is_same_direction(glm::vec3 a, glm::vec3 b)
	{
		float dot = glm::dot(a,b);
		return dot > 0.0f;
	}



}
#endif // GJK_h__
