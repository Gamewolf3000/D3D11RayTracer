#include "ModelImporter.h"

ModelImporter::ModelImporter(ID3D11Device* gDevice, string filename)
{
	ifstream file;
	string temp;
	std::string::size_type sz;

	setCounters(filename); // Count positions, uv coords, normals and faces
	string mName = getMtlName(filename); // Get the mtl file name
	loadMTL(gDevice, mName); // Load the texture from the mtl file

	char data, last = NULL, failsafe = NULL;


	file.open(filename);
	file.get(data);
	while (!file.eof())
	{
		if (data == ' ' && last == 'v') // Read all position data
		{
			for (int i = 0; i < vCount; i++)
			{
				file.get(data);

				while (data != ' ')
				{
					temp += data;
					file.get(data);
				}


				vertices[i].x = stof(temp, &sz);
				temp = "";



				file.get(data);

				while (data != ' ')
				{
					temp += data;
					file.get(data);
				}

				vertices[i].y = stof(temp, &sz);
				temp = "";



				file.get(data);

				while (data != '\n')
				{
					temp += data;
					file.get(data);
				}

				vertices[i].z = stof(temp, &sz);
				temp = "";

				file.get(data);

				if (data == 'g')
					while (data != 'v')
					{
						file.get(data);
					}

				//while (data != 'v')
				//{
				//	file.get(data);
				//}

				file.get(data);


			}
		}

		//---------------------------------------------------------------------------------------------------------------------------------

		if (data == 't' && last == 'v') // Read all UV data
		{
			for (int i = 0; i < vtCount; i++)
			{
				file.get(data);
				file.get(data);

				while (data != ' ')
				{
					temp += data;
					file.get(data);
				}

				uvCoords[i].x = stof(temp, &sz);
				temp = "";



				file.get(data);

				while (data != '\n')
				{
					temp += data;
					file.get(data);
				}

				uvCoords[i].y = 1 - stof(temp, &sz);
				temp = "";

				file.get(data);
				file.get(data);


			}
		}

		//---------------------------------------------------------------------------------------------------------

		if (data == 'n' && last == 'v') // Read all Normals data
		{
			for (int i = 0; i < vnCount; i++)
			{
				file.get(data);
				file.get(data);

				while (data != ' ')
				{
					temp += data;
					file.get(data);
				}

				normals[i].x = stof(temp, &sz);
				temp = "";



				file.get(data);

				while (data != ' ')
				{
					temp += data;
					file.get(data);
				}

				normals[i].y = stof(temp, &sz);
				temp = "";



				file.get(data);

				while (data != '\n')
				{
					temp += data;
					file.get(data);
				}

				normals[i].z = stof(temp, &sz);
				temp = "";

				file.get(data);
				file.get(data);

			}
		}

		//---------------------------------------------------------------------------------------------------------------------------

		if (data == ' ' && last == 'f') // Read all face data and put everything together
		{
			int test = 0;
			int index = 0;

			for (int i = 0; i < iCount; i++)
			{
				int count;
				file.get(data);


				for (int j = 0; j < 3; j++)
				{

					while (data != '/')
					{
						temp += data;
						file.get(data);
					}

					file.get(data);
					index = stoi(temp, &sz);
					temp = "";

					count = iCount * 3 - 1 - i * 3 - j;

					modelData[count].x = vertices[index - 1].x;
					modelData[count].y = vertices[index - 1].y;
					modelData[count].z = vertices[index - 1].z;

					while (data != '/')
					{
						temp += data;
						file.get(data);
					}

					file.get(data);
					index = stoi(temp, &sz);
					temp = "";

					modelData[count].u = uvCoords[index - 1].x;
					modelData[count].v = uvCoords[index - 1].y;

					while (data != ' ' && data != '\n')
					{
						temp += data;
						file.get(data);
					}

					file.get(data);
					index = stoi(temp, &sz);
					temp = "";

					modelData[count].nx = normals[index - 1].x;
					modelData[count].ny = normals[index - 1].y;
					modelData[count].nz = normals[index - 1].z;

				}



				test++;

				while (data != 'f' && !file.eof() )
				{
					failsafe = data;
					file.get(data);

					if (data == 'f' && failsafe != '\n')
						while (!(data == 'f' && failsafe == '\n'))
						{
							failsafe = data;
							file.get(data);
						}

				}

				file.get(data);

			}
		}


		last = data;
		file.get(data);
	}

	setBuffers(gDevice);
}

