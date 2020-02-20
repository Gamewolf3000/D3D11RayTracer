#ifndef MODEL_IMPORTER_H
#define MODEL_IMPORTER_H


#include <iostream>
#include <fstream>
#include <string>
#include "WICTextureLoader.h"
#include <d3d11.h>
#include <DirectXMath.h>
using namespace std;
using namespace DirectX;

struct ObjVertexData
{
	float x, y, z;
	float u, v;
	float nx, ny, nz;
};

class ModelImporter
{
private:

	void setCounters(string filename);
	void setBuffers(ID3D11Device* gDevice);

	XMFLOAT3* vertices;
	XMFLOAT2* uvCoords;
	XMFLOAT3* normals;
	UINT* indices;
	ObjVertexData* modelData;

	int vCount, vtCount, vnCount, iCount;
	ID3D11ShaderResourceView* texture;

	ID3D11Buffer* modelVertexBuffer = nullptr;
	ID3D11Buffer* modelIndexBuffer = nullptr;

public:
	ModelImporter(ID3D11Device* gDevice, string filename);
	virtual ~ModelImporter();

	ID3D11Buffer** getVBuffer();
	ID3D11Buffer* getIBuffer();
	int getNrOfIndices();
	ObjVertexData* getVertexData();
	int getVCount();
	int getVTCount();
	int getVNCount();
	
	ID3D11ShaderResourceView* vertexView = nullptr;
	ID3D11ShaderResourceView* texCoordsView = nullptr;
	ID3D11UnorderedAccessView* uavVertex = nullptr;
	ID3D11UnorderedAccessView* uavTexCoords = nullptr;
	ID3D11Buffer* buffer1 = nullptr;
	ID3D11Buffer* buffer2 = nullptr;

	void loadMTL(ID3D11Device* gDevice, string mtlname);
	ID3D11ShaderResourceView** getTexture();
	string getMtlName(string filename);

};

#endif