#ifndef Model_h__
#define Model_h__


// Std. Includes
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
#include "Skeleton.h"
using namespace std;
// GL Includes
#include <GL/glew.h> // Contains all the necessary OpenGL includes
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <Magick++.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <SOIL.h>
#include "Mesh.h"
#include "IAnimation.h"
#include "KeyFrameAnimator.h"

GLint TextureFromFile(const char* path, string directory);

class Model 
{
public:

	glm::mat4* mAnimationMatrix;
	Skeleton* mSkeleton;
	glm::vec3 m_Direction;
private:
	 Shader* shader;

	vector<Mesh> meshes;
	vector<Texture> textures_loaded;	// Stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.

	string directory;
	GLuint* boneLocation;
	int m_numberOfBone;
	glm::mat4 mModelMatrix;

	glm::mat4 m_Scale,m_Position,m_Rotation;
public:
	/*  Functions   */
	// Constructor, expects a filepath to a 3D model.
	Model(Shader* shader, GLchar* path) :
		shader(shader),
		m_numberOfBone(0)
	{
		assert(shader);
		assert(path);
	 
		mSkeleton = new Skeleton( ); 

		this->loadModel(path);

		//this->m_Direction = glm::vec3(1.0f,0.0f,1.0f);

		if (mSkeleton->getNumberOfBones()>0)
		{
			mAnimationMatrix = (glm::mat4*) malloc(mSkeleton->getNumberOfBones() * sizeof(glm::mat4));

			CleanAnimationMatrix();

			mSkeleton->updateSkeleton();
			mSkeleton->updateAnimationMatrix(mAnimationMatrix); 
		}


	} 

	~Model()
	{
		free(mAnimationMatrix);
		free(boneLocation);
		free(mSkeleton);

	}
	// Draws the model, and thus all its meshes
	void Draw()
	{
		this->shader->Use();

		this->mModelMatrix = GetModelMatrix();

		if (mSkeleton->getNumberOfBones()>0)
			glUniformMatrix4fv (boneLocation[0], mSkeleton->getNumberOfBones(), GL_FALSE, glm::value_ptr(mAnimationMatrix[0]));


		for(GLuint i = 0; i < this->meshes.size(); i++)
			this->meshes[i].Draw(*shader);
	}

	void Scale(glm::vec3 scale_vector)
	{
		this->m_Scale = glm::scale(this->m_Scale,scale_vector);
	}
	void Translate(glm::vec3 translation_vector)
	{
		this->m_Position = glm::translate(this->m_Position,translation_vector);
	}
	void Rotate(glm::vec3 rotation_vector, float radians)
	{
		this->m_Rotation = glm::rotate(glm::mat4(),radians,rotation_vector);
	}
	void Rotate(glm::quat rotation)
	{
		this->m_Rotation = glm::toMat4(rotation);
	}


	glm::vec3 GetPosition()
	{
		return decomposeT(m_Position);
	}

	glm::mat4 GetModelMatrix()
	{
		return m_Position * m_Rotation * m_Scale;
	}

	void Animate(IAnimation* animationInvoker, glm::vec3 target, string boneEffector, int numParent = 4)
	{
		assert(animationInvoker);

		Bone *effector = mSkeleton->GetBone(boneEffector.c_str());

		assert(effector);

		animationInvoker->Animate(this->mModelMatrix, effector ,target,numParent);

		mSkeleton->updateAnimationMatrix(mAnimationMatrix);
	}

	vector<glm::vec3> getBonesOrientation( )
	{
		return mSkeleton->getBonePositions(this->mModelMatrix);
	}

	glm::vec3 getBoneOrientation(const char* name)
	{
		return mSkeleton->GetBone(name)->getWorldSpacePosition(this->mModelMatrix);
	}

	void CleanAnimationMatrix()
	{
		//Initialize bones in the shader for the uniform 
		for (unsigned int i = 0 ; i < mSkeleton->getNumberOfBones(); i++) 
			mAnimationMatrix[i] = glm::mat4(1); 
	}

	void resetAnimation()
	{
		CleanAnimationMatrix();
	}

	void setJointLimit(string boneName, AngleRestriction angleRestriction )
	{ 
		mSkeleton->GetBone(boneName.c_str())->angleRestriction = angleRestriction;
	}

	 

	void ClearJointsLimit(){
		mSkeleton->ResetAllJointLimits();
	}
	

private:
	/*  Model Data  */
	
