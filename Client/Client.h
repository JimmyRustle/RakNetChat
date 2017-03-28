#pragma once
#define BUFFER_SIZE 100 

#include "Application.h"
#include <RakPeerInterface.h>
#include <MessageIdentifiers.h>
#include <BitStream.h>	
#include <FileList.h>
#include <FileListTransfer.h>
#include <glm/mat4x4.hpp>
#include "GameMessages.h"
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <unordered_map>

struct GameObject
{
	glm::vec3 position;
	glm::vec4 colour;
	glm::vec3 velocity;
	bool recentlyUpdated;
};

class Client : public aie::Application {
public:

	Client();
	virtual ~Client();

	virtual bool startup();
	virtual void shutdown();

	virtual void update(float deltaTime);
	virtual void draw();
	
	void OnSetClientIDPacket(RakNet::Packet * packet);

	void OnDisconnectionSendPacket();

	void SendClientGameObject();

	void SendText();

	void SendFileTest();

	//Initialize the connections
	void HandleNetworkConnections();
	void InitializeClientConnection();

	void HandleNetworkMessages(bool loop);

	int ParseIDNumber(const char * str);

protected:
	RakNet::RakPeerInterface *m_pPeerInterface;
	RakNet::FileListTransfer *m_fileSender;
	const char* IP = "127.0.0.1";
	const unsigned short PORT = 5456;
	
	char textInField[BUFFER_SIZE] = "Enter text";
	std::vector<std::string> m_recentMessages;

	int m_sendPacketInterval;
	int m_sendPacketCounter;
	int m_sendFileID;
	glm::mat4	m_viewMatrix;
	glm::mat4	m_projectionMatrix;
	DWORD m_processID;
	unsigned int m_clientID;
	RakNet::SystemAddress serverAddress;

	GameObject m_gameObject;
};