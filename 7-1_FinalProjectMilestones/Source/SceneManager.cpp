///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/


//Method for defining material settings for the objects in the scene

void SceneManager::DefineObjectMaterials()
{
	//Table material
	//Slightly reflective so light can show on it
	OBJECT_MATERIAL tableMaterial;
	tableMaterial.ambientColor = glm::vec3(0.7f, 0.55f, 0.4f);
	tableMaterial.ambientStrength = 0.6f;
	tableMaterial.diffuseColor = glm::vec3(0.65f, 0.50f, 0.35f);
	tableMaterial.specularColor = glm::vec3(0.20f, 0.20f, 0.18f);
	tableMaterial.shininess = 12.0f;
	tableMaterial.tag = "table";
	m_objectMaterials.push_back(tableMaterial);


	//Water bottle material
	//Stronger specular response for metal body
	OBJECT_MATERIAL metalBottleMaterial;
	metalBottleMaterial.ambientColor = glm::vec3(0.5f, 0.5f, 0.5f);
	metalBottleMaterial.ambientStrength = 0.6f;
	metalBottleMaterial.diffuseColor = glm::vec3(0.95f, 0.95f, 0.98f);
	metalBottleMaterial.specularColor = glm::vec3(0.75f, 0.75f, 0.80f);
	metalBottleMaterial.shininess = 48.0f;
	metalBottleMaterial.tag = "metalBottle";
	m_objectMaterials.push_back(metalBottleMaterial);


	//Plastic material for the cap and spout
	OBJECT_MATERIAL plasticMaterial;
	plasticMaterial.ambientColor = glm::vec3(0.6f, 0.6f, 0.6f);
	plasticMaterial.ambientStrength = 0.6f;
	plasticMaterial.diffuseColor = glm::vec3(0.8f, 0.8f, 0.8f);
	plasticMaterial.specularColor = glm::vec3(0.25f, 0.25f, 0.28f);
	plasticMaterial.shininess = 14.0f;
	plasticMaterial.tag = "plastic";
	m_objectMaterials.push_back(plasticMaterial);

	//Rubber handle material
	//Low specular response to give it a more matte look
	OBJECT_MATERIAL rubberMaterial;
	rubberMaterial.ambientColor = glm::vec3(0.6f, 0.6f, 0.6f);
	rubberMaterial.ambientStrength = 0.7f;
	rubberMaterial.diffuseColor = glm::vec3(0.7f, 0.7f, 0.7f);
	rubberMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);
	rubberMaterial.shininess = 6.0f;
	rubberMaterial.tag = "rubber";
	m_objectMaterials.push_back(rubberMaterial);
}


//Method for configuring the light sources for the scene

void SceneManager::SetupSceneLights()
{
	//Enable custom lighting in the shader
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	//Main key light — close, in front, slightly above
	m_pShaderManager->setVec3Value("lightSources[0].position", 0.0f, 3.0f, 5.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.8f, 0.8f, 0.8f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 2.0f, 2.0f, 2.0f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 12.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.8f);

	//Fill light — left side, mid height
	m_pShaderManager->setVec3Value("lightSources[1].position", -4.0f, 3.0f, 3.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.5f, 0.5f, 0.55f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 1.2f, 1.2f, 1.3f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.3f, 0.3f, 0.4f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 8.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.3f);

	//Rim/back light — behind and above to separate bottle from background
	m_pShaderManager->setVec3Value("lightSources[2].position", 1.0f, 6.0f, -3.0f);
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", 0.4f, 0.4f, 0.4f);
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 1.5f, 1.5f, 1.5f);
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", 0.5f, 0.5f, 0.5f);
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 8.0f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.4f);

	//Right side fill light
	m_pShaderManager->setVec3Value("lightSources[3].position", 4.0f, 2.5f, 3.0f);
	m_pShaderManager->setVec3Value("lightSources[3].ambientColor", 0.35f, 0.35f, 0.35f);
	m_pShaderManager->setVec3Value("lightSources[3].diffuseColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setVec3Value("lightSources[3].specularColor", 0.3f, 0.3f, 0.3f);
	m_pShaderManager->setFloatValue("lightSources[3].focalStrength", 7.0f);
	m_pShaderManager->setFloatValue("lightSources[3].specularIntensity", 0.25f);
}


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()