ModelImporter::~ModelImporter()
{
	modelVertexBuffer->Release();
	modelIndexBuffer->Release();
	texture->Release();

	delete vertices;
	delete uvCoords;
	delete normals;
	delete indices;

	delete modelData;
}

void ModelImporter::setCounters(string filename) // Count the different data types
{
	ifstream file;

	vCount = vtCount = vnCount = iCount = 0;

	char data, last = NULL;

	file.open(filename);
	
	if (file.is_open())
	{
		int a = 0;
		a++;
	}
	else
	{
		int b = 0;
		b++;
	}

	file.get(data);
	while (!file.eof())
	{
		if (data == ' ' && last == 'v')
			vCount++;

		if (data == 't' && last == 'v')
			vtCount++;

		if (data == 'n' && last == 'v')
			vnCount++;

		if (data == ' ' && last == 'f')
			iCount++;

		last = data;
		file.get(data);
	}
	file.close();

	vertices = new XMFLOAT3[vCount];
	uvCoords = new XMFLOAT2[vtCount];
	normals = new XMFLOAT3[vnCount];
	modelData = new ObjVertexData[iCount * 3];
}

void ModelImporter::setBuffers(ID3D11Device* gDevice)
{
	HRESULT test;

	D3D11_BUFFER_DESC vBufferDesc;
	memset(&vBufferDesc, 0, sizeof(vBufferDesc));
	vBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vBufferDesc.ByteWidth = sizeof(ObjVertexData) * iCount * 3;

	D3D11_SUBRESOURCE_DATA vData;
	vData.pSysMem = modelData;
	gDevice->CreateBuffer(&vBufferDesc, &vData, &modelVertexBuffer);

	indices = new UINT[iCount*3];

	for (int i = 0; i < iCount * 3; i++)
	{
		indices[i] = iCount*3 - 1 - i;
	}

	D3D11_BUFFER_DESC iBufferDesc;
	iBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	iBufferDesc.ByteWidth = sizeof(int) * iCount * 3;
	iBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	iBufferDesc.CPUAccessFlags = 0;
	iBufferDesc.MiscFlags = 0;
	iBufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA orderData;
	orderData.pSysMem = indices;
	test = gDevice->CreateBuffer(&iBufferDesc, &orderData, &modelIndexBuffer);

	//------------------------------------------------------------
	

	D3D11_BUFFER_DESC positionBufferDesc;
	memset(&positionBufferDesc, 0, sizeof(positionBufferDesc));
	positionBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	positionBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	positionBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	positionBufferDesc.ByteWidth = sizeof(XMFLOAT4) * iCount * 3;
	positionBufferDesc.StructureByteStride = sizeof(XMFLOAT4);

	XMFLOAT4* positionData = new XMFLOAT4[iCount * 3];

	for (int i = 0; i < iCount * 3; i++)
	{
		positionData[i].x = modelData[indices[i]].x;
		positionData[i].y = modelData[indices[i]].y;
		positionData[i].z = modelData[indices[i]].z;
		positionData[i].w = 1.0f;
	}

	D3D11_SUBRESOURCE_DATA posData;
	posData.pSysMem = positionData;
	gDevice->CreateBuffer(&positionBufferDesc, &posData, &buffer1);

	D3D11_UNORDERED_ACCESS_VIEW_DESC uavVertexDesc;
	memset(&uavVertexDesc, 0, sizeof(uavVertexDesc));
	uavVertexDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavVertexDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	uavVertexDesc.Buffer.FirstElement = 0;
	uavVertexDesc.Buffer.Flags = 0;
	uavVertexDesc.Buffer.NumElements = iCount * 3;
	//test = gDevice->CreateUnorderedAccessView(buffer1, &uavVertexDesc, &uavVertex);

	D3D11_SHADER_RESOURCE_VIEW_DESC shaderViewDesc;
	memset(&shaderViewDesc, 0, sizeof(shaderViewDesc));
	shaderViewDesc.Format = DXGI_FORMAT_UNKNOWN;
	shaderViewDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	shaderViewDesc.Buffer.ElementWidth = iCount * 3;

	test = gDevice->CreateShaderResourceView(buffer1, &shaderViewDesc, &vertexView);
	

	
	delete positionData;


	D3D11_BUFFER_DESC texCoordsBufferDesc;
	memset(&texCoordsBufferDesc, 0, sizeof(texCoordsBufferDesc));
	texCoordsBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texCoordsBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	texCoordsBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	texCoordsBufferDesc.ByteWidth = sizeof(XMFLOAT2) * iCount * 3;
	texCoordsBufferDesc.StructureByteStride = sizeof(XMFLOAT2);

	XMFLOAT2* texCoordsData = new XMFLOAT2[iCount * 3];

	for (int i = 0; i < iCount * 3; i++)
	{
		texCoordsData[i].x = modelData[indices[i]].u;
		texCoordsData[i].y = modelData[indices[i]].v;

	}

	D3D11_SUBRESOURCE_DATA texData;
	texData.pSysMem = texCoordsData;
	gDevice->CreateBuffer(&texCoordsBufferDesc, &texData, &buffer2);

	D3D11_UNORDERED_ACCESS_VIEW_DESC uavTexCoordsDesc;
	memset(&uavTexCoordsDesc, 0, sizeof(uavTexCoordsDesc));
	uavTexCoordsDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavTexCoordsDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	uavTexCoordsDesc.Buffer.FirstElement = 0;
	uavTexCoordsDesc.Buffer.Flags = 0;
	uavTexCoordsDesc.Buffer.NumElements = iCount * 3;
	//test = gDevice->CreateUnorderedAccessView(buffer2, &uavTexCoordsDesc, &uavTexCoords);


	test = gDevice->CreateShaderResourceView(buffer2, &shaderViewDesc, &texCoordsView);
	
	delete texCoordsData;


}