	/*  Functions   */
	// Loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
	void loadModel(string path)
	{
		// Read file via ASSIMP
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(path,aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs);
		// Check for errors
		if(!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
		{
			cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
			return;
		}

		// Retrieve the directory path of the filepath
		this->directory = path.substr(0, path.find_last_of('\\'));

		// Process ASSIMP's root node recursively
		this->processNode(scene->mRootNode, scene);

		// there should always be a 'root node', even if no skeleton exists
		if (!mSkeleton->importSkeletonBone ( scene->mRootNode)) {
			fprintf (stderr, "ERROR: Model %s - could not import node tree from mesh\n",path.c_str());
		} // endif 
		mSkeleton->inverseGlobal =  aiMatrix4x4ToGlm(&scene->mRootNode->mTransformation);
		int numOfBones = mSkeleton->getNumberOfBones();
		if (numOfBones > 0)
		{ 
			boneLocation = (GLuint*) malloc( mSkeleton->getNumberOfBones()* sizeof(GLuint));

			for (unsigned int i = 0 ; i < numOfBones; i++) {

				char Name[128];
				//IKMatrices[i] = glm::mat4(1); 

				memset(Name, 0, sizeof(Name));
				sprintf_s(Name, sizeof(Name), "bones[%d]", i);
				GLint location = glGetUniformLocation(shader->Program, Name);
				if (location == INVALID_UNIFORM_LOCATION) {
					fprintf(stderr, "Warning! Unable to get the location of uniform '%s'\n", Name);
				}

				boneLocation[i] = location;
			}
		}

	}

	// Processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
	void processNode(aiNode* node, const aiScene* scene)
	{ 

		// Process each mesh located at the current node
		for(GLuint i = 0; i < node->mNumMeshes; i++)
		{
			// The node object only contains indices to index the actual objects in the scene. 
			// The scene contains all the data, node is just to keep stuff organized (like relations between nodes).
			aiMesh* ai_mesh = scene->mMeshes[node->mMeshes[i]]; 
			Mesh mesh = this->processMesh(ai_mesh, scene );

			this->meshes.push_back(mesh);			
		}
		// After we've processed all of the meshes (if any) we then recursively process each of the children nodes
		for(GLuint i = 0; i < node->mNumChildren; i++)
		{
			this->processNode(node->mChildren[i], scene);
		}

	}


	Mesh processMesh(aiMesh* ai_mesh, const aiScene* scene )
	{
		// Data to fill
		vector<Vertex> vertices;
		vector<GLuint> indices;
		vector<Texture> textures;
		vector<VertexWeight> boneWeights;
		glm::uint numBones = 0; 

#pragma region [ Process Vertices ]
		// Walk through each of the mesh's vertices
		for(GLuint i = 0; i < ai_mesh->mNumVertices; i++)
		{
			Vertex vertex;
			glm::vec3 vector; // We declare a placeholder vector since Assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
			// Positions
			vector.x = ai_mesh->mVertices[i].x;
			vector.y = ai_mesh->mVertices[i].y;
			vector.z = ai_mesh->mVertices[i].z;
			vertex.Position = vector;
			// Normals
			if (ai_mesh->HasNormals())
			{
				vector.x = ai_mesh->mNormals[i].x;
				vector.y = ai_mesh->mNormals[i].y;
				vector.z = ai_mesh->mNormals[i].z;
				vertex.Normal = vector;
			}
			// Texture Coordinates
			if(ai_mesh->mTextureCoords[0]) // Does the mesh contain texture coordinates?
			{
				glm::vec2 vec;
				// A vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't 
				// use models where a vertex can have multiple texture coordinates so we always take the first set (0).
				vec.x = ai_mesh->mTextureCoords[0][i].x; 
				vec.y = ai_mesh->mTextureCoords[0][i].y;
				vertex.TexCoords = vec;
			}
			else
				vertex.TexCoords = glm::vec2(0.0f, 0.0f);
			vertices.push_back(vertex);
		}
#pragma endregion

#pragma region [ Process Faces ]

		// Now walk through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
		for(GLuint i = 0; i < ai_mesh->mNumFaces; i++)
		{
			aiFace face = ai_mesh->mFaces[i];
			// Retrieve all indices of the face and store them in the indices vector
			for(GLuint j = 0; j < face.mNumIndices; j++)
				indices.push_back(face.mIndices[j]);
		}
#pragma endregion

#pragma region [ Process Materials ]
		// Process materials
		if(ai_mesh->mMaterialIndex >= 0)
		{
			aiMaterial* material = scene->mMaterials[ai_mesh->mMaterialIndex];
			// We assume a convention for sampler names in the Shaders. Each diffuse texture should be named
			// as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER. 
			// Same applies to other texture as the following list summarizes:
			// Diffuse: texture_diffuseN
			// Specular: texture_specularN
			// Normal: texture_normalN

			// 1. Diffuse maps
			vector<Texture> diffuseMaps = this->loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
			textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
			// 2. Specular maps
			vector<Texture> specularMaps = this->loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
			textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
		}
#pragma endregion  

#pragma region [ Process Bones ]

		if(ai_mesh->HasBones())
		{
			int bone_count =  ai_mesh->mNumBones;

			boneWeights.resize(ai_mesh->mNumVertices);

			for (GLuint i = 0 ; i < ai_mesh->mNumBones ; i++) {                
				int BoneIndex = 0;        
				string BoneName(ai_mesh->mBones[i]->mName.data);

				if ( mSkeleton->boneMapping.find(BoneName) == mSkeleton->boneMapping.end()) {
					// Allocate an index for a new bone
					BoneInfo info;

					BoneIndex = m_numberOfBone++; 
					info.offset = aiMatrix4x4ToGlm(&ai_mesh->mBones[i]->mOffsetMatrix);
					info.index = BoneIndex;
					mSkeleton->boneMapping[BoneName] = info;
				}
				else
					BoneIndex = mSkeleton->boneMapping[BoneName].index;

				for (GLuint j = 0 ; j < ai_mesh->mBones[i]->mNumWeights ; j++) {
					int VertexID =   ai_mesh->mBones[i]->mWeights[j].mVertexId;
					float Weight  = ai_mesh->mBones[i]->mWeights[j].mWeight;   
					for (glm::uint i = 0 ; i < 4 ; i++) {
						if (boneWeights[VertexID].Weights[i] == 0.0) {
							boneWeights[VertexID].IDs[i]     = BoneIndex;
							boneWeights[VertexID].Weights[i] = Weight;
							break;
						}        
					}
				}
			}     
		}

#pragma endregion  

		// Return a mesh object created from the extracted mesh data
		return Mesh(vertices, indices, textures, boneWeights);
	}

	// Checks all material textures of a given type and loads the textures if they're not loaded yet.
	// The required info is returned as a Texture struct.
	vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName)
	{
		vector<Texture> textures;
		for(GLuint i = 0; i < mat->GetTextureCount(type); i++)
		{

			aiString str;
			mat->GetTexture(type, i, &str);
			// Check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
			GLboolean skip = false;
			for(GLuint j = 0; j < textures_loaded.size(); j++)
			{
				if(textures_loaded[j].path == str)
				{
					textures.push_back(textures_loaded[j]);
					skip = true; // A texture with the same filepath has already been loaded, continue to next one. (optimization)
					break;
				}
			}

			if(!skip)
			{   // If texture hasn't been loaded already, load it
				Texture texture;
				texture.id = TextureFromFile(str.C_Str(), this->directory);
				texture.type = typeName;
				texture.path = str;
				textures.push_back(texture);
				this->textures_loaded.push_back(texture);  // Store it as texture loaded for entire model, to ensure we won't unnecessary load duplicate textures.
			}
		}
		return textures;
	}

};


