#include "precompiled.h"
#include "TextureManager.h"

TextureManager * TextureManager:: m_instance = 0;

TextureManager * TextureManager::Instance()
{
	if(m_instance == 0)
	{
		m_instance = new TextureManager();
	}
	return m_instance;
}

TextureManager::TextureManager(void)
{

}

TextureManager::~TextureManager(void)
{
	// release all of our texture resources
	map<string,ID3D10ShaderResourceView*>::iterator current = m_textureIDmap.begin();

	for(; current != m_textureIDmap.end(); current++)
	{
		if(current->second)
		{
			current->second->Release();
		}
	}
	
	// clear the UI cache
	map<string,ID3D10ShaderResourceView*>::iterator current_ui = m_textureIDmap_ui.begin();

	for(; current_ui != m_textureIDmap_ui.end(); current_ui++)
	{
		if(current_ui->second)
		{
			current_ui->second->Release();
		}
	}
}

void TextureManager::Initialise(ID3D10Device * graphicsDevice)
{
	m_graphicsDevice = graphicsDevice;
}

ID3D10ShaderResourceView* TextureManager::LoadTexture(const char * fileName)
{
	// first convert our char* to wchar_t*
	wchar_t* fileNameWide = Utilities::ConvertCharStringToWcharString(fileName);

	// search the map and see have we already got this texture
	if(m_textureIDmap[fileName] != 0)
	{
		return m_textureIDmap[fileName]; // we already have this texture
	}
	
	// we dont already have it so let's load it
	HRESULT hr = S_OK;

	ID3D10ShaderResourceView* texture = nullptr;

	D3DX10CreateShaderResourceViewFromFile( m_graphicsDevice, fileNameWide, nullptr, nullptr, &texture, nullptr );

    if( FAILED( hr ) || !texture)
	{
		LOG_ERROR("Couldn't load texture: %s", fileName);
		GAME_ASSERT(false);
	}
	else
	{
		// add to the map
		m_textureIDmap[fileName] = texture;
	}

	return texture;
}

ID3D10ShaderResourceView* TextureManager::LoadTexture_ui(const char * fileName)
{
	// first convert our char* to wchar_t*
	wchar_t* fileNameWide = Utilities::ConvertCharStringToWcharString(fileName);

	// search the map and see have we already got this texture
	if(m_textureIDmap_ui[fileName] != nullptr)
	{
		return m_textureIDmap_ui[fileName]; // we already have this texture
	}
	
	// we dont already have it so let's load it
	HRESULT hr = S_OK;

	ID3D10ShaderResourceView* texture = nullptr;

	D3DX10CreateShaderResourceViewFromFile( m_graphicsDevice, fileNameWide, nullptr, nullptr, &texture, nullptr );

    if( FAILED( hr ) || !texture)
	{
		LOG_ERROR("Couldn't load texture: %s", fileName);
		GAME_ASSERT(false);
	}
	else
	{
		// add to the map
		m_textureIDmap_ui[fileName] = texture;
	}

	return texture;
}

void TextureManager::Release()
{
	delete m_instance;
	m_instance = nullptr;
}