ID3D11Buffer** ModelImporter::getVBuffer()
{
	return &modelVertexBuffer;
}

ID3D11Buffer* ModelImporter::getIBuffer()
{
	return modelIndexBuffer;
}

int ModelImporter::getNrOfIndices()
{
	return iCount * 3;
}

ObjVertexData * ModelImporter::getVertexData()
{
	return modelData;
}

int ModelImporter::getVCount()
{
	return vCount;
}

int ModelImporter::getVTCount()
{
	return vtCount;
}

int ModelImporter::getVNCount()
{
	return vnCount;
}

void ModelImporter::loadMTL(ID3D11Device* gDevice, string mtlname)
{

	if (mtlname.compare("") == 0) // If no name was found, use default texture
	{
		CreateWICTextureFromFile(gDevice, nullptr, L"dragonTex.jpg", 0, &texture, NULL);
		return;
	}

	ifstream file;
	string temps;
	wstring temp;

	std::string::size_type sz;

	char data, last = NULL, failsafe = NULL;


	file.open(mtlname);

	if (file.fail()) // If file can't be opened, use default texture
	{
		CreateWICTextureFromFile(gDevice, nullptr, L"dragonTex.jpg", 0, &texture, NULL);
		file.close();
		return;
	}

	file.get(data);
	while (!file.eof())
	{
			file.get(data);

			while (data != ' ' && data != '\n')
			{
				temps += data;
				file.get(data);
			}
			

			if (temps.compare("map_Kd") == 0)
			{
				temp = L"";
				file.get(data);

				while (data != ' ' && data != '\n')
				{
					temp += data;
					file.get(data);
				}

				CreateWICTextureFromFile(gDevice, nullptr, temp.c_str(), 0, &texture, NULL);

				file.close();
				return;
			}
			temp = L"";
			temps = "";


		}

}

ID3D11ShaderResourceView** ModelImporter::getTexture()
{
	return &texture;
}

string ModelImporter::getMtlName(string filename)
{
	ifstream file;
	string temp;

	char data;

	file.open(filename);
	file.get(data);
	while (!file.eof())
	{

		while (data != ' ' && data != '\n')
		{
			temp += data;
			file.get(data);
		}


		if (temp.compare("mtllib") == 0)
		{
			temp = "";
			file.get(data);

			while (data != ' ' && data != '\n')
			{
				temp += data;
				file.get(data);
			}

			file.close();
			return temp;
		}

		temp = "";
		file.get(data);
	}
	file.close();

	return "";
}