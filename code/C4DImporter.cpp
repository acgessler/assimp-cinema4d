/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2012, assimp team
All rights reserved.

Redistribution and use of this software in source and binary forms, 
with or without modification, are permitted provided that the 
following conditions are met:

* Redistributions of source code must retain the above
  copyright notice, this list of conditions and the
  following disclaimer.

* Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the
  following disclaimer in the documentation and/or other
  materials provided with the distribution.

* Neither the name of the assimp team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the assimp team.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

----------------------------------------------------------------------
*/

/** @file  C4DImporter.cpp
 *  @brief Implementation of the Cinema4D importer class.
 */
#include "AssimpPCH.h"

// no #ifdefing here, Cinema4D support is carried out in a branch of assimp
// where it is turned on in the CMake settings. 

#ifndef _MSC_VER
#	error C4D support is currently MSVC only
#endif 

#include "C4DImporter.h"
#include "TinyFormatter.h"

#define __PC 
#include "c4d_file.h"
#include "default_alien_overloads.h"

using namespace _melange_;

// overload this function and fill in your own unique data
void GetWriterInfo(LONG &id, String &appname)
{
	id = 2424226; 
	appname = "Open Asset Import Library";
}

using namespace Assimp;
using namespace Assimp::Formatter;

namespace Assimp {
	template<> const std::string LogFunctions<C4DImporter>::log_prefix = "C4D: ";
}

static const aiImporterDesc desc = {
	"Cinema4D Importer",
	"",
	"",
	"",
	aiImporterFlags_SupportBinaryFlavour,
	0,
	0,
	0,
	0,
	"c4d"
};


// ------------------------------------------------------------------------------------------------
C4DImporter::C4DImporter()
{}

// ------------------------------------------------------------------------------------------------
C4DImporter::~C4DImporter()
{}

// ------------------------------------------------------------------------------------------------
bool C4DImporter::CanRead( const std::string& pFile, IOSystem* pIOHandler, bool checkSig) const
{
	const std::string& extension = GetExtension(pFile);
	if (extension == "c4d") {
		return true;
	}

	else if ((!extension.length() || checkSig) && pIOHandler)	{
		// TODO
	}
	return false;
}

// ------------------------------------------------------------------------------------------------
const aiImporterDesc* C4DImporter::GetInfo () const
{
	return &desc;
}

// ------------------------------------------------------------------------------------------------
void C4DImporter::SetupProperties(const Importer* /*pImp*/)
{
	// nothing to be done for the moment
}


// ------------------------------------------------------------------------------------------------
// Imports the given file into the given scene structure. 
void C4DImporter::InternReadFile( const std::string& pFile, 
	aiScene* pScene, IOSystem* pIOHandler)
{
	boost::scoped_ptr<IOStream> file( pIOHandler->Open( pFile));

	if( file.get() == NULL) {
		ThrowException("failed to open file " + pFile);
	}

	const size_t file_size = file->FileSize();

	std::vector<uint8_t> mBuffer(file_size);
	file->Read(&mBuffer[0], 1, file_size);
	
	Filename f;
	f.SetMemoryReadMode(&mBuffer[0], file_size);

	// open document first
	BaseDocument* doc = LoadDocument(f, SCENEFILTER_OBJECTS);
	if(doc == NULL) {
		ThrowException("failed to read document " + pFile);
	}

	pScene->mRootNode = new aiNode("<C4DRoot>");

	// process C4D scenegraph recursively
	try {
		RecurseHierarchy(doc->GetFirstObject(), pScene->mRootNode);
	}
	catch(...) {
		BOOST_FOREACH(aiMesh* mesh, meshes) {
			delete mesh;
		}
		BaseDocument::Free(doc);
		throw;
	}
	BaseDocument::Free(doc);

	// copy meshes over
	pScene->mNumMeshes = static_cast<unsigned int>(meshes.size());
	pScene->mMeshes = new aiMesh*[pScene->mNumMeshes]();
	std::copy(meshes.begin(), meshes.end(), pScene->mMeshes);

	// default material
	pScene->mNumMaterials = 1;
	pScene->mMaterials = new aiMaterial*[1];
	pScene->mMaterials[0] = new aiMaterial();
}

