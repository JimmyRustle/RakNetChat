	#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <RakPeerInterface.h>
#include <MessageIdentifiers.h>
#include <BitStream.h>	
#include <FileListTransfer.h>
#include <FileListTransferCBInterface.h>
//FIGURE OUT FILE TRANSFER
#include "..\Client\GameMessages.h"

//dependencies/raknet/include
/*
1. Extend the program from the tutorial to work like a simple chat server.

a. Add a new message type for chat messages.

b. The client should send a chat message packet when requested by the user.

c. When the server receives a chat message, it should forward the message on to all connected users.
*/

class TestCB;

RakNet::FileListTransfer fileReceiver;
TestCB *testCB;
int nextClientID = 1;
int fileSendID;
bool sendChatMsgBackToSender = true;

bool transfer_complete = false;

class TestCB : public RakNet::FileListTransferCBInterface
{
public:
	bool OnFile(OnFileStruct *onFileStruct)
	{
		std::cout << onFileStruct->fileName << std::endl;
		return true;
	}

	void OnFileProgress(FileProgressStruct *fps)
	{
		
	}

	void OnFile(
		unsigned fileIndex,
		char *filename,
		char *fileData,
		unsigned compressedTransmissionLength,
		unsigned finalDataLength,
		unsigned short setID,
		unsigned setCount,
		unsigned setTotalCompressedTransmissionLength,
		unsigned setTotalFinalLength,
		unsigned char context)
	{


		if (setCount == fileIndex + 1)
		{
			transfer_complete = true;
		}
	
	}
} transferCallback;

//sends a client ID back to the client when it first connects
void SendNewClientID(RakNet::RakPeerInterface* pPeerInterface, RakNet::SystemAddress& address)
{
	RakNet::BitStream bs;
	bs.Write((unsigned char)Messages::ID_SERVER_SET_CLIENT_ID);
	bs.Write(nextClientID);
	nextClientID++;

	pPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, address, false);

}

void SendFileSendID(RakNet::RakPeerInterface* pPeerInterface, int fileSendID) 
{
	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_SERVER_SET_FILE_SEND_ID);
	bs.Write(fileSendID);

	pPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
}

void SentClientPing(RakNet::RakPeerInterface* pPeerInterface)
{
	while (true)
	{
		RakNet::BitStream bs;
		bs.Write((unsigned char)Messages::ID_SERVER_TEXT_MESSAGE);
		bs.Write("Ping!");

		pPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}

void HandleNetworkMessages(RakNet::RakPeerInterface* pPeerInterface)
{
	RakNet::Packet *packet = nullptr;
	
	while (true)
	{
		for (packet = pPeerInterface->Receive(); packet; pPeerInterface->DeallocatePacket(packet), packet = pPeerInterface->Receive())
		{	
			testCB->Update();
			//inspect first byte of the packet to receive its ID
			switch (packet->data[0])
			{
			case ID_NEW_INCOMING_CONNECTION:
				std::cout << "A connection is incoming." << std::endl;
				//packet->systemAddress = address of system that sent this packet
				SendNewClientID(pPeerInterface, packet->systemAddress);
				SendFileSendID(pPeerInterface, fileSendID);
				break;
			case ID_DISCONNECTION_NOTIFICATION:
			{
				//Inform all other clients about which client has been disconnected
				std::cout << "A client has disconnected." << std::endl;
				RakNet::BitStream bsIn(packet->data, packet->length, false);
				RakNet::RakString str;
				bsIn.IgnoreBytes(sizeof(unsigned char));
				int clientID;
				bsIn.Read(str);
				clientID = std::atoi(str);
				std::cout << "Client ID: " << clientID << std::endl;

				RakNet::BitStream bsOut;
				bsOut.Write((unsigned char)ID_DISCONNECTED_CLIENT_ID);
				bsOut.Write(clientID);

				pPeerInterface->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
				
				break;
			}
			case ID_CONNECTION_LOST:
				std::cout << "A client lost the connection." << std::endl;
				break;
			case ID_CHAT_MESSAGE:
			{
				std::cout << "Received chat message." << std::endl;
				RakNet::BitStream bsIn(packet->data, packet->length, false);
				RakNet::RakString str;
				bsIn.IgnoreBytes(sizeof(unsigned char));
				bsIn.Read(str);
				std::cout << "Chat message: " << str << std::endl;

				//send message back to all clients
				RakNet::BitStream bsOut;
				bsOut.Write((unsigned char)Messages::ID_CHAT_MESSAGE);
				bsOut.Write(str);

				if (sendChatMsgBackToSender == true)
				{
					pPeerInterface->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
				}
				else
				{
					pPeerInterface->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, true); //send to everyone except packet->systemAddress, i.e. original sender
				}

				break;	
			}

			case ID_CLIENT_CLIENT_DATA:
			{
				RakNet::BitStream bsOut(packet->data, packet->length, false);
				pPeerInterface->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, true);
				break;
			}

			default:
			{
				std::cout << "Received a message with an unknown ID: " << packet->data[0] << std::endl;
				RakNet::FileList *fileList = RakNet::FileList::GetInstance();
				break;
			}

			}
		}
	}
}

int main()
{
	const unsigned short PORT = 5456;
	RakNet::RakPeerInterface* pPeerInterface = nullptr;
	 
	

	//start up the server and start listening to clients
	std::cout << "Starting up server..." << std::endl;

	//Initialize the Raknet peer interface first
	pPeerInterface = RakNet::RakPeerInterface::GetInstance(); //singleton instance

	//Create a socket descriptor to describe this connection
	RakNet::SocketDescriptor sd(PORT, 0);

	//Now call startup - max of 32 connections, on the assigned port
	pPeerInterface->Startup(32, &sd, 1); //max connections, socket descriptors, num of socket descriptors
	pPeerInterface->SetMaximumIncomingConnections(32);
	
	//setup for file receive
	
	testCB = new TestCB;
	fileSendID = fileReceiver.SetupReceive(testCB, true, pPeerInterface->GetSystemAddressFromGuid(pPeerInterface->GetMyGUID()));
	SendFileSendID(pPeerInterface, fileSendID);
	//Raknet now is listening to connections on port 5456
	//ping all clients every second
	//std::thread pingThread(SentClientPing, pPeerInterface);

	HandleNetworkMessages(pPeerInterface);
	RakNet::RakPeerInterface::DestroyInstance(pPeerInterface);
	system("pause");
	
	return 0;
}