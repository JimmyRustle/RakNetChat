#include "Client.h"
#include "Gizmos.h"
#include "Input.h"
#include <iostream>
#include <sstream>
#include <thread>
#include "..\bootstrap\imgui_glfw3.h"

#define SPEED 5

using glm::vec3;
using glm::vec4;
using glm::mat4;
using aie::Gizmos;

Client::Client() {

}

Client::~Client() {
}

bool Client::startup() {

	m_processID = GetCurrentProcessId();
	m_fileSender = new RakNet::FileListTransfer;
	std::cout << "Process ID: " << m_processID << std::endl;
	
	m_sendPacketCounter = 0;
	m_sendPacketInterval = 1;
	setBackgroundColour(0.25f, 0.25f, 0.25f);

	// initialise gizmo primitive counts
	Gizmos::create(10000, 10000, 10000, 10000);

	// create simple camera transforms
	m_viewMatrix = glm::lookAt(vec3(10), vec3(0), vec3(0, 1, 0));
	m_projectionMatrix = glm::perspective(glm::pi<float>() * 0.25f,
										  getWindowWidth() / (float)getWindowHeight(),
										  0.1f, 1000.f);

	

	HandleNetworkConnections();

	//std::cout << "Y/N Send Message" << std::endl;
	//char choice;
	//std::cin >> choice;
	//if (choice == 'Y' || choice == 'y')
	//{
	//	std::cout << "Enter message: " << std::endl;
	//	/*
	//	char stringInput[100];
	//	std::cin.ignore();
	//	std::cin.getline(stringInput, 100);
	//    */
	//	std::string stringInput;
	//	std::cin.ignore();
	//	std::getline(std::cin, stringInput);

	//	std::stringstream ss;
	//	ss << "(" <<  m_processID << "): " << stringInput << std::endl;

	////	std::getline(std::cin, input);
	//	RakNet::BitStream bs;
	//	bs.Write((unsigned char)Messages::ID_CHAT_MESSAGE);
	//	bs.Write(ss.str().c_str());

	//	m_pPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
	//}

	return true;
}

void Client::shutdown() {	

	delete[] textInField;
	delete m_fileSender;
	Gizmos::destroy();
}

void Client::update(float deltaTime) {
	//Listen for incoming message
	HandleNetworkMessages(false);

	// query time since application started
	float time = getTime();

	// wipe the gizmos clean for this frame
	Gizmos::clear();

	// quit if we press escape
	aie::Input* input = aie::Input::getInstance();

	ImGui::Begin("Chat Window");
	std::stringstream ss;
	ss << "Client " << m_clientID;

	ImGui::InputText(ss.str().c_str(), textInField, BUFFER_SIZE);
	if (ImGui::Button("Send") && textInField[0] != 0)
	{	
		SendText();
		memset(textInField, 0, sizeof(textInField));
	}

	std::string toPrint = "";

	for (int i = 0; i < m_recentMessages.size(); i++)
	{
		toPrint += m_recentMessages[i] + '\n';
	}

	ImGui::Text(toPrint.c_str());
	
	ImGui::End();

	if (input->isKeyDown(aie::INPUT_KEY_F))
	{
		SendFileTest();
	}

	if (input->isKeyDown(aie::INPUT_KEY_ESCAPE))
	{
		//send ID of disconnected client
		RakNet::BitStream bsOut;
		bsOut.Write((unsigned char)ID_DISCONNECTION_NOTIFICATION);
		bsOut.Write(std::to_string(m_clientID).c_str());
		std::cout << "Sending" << std::endl;
		std::cout << m_pPeerInterface->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true) << std::endl;
	   


		//	m_pPeerInterface->CloseConnection(serverAddress, false);
		quit();
	}
}

void Client::draw() {

	// wipe the screen to the background colour
	clearScreen();

	// update perspective in case window resized
	m_projectionMatrix = glm::perspective(glm::pi<float>() * 0.25f,
										  getWindowWidth() / (float)getWindowHeight(),
										  0.1f, 1000.f);

	Gizmos::draw(m_projectionMatrix * m_viewMatrix);

}

//when server sends back a packet containing client ID
void Client::OnSetClientIDPacket(RakNet::Packet* packet)
{
	RakNet::BitStream bsIn(packet->data, packet->length, false);
	bsIn.IgnoreBytes(sizeof(unsigned char));
	bsIn.Read(m_clientID);
	
	std::cout << "Set client ID to: " << m_clientID << std::endl;
}

void Client::OnDisconnectionSendPacket()
{
	RakNet::BitStream bsOut;
	bsOut.Write((unsigned char)ID_DISCONNECTION_NOTIFICATION);
	bsOut.Write(m_clientID);

	m_pPeerInterface->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
}



void Client::SendClientGameObject()
{
	RakNet::BitStream bs;
	bs.Write((unsigned char)Messages::ID_CLIENT_CLIENT_DATA);
	bs.Write(m_clientID);
	bs.Write((char*)&m_gameObject, sizeof(GameObject));

	m_pPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
}