// ------------------------------------------------------------------------------------------------
void C4DImporter::RecurseHierarchy(BaseObject* object, aiNode* parent)
{
	ai_assert(parent != NULL);
	std::vector<aiNode*> nodes;

	// based on Melange sample code
	while (object)
	{
		const String& name = object->GetName();
		const LONG type = object->GetType(); 
		const Matrix& ml = object->GetMl(); 

		aiString string;
		name.GetCString(string.data, MAXLEN-1);
		aiNode* const nd = new aiNode();

		nd->mParent = parent;
		nd->mName = string;

		nd->mTransformation.a1 = ml.v1.x;
		nd->mTransformation.b1 = ml.v1.y;
		nd->mTransformation.c1 = ml.v1.z;

		nd->mTransformation.a2 = ml.v2.x;
		nd->mTransformation.b2 = ml.v2.y;
		nd->mTransformation.c2 = ml.v2.z;

		nd->mTransformation.a3 = ml.v3.x;
		nd->mTransformation.b3 = ml.v3.y;
		nd->mTransformation.c3 = ml.v3.z;

		nd->mTransformation.a4 = ml.off.x;
		nd->mTransformation.b4 = ml.off.y;
		nd->mTransformation.c4 = ml.off.z;

		nodes.push_back(nd);
		
		GeData data; 
		if (type == Ocamera)
		{
			object->GetParameter(CAMERAOBJECT_FOV, data);
			// TODO: read camera
		}
		else if (type == Olight)
		{
			// TODO: read light
		}
		else if (type == Opolygon)
		{
			aiMesh* const mesh = ReadMesh(object);
			if(mesh != NULL) {
				nd->mNumMeshes = 1;
				nd->mMeshes = new unsigned int[1];
				nd->mMeshes[0] = static_cast<unsigned int>(meshes.size());
				meshes.push_back(mesh);
			}
		}
		
		RecurseHierarchy(object->GetDown(), nd);
		object = object->GetNext();
	}

	// copy nodes over to parent
	parent->mNumChildren = static_cast<unsigned int>(nodes.size());
	parent->mChildren = new aiNode*[parent->mNumChildren]();
	std::copy(nodes.begin(), nodes.end(), parent->mChildren);
}

// ------------------------------------------------------------------------------------------------
aiMesh* C4DImporter::ReadMesh(BaseObject* object)
{
	assert(object != NULL && object->GetType() == Opolygon);
	
	// based on Melange sample code

	PolygonObject* const polyObject = dynamic_cast<PolygonObject*>(object);
	const LONG pointCount = polyObject->GetPointCount();
	const LONG polyCount = polyObject->GetPolygonCount();
	const Vector* points = polyObject->GetPointR();
	const CPolygon* polys = polyObject->GetPolygonR();

	ScopeGuard<aiMesh> mesh(new aiMesh());
	mesh->mNumFaces = static_cast<unsigned int>(polyCount);
	aiFace* face = mesh->mFaces = new aiFace[mesh->mNumFaces]();

	mesh->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;
	mesh->mMaterialIndex = 0;

	unsigned int vcount = 0;

	// first count vertices
	for (LONG i = 0; i < polyCount; i++)
	{
		vcount += 3;
		
		// TODO: do we also need to handle lines or points with similar checks?
		if (polys[i].c != polys[i].d)
		{
			mesh->mPrimitiveTypes |= aiPrimitiveType_POLYGON;
			++vcount;
		}
	}

	mesh->mNumVertices = vcount;
	aiVector3D* verts = mesh->mVertices = new aiVector3D[mesh->mNumVertices];
	unsigned int n = 0;

	// copy vertices over and populate faces
	for (LONG i = 0; i < polyCount; ++i, ++face)
	{
		const Vector& pointA = points[polys[i].a];
		verts->x = pointA.x;
		verts->y = pointA.y;
		verts->z = pointA.z;
		++verts;

		const Vector& pointB = points[polys[i].b];
		verts->x = pointB.x;
		verts->y = pointB.y;
		verts->z = pointB.z;
		++verts;

		const Vector& pointC = points[polys[i].c];
		verts->x = pointC.x;
		verts->y = pointC.y;
		verts->z = pointC.z;
		++verts;

		// TODO: do we also need to handle lines or points with similar checks?
		if (polys[i].c != polys[i].d)
		{
			face->mNumIndices = 4;
			mesh->mPrimitiveTypes |= aiPrimitiveType_POLYGON;
			const Vector& pointD = points[polys[i].d];
			verts->x = pointD.x;
			verts->y = pointD.y;
			verts->z = pointD.z;
			++verts;
		}
		else {
			face->mNumIndices = 3;
		}
		face->mIndices = new unsigned int[face->mNumIndices];
		for(unsigned int j = 0; j < face->mNumIndices; ++j) {
			face->mIndices[j] = n++;
		}
	}

	return mesh.dismiss();
}

