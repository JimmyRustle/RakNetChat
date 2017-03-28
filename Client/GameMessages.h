#pragma once
#include <RakPeerInterface.h>
#include <MessageIdentifiers.h>
#include <BitStream.h>	

enum Messages
{
	ID_SERVER_TEXT_MESSAGE = ID_USER_PACKET_ENUM + 1,
	ID_CHAT_MESSAGE = ID_USER_PACKET_ENUM + 2,
	ID_SERVER_SET_CLIENT_ID,
	ID_SERVER_SET_FILE_SEND_ID,
	ID_CLIENT_CLIENT_DATA,
	ID_DISCONNECTED_CLIENT_ID,
};
class GameMessages
{
public:
	GameMessages();
	~GameMessages();

private:
};

