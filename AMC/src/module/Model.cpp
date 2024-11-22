#include <Model.h>
#include<TextureManager.h>
#include<assimp/Importer.hpp>
#include<assimp/scene.h>
#include<assimp/postprocess.h>

namespace AMC {

	glm::mat4 ConvertMatrix(const aiMatrix4x4* from) {
		glm::mat4 to;
		to[0][0] = from->a1; to[1][0] = from->a2; to[2][0] = from->a3; to[3][0] = from->a4;
		to[0][1] = from->b1; to[1][1] = from->b2; to[2][1] = from->b3; to[3][1] = from->b4;
		to[0][2] = from->c1; to[1][2] = from->c2; to[2][2] = from->c3; to[3][2] = from->c4;
		to[0][3] = from->d1; to[1][3] = from->d2; to[2][3] = from->d3; to[3][3] = from->d4;
		return to;
	}

	void LoadMaterials(const aiScene* scene, Model* model, std::string directory) {

		auto GetTexturePath = [](aiString name, std::string& directory) {
			std::string fileName = std::string(name.C_Str());
			fileName = directory + "\\" + fileName;
			return std::string(fileName.begin(), fileName.end());
		};

		for (UINT i = 0; i < scene->mNumMaterials; i++){

			aiMaterial* mat = scene->mMaterials[i];

			aiColor3D albedo;
			mat->Get(AI_MATKEY_COLOR_DIFFUSE, albedo);

			aiColor3D emission;
			mat->Get(AI_MATKEY_COLOR_EMISSIVE, emission);

			FLOAT emissiveIntensity;
			mat->Get(AI_MATKEY_EMISSIVE_INTENSITY, emissiveIntensity);

			FLOAT metallic;
			mat->Get(AI_MATKEY_METALLIC_FACTOR, metallic);

			FLOAT roughness;
			mat->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness);

			FLOAT alpha;
			mat->Get(AI_MATKEY_OPACITY, alpha);

			Material * material = new Material();

			// Loading Texture Manually For Now
			aiString name;
			UINT textureFlag = 0;
			// Diffuse Map
			if (mat->GetTextureCount(aiTextureType_DIFFUSE) > 0)
			{
				mat->GetTexture(aiTextureType_DIFFUSE, 0, &name);
				//check if embedded texture otherwise load from file
				const aiTexture* embeddedTex = scene->GetEmbeddedTexture(name.C_Str());
				if (embeddedTex != nullptr) {
					material->LoadMaterialTexturesFromMemory(embeddedTex, TextureType::TextureTypeDiffuse);
				}
				else {
					material->LoadMaterialTexturesFromFile(GetTexturePath(name, directory), TextureType::TextureTypeDiffuse);
				}
				textureFlag |= (1 << 0);
			}

			// Normal Map
			if (mat->GetTextureCount(aiTextureType_NORMALS) > 0)
			{
				mat->GetTexture(aiTextureType_NORMALS, 0, &name);
				const aiTexture* embeddedTex = scene->GetEmbeddedTexture(name.C_Str());
				if (embeddedTex != nullptr) {
					material->LoadMaterialTexturesFromMemory(embeddedTex, TextureType::TextureTypeNormalMap);
				}
				else {
					material->LoadMaterialTexturesFromFile(GetTexturePath(name, directory), TextureType::TextureTypeNormalMap);
				}
				textureFlag |= (1 << 1);
			}

			// Metallic Rougness Map Assuming That Metal and Roughness Maps are stored in same texture ...
			if (mat->GetTextureCount(aiTextureType_METALNESS) > 0)
			{
				mat->GetTexture(aiTextureType_METALNESS, 0, &name);
				const aiTexture* embeddedTex = scene->GetEmbeddedTexture(name.C_Str());
				if (embeddedTex != nullptr) {
					material->LoadMaterialTexturesFromMemory(embeddedTex, TextureType::TextureTypeMetallicRoughnessMap);
				}
				else {
					material->LoadMaterialTexturesFromFile(GetTexturePath(name, directory), TextureType::TextureTypeMetallicRoughnessMap);
				}
				textureFlag |= (1 << 2);
			}

			// Emission Map
			if (mat->GetTextureCount(aiTextureType_EMISSIVE) > 0)
			{
				mat->GetTexture(aiTextureType_EMISSIVE, 0, &name);
				const aiTexture* embeddedTex = scene->GetEmbeddedTexture(name.C_Str());
				if (embeddedTex != nullptr) {
					material->LoadMaterialTexturesFromMemory(embeddedTex, TextureType::TextureTypeEmissive);
				}
				else {
					material->LoadMaterialTexturesFromFile(GetTexturePath(name, directory), TextureType::TextureTypeEmissive);
				}
				textureFlag |= (1 << 3);
			}

			if (mat->GetTextureCount(aiTextureType_LIGHTMAP) > 0)
			{
				mat->GetTexture(aiTextureType_LIGHTMAP, 0, &name);
				const aiTexture* embeddedTex = scene->GetEmbeddedTexture(name.C_Str());
				if (embeddedTex != nullptr) {
					material->LoadMaterialTexturesFromMemory(embeddedTex, TextureType::TextureTypeAmbient);
				}
				else {
					material->LoadMaterialTexturesFromFile(GetTexturePath(name, directory), TextureType::TextureTypeAmbient);
				}
				textureFlag |= (1 << 4);
			}

			material->albedo = glm::vec3(albedo.r, albedo.g, albedo.b);
			material->metallic = metallic;
			material->roughness = roughness;
			material->emission = glm::vec3(emission.r, emission.g, emission.b);
			material->alpha = alpha;
			material->textureFlag = textureFlag;
			model->materials.push_back(material);
		}
	}

	void LoadMeshes(const aiScene* scene, Model* model, std::string directory) {

		if (!model || !scene)
			return;

		std::vector<AABB> aabbs;
		for (UINT i = 0; i < scene->mNumMeshes; i++) {

			aiMesh* mesh = scene->mMeshes[i];
			Mesh* m = new Mesh();
			AABB meshAABB;
			GLuint VAO, VBO[7], IBO;
			std::vector<GLuint> vertexBuffers;
			std::vector<GLintptr> offsets;
			std::vector<GLsizei> strides;
			GLuint bindingIndex = 0;

			glCreateVertexArrays(1, &VAO);
			glCreateBuffers(7, VBO);// Position, Normal, Texcoord, Tangent, Bitangent, BondId, Weight
			glNamedBufferData(VBO[0], mesh->mNumVertices * sizeof(aiVector3D), mesh->mVertices, GL_STATIC_DRAW);
			vertexBuffers.push_back(VBO[0]);
			offsets.push_back(0);
			strides.push_back(sizeof(aiVector3D));
			glEnableVertexArrayAttrib(VAO, 0);
			glVertexArrayAttribFormat(VAO, 0, 3, GL_FLOAT, GL_FALSE, 0);
			glVertexArrayAttribBinding(VAO, 0, bindingIndex);
			bindingIndex++;

			if (mesh->HasNormals()) {

				glNamedBufferData(VBO[1],mesh->mNumVertices * sizeof(aiVector3d), mesh->mNormals, GL_STATIC_DRAW);
				vertexBuffers.push_back(VBO[1]);
				offsets.push_back(0);
				strides.push_back(sizeof(aiVector3D));
				glEnableVertexArrayAttrib(VAO, 1);
				glVertexArrayAttribFormat(VAO, 1, 3, GL_FLOAT, GL_FALSE, 0);
				glVertexArrayAttribBinding(VAO, 1, bindingIndex);
				bindingIndex++;
			}
			else {
				glDisableVertexArrayAttrib(VAO, 1);
			}

			if (mesh->HasTextureCoords(0)) {
				std::vector<aiVector2D> texCoords(mesh->mNumVertices);
				for (UINT i = 0; i < mesh->mNumVertices; i++) {
					texCoords[i] = aiVector2D(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
				}
				glNamedBufferData(VBO[2], mesh->mNumVertices * sizeof(aiVector2D), texCoords.data(), GL_STATIC_DRAW);
				vertexBuffers.push_back(VBO[2]);
				offsets.push_back(0);
				strides.push_back(sizeof(aiVector2D));
				glEnableVertexArrayAttrib(VAO, 2);
				glVertexArrayAttribFormat(VAO, 2, 2, GL_FLOAT, GL_FALSE, 0);
				glVertexArrayAttribBinding(VAO, 2, bindingIndex);
				bindingIndex++;
			}
			else {
				glDisableVertexArrayAttrib(VAO, 2);
			}

			if (mesh->HasTangentsAndBitangents()) {
				glNamedBufferData(VBO[3], mesh->mNumVertices * sizeof(aiVector3D), mesh->mTangents, GL_STATIC_DRAW);
				glNamedBufferData(VBO[4], mesh->mNumVertices * sizeof(aiVector3D), mesh->mBitangents, GL_STATIC_DRAW);
				vertexBuffers.push_back(VBO[3]);
				offsets.push_back(0);
				strides.push_back(sizeof(aiVector3D));
				glEnableVertexArrayAttrib(VAO, 3);
				glVertexArrayAttribFormat(VAO, 3, 3, GL_FLOAT, GL_FALSE, 0);
				glVertexArrayAttribBinding(VAO, 3, bindingIndex);
				bindingIndex++;
				vertexBuffers.push_back(VBO[4]);
				offsets.push_back(0);
				strides.push_back(sizeof(aiVector3D));
				glEnableVertexArrayAttrib(VAO, 4);
				glVertexArrayAttribFormat(VAO, 4, 3, GL_FLOAT, GL_FALSE, 0);
				glVertexArrayAttribBinding(VAO, 4, bindingIndex);
				bindingIndex++;
			}
			else {
				glDisableVertexArrayAttrib(VAO, 3);
				glDisableVertexArrayAttrib(VAO, 4);
			}

			if (mesh->HasBones()) {

				std::vector<int> boneIDs(mesh->mNumVertices * 4, 0); // Assuming max 4 bones per vertex
				std::vector<float> weights(mesh->mNumVertices * 4, 0.0f);
				
				for (unsigned int i = 0; i < mesh->mNumBones; ++i) {
					aiBone* bone = mesh->mBones[i];
					std::string boneName(bone->mName.C_Str());

					int boneIndex = 0;
					if (model->BoneInfoMap.find(boneName) == model->BoneInfoMap.end()) {
						BoneInfo newBoneInfo;
						newBoneInfo.id = model->BoneCounter;
						newBoneInfo.offset = ConvertMatrix(&bone->mOffsetMatrix);
						model->BoneInfoMap[boneName] = newBoneInfo;
						boneIndex = model->BoneCounter;
						model->BoneCounter++;
					}
					else {
						boneIndex = model->BoneInfoMap[boneName].id;
					}

					// Assign weights to vertices
					for (unsigned int j = 0; j < bone->mNumWeights; ++j) {
						unsigned int vertexID = bone->mWeights[j].mVertexId;
						float weight = bone->mWeights[j].mWeight;
						for (int k = 0; k < MAX_BONE_INFLUENCE; ++k) {
							if (weights[vertexID * MAX_BONE_INFLUENCE + k] == 0.0f) {
								boneIDs[vertexID * MAX_BONE_INFLUENCE + k] = boneIndex;
								weights[vertexID * MAX_BONE_INFLUENCE + k] = weight;
								break;
							}
						}
					}
				}
				// Upload Bone IDs and Weights
				glNamedBufferData(VBO[5], boneIDs.size() * sizeof(int), boneIDs.data(), GL_STATIC_DRAW);
				glNamedBufferData(VBO[6], weights.size() * sizeof(float), weights.data(), GL_STATIC_DRAW);

				vertexBuffers.push_back(VBO[5]);
				offsets.push_back(0);
				strides.push_back(sizeof(int) * 4);
				glEnableVertexArrayAttrib(VAO, 5);
				glVertexArrayAttribIFormat(VAO, 5, 4, GL_INT, 0);
				glVertexArrayAttribBinding(VAO, 5, bindingIndex);
				bindingIndex++;

				vertexBuffers.push_back(VBO[6]);
				offsets.push_back(0);
				strides.push_back(sizeof(float) * 4);
				glEnableVertexArrayAttrib(VAO, 6);
				glVertexArrayAttribFormat(VAO, 6, 4, GL_FLOAT, GL_FALSE, 0);
				glVertexArrayAttribBinding(VAO, 6, bindingIndex);
				bindingIndex++;
			}
			else {
				glDisableVertexArrayAttrib(VAO, 5);
				glDisableVertexArrayAttrib(VAO, 6);
			}

			std::vector<UINT> indices;
			for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
				aiFace face = mesh->mFaces[i];
				for (unsigned int j = 0; j < face.mNumIndices; ++j) {
					indices.push_back(face.mIndices[j]);
				}
			}

			glCreateBuffers(1, &IBO);
			glNamedBufferData(IBO, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
			glVertexArrayElementBuffer(VAO, IBO);

			glVertexArrayVertexBuffers(VAO, 0, bindingIndex, vertexBuffers.data(), offsets.data(), strides.data());

			// Process material
			UINT mIndex = 0;
			if (mesh->mMaterialIndex >= 0) {
				mIndex = mesh->mMaterialIndex;
			}

			m->mTriangleCount = (UINT)indices.size();
			m->mMaterial = mIndex;
			m->vao = VAO;
			model->meshes.push_back(m);

			// AABB
			meshAABB.mMin = glm::vec3(mesh->mAABB.mMin.x, mesh->mAABB.mMin.y, mesh->mAABB.mMin.z);
			meshAABB.mMax = glm::vec3(mesh->mAABB.mMax.x, mesh->mAABB.mMax.y, mesh->mAABB.mMax.z);
			aabbs.push_back(meshAABB);
		}
		
		AABB modelAABB;
		modelAABB.mMin = glm::vec3(FLT_MAX, FLT_MAX, FLT_MAX);
		modelAABB.mMax = glm::vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
		for (auto& meshAABB : aabbs) {
			modelAABB.mMin.x = std::min(modelAABB.mMin.x, meshAABB.mMin.x);
			modelAABB.mMin.y = std::min(modelAABB.mMin.y, meshAABB.mMin.y);
			modelAABB.mMin.z = std::min(modelAABB.mMin.z, meshAABB.mMin.z);

			modelAABB.mMax.x = std::max(modelAABB.mMax.x, meshAABB.mMax.x);
			modelAABB.mMax.y = std::max(modelAABB.mMax.y, meshAABB.mMax.y);
			modelAABB.mMax.z = std::max(modelAABB.mMax.z, meshAABB.mMax.z);
		}

		model->aabb = modelAABB;
		aabbs.clear();
	}

	void readNodeHierarchy(NodeData& dest, const aiNode* src) {

		dest.name = src->mName.data;
		dest.transformation = ConvertMatrix(&src->mTransformation);
		dest.globalTransform = glm::mat4(1.0f);

		for (UINT i = 0; i < src->mNumMeshes; i++) {
			dest.meshIndices.push_back(src->mMeshes[i]);
		}

		for (UINT i = 0; i < src->mNumChildren; i++) {
			NodeData childNode;
			readNodeHierarchy(childNode, src->mChildren[i]);
			dest.children.push_back(childNode);
		}
	}

	void readHeirarchyData(AssimpNodeData& dest, const aiNode* src) {
		dest.name = src->mName.data;
		dest.transformation = ConvertMatrix(&src->mTransformation);
		dest.childCount = src->mNumChildren;

		for (UINT i = 0; i < src->mNumChildren; i++) {
			AssimpNodeData newData;
			readHeirarchyData(newData, src->mChildren[i]);
			dest.children.push_back(newData);
		}
	}

	void LoadSkeletalAnimation(const aiScene* scene,const aiAnimation* animation, Model* model) {

		if (!model || !animation || !scene)
			return;

		SkeletonAnimator animator;
		animator.currentTime = 0.0f;

		for (UINT m = 0; m < MAX_BONE_COUNT; m++)
			animator.finalBoneMatrices.push_back(glm::mat4(1.0f));
		animator.duration = (FLOAT)animation->mDuration;
		animator.ticksPerSecond = (INT)animation->mTicksPerSecond;
		readHeirarchyData(animator.rootNode, scene->mRootNode);

		for (UINT i = 0; i < animation->mNumChannels; i++){
			aiNodeAnim* channel = animation->mChannels[i];
			std::string boneName = channel->mNodeName.C_Str();

			if (model->BoneInfoMap.count(boneName) == 0) {
				model->BoneInfoMap[boneName].id = model->BoneCounter;
				model->BoneCounter++;
			}
			Bone b;
			b.name = channel->mNodeName.C_Str();
			b.id = model->BoneInfoMap[channel->mNodeName.data].id;
			b.localTransform = glm::mat4(1.0f);
			for (UINT positionIndex = 0; positionIndex < channel->mNumPositionKeys; ++positionIndex)
			{
				aiVector3D aiPosition = channel->mPositionKeys[positionIndex].mValue;
				float timeStamp = (float)channel->mPositionKeys[positionIndex].mTime;
				KeyPosition data;
				data.position = glm::vec3(aiPosition.x, aiPosition.y, aiPosition.z);
				data.time = timeStamp;
				b.positions.push_back(data);
			}

			for (UINT rotationIndex = 0; rotationIndex < channel->mNumRotationKeys; ++rotationIndex)
			{
				aiQuaternion aiOrientation = channel->mRotationKeys[rotationIndex].mValue;
				float timeStamp = (float)channel->mRotationKeys[rotationIndex].mTime;
				KeyRotation data;
				data.orientation = glm::quat(aiOrientation.w, aiOrientation.x, aiOrientation.y, aiOrientation.z);
				data.time = timeStamp;
				b.rotations.push_back(data);
			}

			for (UINT keyIndex = 0; keyIndex < channel->mNumScalingKeys; ++keyIndex)
			{
				aiVector3D scale = channel->mScalingKeys[keyIndex].mValue;
				float timeStamp = (float)channel->mScalingKeys[keyIndex].mTime;
				KeyScale data;
				data.scale = glm::vec3(scale.x, scale.y, scale.z);
				data.time = timeStamp;
				b.scales.push_back(data);
			}
			animator.bones.push_back(b);
		}
		model->skeletonAnimator.push_back(animator);
	}

	NodeData* FindNodeByName(NodeData* node, const std::string& name) {

		if (node->name == name)
			return node;

		for (NodeData& child : node->children) {
			NodeData* foundNode = FindNodeByName(&child, name);
			if (foundNode)
				return foundNode;
		}
		return nullptr;
	}

	void LoadNodeAnimation(const aiScene* scene, const aiAnimation *animation, Model* model) {
		
		if (!model || !animation || !scene)
			return;

		NodeAnimator animator;
		animator.currentTime = 0.0f;
		animator.duration = (FLOAT)animation->mDuration;
		animator.ticksPerSecond = (INT)(animation->mTicksPerSecond != 0 ? animation->mTicksPerSecond : 25.0f);
		animator.name = animation->mName.C_Str();
		animator.rootNode = model->rootNode;

		for (UINT i = 0; i < animation->mNumChannels; i++) {

			aiNodeAnim* channel = animation->mChannels[i];
			std::string nodeName = channel->mNodeName.C_Str();

			NodeData* node = FindNodeByName(&animator.rootNode, nodeName);

			if (node) {
				NodeAnimation nodeAnim;
				for (UINT positionIndex = 0; positionIndex < channel->mNumPositionKeys; ++positionIndex)
				{
					aiVector3D aiPosition = channel->mPositionKeys[positionIndex].mValue;
					float timeStamp = (float)channel->mPositionKeys[positionIndex].mTime;
					KeyPosition data;
					data.position = glm::vec3(aiPosition.x, aiPosition.y, aiPosition.z);
					data.time = timeStamp;
					nodeAnim.positions.push_back(data);
				}

				for (UINT rotationIndex = 0; rotationIndex < channel->mNumRotationKeys; ++rotationIndex)
				{
					aiQuaternion aiOrientation = channel->mRotationKeys[rotationIndex].mValue;
					float timeStamp = (float)channel->mRotationKeys[rotationIndex].mTime;
					KeyRotation data;
					data.orientation = glm::quat(aiOrientation.w, aiOrientation.x, aiOrientation.y, aiOrientation.z);
					data.time = timeStamp;
					nodeAnim.rotations.push_back(data);
				}

				for (UINT keyIndex = 0; keyIndex < channel->mNumScalingKeys; ++keyIndex)
				{
					aiVector3D scale = channel->mScalingKeys[keyIndex].mValue;
					float timeStamp = (float)channel->mScalingKeys[keyIndex].mTime;
					KeyScale data;
					data.scale = glm::vec3(scale.x, scale.y, scale.z);
					data.time = timeStamp;
					nodeAnim.scales.push_back(data);
				}
				animator.nodeAnimations[nodeName] = nodeAnim;
			}
		}
		model->nodeAnimator.push_back(animator);
	}

	void LoadMorphAnimation(const aiScene* scene, const aiAnimation* animation ,Model* model) {
		if (!model || !animation || !scene)
			return;
		for (UINT morphChannelIndex = 0; morphChannelIndex < animation->mNumMorphMeshChannels; ++morphChannelIndex){
			aiMeshMorphAnim* morphAnim = animation->mMorphMeshChannels[morphChannelIndex];
			MorphTargetAnimator mta;
			mta.meshName = morphAnim->mName.C_Str();
			mta.currentTime = 0.0f;
			mta.duration = (FLOAT)animation->mDuration;
			mta.ticksPerSecond = (INT)animation->mTicksPerSecond;

			for (UINT keyIndex = 0; keyIndex < morphAnim->mNumKeys; ++keyIndex){
				
				aiMeshMorphKey& key = morphAnim->mKeys[keyIndex];

				mta.times.push_back(static_cast<float>(key.mTime));

				std::vector<float> keyWeights;
				std::vector<unsigned int> keyIndices;

				for (UINT i = 0; i < key.mNumValuesAndWeights; ++i){
					keyIndices.push_back(key.mValues[i]);
					keyWeights.push_back(static_cast<float>(key.mWeights[i]));
				}
				mta.weights.push_back(keyWeights);
				mta.indices.push_back(keyIndices);
			}
			model->morphAnimator.push_back(mta);
		}
	}

	void LoadAnimations(const aiScene* scene, Model* model) {
		if (!model || !scene)
			return;

		model->haveAnimation = TRUE;
		
		// This seems like a hack but should work for models with animations
		if ((scene->mAnimations[0]->mNumChannels > 0) && (model->BoneCounter > 0)) {
			model->animType = SKELETALANIM;
		}
		else if (scene->mAnimations[0]->mNumChannels > 0) {
			model->animType = KEYFRAMEANIM;
		}
		else if (scene->mAnimations[0]->mNumMorphMeshChannels > 0) {
			model->animType = MORPHANIM;
		}

		for (UINT i = 0; i < scene->mNumAnimations; i++){
			aiAnimation* animation = scene->mAnimations[i];
			//LOG_WARNING(L"%d", animation->mNumMeshChannels);
			//LOG_WARNING(L"%d", animation->mNumMorphMeshChannels);
			//LOG_WARNING(L"%d", animation->mNumChannels);
			switch (model->animType){
				case AMC::SKELETALANIM:
					LoadSkeletalAnimation(scene, animation, model);
				break;
				case AMC::KEYFRAMEANIM:
					LoadNodeAnimation(scene, animation, model);
				break;
				case AMC::MORPHANIM:
					LoadMorphAnimation(scene, animation, model);
				break;
			}
		}
	}

	Bone* findBone(SkeletonAnimator* a, std::string name) {
		for (UINT i = 0; i < a->bones.size(); i++) {
			if (a->bones[i].name == name) {
				return &a->bones[i];
			}
		}
		return NULL;
	}

	float getScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime) {
		float scaleFactor = 0.0f;
		float midWayLength = animationTime - lastTimeStamp;
		float framesDiff = nextTimeStamp - lastTimeStamp;
		scaleFactor = midWayLength / framesDiff;
		return scaleFactor;
	}

	void CalculateBoneTransform(Model* model, SkeletonAnimator* a, const AssimpNodeData* node, glm::mat4 parentTransform) {
		
		std::string nodeName = node->name;
		glm::mat4 nodeTransform = node->transformation;

		Bone* bone = findBone(a, nodeName);

		if (bone) {

			glm::mat4 translationMat = glm::mat4(1.0f);
			glm::mat4 rotationMat = glm::mat4(1.0f);
			glm::mat4 scalingMat = glm::mat4(1.0f);

			int p0Index = -1;
			int p1Index = -1;

			float animationTime = a->currentTime;

			// Calculate Translation
			if (bone->positions.size() == 1) {
				translationMat = glm::translate(glm::mat4(1.0f), bone->positions[0].position);
			}
			else {
				for (p0Index = 0; p0Index < bone->positions.size() - 1; ++p0Index) {
					if (animationTime < bone->positions[p0Index + 1].time) {
						break;
					}
				}
				p1Index = p0Index + 1;
				float scaleFactor = getScaleFactor(bone->positions[p0Index].time, bone->positions[p1Index].time, animationTime);
				glm::vec3 interpolatedPosition = glm::mix(bone->positions[p0Index].position, bone->positions[p1Index].position, scaleFactor);
				translationMat = glm::translate(glm::mat4(1.0f), interpolatedPosition);
			}

			// Calculate Rotation
			if (bone->rotations.size() == 1) {
				glm::quat rotation = glm::normalize(bone->rotations[0].orientation);
				rotationMat = glm::mat4_cast(rotation);
			}
			else {
				for (p0Index = 0; p0Index < bone->rotations.size() - 1; ++p0Index) {
					if (animationTime < bone->rotations[p0Index + 1].time) {
						break;
					}
				}
				p1Index = p0Index + 1;
				float scaleFactor = getScaleFactor(bone->rotations[p0Index].time, bone->rotations[p1Index].time, animationTime);
				glm::quat startRotation = bone->rotations[p0Index].orientation;
				glm::quat endRotation = bone->rotations[p1Index].orientation;
				glm::quat interpolatedRotation = glm::normalize(glm::slerp(startRotation, endRotation, scaleFactor));
				rotationMat = glm::mat4_cast(interpolatedRotation);
			}

			// Calculate Scaling
			if (bone->scales.size() == 1) {
				scalingMat = glm::scale(glm::mat4(1.0f), bone->scales[0].scale);
			}
			else {
				for (p0Index = 0; p0Index < bone->scales.size() - 1; ++p0Index) {
					if (animationTime < bone->scales[p0Index + 1].time) {
						break;
					}
				}
				p1Index = p0Index + 1;
				float scaleFactor = getScaleFactor(bone->scales[p0Index].time, bone->scales[p1Index].time, animationTime);
				glm::vec3 interpolatedScale = glm::mix(bone->scales[p0Index].scale, bone->scales[p1Index].scale, scaleFactor);
				scalingMat = glm::scale(glm::mat4(1.0f), interpolatedScale);
			}

			// Combine transformations
			bone->localTransform = translationMat * rotationMat * scalingMat;
			nodeTransform = bone->localTransform;
		}

		glm::mat4 globalTransformation = parentTransform * nodeTransform;
		auto boneInfoMap = model->BoneInfoMap;
		if (boneInfoMap.find(nodeName) != boneInfoMap.end()) {
			int index = boneInfoMap[nodeName].id;
			glm::mat4 offsetMatrix = boneInfoMap[nodeName].offset;
			a->finalBoneMatrices[index] = globalTransformation * offsetMatrix;
		}

		for (size_t i = 0; i < node->children.size(); i++) {
			CalculateBoneTransform(model, a, &node->children[i], globalTransformation);
		}
	}

	void CalculateNodeTransform(NodeData* node, const glm::mat4& parentTramsform, NodeAnimator& animator) {

		std::string nodeName = node->name;
		glm::mat4 nodeTransform = node->transformation;

		auto it = animator.nodeAnimations.find(nodeName);
		if (it != animator.nodeAnimations.end()) {
			NodeAnimation& nodeAnim = it->second;
			glm::mat4 translationMat(1.0f);
			glm::mat4 rotationMat(1.0f);
			glm::mat4 scalingMat(1.0f);
			int p0Index = -1;
			int p1Index = -1;
			float currentTime = animator.currentTime;

			// Calculate Translation
			if (nodeAnim.positions.size() == 1) {
				translationMat = glm::translate(glm::mat4(1.0f), nodeAnim.positions[0].position);
			}
			else {
				for (p0Index = 0; p0Index < nodeAnim.positions.size() - 1; ++p0Index) {
					if (currentTime < nodeAnim.positions[p0Index + 1].time) {
						break;
					}
				}
				if (p0Index == nodeAnim.positions.size() - 1) {
					translationMat = glm::translate(glm::mat4(1.0f), nodeAnim.positions[p0Index].position);
				}
				else {
					p1Index = p0Index + 1;
					float scaleFactor = getScaleFactor(nodeAnim.positions[p0Index].time, nodeAnim.positions[p1Index].time, currentTime);
					glm::vec3 interpolatedPosition = glm::mix(nodeAnim.positions[p0Index].position, nodeAnim.positions[p1Index].position, scaleFactor);
					translationMat = glm::translate(glm::mat4(1.0f), interpolatedPosition);
				}
			}

			// Calculate Rotation
			if (nodeAnim.rotations.size() == 1) {
				glm::quat rotation = glm::normalize(nodeAnim.rotations[0].orientation);
				rotationMat = glm::mat4_cast(rotation);
			}
			else {
				for (p0Index = 0; p0Index < nodeAnim.rotations.size() - 1; ++p0Index) {
					if (currentTime < nodeAnim.rotations[p0Index + 1].time) {
						break;
					}
				}
				if (p0Index == nodeAnim.rotations.size() - 1) {
					glm::quat rotation = glm::normalize(nodeAnim.rotations[p0Index].orientation);
					rotationMat = glm::mat4_cast(rotation);
				}
				else {
					p1Index = p0Index + 1;
					float scaleFactor = getScaleFactor(nodeAnim.rotations[p0Index].time, nodeAnim.rotations[p1Index].time, currentTime);
					glm::quat startRotation = nodeAnim.rotations[p0Index].orientation;
					glm::quat endRotation = nodeAnim.rotations[p1Index].orientation;
					glm::quat interpolatedRotation = glm::normalize(glm::slerp(startRotation, endRotation, scaleFactor));
					rotationMat = glm::mat4_cast(interpolatedRotation);
				}
			}

			// Calculate Scaling
			if (nodeAnim.scales.size() == 1) {
				scalingMat = glm::scale(glm::mat4(1.0f), nodeAnim.scales[0].scale);
			}
			else {
				for (p0Index = 0; p0Index < nodeAnim.scales.size() - 1; ++p0Index) {
					if (currentTime < nodeAnim.scales[p0Index + 1].time) {
						break;
					}
				}
				if (p0Index == nodeAnim.scales.size() - 1) {
					scalingMat = glm::scale(glm::mat4(1.0f), nodeAnim.scales[p0Index].scale);
				}
				else {
					p1Index = p0Index + 1;
					float scaleFactor = getScaleFactor(nodeAnim.scales[p0Index].time, nodeAnim.scales[p1Index].time, currentTime);
					glm::vec3 interpolatedScale = glm::mix(nodeAnim.scales[p0Index].scale, nodeAnim.scales[p1Index].scale, scaleFactor);
					scalingMat = glm::scale(glm::mat4(1.0f), interpolatedScale);
				}
			}
			// Combine transformations
			nodeTransform = translationMat * rotationMat * scalingMat;
		}
		// Combine with parent transformation
		glm::mat4 globalTransform = parentTramsform * nodeTransform;
		node->globalTransform = globalTransform;

		for (NodeData& child : node->children) {
			CalculateNodeTransform(&child, node->globalTransform, animator);
		}
	}

	Material::Material(){
		albedo = glm::vec3(0.0f);
		metallic = 0.0f;
		roughness = 0.0f;
		ao = 0.0f;
		emission = glm::vec3(0.0f);
		alpha = 0.0f;
		textureFlag = 0;
	}

	void Material::Apply(ShaderProgram* program)
	{
		// Upload Material Data Here
		for (auto t : textures) {
			switch (t.type){
				case TextureTypeDiffuse:
					glBindTextureUnit(TextureTypeDiffuse,t.texture);
				break;
				case TextureTypeNormalMap:
					glBindTextureUnit(TextureTypeNormalMap, t.texture);
				break;
				case TextureTypeMetallicRoughnessMap:
					glBindTextureUnit(TextureTypeMetallicRoughnessMap, t.texture);
				break;
				case TextureTypeEmissive:
					glBindTextureUnit(TextureTypeEmissive, t.texture);
				break;
				case TextureTypeAmbient:
					glBindTextureUnit(TextureTypeAmbient, t.texture);
				break;
				default:
			  	break;
			}
		}
	}

	void Material::LoadMaterialTexturesFromFile(const std::string& path, TextureType type)
	{
		ModelTexture tex;
		tex.type = type;
		tex.texture = AMC::TextureManager::LoadTexture(path);
		this->textures.push_back(tex);
	}

	// TODO Later
	void Material::LoadMaterialTexturesFromMemory(const aiTexture* t, TextureType type)
	{
	}

	Model::Model(std::string path, int iAssimpFlags){
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(path, iAssimpFlags);
		if (!scene) {
			LOG_ERROR(L"Assimp Could Not Load Scene For Model : %s", CString(importer.GetErrorString()));
			return;
		}
		else if (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
			LOG_ERROR(L"Assimp Scene incomplete For Model : %s", CString(importer.GetErrorString()));
			return;
		}
		else if (!scene->mRootNode) {
			LOG_ERROR(L"Assimp Root Node Empty For Model : %s", CString(importer.GetErrorString()));
			//AMC::Log::GetInstance()->WriteLogFile(__FUNCTION__, AMC::LOG_ERROR, L"Assimp Root Node Empty For Model : %s", CString(importer.GetErrorString()));
			return;
		}

		// Load Materials
		std::string directory = path.substr(0, path.find_last_of("/\\"));
		if (scene->HasMaterials()) {
			LoadMaterials(scene, this, directory);
		}

		// Load Meshes
		if (scene->HasMeshes()) {
			LoadMeshes(scene, this, directory);
		}

		// store node heirarchy because we'll render in that order
		readNodeHierarchy(this->rootNode, scene->mRootNode);

		// Load Animation Data
		if (scene->HasAnimations()) {
			LoadAnimations(scene, this);
		}

		//Print Info
#ifdef _MYDEBUG
		LOG_INFO(L" Model Details %s", CString(path.c_str()));
		LOG_INFO(L" Number Of Nodes : %d", rootNode.children.size());
		LOG_INFO(L" Number Of Meshes : %d", meshes.size());
		LOG_INFO(L" Number Of Materials : %d", materials.size());
		if (haveAnimation) {
			switch (animType){
				case AMC::SKELETALANIM:
					LOG_INFO(L"Skeletal Animation : %d",skeletonAnimator.size());
				break;
				case AMC::KEYFRAMEANIM:
					LOG_INFO(L"Keyframe Animation : %d", nodeAnimator.size());
				break;
				case AMC::MORPHANIM:
					LOG_INFO(L"Morph Animation : %d", morphAnimator.size());
				break;
			}
		}
		LOG_INFO(L" AABB min : %f %f %f \t max : %f %f %f", aabb.mMin.x, aabb.mMin.y, aabb.mMin.z, aabb.mMax.x, aabb.mMax.y, aabb.mMax.z);
#endif
		importer.FreeScene();
	}

	Model::~Model(){
		
		// Clean up meshes
		for (Mesh* mesh : meshes) {
			if (mesh) {
				glDeleteVertexArrays(1, &mesh->vao);  // Delete VAO
				glDeleteBuffers(7, nullptr);  // Adjust the count based on your VBOs
				delete mesh;
			}
		}
		meshes.clear();

		// Clean up materials
		for (Material* material : materials) {
			if (material) {
				for (const ModelTexture& texture : material->textures) {
					glDeleteTextures(1, &texture.texture);  // Delete textures
				}
				delete material;
			}
		}
		materials.clear();
	}


	void Model::drawNodes(const NodeData& node, const glm::mat4& parentTransform, ShaderProgram* program, UINT iNumInstance, bool iUseMaterial) {

		glm::mat4 globalTransform = parentTransform * node.globalTransform;

		//TODO:  set Node matrix here
		glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(globalTransform));
		for (UINT meshIndex : node.meshIndices) {
			Mesh* mesh = meshes[meshIndex];

			if (iUseMaterial)
				materials[mesh->mMaterial]->Apply(program);

			glBindVertexArray(mesh->vao);
			glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLES, mesh->mTriangleCount, GL_UNSIGNED_INT, 0, iNumInstance, 0, 0);
		}

		// Recursively draw all child nodes
		for (const NodeData& childNode : node.children) {
			drawNodes(childNode, globalTransform, program, iNumInstance, iUseMaterial);
		}
	}

	void Model::draw(ShaderProgram* program, UINT iNumInstance, bool iUseMaterial){
		
		glm::mat4 identity = glm::mat4(1.0f);

		if (haveAnimation) {
			switch (animType){
				case AMC::SKELETALANIM:
					if (this->CurrentAnimation >= 0 && this->CurrentAnimation < this->skeletonAnimator.size()) {
						// Set Bone Matrices?
						glUniformMatrix4fv(program->getUniformLocation("bMat[0]"), MAX_BONE_COUNT, GL_FALSE, glm::value_ptr(this->skeletonAnimator[this->CurrentAnimation].finalBoneMatrices[0]));
					}
				break;
				case AMC::KEYFRAMEANIM:
				break;
				case AMC::MORPHANIM:
				break;
			}
		}
		drawNodes(rootNode, identity, program, iNumInstance, iUseMaterial);
	}

	void Model::update(float dt){

		if (!haveAnimation)
			 return;

		switch (animType) {
			case SKELETALANIM:
				if(this->CurrentAnimation >= 0 && this->CurrentAnimation < this->skeletonAnimator.size()){
					this->skeletonAnimator[this->CurrentAnimation].currentTime += this->skeletonAnimator[this->CurrentAnimation].ticksPerSecond * dt;
					this->skeletonAnimator[this->CurrentAnimation].currentTime = fmod(this->skeletonAnimator[this->CurrentAnimation].currentTime, this->skeletonAnimator[this->CurrentAnimation].duration);
					CalculateBoneTransform(this, &this->skeletonAnimator[this->CurrentAnimation], &this->skeletonAnimator[this->CurrentAnimation].rootNode, glm::mat4(1.0f));
				}
			break;
			case KEYFRAMEANIM:
				if(this->CurrentAnimation >= 0 && this->CurrentAnimation < this->nodeAnimator.size()) {
					this->nodeAnimator[this->CurrentAnimation].currentTime += this->nodeAnimator[this->CurrentAnimation].ticksPerSecond * dt;
					this->nodeAnimator[this->CurrentAnimation].currentTime = fmod(this->nodeAnimator[this->CurrentAnimation].currentTime, this->nodeAnimator[this->CurrentAnimation].duration);
					CalculateNodeTransform(&this->rootNode,glm::mat4(1.0f), this->nodeAnimator[this->CurrentAnimation]);
				}
			break;
			case MORPHANIM:
				if (this->CurrentAnimation >= 0 && this->CurrentAnimation < this->nodeAnimator.size()) {
					//float& currentTime = this->morphAnimator[this->CurrentAnimation].currentTime;
					//currentTime += this->morphAnimator[this->CurrentAnimation].ticksPerSecond * dt;
					//currentTime = fmod(currentTime, this->morphAnimator[this->CurrentAnimation].duration);
					//calculateMorphTargets();
				}
			break;
		}
	}

	void Model::lerpAnimation(float t){
		if (t < 0.0f) t = 0.0f;
		if (t > 1.0f) t = 1.0f;

		switch (animType) {
			case SKELETALANIM:
				if (this->CurrentAnimation >= 0 && this->CurrentAnimation < this->skeletonAnimator.size()) {
					this->skeletonAnimator[this->CurrentAnimation].currentTime += this->skeletonAnimator[this->CurrentAnimation].ticksPerSecond * t;
					this->skeletonAnimator[this->CurrentAnimation].currentTime = fmod(this->skeletonAnimator[this->CurrentAnimation].currentTime, this->skeletonAnimator[this->CurrentAnimation].duration);
					//calculateBoneTransform(this, &this->skeletonAnimator[this->CurrentAnimation], &this->skeletonAnimator[this->CurrentAnimation].rootNode, DirectX::XMMatrixIdentity());
				}
			break;
			case KEYFRAMEANIM:
				if (this->CurrentAnimation >= 0 && this->CurrentAnimation < this->nodeAnimator.size()) {
					this->nodeAnimator[this->CurrentAnimation].currentTime += this->nodeAnimator[this->CurrentAnimation].ticksPerSecond * t;
					this->nodeAnimator[this->CurrentAnimation].currentTime = fmod(this->nodeAnimator[this->CurrentAnimation].currentTime, this->nodeAnimator[this->CurrentAnimation].duration);
					//calculateNodeTransform();
				}
			break;
			case MORPHANIM:
				if (this->CurrentAnimation >= 0 && this->CurrentAnimation < this->nodeAnimator.size()) {
					float& currentTime = this->morphAnimator[this->CurrentAnimation].currentTime;
					currentTime += this->morphAnimator[this->CurrentAnimation].ticksPerSecond * t;
					currentTime = fmod(currentTime, this->morphAnimator[this->CurrentAnimation].duration);
					//calculateMorphTargets();
				}
			break;
		}
	}

	void Model::setActiveAnimation(int index){
		switch (animType) {
			case SKELETALANIM:
				if (index < skeletonAnimator.size() && index >= 0) {
					this->CurrentAnimation = index;
				}
			break;
			case KEYFRAMEANIM:
				if (index < nodeAnimator.size() && index >= 0) {
					this->CurrentAnimation = index;
				}
			break;
			case MORPHANIM:
				if (index < morphAnimator.size() && index >= 0) {
					this->CurrentAnimation = index;
				}
			break;
		}
	}
};
