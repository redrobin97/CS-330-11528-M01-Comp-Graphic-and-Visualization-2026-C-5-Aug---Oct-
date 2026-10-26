///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
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
	const char* g_TextureOverlayValueName = "overlayTexture";
	const char* g_UseTextureOverlayName = "bUseTextureOverlay";
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

	modelView = translation * rotationZ * rotationY * rotationX * scale;

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

/***********************************************************
* LoadSceneTextures()
*
* This method is used for preparing the 3D scene by loading
* the shapes, textures in memory to support the 3D scene
* rendering
***********************************************************/
void SceneManager::LoadSceneTextures()
{
	/*** STUDENTS - add the code BELOW for loading the textures that ***/
	/*** will be used for mapping to objects in the 3D scene. Up to ***/
	/*** 16 textures can be loaded per scene. Refer to the code in ***/
	/*** the OpenGL Sample for help. ***/
	bool bReturn = false;
	
	//load texture images
	bReturn = CreateGLTexture(
		"textures/ceramic-white.jpg",
		"ceramic-white");

	bReturn = CreateGLTexture(
		"textures/coffee.jpg",
		"coffee");

	bReturn = CreateGLTexture(
		"textures/lion-label.png",
		"lion-label");

	bReturn = CreateGLTexture(
		"textures/coaster-wood.jpg",
		"coaster-wood");

	bReturn = CreateGLTexture(
		"textures/desk-wood.jpg",
		"desk-wood");

	bReturn = CreateGLTexture(
		"textures/lamp-metal.jpg",
		"lamp-metal");

	bReturn = CreateGLTexture(
		"textures/keyboard.png",
		"keyboard");


	

	
	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}