{
	//Define materials for the scene objects
	DefineObjectMaterials();

	//Setup the scene lights
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();

	//Load the meshes for the water bottle
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadSphereMesh();

	//Textures

	CreateGLTexture("../../Utilities/textures/darkwood.png", "wood"); //Texture for table
	CreateGLTexture("../../Utilities/textures/metal2.jpg", "metal"); //Textre for main body of water bottle
	CreateGLTexture("../../Utilities/textures/black.jpg", "plastic"); //Texture for cap, lid, and spout of water bottle)
	CreateGLTexture("../../Utilities/textures/plastic2.png", "plastic2"); //Texture for rubik's cube and parts of mouse
	CreateGLTexture("../../Utilities/textures/plastic3.jpg", "plastic3"); //Texture for parts of mouse
	CreateGLTexture("../../Utilities/textures/rubber.jpg", "rubber"); //Texture for handle
	CreateGLTexture("../../Utilities/textures/stainless.jpg", "stainless");
	CreateGLTexture("../../Utilities/textures/stainless_end.jpg", "stainlessEnd");
	
	BindGLTextures();

}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/

	/****   Readjusted the plane mesh to be more of a table top ****/
	//Table top color is not final, just used to differentiate the plane from the water bottle for now.

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(8.0f, 1.0f, 6.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Applly table material and texture
	SetShaderColor(0.15f, 0.15f, 0.15f, 1.0f);
	SetShaderMaterial("table");
	SetShaderTexture("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	

	/*****************************************************************
	 *****************************************************************
	 **                                                             **
	 **                       Water Bottle                          **
	 **                                                             **
	 *****************************************************************
	 *****************************************************************/

	//Will use slightly different shades for the parts to make them distinguishable.
	//In the final product the colors of the water bottle will be the same.

	/********** Main Body -- Cylinder **********/


	//Set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.05f, 4.0f, 1.05f);

	//Set the XYZ rotation for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	//Set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//Set transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set color
	//SetShaderColor(0.1f, 0.1f, 0.1f, 1.0f); //Dark Gray

	SetTextureUVScale(3.0f, 6.0f); //Tile metal texture to add detail

	//Apply material and texture
	SetShaderColor(0.0f, 0.0f, 0.0f, 0.0f);
	SetShaderMaterial("metalBottle");
	SetShaderTexture("metal");
	
	//Draw cylinder mesh
	m_basicMeshes->DrawCylinderMesh();


	/********************************************/



	/********** Cap -- Cylinder ***********/

	//Set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.9f, 0.9f, 0.9f);

	//Set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 4.0f, 0.0f);

	//Set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//Set transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set color
	//SetShaderColor(0.15f, 0.15f, 0.15f, 1.0f); //Slightly lighter gray than the main body

	//Apply plastic texture to cap
	//SetShaderTexture("plastic");
	SetTextureUVScale(1.0f, 1.0f); //Default UV scale for cap

	//Apply material and texture
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	SetShaderMaterial("plastic");
	SetShaderTexture("plastic");

	//Draw cylinder mesh
	m_basicMeshes->DrawCylinderMesh();

	/*****************************************/



	/********** Spout -- Tapered Cylinder *********/

	//Set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.28f, 0.55f, 0.28f);

	//Set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.45f, 4.75f, 0.0f);

	//Set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -20.0f;

	//Set transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set color
	//SetShaderColor(0.1f, 0.1f, 0.1f, 1.0f); //Same Dark Gray as the main body

	//Set plastic texture for spout
	//SetShaderTexture("plastic");
	SetTextureUVScale(1.0f, 1.0f);

	//Apply material and texture
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	SetShaderMaterial("plastic");
	SetShaderTexture("plastic");

	//Draw tapered cylinder mesh
	m_basicMeshes->DrawTaperedCylinderMesh();

	/**********************************************/



	/********** Small lid port -- Tapered Cylinder ***********/

	//Set the XYZ position for the mesh
	scaleXYZ = glm::vec3(0.350f, 0.60f, 0.40f);

	//Set the XYZ position for the mesh
	positionXYZ = glm::vec3(-0.45f, 4.75f, 0.0f);

	//Set the XYZ position for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//Set transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set color
	//SetShaderColor(0.12f, 0.12f, 0.12f, 1.0f); //Slightly lighter gray than the main body

	//Set plastic texture for lid port
	//SetShaderTexture("plastic");
	SetTextureUVScale(1.0f, 1.0f);

	//Apply material and texture
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	SetShaderMaterial("plastic");
	SetShaderTexture("plastic");

	//Draw tapered cylinder mesh
	m_basicMeshes->DrawCylinderMesh();

	/*********************************************************/



	/********** Handle -- Torus *********/

	//Set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.5f, 1.2f, 1.5f);

	//Set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 5.40f, 0.0f);

	//Set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	//Set transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set color
	//SetShaderColor(0.1f, 0.1f, 0.1f, 1.0f); //Same Dark Gray as the main body

	//Set rubber texture for handle
	//SetShaderTexture("rubber");
	SetTextureUVScale(6.0f, 3.0f); //Tile rubber texture and avoid stretching across the handle

	//Apply material and texture
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	SetShaderMaterial("rubber");
	SetShaderTexture("rubber");

	//Draw torus mesh
	m_basicMeshes->DrawTorusMesh();

	/**************************************/



	/*****************************************************************
	 *****************************************************************
	 **                                                             **
	 **                    							                **
	 **						Rubik's Cube							**
	 **                                                             **
	 *****************************************************************
	 *****************************************************************/

	//Main body
	SetTransformations(
		glm::vec3(1.2f, 1.2f, 1.2f),
		0.0f,
		25.0f,
		0.0f,
		glm::vec3(-2.2f, 0.60f, 2.0));

	SetTextureUVScale(3.0f, 3.0f);
	SetShaderTexture("plastic2");
	m_basicMeshes->DrawBoxMesh();

	/******************************************/

	//Parameters for the stickers on the cube
	float stickerWidth = 0.29f;
	float stickerHeight = 0.32f;
	float stickerDepth = 0.03f;
	float spacing = 0.35f;

	float cubeX = -2.2f;
	float cubeY = 0.60f;
	float cubeZ = 2.0f;

	float frontCenterX = cubeX + 0.25f;
	float frontCenterZ = cubeZ + 0.54f;

	float xStep = 0.30f;
	float zStep = -0.15f;


	//Front stickers

	//Top row
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); //white
	SetTransformations(glm::vec3(stickerWidth, stickerHeight, stickerDepth), 0.0f, 25.0f, 0.0f,  //Same rotation as the cube so they are aligned, moved slightly forward. This is the same for all the stickers on the front face
		glm::vec3(frontCenterX - xStep, cubeY + spacing, frontCenterZ - zStep));
	m_basicMeshes->DrawBoxMesh();

	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); //white
	SetTransformations(glm::vec3(stickerWidth, stickerHeight, stickerDepth), 0.0f, 25.0f, 0.0f,
		glm::vec3(frontCenterX, cubeY + spacing, frontCenterZ));
	m_basicMeshes->DrawBoxMesh();

	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); //white
	SetTransformations(glm::vec3(stickerWidth, stickerHeight, stickerDepth), 0.0f, 25.0f, 0.0f,
		glm::vec3(frontCenterX + xStep, cubeY + spacing, frontCenterZ + zStep));
	m_basicMeshes->DrawBoxMesh();

	//Middle row
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); //white
	SetTransformations(glm::vec3(stickerWidth, stickerHeight, stickerDepth), 0.0f, 25.0f, 0.0f, 
		glm::vec3(frontCenterX - xStep, cubeY, frontCenterZ - zStep));
	m_basicMeshes->DrawBoxMesh();

	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); //white
	SetTransformations(glm::vec3(stickerWidth, stickerHeight, stickerDepth), 0.0f, 25.0f, 0.0f,
		glm::vec3(frontCenterX, cubeY, frontCenterZ));
	m_basicMeshes->DrawBoxMesh();

	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); //white
	SetTransformations(glm::vec3(stickerWidth, stickerHeight, stickerDepth), 0.0f, 25.0f, 0.0f,
		glm::vec3(frontCenterX + xStep, cubeY, frontCenterZ + zStep));
	m_basicMeshes->DrawBoxMesh();

	//Bottom row
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); //white
	SetTransformations(glm::vec3(stickerWidth, stickerHeight, stickerDepth), 0.0f, 25.0f, 0.0f,
		glm::vec3(frontCenterX - xStep, cubeY - spacing, frontCenterZ - zStep));
	m_basicMeshes->DrawBoxMesh();

	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); //white
	SetTransformations(glm::vec3(stickerWidth, stickerHeight, stickerDepth), 0.0f, 25.0f, 0.0f,
		glm::vec3(frontCenterX, cubeY - spacing, frontCenterZ));
	m_basicMeshes->DrawBoxMesh();

	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); //white
	SetTransformations(glm::vec3(stickerWidth, stickerHeight, stickerDepth), 0.0f, 25.0f, 0.0f,
		glm::vec3(frontCenterX + xStep, cubeY - spacing, frontCenterZ + zStep));
	m_basicMeshes->DrawBoxMesh();

	/******************************************************************************************/


	//Right face stickers

	//Calculate center positions for the right face
	float rightCenterX = cubeX + 0.54f;
	float rightCenterZ = cubeZ - 0.25f;

	//Top row
	SetShaderColor(0.0f, 0.2f, 0.8f, 1.0f); //blue
	SetTransformations(glm::vec3(stickerWidth, stickerHeight, stickerDepth), 0.0f, 115.0f, 0.0f,  //Same rotation as the cube so they are aligned, moved slightly forward. This is the same for all the stickers on the right face
		glm::vec3(rightCenterX + zStep, cubeY + spacing, rightCenterZ - xStep));
	m_basicMeshes->DrawBoxMesh();

	SetShaderColor(0.8f, 0.3f, 0.0f, 1.0f); //orange
	SetTransformations(glm::vec3(stickerWidth, stickerHeight, stickerDepth), 0.0f, 115.0f, 0.0f,
		glm::vec3(rightCenterX, cubeY + spacing, rightCenterZ));
	m_basicMeshes->DrawBoxMesh();

	SetShaderColor(0.8f, 0.3f, 0.0f, 1.0f); //orange
	SetTransformations(glm::vec3(stickerWidth, stickerHeight, stickerDepth), 0.0f, 115.0f, 0.0f,
		glm::vec3(rightCenterX - zStep, cubeY + spacing, rightCenterZ + xStep));
	m_basicMeshes->DrawBoxMesh();

	//Middle row
	SetShaderColor(1.0f, 0.85f, 0.0f, 1.0f); //yellow
	SetTransformations(glm::vec3(stickerWidth, stickerHeight, stickerDepth), 0.0f, 115.0f, 0.0f,
		glm::vec3(rightCenterX + zStep, cubeY, rightCenterZ - xStep));
	m_basicMeshes->DrawBoxMesh();

	SetShaderColor(0.8f, 0.3f, 0.0f, 1.0f); //orange
	SetTransformations(glm::vec3(stickerWidth, stickerHeight, stickerDepth), 0.0f, 115.0f, 0.0f,
		glm::vec3(rightCenterX, cubeY, rightCenterZ));
	m_basicMeshes->DrawBoxMesh();

	SetShaderColor(0.8f, 0.3f, 0.0f, 1.0f); //orange
	SetTransformations(glm::vec3(stickerWidth, stickerHeight, stickerDepth), 0.0f, 115.0f, 0.0f,
		glm::vec3(rightCenterX - zStep, cubeY, rightCenterZ + xStep));
	m_basicMeshes->DrawBoxMesh();


	//Bottom row
	SetShaderColor(1.0f, 0.85f, 0.0f, 1.0f); //yellow
	SetTransformations(glm::vec3(stickerWidth, stickerHeight, stickerDepth), 0.0f, 115.0f, 0.0f,
		glm::vec3(rightCenterX + zStep, cubeY - spacing, rightCenterZ - xStep));
	m_basicMeshes->DrawBoxMesh();

	SetShaderColor(0.8f, 0.3f, 0.0f, 1.0f); //orange
	SetTransformations(glm::vec3(stickerWidth, stickerHeight, stickerDepth), 0.0f, 115.0f, 0.0f,
		glm::vec3(rightCenterX, cubeY - spacing, rightCenterZ));
	m_basicMeshes->DrawBoxMesh();

	SetShaderColor(0.8f, 0.3f, 0.0f, 1.0f); //orange
	SetTransformations(glm::vec3(stickerWidth, stickerHeight, stickerDepth), 0.0f, 115.0f, 0.0f,
		glm::vec3(rightCenterX - zStep, cubeY - spacing, rightCenterZ + xStep));
	m_basicMeshes->DrawBoxMesh();
		
	/*************************************************************************/


	//Left face stickers

	//Calculate center positions for the left face
	float leftCenterX = cubeX - 0.54f;
	float leftCenterZ = cubeZ + 0.25f;

	//Top row
	SetShaderColor(0.8f, 0.0f, 0.0f, 1.0f); //red
	SetTransformations(glm::vec3(stickerWidth, stickerHeight, stickerDepth), 0.0f, -65.0f, 0.0f, //Same rotation as the cube so they are aligned, moved slightly forward. This is the same for all the stickers on the left face
		glm::vec3(leftCenterX - zStep, cubeY + spacing, leftCenterZ + xStep));
	m_basicMeshes->DrawBoxMesh();

	SetShaderColor(0.8f, 0.0f, 0.0f, 1.0f); //red
	SetTransformations(glm::vec3(stickerWidth, stickerHeight, stickerDepth), 0.0f, -65.0f, 0.0f,
		glm::vec3(leftCenterX, cubeY + spacing, leftCenterZ));
	m_basicMeshes->DrawBoxMesh();

	SetShaderColor(0.8f, 0.0f, 0.0f, 1.0f); //red
	SetTransformations(glm::vec3(stickerWidth, stickerHeight, stickerDepth), 0.0f, -65.0f, 0.0f,
		glm::vec3(leftCenterX + zStep, cubeY + spacing, leftCenterZ - xStep));
	m_basicMeshes->DrawBoxMesh();


	//Middle row
	SetShaderColor(0.8f, 0.0f, 0.0f, 1.0f); //red
	SetTransformations(glm::vec3(stickerWidth, stickerHeight, stickerDepth), 0.0f, -65.0f, 0.0f,
		glm::vec3(leftCenterX - zStep, cubeY, leftCenterZ + xStep));
	m_basicMeshes->DrawBoxMesh();

	SetShaderColor(0.8f, 0.0f, 0.0f, 1.0f); //red
	SetTransformations(glm::vec3(stickerWidth, stickerHeight, stickerDepth), 0.0f, -65.0f, 0.0f,
		glm::vec3(leftCenterX, cubeY, leftCenterZ));
	m_basicMeshes->DrawBoxMesh();

	SetShaderColor(1.0f, 0.85f, 0.0f, 1.0f); //yellow
	SetTransformations(glm::vec3(stickerWidth, stickerHeight, stickerDepth), 0.0f, -65.0f, 0.0f,
		glm::vec3(leftCenterX + zStep, cubeY, leftCenterZ - xStep));
	m_basicMeshes->DrawBoxMesh();

	//Bottom row
	SetShaderColor(0.8f, 0.0f, 0.0f, 1.0f); //red
	SetTransformations(glm::vec3(stickerWidth, stickerHeight, stickerDepth), 0.0f, -65.0f, 0.0f,
		glm::vec3(leftCenterX - zStep, cubeY - spacing, leftCenterZ + xStep));
	m_basicMeshes->DrawBoxMesh();

	SetShaderColor(0.8f, 0.0f, 0.0f, 1.0f); //red
	SetTransformations(glm::vec3(stickerWidth, stickerHeight, stickerDepth), 0.0f, -65.0f, 0.0f,
		glm::vec3(leftCenterX, cubeY - spacing, leftCenterZ));
	m_basicMeshes->DrawBoxMesh();

	SetShaderColor(1.0f, 0.85f, 0.0f, 1.0f); //yellow
	SetTransformations(glm::vec3(stickerWidth, stickerHeight, stickerDepth), 0.0f, -65.0f, 0.0f,
		glm::vec3(leftCenterX + zStep, cubeY - spacing, leftCenterZ - xStep));
	m_basicMeshes->DrawBoxMesh();

	/************************************************************************/



	//Top face stickers

	//Calculate center positions for the top face
	float topCenterX = cubeX - 0.13f;
	float topCenterY = cubeY + 0.60f;
	float topCenterZ = cubeZ + 0.10f;

	//Calculate the X and Z offsets for the columns on the top face since they are not aligned with the rows like the other faces
	float topColX = 0.30f;
	float topColZ = -0.15f;

	//Top row
	SetShaderColor(0.0f, 0.6f, 0.0f, 1.0f); //green
	SetTransformations(glm::vec3(stickerWidth, stickerHeight, 0.005f), -90.0f, 0.0f, 25.0f, //Same rotation as the cube so they are aligned, moved slightly forward. This is the same for all the stickers on the top face
		glm::vec3(topCenterX - topColX, topCenterY, topCenterZ - topColZ - 0.45f)); //Moved slightly more forward than the other faces since the top face is angled away from the camera
	m_basicMeshes->DrawBoxMesh();

	SetShaderColor(0.0f, 0.6f, 0.0f, 1.0f); //green
	SetTransformations(glm::vec3(stickerWidth, stickerHeight, 0.005f), -90.0f, 0.0f, 25.0f,
		glm::vec3(topCenterX, topCenterY, topCenterZ - 0.45f)); 
	m_basicMeshes->DrawBoxMesh();

	SetShaderColor(0.8f, 0.3f, 0.0f, 1.0f); //orange
	SetTransformations(glm::vec3(stickerWidth, stickerHeight, 0.005f), -90.0f, 0.0f, 25.0f,
		glm::vec3(topCenterX + topColX, topCenterY, topCenterZ + topColZ - 0.45f));
	m_basicMeshes->DrawBoxMesh();


	//Middle row
	SetShaderColor(0.0f, 0.6f, 0.0f, 1.0f); //green
	SetTransformations(glm::vec3(stickerWidth, stickerHeight, 0.005f), -90.0f, 0.0f, 25.0f,
		glm::vec3(topCenterX - topColX + 0.16f, topCenterY, topCenterZ - topColZ - 0.10f));   //The other two stickers on the top face are not perfectly aligned with the center sticker like the other faces 
																							 //so I had to calculate the X and Z offsets to position them correctly, same concept for the bottom row of stickers on the top face
	m_basicMeshes->DrawBoxMesh();

	SetShaderColor(0.0f, 0.6f, 0.0f, 1.0f); //green
	SetTransformations(glm::vec3(stickerWidth, stickerHeight, 0.005f), -90.0f, 0.0f, 25.0f,
		glm::vec3(topCenterX + 0.16f, topCenterY, topCenterZ - 0.10f));
	m_basicMeshes->DrawBoxMesh();

	SetShaderColor(0.0f, 0.6f, 0.0f, 1.0f); //green
	SetTransformations(glm::vec3(stickerWidth, stickerHeight, 0.005f), -90.0f, 0.0f, 25.0f,
		glm::vec3(topCenterX + topColX + 0.16f, topCenterY, topCenterZ + topColZ - 0.10f));
	m_basicMeshes->DrawBoxMesh();


	//Bottom row

	//The bottom row of stickers on the top face is angled away from the camera more than the other two rows so I had to calculate the X and Z offsets to position them correctly
	float bottomRowOffset = 0.24f;
	float bottomRowXOffset = 0.30f;

	SetShaderColor(0.0f, 0.6f, 0.0f, 1.0f); //green
	SetTransformations(glm::vec3(stickerWidth, stickerHeight, 0.005f), -90.0f, 0.0f, 25.0f,
		glm::vec3(topCenterX - topColX + bottomRowXOffset, topCenterY, topCenterZ - topColZ + bottomRowOffset));
	m_basicMeshes->DrawBoxMesh();

	SetShaderColor(0.0f, 0.6f, 0.0f, 1.0f); //green
	SetTransformations(glm::vec3(stickerWidth, stickerHeight, 0.005f), -90.0f, 0.0f, 25.0f,
		glm::vec3(topCenterX + bottomRowXOffset, topCenterY, topCenterZ + bottomRowOffset));
	m_basicMeshes->DrawBoxMesh();

	SetShaderColor(0.0f, 0.6f, 0.0f, 1.0f); //green
	SetTransformations(glm::vec3(stickerWidth, stickerHeight, 0.005f), -90.0f, 0.0f, 25.0f,
		glm::vec3(topCenterX + topColX + bottomRowXOffset, topCenterY, topCenterZ + topColZ + bottomRowOffset));
	m_basicMeshes->DrawBoxMesh();

	/*********************************************************************************/



	//Rear face stickers

	//Calculate center positions for the rear face
	float backCenterX = cubeX - 0.25f;
	float backCenterZ = cubeZ - 0.54f;

	//Top row
	SetShaderColor(1.0f, 0.85f, 0.0f, 1.0f); //yellow
	SetTransformations(glm::vec3(stickerWidth, stickerHeight, stickerDepth), 0.0f, 205.0f, 0.0f, //Same rotation as the cube so they are aligned, moved slightly forward. This is the same for all the stickers on the rear face
		glm::vec3(backCenterX + xStep, cubeY + spacing, backCenterZ + zStep));
	m_basicMeshes->DrawBoxMesh();

	SetShaderColor(1.0f, 0.85f, 0.0f, 1.0f); //yellow
	SetTransformations(glm::vec3(stickerWidth, stickerHeight, stickerDepth), 0.0f, 205.0f, 0.0f,
		glm::vec3(backCenterX, cubeY + spacing, backCenterZ));
	m_basicMeshes->DrawBoxMesh();

	SetShaderColor(1.0f, 0.85f, 0.0f, 1.0f); //yellow
	SetTransformations(glm::vec3(stickerWidth, stickerHeight, stickerDepth), 0.0f, 205.0f, 0.0f,
		glm::vec3(backCenterX - xStep, cubeY + spacing, backCenterZ - zStep));
	m_basicMeshes->DrawBoxMesh();


	//Middle row
	SetShaderColor(0.8f, 0.0f, 0.0f, 1.0f); //red
	SetTransformations(glm::vec3(stickerWidth, stickerHeight, stickerDepth), 0.0f, 205.0f, 0.0f,
		glm::vec3(backCenterX + xStep, cubeY, backCenterZ + zStep));
	m_basicMeshes->DrawBoxMesh();

	SetShaderColor(1.0f, 0.85f, 0.0f, 1.0f); //yellow
	SetTransformations(glm::vec3(stickerWidth, stickerHeight, stickerDepth), 0.0f, 205.0f, 0.0f,
		glm::vec3(backCenterX, cubeY, backCenterZ));
	m_basicMeshes->DrawBoxMesh();

	SetShaderColor(0.8f, 0.3f, 0.0f, 1.0f); //orange
	SetTransformations(glm::vec3(stickerWidth, stickerHeight, stickerDepth), 0.0f, 205.0f, 0.0f,
		glm::vec3(backCenterX - xStep, cubeY, backCenterZ - zStep));
	m_basicMeshes->DrawBoxMesh();


	//Bottom row
	SetShaderColor(0.8f, 0.3f, 0.0f, 1.0f); //orange
	SetTransformations(glm::vec3(stickerWidth, stickerHeight, stickerDepth), 0.0f, 205.0f, 0.0f,
		glm::vec3(backCenterX + xStep, cubeY - spacing, backCenterZ + zStep));
	m_basicMeshes->DrawBoxMesh();

	SetShaderColor(1.0f, 0.85f, 0.0f, 1.0f); //yellow
	SetTransformations(glm::vec3(stickerWidth, stickerHeight, stickerDepth), 0.0f, 205.0f, 0.0f,
		glm::vec3(backCenterX, cubeY - spacing, backCenterZ));
	m_basicMeshes->DrawBoxMesh();

	SetShaderColor(0.0f, 0.2f, 0.8f, 1.0f); //blue
	SetTransformations(glm::vec3(stickerWidth, stickerHeight, stickerDepth), 0.0f, 205.0f, 0.0f,
		glm::vec3(backCenterX - xStep, cubeY - spacing, backCenterZ - zStep));
	m_basicMeshes->DrawBoxMesh();

	/********************************************************************************/




	/*****************************************************************
	 *****************************************************************
	 **                                                             **
	 **                    							                **
	 **						      Mouse							    **
	 **                                                             **
	 *****************************************************************
	 *****************************************************************/


	//Position and scale
	float mx = 4.5f;
	float my = 0.04f;
	float mz = 3.0f;
	float s = 0.75f;

	
	//Main body
	SetShaderColor(0.96f, 0.96f, 0.98f, 1.0f);
	SetShaderTexture("plastic3");

	SetTransformations(
		glm::vec3(0.60f * s, 0.38f * s, 1.00f * s),
		0.0f, 0.0f, 0.0f,
		glm::vec3(mx, my + 0.10f, mz)
	);
	m_basicMeshes->DrawSphereMesh();


	//Rear hump
	SetShaderColor(0.96f, 0.96f, 0.98f, 1.0f);
	SetShaderTexture("plastic3");
	SetTransformations(
		glm::vec3(0.55f * s, 0.29f * s, 0.68f * s),
		0.0f, 0.0f, 0.0f,
		glm::vec3(mx, my + 0.19f, mz + 0.19f)
	);
	m_basicMeshes->DrawSphereMesh();

	//Left click button
	SetShaderColor(0.96f, 0.96f, 0.98f, 1.0f);
	SetShaderTexture("plastic3");
	SetTransformations(
		glm::vec3(0.40f * s, 0.10f * s, 0.70f * s),
		-10.0f, 0.0f, 25.0f,
		glm::vec3(mx - 0.30f * s, my + 0.32f, mz - 0.10f)
	);
	m_basicMeshes->DrawBoxMesh();

	//Right click button
	SetShaderColor(0.96f, 0.96f, 0.98f, 1.0f);
	SetShaderTexture("plastic3");
	SetTransformations(
		glm::vec3(0.40f * s, 0.10f * s, 0.70f * s),
		-10.0f, 0.0f, -25.0f,
		glm::vec3(mx + 0.30f * s, my + 0.32f, mz - 0.10f)
	);
	m_basicMeshes->DrawBoxMesh();

	//Dark seam between buttons
	SetShaderColor(0.05f, 0.05f, 0.05f, 0.85f);
	SetShaderTexture("plastic2");
	SetTransformations(
		glm::vec3(0.13f * s, 0.10f * s, 0.98f * s),
		-11.0f, 0.0f, 0.0f,
		glm::vec3(mx, my + 0.34f, mz - 0.27f)
	);
	m_basicMeshes->DrawBoxMesh();

	
	//Scroll wheel
	SetShaderColor(0.05f, 0.05f, 0.05f, 1.0f);
	SetShaderTexture("plastic2");
	SetTransformations(
		glm::vec3(0.15f * s, 0.20f * s, 0.20f * s),
		0.0f, 0.0f, 90.0f,
		glm::vec3(mx + 0.08f, my + 0.37f, mz - 0.02f)
	);
	m_basicMeshes->DrawCylinderMesh();
}
