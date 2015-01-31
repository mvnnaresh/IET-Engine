#ifndef RigidBody_h__
#define RigidBody_h__

#include "Model.h"
#include "Friction.h"
#include <glm/gtx/orthonormalize.hpp>

#define DUMPING_FACTOR 0.6f

namespace Physics
{
	using namespace Drag;

	class RigidBody
	{
	public:
		Model& m_model;


		RigidBody(Model&);
		~RigidBody();
	private: 

		float					d_area;
		float					d_calculated_area;

		float					d_mass;
		float					d_polyhedral_mass;

		glm::vec3				d_center_of_mass;
		glm::vec3				d_polyhedral_center_of_mass;

		glm::vec3				d_force;
		glm::vec3				d_velocity;
		glm::vec3				d_acceleration;
		glm::vec3				d_linear_momentum; 

		glm::vec3				d_angular_momentum;
		glm::vec3				d_torque;
		glm::vec3				d_angular_velocity;
		glm::vec3				d_force_application_point;

		glm::mat3				d_inertial_tensor;
		glm::mat3				d_inverse_inertial_tensor;
		glm::mat3				d_inverse_polyhedral_tensor;
		glm::mat3				d_polyhedral_tensor;


		BoundingBox				d_bounding_box;
		BoundingSphere*			d_bounding_sphere;

		glm::vec3				d_position;
	public:
		void					Update(float delta_time, bool use_polyhedral);
		void					Apply_Impulse(glm::vec3 force, glm::vec3 application_point, float delta_time);

		glm::vec3				Center_of_mass() const;

		float					Mass() const;
		float					Area() const;
		float					Polyhedral_Mass() const;

		Physics::BoundingSphere* Bounding_sphere() { return  d_bounding_sphere; } 

	private:
		glm::mat3				set_as_cross_product_matrix(glm::vec3 v);

		void					calculate_torque();
		void					calculate_mesh_stats(); 

	};

#pragma region [ Getters ]

		float		RigidBody::Mass()			 const   { return d_mass; } 
		float		RigidBody::Area()			 const	 { return d_area; }
		float		RigidBody::Polyhedral_Mass() const   { return d_polyhedral_mass; } 
		glm::vec3	RigidBody::Center_of_mass()  const	 { return d_center_of_mass; } 

#pragma endregion 

	RigidBody::RigidBody(Model& model) 
		: 
		m_model(model),
		d_area(0.0f),
		d_calculated_area(0.0f),
		d_mass(1.0f),
		d_polyhedral_mass(0.0f)
	{
		
		calculate_mesh_stats();
		d_center_of_mass += model.Position();
		d_bounding_sphere->center += model.Position();
	}


	void RigidBody::Update(float delta_time, bool use_polyhedral)
	{ 
		glm::quat orientation =  m_model.Rotation();
		glm::mat3 tensor = use_polyhedral ? d_inverse_polyhedral_tensor : d_inverse_inertial_tensor;
		float mass = use_polyhedral ? d_polyhedral_mass: d_mass;

		d_angular_velocity =  d_angular_momentum * tensor;

		orientation += delta_time *  orientation * glm::quat(set_as_cross_product_matrix(d_angular_velocity)) ;

		calculate_torque();

		float damping = glm::pow(DUMPING_FACTOR, delta_time);

		d_linear_momentum  *= damping ;
		d_angular_momentum *= damping ;

		orientation = glm::normalize(orientation);
		d_position =  d_linear_momentum * delta_time / mass;
		d_center_of_mass += d_position ;

		d_bounding_sphere->center += d_position;

		m_model.Rotate(orientation);
		m_model.Translate(d_position);

	}

	void RigidBody::Apply_Impulse(glm::vec3 force, glm::vec3 application_point, float delta_time)
	{
		d_force = force;
		//d_acceleration = force / d_mass;
		d_force_application_point = application_point ;
		calculate_torque();
		d_angular_momentum +=	d_torque * delta_time;
		d_linear_momentum +=	d_force  * delta_time;

	}

	glm::mat3 RigidBody::set_as_cross_product_matrix(glm::vec3 v )
	{
		glm::mat3 cross_product_matrix;

		cross_product_matrix[1][0] = -v.z;
		cross_product_matrix[2][0] =  v.y;
		cross_product_matrix[0][1] =  v.z;
		cross_product_matrix[2][1] = -v.x;
		cross_product_matrix[0][2] = -v.y;
		cross_product_matrix[1][2] =  v.x;

		return cross_product_matrix;		
	}

	void RigidBody::calculate_torque()
	{
		glm::vec3 point_world_space =  d_force_application_point + d_center_of_mass ;

		glm::vec3 point_com_distance =  point_world_space -  d_center_of_mass ;

		d_torque = glm::cross(point_com_distance, d_force);

	} 

	void  RigidBody::calculate_mesh_stats()
	{
		auto meshes = *this->m_model.Meshes();

		for (auto mesh : meshes)
		{
			d_area += mesh.Area();

			d_bounding_box += mesh.Bounding_box();
			d_bounding_sphere = new BoundingSphere(mesh.Bounding_sphere()); 
			d_center_of_mass += mesh.m_center_of_mass;
			d_polyhedral_center_of_mass += mesh.m_polyhedral_center_of_mass;

			Inertia::Compute_Tensor_With_AABB(d_bounding_box,d_mass,d_inertial_tensor);
			Inertia::Compute_Polyhedral_Tensor(mesh,d_polyhedral_mass,d_polyhedral_tensor);
		}

		d_center_of_mass /= meshes.size();
		d_polyhedral_center_of_mass /= meshes.size();

		d_inverse_inertial_tensor	= glm::inverse(d_inertial_tensor);
		d_inverse_polyhedral_tensor = glm::inverse(d_polyhedral_tensor);
	} 

	RigidBody::~RigidBody()
	{
		free(d_bounding_sphere);
	}

}
#endif // RigidBody_h__
