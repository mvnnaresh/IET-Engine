#ifndef RigidBody_h__
#define RigidBody_h__

#include "Model.h"
#include "Friction.h"
#include <glm/gtx/orthonormalize.hpp>


namespace Physics
{
	using namespace Drag;

	class RigidBody
	{
	public:
		Model& d_model;


		RigidBody(Model&);

	private:
		float d_mass; 
		float d_polyhedral_mass;

		glm::vec3 d_center_of_mass;
		glm::vec3 d_force;
		glm::vec3 d_velocity;
		glm::vec3 d_acceleration;
		glm::vec3 d_linear_momentum; 
		glm::vec3 d_angular_momentum;
		glm::vec3 d_torque;
		glm::vec3 d_angular_velocity;
		glm::vec3 d_point;
		glm::mat3 d_inverse_inertial_tensor;
		glm::mat3 d_inverse_polyhedral_tensor;

		glm::vec3 d_position;
	public:
		void Update(float delta_time, bool use_polyhedral);

		void calculate_torque();

		void Apply_Impulse(glm::vec3 force, glm::vec3 application_point, float delta_time);

		glm::vec3 Center_of_mass() const;



	private:
		glm::mat3 set_as_cross_product_matrix(glm::vec3 v);
	};

	RigidBody::RigidBody(Model& model) : d_model(model)
	{
		d_mass = model.Mass();
		d_polyhedral_mass = model.PolyhedralMass();
		d_inverse_inertial_tensor = glm::inverse(model.InertialTensor());
		d_inverse_polyhedral_tensor = glm::inverse(model.PolyhedralTensor());
		d_center_of_mass = d_model.Center_of_mass();

	}

	glm::vec3  RigidBody::Center_of_mass() const { return d_center_of_mass; } 

	void RigidBody::Update(float delta_time, bool use_polyhedral)
	{ 
		glm::quat orientation =  d_model.Rotation();
		glm::mat3 tensor = use_polyhedral ? d_inverse_polyhedral_tensor : d_inverse_inertial_tensor;
		float mass = use_polyhedral ? d_polyhedral_mass: d_mass;


		d_angular_velocity =  d_angular_momentum * tensor;

		orientation += delta_time *  orientation * glm::quat(set_as_cross_product_matrix(d_angular_velocity)) ;

		calculate_torque();

		float damping = glm::pow(0.6f,delta_time);

		d_linear_momentum  *= damping ;
		d_angular_momentum *= damping ;

		orientation = glm::normalize(orientation);
		d_position =  d_linear_momentum * delta_time / mass;
		d_center_of_mass += d_position ;


		d_model.Rotate(orientation);
		d_model.Translate(d_position);

	}

	void RigidBody::Apply_Impulse(glm::vec3 force, glm::vec3 application_point, float delta_time)
	{
		d_force = force;
		//d_acceleration = force / d_mass;
		d_point = application_point ;
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
		glm::vec3 point_world_space =  d_point + d_center_of_mass ;

		glm::vec3 point_com_distance =  point_world_space -  d_center_of_mass ;

		d_torque = glm::cross(point_com_distance, d_force);

	}

}
#endif // RigidBody_h__