/***********************************************************
 *  SetShaderTextureOverlay()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTextureOverlay(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		if (textureTag.size() > 0)
		{
			m_pShaderManager->setIntValue(g_UseTextureOverlayName, true);

			int textureID = -1;
			textureID = FindTextureSlot(textureTag);
			m_pShaderManager->setSampler2DValue(g_TextureOverlayValueName, textureID);
		}
		else
		{
			m_pShaderManager->setIntValue(g_UseTextureOverlayName, false);
		}
	}
}

/***********************************************************
 * DefineObjectMaterials()
 *
 * This method is used for configuring the various material
 * settings for all of the objects within the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	// Define plastic material
	OBJECT_MATERIAL plasticMaterial;
	plasticMaterial.diffuseColor = glm::vec3(0.8f, 0.8f, 0.8f);
	plasticMaterial.specularColor = glm::vec3(0.3f, 0.3f, 0.3f);
	plasticMaterial.shininess = 4.0f;
	plasticMaterial.tag = "plastic";
	m_objectMaterials.push_back(plasticMaterial);

	// Define glass material
	OBJECT_MATERIAL glassMaterial;
	glassMaterial.diffuseColor = glm::vec3(0.8f, 0.8f, 0.8f);
	glassMaterial.specularColor = glm::vec3(0.9f, 0.9f, 0.9f);
	glassMaterial.shininess = 32.0f;
	glassMaterial.tag = "glass";
	m_objectMaterials.push_back(glassMaterial);

	// Define wood material
	OBJECT_MATERIAL woodMaterial;
	woodMaterial.diffuseColor = glm::vec3(0.7f, 0.7f, 0.7f);
	woodMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.shininess = 2.0f;
	woodMaterial.tag = "wood";
	m_objectMaterials.push_back(woodMaterial);

	// Define metal material
	OBJECT_MATERIAL metalMaterial;
	metalMaterial.diffuseColor = glm::vec3(0.7f, 0.7f, 0.7f);
	metalMaterial.specularColor = glm::vec3(0.8f, 0.8f, 0.8f);
	metalMaterial.shininess = 32.0f;
	metalMaterial.tag = "metal";
	m_objectMaterials.push_back(metalMaterial);

	// Define felt material for desk cover
	OBJECT_MATERIAL feltMaterial;
	feltMaterial.diffuseColor = glm::vec3(0.7f, 0.7f, 0.7f);
	feltMaterial.specularColor = glm::vec3(0.03f, 0.03f, 0.03f);
	feltMaterial.shininess = 1.0f;
	feltMaterial.tag = "felt";
	m_objectMaterials.push_back(feltMaterial);
}

/***********************************************************
 * SetupSceneLights()
 *
 * This method configures the light sources for the 3D scene.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// Enable custom lighting
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// Dim ambient light
	m_pShaderManager->setVec3Value("directionalLight.direction", 0.0f, -1.0f, -0.5f);
	m_pShaderManager->setVec3Value("directionalLight.ambient", 0.5f, 0.5f, 0.5f);
	m_pShaderManager->setVec3Value("directionalLight.diffuse", 0.45f, 0.45f, 0.45f);
	m_pShaderManager->setVec3Value("directionalLight.specular", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setBoolValue("directionalLight.bActive", true);


	/**************************************************************************/
	//
	// Point light in lamp shade
	// Bright white light inside lamp shade
	m_pShaderManager->setVec3Value("pointLights[1].position", 9.0f, 10.5f, -6.2f);

	// light bulb point light
	m_pShaderManager->setVec3Value("pointLights[1].ambient", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", 0.5f, 0.5f, 0.5f);
	m_pShaderManager->setVec3Value("pointLights[1].specular", 1.0f, 1.0f, 1.0f);

	// Very strong falloff
	m_pShaderManager->setFloatValue("pointLights[1].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[1].linear", 2.0f);
	m_pShaderManager->setFloatValue("pointLights[1].quadratic", 100.0f);

	// Enable light
	m_pShaderManager->setBoolValue("pointLights[1].bActive", true);

	/**************************************************************************/
	
	// Bright spotlight inside desk lamp
	m_pShaderManager->setVec3Value("spotLight.position", 9.0f, 11.0f, -6.2f);
	m_pShaderManager->setVec3Value("spotLight.direction", 0.0f, -1.0f, 0.0f);

	// Bright white lighting
	m_pShaderManager->setVec3Value("spotLight.ambient", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setVec3Value("spotLight.diffuse", 0.75f, 0.75f, 0.75f);
	m_pShaderManager->setVec3Value("spotLight.specular", 3.0f, 3.0f, 3.0f);

	// Reduce light falloff
	m_pShaderManager->setFloatValue("spotLight.constant", 1.0f);
	m_pShaderManager->setFloatValue("spotLight.linear", 0.01f);
	m_pShaderManager->setFloatValue("spotLight.quadratic", 0.001f);

	// Wide spotlight cone
	m_pShaderManager->setFloatValue("spotLight.cutOff", glm::cos(glm::radians(35.0f)));
	m_pShaderManager->setFloatValue("spotLight.outerCutOff", glm::cos(glm::radians(45.0f)));

	// Enable spotlight
	m_pShaderManager->setBoolValue("spotLight.bActive", true);

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

	//load textures
	
	LoadSceneTextures();


	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadPyramid3Mesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();

	//prepare lighting
	DefineObjectMaterials();
	SetupSceneLights();

}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	RenderDesk();
	RenderDeskMat();
	RenderMug();
	RenderBottle();
	RenderLamp();
	RenderLaptop();
}

// Draw the desk portion of the scene.
void SceneManager::RenderDesk()
{

	
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

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


	//set shader texture
	SetShaderTexture("desk-wood");
	
	//set shader material
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 10.0f, -10.0);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//set shader color
	SetShaderColor(0.90f, 0.86f, 0.76f, 1.0f);

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/******************************************************************/
}