GLint TextureFromFile(const char* fileName, string directory)
{
	Magick::Blob m_blob;
	Magick::Image* m_pImage; 
	string stringFileName(fileName);
	string fullPath = directory + "\\" + stringFileName;
	try {
		m_pImage = new Magick::Image(fullPath.c_str());
		m_pImage->write(&m_blob, "RGB");
	}
	catch (Magick::Error& Error) {
		std::cout << "Error loading texture '" << fullPath << "': " << Error.what() << std::endl;
		return false;
	}

	//Generate texture ID and load texture data 
	//
	//filename = directory + '/' + filename;
	GLuint textureID;


	//SOIL_load_OGL_texture(filename.c_str(),SOIL_LOAD_AUTO,SOIL_CREATE_NEW_ID,SOIL_FLAG_MIPMAPS|SOIL_FLAG_INVERT_Y|SOIL_FLAG_NTSC_SAFE_RGB|SOIL_FLAG_COMPRESS_TO_DXT);;
	glGenTextures(1, &textureID);
	//int width,height,channels;
	//unsigned char* image = SOIL_load_image(filename.c_str(), &width, &height, nullptr, 0);
	// Assign texture to ID
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D,  0, GL_RGB, m_pImage->columns(), m_pImage->rows(), 0, GL_RGB, GL_UNSIGNED_BYTE, m_blob.data());

	// Parameters
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
	glGenerateMipmap(GL_TEXTURE_2D);	

	glBindTexture(GL_TEXTURE_2D, 0);
	//	SOIL_free_image_data(image);
	return textureID;
}
#endif // Model_h__