void Client::SendText()
{
	std::stringstream ss;
	ss << "(Client " << m_clientID << "): " << textInField;
	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_CHAT_MESSAGE);
	bs.Write(ss.str().c_str());

	m_pPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
	
}

void Client::SendFileTest()
{
	RakNet::FileList *fileList = RakNet::FileList::GetInstance();
	FileListNodeContext context;

	fileList->AddFile("C:/Users/munib/Desktop/TestTextFile.txt", "TestTextFile", context);
	//two variations of AddFile exist, one for adding a file from memory and one for adding a file from disk
	m_fileSender->Send(fileList, m_pPeerInterface, serverAddress, m_sendFileID, HIGH_PRIORITY, 0);
}

void Client::HandleNetworkConnections()
{
	//Initialize the Raknet peer interface first
	m_pPeerInterface = RakNet::RakPeerInterface::GetInstance();
	InitializeClientConnection();
}

void Client::InitializeClientConnection()
{
	//startup the RakPeerInterface, but allow only one connection to the server, then attempt to connect to server

	//Create socket descriptor, no data needed
	RakNet::SocketDescriptor sd;
		
	//call startup on m_peerInterface with max connections = 1
	m_pPeerInterface->Startup(1, &sd, 1);

	std::cout << "Connecting to server at " << IP << std::endl;

	//Now call connect to attempt to connect with given server
	RakNet::ConnectionAttemptResult result = m_pPeerInterface->Connect(IP, PORT, nullptr, 0);

	//check if connection
	if (result != RakNet::CONNECTION_ATTEMPT_STARTED)
	{
		std::cout << "Unable to start connection. Error number " << result << std::endl;
	}
	
}
	
void Client::HandleNetworkMessages(bool loop)
{
	RakNet::Packet *packet = nullptr;
	do
	{
		for (packet = m_pPeerInterface->Receive(); packet; m_pPeerInterface->DeallocatePacket(packet), packet = m_pPeerInterface->Receive())
		{
			//inspect first byte of the packet to receive its ID
			switch (packet->data[0])
			{

			case ID_REMOTE_DISCONNECTION_NOTIFICATION:
				std::cout << "Another client has disconnected." << std::endl;
				break;
			case ID_REMOTE_CONNECTION_LOST:
				std::cout << "Another client has lost the connection." << std::endl;
				break;

			case ID_REMOTE_NEW_INCOMING_CONNECTION:
				std::cout << "Another client has connected." << std::endl;
				break;

			case ID_CONNECTION_REQUEST_ACCEPTED:
			{
				std::cout << "Our connection request has been accepted." << std::endl;
				serverAddress = packet->systemAddress;
				break;
			}

			case ID_NO_FREE_INCOMING_CONNECTIONS:
				std::cout << "Server is full." << std::endl;
				break;

			case ID_DISCONNECTION_NOTIFICATION:
			{
				std::cout << "We have been disconnected." << std::endl;
				OnDisconnectionSendPacket();
				break;
			}

			case ID_CONNECTION_LOST:
				std::cout << "Connection lost." << std::endl;
				break;

			case ID_CHAT_MESSAGE:
			{
				RakNet::BitStream bsIn(packet->data, packet->length, false);
				bsIn.IgnoreBytes(sizeof(RakNet::MessageID));

				RakNet::RakString str;
				bsIn.Read(str);
				std::string message = str.C_String();

				int senderID = ParseIDNumber(message.c_str());
				if (senderID == m_clientID)
				{
					std::string modifiedStr = "(You) ";
					bool foundEnd = false;
					for (int i = 0; i < message.length(); i++)
					{
						if (foundEnd == true)
						{
							modifiedStr += message[i];
						}

						if (message[i] == ')')
							foundEnd = true;
					}
					message = modifiedStr;
				}


				m_recentMessages.push_back(message);
		
				break;

			}

			case ID_SERVER_SET_CLIENT_ID:
			{
				OnSetClientIDPacket(packet);
				break;
			}

			case ID_CLIENT_CLIENT_DATA:
			{
				break;
			}

			case ID_DISCONNECTED_CLIENT_ID:
			{
				std::cout << "Someone disconnected, deleting their entry." << std::endl;
			
				break;
			}

			case ID_SERVER_SET_FILE_SEND_ID:
			{
				RakNet::BitStream bsIn(packet->data, packet->length, false);
				bsIn.IgnoreBytes(sizeof(unsigned char));
				serverAddress = packet->systemAddress;
				int fileSendID;
				bsIn.Read(fileSendID);
				std::cout << "Setting send ID" << std::endl;
				m_sendFileID = fileSendID;
				break;
			}

			default:
				std::cout << "Received a message with an unknown ID: " << packet->data[0] << std::endl;
				break;
			}
			
		}
	} while (loop);
}

int Client::ParseIDNumber(const char* str)
{
	std::string result;
	for (int i = 0; i < strlen(str); i++)
	{
		if (str[i] != '(')
		{
			if (str[i] == ')')
			{
				break;
			}

			else if (str[i] >= 48 && str[i] <= 57)
			{
				result += str[i];
			}
		}
	}
	int idNum = std::stoi(result);
	return idNum;
}