// Draw the deskmat portion of the scene.
void SceneManager::RenderDeskMat()
{
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// DRAW DESK MAT MESH
	//
	// 
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(30.0f, 0.125f, 13.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.01f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//set shader color
	SetShaderColor(0.10f, 0.05f, 0.03f, 1.0f);

	//set shader material
	SetShaderMaterial("felt");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

}

// Draw the mug portion of the scene.
void SceneManager::RenderMug()
{
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	//MUG MESHES
	// 
	// 
	//draw cylinder body (mug)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 1.75f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 100.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(9.0f, 0.1925f, -3.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//set shader material
	SetShaderMaterial("glass");

	//set shader texture
	SetShaderTexture("ceramic-white");
	SetShaderTextureOverlay("lion-label");
	SetTextureUVScale(1.0f, 1.0f);

	// Draw only the cylinder sides
	m_basicMeshes->DrawCylinderMesh(false, false, true);

	// Disable overlay
	SetShaderTextureOverlay("");

	/******************************************************************/

	//draw cylinder coffee 
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.99f, 1.0f, 0.99f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(9.0f, 0.1925f, -3.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	SetShaderColor(0.231f, 0.141f, 0.094f, 0.86f);

	//set shader material
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	/******************************************************************/
	//draw coaster (mug)

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.25f, 0.125f, 1.25f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(9.0f, 0.0675f, -3.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//set texture
	SetShaderTexture("coaster-wood");

	//set uv for texture
	SetTextureUVScale(2, 2);

	//set shader material
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	// draw top arm (mug)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.0625f, 0.25f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(10.25f, 1.6415f, -3.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//set texture for remaing mug components
	SetShaderTexture("ceramic-white");

	//set shader material for remaing mug components
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	// draw bottom arm (mug)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.6f, 0.0625f, 0.25f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 25.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(10.25f, 0.4415f, -3.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	// draw middle arm (mug)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.12f, 0.0625f, 0.25f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(10.5f, 1.1115f, -3.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

}

// Draw the bottle portion of the scene.
void SceneManager::RenderBottle()
{
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	//KEVITA Bottle Mesh
	// 
	// 		
	//bottom glass cylinder
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.75f, 0.475f, 0.75f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(7.0f, 0.0675f, -4.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0f, 1.0f, 1.0f, 0.25f);

	//set shader material
	SetShaderMaterial("glass");


	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/******************************************************************/

	//middle colored cylinder
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.75f, 1.5f, 0.75f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(7.0f, 0.5425f, -4.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0f, 0.0f, 0.0f, 1.0f);

	//set shader material
	SetShaderMaterial("plastic");


	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/******************************************************************/

	//top glass cylinder
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.75f, 0.35f, 0.75f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(7.0f, 2.0425f, -4.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0f, 1.0f, 1.0f, 0.25f);

	//set shader material
	SetShaderMaterial("glass");


	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/******************************************************************/

	//top glass sphere
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.75f, 0.5f, 0.75f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(7.0f, 2.3925f, -4.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0f, 1.0f, 1.0f, 0.25f);

	//set shader material
	SetShaderMaterial("glass");


	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();
	/******************************************************************/

	//top glass opening
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.25f, 0.25f, 0.25f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(7.0f, 2.8925f, -4.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0f, 1.0f, 1.0f, 0.25f);

	//set shader material
	SetShaderMaterial("glass");


	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();
	/******************************************************************/

	//top black lid
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.3f, 0.3f, 0.3f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(7.0f, 2.9425f, -4.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f);

	//set shader material
	SetShaderMaterial("plastic");


	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/******************************************************************/

}

// Draw the lamp portion of the scene.
void SceneManager::RenderLamp()
{
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	//Lamp Mesh
	// 
	// 
	//base
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.5f, 0.4f, 2.5f);

	//set shader material for all lamp components
	SetShaderMaterial("metal");

	//set shader texture for all components
	SetShaderTexture("lamp-metal");

	//set texture uv scalw
	SetTextureUVScale(5.0f, 5.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(15.0f, 0.0675f, -6.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/******************************************************************/

	//base tapered cylinder
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.5f, 0.5f, 2.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(15.0f, 0.4675f, -6.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();
	/******************************************************************/

	// on/off knob tapered cylinder
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.3f, 0.45f, 0.3f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 40.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(13.5f, 0.7425f, -6.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();
	/******************************************************************/

	//stem to base tapered cyliner
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.5f, 0.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(15.0f, 0.9425f, -6.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();
	/******************************************************************/

	//weird cylinder connector bottom
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.3f, 1.5f, 0.3f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(15.0f, 1.6425f, -7.25f);
	//(-0.225f, 1.7f, -0.4f)

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/******************************************************************/

	//cylinder bottom front arm
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.1f, 3.0f, 0.15f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(15.0f, 1.9425f, -6.2f);
	//(-0.225f, 1.7f, -0.4f)

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/******************************************************************/

	//cylinder bottom back arm
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.1f, 3.0f, 0.15f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(15.0f, 1.9425f, -6.8f);
	//(-0.225f, 1.7f, -0.4f)

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/******************************************************************/

	//cylinder bottom 45 deg left front arm
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.1f, 3.0f, 0.15f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 45.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(15.0f, 4.9425f, -6.2f);
	//(-0.225f, 1.7f, -0.4f)

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/******************************************************************/

	//cylinder bottom 45 deg left back arm
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.1f, 3.0f, 0.15f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 45.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(15.0f, 4.9425f, -6.8f);


	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/******************************************************************/

	//cylinder bottom 45 deg right front arm
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.1f, 3.0f, 0.15f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -45.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(12.88f, 7.0625f, -6.2f);
	//(-0.225f, 1.7f, -0.4f)

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/******************************************************************/

	//cylinder bottom 45 deg right back arm
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.1f, 3.0f, 0.15f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -45.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(12.88f, 7.0625f, -6.8f);
	//(-0.225f, 1.7f, -0.4f)

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/******************************************************************/


	//weird cylinder connector middle
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 1.5f, 0.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(15.0f, 9.1825f, -7.25f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/******************************************************************/

		//cylinder top 45 deg left arm
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.1f, 3.0f, 0.15f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 45.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(15.0f, 9.1825f, -6.2f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/******************************************************************/

	//cylinder top straight left arm
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.1f, 3.0f, 0.15f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(12.8787f, 11.3038f, -6.2f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/******************************************************************/

	//lamp head support
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.125f, 0.75f, 0.1875f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(9.8787f, 11.3038f, -6.2f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/******************************************************************/

	//lamp head top
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.5f, 0.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(9.0f, 11.0538f, -6.2f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();
	/******************************************************************/

	//lamp head bottom half sphere
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 0.75f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(9.0f, 10.5538f, -6.2f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	// draw the mesh with transformation values
	m_basicMeshes->DrawHalfSphereMesh();
	/******************************************************************/

	// Light bulb inside lamp shade
	scaleXYZ = glm::vec3(0.3f, 0.1f, 0.3f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(9.0f, 11.0f, -6.2f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Warm white bulb
	SetShaderColor(1.0f, 1.0f, 1.0f, 0.5f);

	//set shader material
	SetShaderMaterial("glass");

	m_basicMeshes->DrawBoxMesh();
}



void SceneManager::RenderLaptop()
{

	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	//Laptop base
	//
	//
	//
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(9.0f, 0.3f, 6.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0675f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//setuv scale for texture
	SetTextureUVScale(1.0f, 1.0f);

	//set shader color
	SetShaderColor(0.003f, 0.008f, 0.025f, 1.0f);

	//set shader material
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	//Laptop keyboard area color over lay work around
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(7.875f, 0.075f, 3.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.219375f, -1.125f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//set shader color
	SetShaderColor(0.035f, 0.035f, 0.035f, 1.0f);

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	//Laptop keyboard area
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(7.875f, 0.075f, 3.75f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.22575f, -1.125f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//set shader texture
	SetShaderTexture("keyboard");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	//Laptop trackpad
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.375f, 0.0075f, 1.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.22575f, 1.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//set shader color
	SetShaderColor(0.035f, 0.035f, 0.035f, 1.0f);

	//set shader material
	SetShaderMaterial("felt");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	//Laptop screen back
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(9.0f, 0.15f, 6.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 3.2175f, -3.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//set shader color
	SetShaderColor(0.003f, 0.008f, 0.025f, 1.0f);

	//set shader material
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	//Laptop screen hinge
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.12f, 7.2f, 0.12f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.6f, 0.2175f, -3.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//set shader color
	SetShaderColor(0.035f, 0.035f, 0.035f, 1.0f);

	//set shader material
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	//Laptop screen display
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(4.3f, 1.0f, 2.8f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 3.2175f, -2.915f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	//set shader material
	SetShaderMaterial("glass");

	//set shader color
	SetShaderColor(0.003f, 0.008f, 0.025f, 1.0f);

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

}


	


